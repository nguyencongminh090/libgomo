---
id: CORE-02-serialize-deadline-callbacks
type: todo
size: M
status: done
owner: codex
created: 2026-11-22
supersedes: []
links: [engine-search-deadline]
planning:
---
# Tuần tự hóa callback của DeadlineEngine khi hết hạn

## Scope

- Sửa việc `DeadlineEngine` có thể gọi `onEvent(ErrorEvent)` từ watchdog khi callback
  `onEvent(InfoEvent)` trên luồng của adapter vẫn đang chạy trên cùng một sink.
- Bảo đảm sự kiện kết thúc đến sau các callback đã bắt đầu của cùng lượt tìm kiếm và sink
  có thể được giải phóng sau khi callback kết thúc đã trả về. Làm rõ điều này trong tài liệu
  của decorator.
- Giữ một kiểm thử hồi quy với `EnginePort` giả và ghi nguyên nhân cùng cách sửa vào fix log.

## Traps

- Luồng adapter đọc `sink_` dưới mutex rồi nhả khóa trước khi gọi `onEvent`. Watchdog cũng
  lấy con trỏ và nhả khóa trước khi báo lỗi. Mutex hiện tại bảo vệ trạng thái nội bộ nhưng
  hai lời gọi vào sink vẫn có thể chồng lên nhau.
- Sink có thể gọi lại `startSearch` từ callback. Giữ khả năng này và hành vi `Busy` khi
  engine còn nợ sự kiện kết thúc của lượt đã hết hạn.

## Done-when

Acceptance in `docs/features.json` (`"todo": "CORE-02"`), đã được chủ repo duyệt.

- Khi watchdog hết hạn trong lúc callback thông tin còn chạy, sink chỉ nhận sự kiện
  kết thúc sau khi callback đó trả về. Mỗi lượt có đúng một sự kiện kết thúc và sự kiện
  đến muộn của engine không lọt vào lượt sau.
- Kiểm chứng bằng ca hồi quy mới trong `tests/engine/deadline_engine_test.cpp`,
  `ctest --preset default` và `ctest --preset tsan`.

## Evidence

Probe tạm dùng `EnginePort` giả, `turnTime = 50 ms` và `grace = 10 ms`. Sink giữ callback
`InfoEvent` đang chạy cho tới khi callback báo hết hạn xuất hiện. Kết quả
`Timeout callback overlapped active info callback: true` xác nhận lỗi trước khi sửa.

- `dead1_serialization` thất bại trước bản sửa vì timeout được giao trước khi callback
  `InfoEvent` trả về. Sau bản sửa, ca kiểm thử đạt 20 lần liên tiếp ở cả hai preset.
  Ca này kiểm tra kết quả đến muộn cả trước và sau khi giao lỗi timeout.
- `ctest --preset default --timeout 60` và `ctest --preset tsan --timeout 60` đều đạt
  66/66 với `PYGOMO_RAPFI` trỏ đến Rapfi thật. `scripts/check-core-deps.sh` đạt.
- `latexmk` dựng lại bản PDF của developer guide. `harness-check` báo lỗi `FM-MISSING`
  có sẵn ở `docs/design/architecture.md` và cảnh báo `FEATURE-NONE` cho CORE-02.

## Links

[DeadlineEngine](../../src/engine/deadline_engine.cpp) ·
[Developer guide](../guide/developer-guide.tex) ·
[Audit về deadline](../audit/AUD-03-deadline-in-decorator.md) ·
[Audit về thứ tự callback](../audit/AUD-05-terminal-callback-order.md)
