---
id: PORT-03-closed-during-start-double-outcome
type: todo
size: M
status: done
owner: claude
created: 2026-11-22
supersedes: []
links: [CORE-04-failed-start-clears-newer-search]
planning: 
---
# Adapter: transport đóng giữa startSearch gây hai kết quả cho một lời gọi

## Scope

- Ghi nhận khi làm CORE-04, chưa tái hiện trên adapter thật. `GomocupEngine`, `KataGomoEngine`
  và `GtpEngine` cài `sink_` dưới khóa rồi ghi request ngoài khóa. Nếu transport đóng giữa hai
  bước, `onClosed` giao `ErrorEvent{"engine connection closed"}` cho sink, rồi `writeLine` thất
  bại và `startSearch` trả `TransportFailed`. Người gọi nhận cả sự kiện kết thúc lẫn lỗi, trái
  với câu "A refused call never starts a search" của `EnginePort`.
- GTP còn một biến thể: engine trả `?` cho lệnh setup đầu tiên (search kết thúc) trước khi một
  lần ghi sau thất bại.
- Tái hiện bằng transport giả, rồi chủ repo chọn: sửa adapter (vd. trả thành công khi sink đã
  bị lấy đi) hay nới hợp đồng `EnginePort` (việc đó là size L).

## Decision (chủ repo, 2026-11-22)

Sửa adapter, giữ hợp đồng `EnginePort`. Khi ghi thất bại mà search của lời gọi này đã kết thúc
(terminal event đã giao), `startSearch` trả thành công: search coi như đã bắt đầu và kết thúc
bằng đúng một terminal event. Chỉ xóa `sink_` khi nó vẫn thuộc lời gọi này.

## Don't

- Không đổi `DeadlineEngine`; CORE-04 đã chịu được trường hợp này.
- Không đổi hợp đồng `EnginePort`.

## Done-when

Acceptance lives in `docs/features.json` (`"todo": "PORT-03"`).

## Evidence

Đọc mã khi làm CORE-04 (2026-11-22): `gomocup_engine.cpp:75-84,125-134`,
`katagomo_engine.cpp:68-78,109-118`, `gtp_engine.cpp:95-109,215-225`.

Tái hiện (spike, 2026-11-22, transport giả trong scratchpad): cả ba adapter, transport đóng ở lần
ghi thứ 2, đều giao `ErrorEvent{"engine connection closed"}` rồi trả `TransportFailed`; GTP với
`?1` cho lệnh setup đầu rồi lần ghi thứ 2 thất bại giao `ErrorEvent{"unacceptable size"}` rồi
trả `TransportFailed`. Các test `port3_*` giữ lại các tình huống này làm kiểm tra hồi quy.

## Links

[CORE-04](CORE-04-failed-start-clears-newer-search.md) ·
[EnginePort](../../include/gomo/core/engine_port.hpp)
