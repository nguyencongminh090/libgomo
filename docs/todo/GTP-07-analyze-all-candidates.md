---
id: GTP-07-analyze-all-candidates
type: todo
size: M
status: active
owner: claude
created: 2026-11-22
supersedes: []
links: [CORE-06-info-event-candidates]
planning: detailed-search-analysis
---
# GTP: đọc mọi ứng viên trong một dòng analyze

## Scope

- `parseGtpAnalyzeLine` (`src/parse/gtp_parser.cpp:137`) đọc mọi entry `info` thành `Candidate`
  theo thứ tự in: `move`, `visits`, `winrate`, `order` (rank = order + 1), `pv`.
- Thang winrate: entry có khóa `utility` là 0..1, không có là ×10000; ngoài 0..1 để trống.
  Không đọc `scoreLead`/`scoreMean` làm drawrate.
- Entry đầu hỏng: cả dòng là `UnparsedEvent` như hiện nay. Entry sau hỏng: giữ các ứng viên trước
  đó và dừng.
- Tóm tắt: PV lấy từ ứng viên hạng 1; nếu không entry nào có `order` thì từ entry đầu. `nodes`
  để trống (GTP không in tổng).
- Chuyển các kiểm tra đã được duyệt sang `candidates` (không xóa, không nới):
  `tests/parse/gtp_parser_test.cpp:130`, `:143`, `tests/gtp/gtp_engine_test.cpp:271`.
- Test `gtp7_*`: cặp probe `info move H8 visits 120 pv H8` với và không có
  ` info move G8 visits 50 pv G8` phân biệt được; hai fixture (kata, lz) cho đúng hạng và
  winrate; entry sau hỏng; dòng chưa nhận dạng.

## Don't

- Không đổi chữ ký `parseGtpAnalyzeLine` và không đổi lệnh `GtpEngine` gửi.
- Không đọc `prior`, `lcb`, `utility` làm trường (non-goal).

## Traps

- PV dừng ở `pvVisits`, `pvEdgeVisits` và `info` kế tiếp (đã có); thêm `isSymmetryOf` vào danh
  sách khóa có giá trị của kata-analyze.
- `move pass`: `move` để trống, ứng viên vẫn giữ.
- Fixture lz trong `gtp_engine_test.cpp:270` gửi winrate 5600 cho lệnh kata: không có `utility`
  nên đọc theo thang ×10000, đúng.

## Done-when

Features `GTP-07-*` trong `docs/features.json`.

## Evidence

## Links

`features/detailed-search-analysis/planning.md`, `tests/fixtures/gtp/`
