#include "pygomo/transport/stream_transport.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace pygomo {

StreamTransport::StreamTransport(std::istream& in, std::ostream& out,
                                 StreamTransportOptions options)
    : in_(in), out_(out), options_(options) {}

StreamTransport::~StreamTransport() {
    close();
    if (reader_.joinable()) reader_.join();
}

std::expected<void, TransportError> StreamTransport::start(TransportListener& listener) {
    if (closed_.load()) return std::unexpected(TransportError::Closed);
    TransportListener* expected = nullptr;
    if (!listener_.compare_exchange_strong(expected, &listener)) {
        return std::unexpected(TransportError::AlreadyStarted);
    }
    reader_ = std::thread([this] { readLoop(); });
    return {};
}

std::expected<void, TransportError> StreamTransport::writeLine(std::string_view line) {
    if (listener_.load() == nullptr) return std::unexpected(TransportError::NotStarted);
    if (line.find('\n') != std::string_view::npos) {
        return std::unexpected(TransportError::InvalidLine);
    }
    std::scoped_lock lock(writeMutex_);
    if (closed_.load()) return std::unexpected(TransportError::Closed);
    out_ << line << '\n';
    out_.flush();
    if (!out_) return std::unexpected(TransportError::IoError);
    return {};
}

void StreamTransport::close() { finish(CloseReason::Requested); }

void StreamTransport::readLoop() {
    std::string line;
    char        ch{};
    while (in_.get(ch)) {
        if (closed_.load()) return;
        if (ch == '\n') {
            deliverLine(line);
            line.clear();
            continue;
        }
        if (line.size() >= options_.maxLineLength) {
            finish(CloseReason::LineTooLong);
            return;
        }
        line.push_back(ch);
    }
    if (closed_.load()) return;
    if (!line.empty()) deliverLine(line);  // final line without a newline
    finish(in_.bad() ? CloseReason::IoError : CloseReason::Eof);
}

void StreamTransport::deliverLine(std::string_view line) {
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    // The lock is held across the call so that onClosed can never overlap an onLine.
    std::scoped_lock lock(deliverMutex_);
    if (closed_.load()) return;
    listener_.load()->onLine(line);
}

void StreamTransport::finish(CloseReason reason) {
    std::scoped_lock lock(deliverMutex_);
    closed_.store(true);
    if (closedNotified_) return;
    closedNotified_ = true;
    if (TransportListener* listener = listener_.load()) listener->onClosed(reason);
}

}  // namespace pygomo
