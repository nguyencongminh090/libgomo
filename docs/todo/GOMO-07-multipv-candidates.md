---
id: GOMO-07-multipv-candidates
type: todo
size: M
status: active
owner: claude
created: 2026-11-22
supersedes: []
links: [CORE-06-info-event-candidates]
planning: detailed-search-analysis
---
# Gomocup: giữ hạng MultiPV và dữ liệu ứng viên

## Scope

- Trong `src/parse/gomocup_parser.cpp`, dòng có hạng tạo một `Candidate` theo quy tắc của
  planning, cho cả bốn dạng Rapfi (`search/searchoutput.cpp:129-209`):
  `(N) v | d-s | pv`, `(N) v (W w, D d, S s) | V n | SD sd | pv`, `depth … multipv N … pv …`,
  `multipv N ev v w W d D stdev s v n seldepth sd n N … prior p pv …`.
- Trường: `rank` = N; `move` = nước đầu PV; `score`; `visits` từ `V`/`v`; `winrate`/`drawrate`
  từ `W`/`D` hoặc `w`/`d` chia 100.
- Tóm tắt: `nodes` chỉ từ `n`, `Node`, `Visit` của dòng `Speed`; `score` và PV tóm tắt chỉ khi
  dòng không hạng hoặc hạng 1; `depth`, `elapsed` như cũ.
- Dòng không hạng (`depth … ev … pv`, `Bestline`, `Speed …`) giữ `candidates` rỗng.
- Chuyển các kiểm tra đã được duyệt sang `candidates` (không xóa, không nới):
  `tests/parse/rapfi_format_test.cpp:51-59`, `tests/parse/gomocup_parser_test.cpp:67-69`.
- Test `gomo7_*`: cặp probe `MESSAGE multipv 1 ev 7 pv 1,1` / `MESSAGE multipv 2 ev 7 pv 1,1`
  cho hai `InfoEvent` khác nhau, hạng 1 và 2; mỗi dạng Rapfi; dòng không hạng; dòng chưa nhận
  dạng vẫn là `MessageEvent`/`UnparsedEvent`.
- Chạy thật Rapfi với MultiPV > 1 (`PYGOMO_RAPFI`): ít nhất một `InfoEvent` có ứng viên hạng 2.

## Don't

- Không gộp các dòng thành snapshot trong `GomocupEngine`.
- Không đọc dạng `MESSAGE Visits N Winrate …` của KataGomo `gom` (non-goal).
- Không đổi chữ ký `parseGomocupLine`.

## Traps

- `V 1K`, `n 1.2M`: dùng `toCount` có sẵn cho hậu tố K/M.
- Dạng UCI realtime có cả `v` (của nước) và `n` (tổng): đừng để vòng lặp khóa/giá trị đọc `v`
  vào `nodes`.
- `W 50.00,` có dấu phẩy dính số. `[Pondering] ` đứng trước dạng normal.
- Cách bật MultiPV ở Rapfi qua giao thức Gomocup: đọc `command/gomocup.cpp` trước khi viết
  test live.

## Done-when

Features `GOMO-07-*` trong `docs/features.json`.

## Evidence

## Links

`features/detailed-search-analysis/planning.md`, `src/parse/gomocup_parser.cpp`
