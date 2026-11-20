#include "pygomo/process/process_transport.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <csignal>
#include <ctime>
#include <mutex>

extern char** environ;

namespace pygomo {
namespace {

// Blocks SIGPIPE on the calling thread only, so a write to a dead child returns EPIPE without
// the process-wide disposition being touched. discardPending() consumes the signal our own
// failed write raised (it is thread-directed), before the old mask is restored.
class ScopedSigpipeBlock {
public:
    ScopedSigpipeBlock() {
        sigemptyset(&set_);
        sigaddset(&set_, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &set_, &old_);
    }
    ~ScopedSigpipeBlock() { pthread_sigmask(SIG_SETMASK, &old_, nullptr); }
    ScopedSigpipeBlock(const ScopedSigpipeBlock&) = delete;
    ScopedSigpipeBlock& operator=(const ScopedSigpipeBlock&) = delete;

    void discardPending() {
        timespec none{0, 0};
        while (::sigtimedwait(&set_, nullptr, &none) < 0 && errno == EINTR) {
        }
    }

private:
    sigset_t set_{};
    sigset_t old_{};
};

void closeFd(int& fd) {
    if (fd >= 0) ::close(fd);
    fd = -1;
}

}  // namespace

ProcessTransport::ProcessTransport(ProcessOptions options) : options_(std::move(options)) {}

ProcessTransport::~ProcessTransport() {
    close();
    if (reader_.joinable() && reader_.get_id() != std::this_thread::get_id()) reader_.join();
    closeFd(wakeRead_);
    closeFd(wakeWrite_);
}

std::expected<void, TransportError> ProcessTransport::start(TransportListener& listener) {
    if (started_.exchange(true)) return std::unexpected(TransportError::AlreadyStarted);
    if (requested_) return std::unexpected(TransportError::Closed);

    int in[2] = {-1, -1}, out[2] = {-1, -1}, wake[2] = {-1, -1};
    auto cleanup = [&] {
        for (int* p : {in, out, wake}) { closeFd(p[0]); closeFd(p[1]); }
    };
    if (::pipe2(in, O_CLOEXEC) != 0 || ::pipe2(out, O_CLOEXEC) != 0 || ::pipe2(wake, O_CLOEXEC) != 0) {
        cleanup();
        return std::unexpected(TransportError::IoError);
    }

    posix_spawn_file_actions_t fa;
    posix_spawnattr_t attr;
    posix_spawn_file_actions_init(&fa);
    posix_spawnattr_init(&attr);
    posix_spawn_file_actions_adddup2(&fa, in[0], 0);
    posix_spawn_file_actions_adddup2(&fa, out[1], 1);
    if (options_.mergeStderr) {
        posix_spawn_file_actions_adddup2(&fa, out[1], 2);
    } else {
        posix_spawn_file_actions_addopen(&fa, 2, "/dev/null", O_WRONLY, 0);
    }
    if (!options_.workingDirectory.empty()) {
        posix_spawn_file_actions_addchdir_np(&fa, options_.workingDirectory.c_str());
    }
    sigset_t def;
    sigemptyset(&def);
    sigaddset(&def, SIGPIPE);
    posix_spawnattr_setsigdefault(&attr, &def);
    posix_spawnattr_setpgroup(&attr, 0);  // own process group: terminateChild signals it whole
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETPGROUP);

    std::vector<char*> argv;
    argv.push_back(options_.program.data());
    for (auto& a : options_.args) argv.push_back(a.data());
    argv.push_back(nullptr);

    pid_t pid = -1;
    int rc = ::posix_spawnp(&pid, options_.program.c_str(), &fa, &attr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&attr);
    if (rc != 0) {
        cleanup();
        return std::unexpected(TransportError::IoError);
    }

    closeFd(in[0]);
    closeFd(out[1]);
    pid_ = pid;
    stdinFd_ = in[1];
    stdoutFd_ = out[0];
    wakeRead_ = wake[0];
    wakeWrite_ = wake[1];
    listener_ = &listener;
    reader_ = std::thread([this] { readLoop(); });
    return {};
}

