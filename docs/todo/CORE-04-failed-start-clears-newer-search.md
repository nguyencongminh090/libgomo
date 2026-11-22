---
id: CORE-04-failed-start-clears-newer-search
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-22
supersedes: []
links: [CORE-02-serialize-deadline-callbacks]
planning: 
---
# DeadlineEngine: startSearch thất bại không được xóa sink của lượt mới hơn

## Scope

- Tái hiện trước khi sửa. Khi `inner_.startSearch` trả lỗi, nhánh thất bại trong
  `DeadlineEngine::startSearch` (`src/engine/deadline_engine.cpp:55`) đặt `sink_ = nullptr` và
  `deadline_.reset()` mà không kiểm tra lượt hiện tại còn là lượt của lời gọi này hay không. Nếu
  engine con đã giao sự kiện kết thúc qua relay trước khi trả lỗi, và sink đã gọi lại
  `startSearch` thành công từ callback đó (hoặc một luồng khác đã mở lượt mới), thì nhánh thất bại
  xóa sink và deadline của lượt mới. Lượt mới sẽ mất mọi sự kiện, kể cả sự kiện kết thúc.
- Nếu tái hiện được: chỉ xóa trạng thái khi `searchId_` vẫn là id của lời gọi này (ghi lại id
  dưới khóa lúc cài sink), thêm ca hồi quy với `EnginePort` giả và ghi fix-log.
- Nếu chỉ tái hiện được với engine con vi phạm hợp đồng: ghi kết luận vào Evidence và để chủ
  repo quyết định giữ lớp phòng thủ hay đóng todo.

## Don't

- Không đổi hợp đồng của `EnginePort` hay `EventSink`.
- Không sửa các adapter và không đụng vào cơ chế tuần tự hóa callback của CORE-02
  (`finishSearch`, `progressCallbacks_`).

## Traps

- Hợp đồng `EnginePort` nói "A refused call never starts a search", nên một engine con tuân thủ
  hợp đồng không giao sự kiện kết thúc rồi mới trả lỗi. Comment ở dòng 54 lại cho rằng điều đó có
  thể xảy ra. Bước đầu tiên là làm rõ với chủ repo kịch bản nào là thật: engine con vi phạm hợp
  đồng, hay race giữa luồng đọc của adapter (ví dụ transport đóng) và luồng gọi `startSearch`.
- Lời gọi `startSearch` lồng nhau từ callback cũng chạy nhánh thất bại của chính nó; kiểm tra
  theo id phải đúng cho cả lượt ngoài lẫn lượt trong.

## Done-when

Acceptance lives in `docs/features.json` (`"todo": "CORE-04"`). Chủ repo duyệt feature sau khi
có kết quả tái hiện.

## Evidence

Phát hiện khi review CORE-02 (2026-11-22), chưa tái hiện.

## Links

[DeadlineEngine](../../src/engine/deadline_engine.cpp) ·
[EnginePort](../../include/pygomo/core/engine_port.hpp) ·
[CORE-02](CORE-02-serialize-deadline-callbacks.md)