std::expected<void, TransportError> ProcessTransport::writeLine(std::string_view line) {
    if (line.find('\n') != std::string_view::npos) return std::unexpected(TransportError::InvalidLine);
    if (!started_) return std::unexpected(TransportError::NotStarted);
    std::string data(line);
    data.push_back('\n');

    std::lock_guard lock(writeMutex_);
    if (requested_ || stdinFd_ < 0) return std::unexpected(TransportError::Closed);
    ScopedSigpipeBlock noSigpipe;
    const char* p = data.data();
    std::size_t left = data.size();
    while (left > 0) {
        ssize_t n = ::write(stdinFd_, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EPIPE) noSigpipe.discardPending();
            return std::unexpected(TransportError::IoError);
        }
        p += n;
        left -= static_cast<std::size_t>(n);
    }
    return {};
}

void ProcessTransport::close() {
    if (requested_.exchange(true)) {
        // Already closing; a second caller still waits for the cleanup below.
    }
    if (!started_) return;
    char b = 1;
    if (wakeWrite_ >= 0) { ssize_t r = ::write(wakeWrite_, &b, 1); (void)r; }
    if (reader_.joinable() && reader_.get_id() != std::this_thread::get_id()) {
        static std::mutex joinMutex;
        std::lock_guard lock(joinMutex);
        if (reader_.joinable()) reader_.join();
    }
}

std::optional<int> ProcessTransport::exitStatus() const noexcept {
    if (!exited_) return std::nullopt;
    return exit_.load();
}

CloseReason ProcessTransport::pump() {
    std::string pending;
    char buf[4096];
    while (true) {
        if (requested_) return CloseReason::Requested;
        pollfd fds[2] = {{stdoutFd_, POLLIN, 0}, {wakeRead_, POLLIN, 0}};
        int pr = ::poll(fds, 2, -1);
        if (pr < 0) {
            if (errno == EINTR) continue;
            return CloseReason::IoError;
        }
        if (fds[1].revents != 0) return CloseReason::Requested;
        if (fds[0].revents == 0) continue;

        ssize_t n = ::read(stdoutFd_, buf, sizeof buf);
        if (n < 0) {
            if (errno == EINTR) continue;
            return CloseReason::IoError;
        }
        if (n == 0) {
            if (!pending.empty() && !requested_) {
                if (pending.back() == '\r') pending.pop_back();
                listener_->onLine(pending);
            }
            return CloseReason::Eof;
        }
        for (ssize_t i = 0; i < n; ++i) {
            char c = buf[i];
            if (c == '\n') {
                if (!pending.empty() && pending.back() == '\r') pending.pop_back();
                if (requested_) return CloseReason::Requested;
                listener_->onLine(pending);
                pending.clear();
            } else {
                pending.push_back(c);
                if (pending.size() > options_.maxLineLength) return CloseReason::LineTooLong;
            }
        }
    }
}

void ProcessTransport::readLoop() {
    CloseReason reason = pump();
    if (requested_) reason = CloseReason::Requested;
    terminateChild();
    closeFd(stdoutFd_);
    listener_->onClosed(reason);
}

void ProcessTransport::terminateChild() {
    auto reap = [&](bool block) {
        int status = 0;
        pid_t r = ::waitpid(pid_, &status, block ? 0 : WNOHANG);
        if (r != pid_) return false;
        exit_ = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
        exited_ = true;
        return true;
    };
    auto waitExit = [&](std::chrono::milliseconds limit) {
        auto end = std::chrono::steady_clock::now() + limit;
        while (std::chrono::steady_clock::now() < end) {
            if (reap(false)) return true;
            ::usleep(5000);
        }
        return reap(false);
    };

    {
        // A writer stuck on a full pipe holds the mutex; skip the polite step then.
        std::unique_lock lock(writeMutex_, std::try_to_lock);
        if (lock.owns_lock()) closeFd(stdinFd_);
    }
    if (!waitExit(options_.grace)) {
        ::kill(-pid_, SIGTERM);  // the group: wrappers and their children go too
        if (!waitExit(options_.grace)) {
            ::kill(-pid_, SIGKILL);
            while (!reap(true)) {
                if (errno != EINTR) break;
            }
        }
    }
    std::lock_guard lock(writeMutex_);
    closeFd(stdinFd_);
}

}  // namespace pygomo
