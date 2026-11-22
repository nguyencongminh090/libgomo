---
id: CORE-06-info-event-candidates
type: todo
size: M
status: done
owner: claude
created: 2026-11-22
supersedes: []
links: []
planning: detailed-search-analysis
---
# Core: kiểu `Candidate` và `InfoEvent::candidates`

## Scope

- Thêm `struct Candidate` vào `include/pygomo/core/search_event.hpp` đúng như Decision của
  planning: `rank`, `move`, `score`, `visits`, `winrate`, `drawrate`, `principalVariation`, với
  `operator==` mặc định.
- Thêm `std::vector<Candidate> candidates{}` làm thành viên cuối của `InfoEvent`.
- Doc comment `///` cho kiểu và `///<` cho mọi thành viên: hạng 1-based, rỗng khi engine không
  in; winrate/drawrate trong 0..1, phối cảnh theo engine; `visits` chỉ của nước đó. Ghi trong doc
  của `InfoEvent` nghĩa mới của tóm tắt: `nodes` là tổng của search; `score` và PV chỉ từ dòng
  không hạng hoặc hạng 1.
- Test `cand1_*`: khởi tạo mặc định, so sánh bằng nhau và khác nhau theo `candidates`, khởi tạo
  aggregate cũ (`InfoEvent{}`, `InfoEvent{e}`) vẫn biên dịch.
- Cập nhật `docs/design/architecture.md` ở chỗ mô tả `InfoEvent`.

## Don't

- Không thêm kiểu vào `SearchEvent`.
- Không đổi parser hay adapter (GOMO-07, GTP-07). Các test parser hiện có phải vẫn qua nguyên.

## Traps

- Thêm thành viên ở cuối, không chen vào giữa: chen vào sẽ làm sai khởi tạo aggregate theo vị
  trí ở mọi nơi.
- `scripts/check-core-deps.sh`: `Candidate` chỉ dùng kiểu core (`Coord`, `Score`).

## Done-when

Features `CORE-06-*` trong `docs/features.json`.

## Evidence

Implemented by agy (Gemini 4 Neon High, accept-edits mode; it could not run shell commands).
Reviewed by Claude: one 104-column line in `architecture.md` rewrapped, InfoEvent doc comment
made precise. Checks run by Claude on 2026-11-22:

- `ctest --preset default -R "^cand1_" --output-on-failure`: 3/3 (`cand1_defaults`,
  `cand1_equality`, `cand1_compat`; 4 doctest cases).
- `ctest --preset default`: 73/73 (gomo2 live tests Skipped, no `PYGOMO_RAPFI`), including
  `ctest_coverage`, `style_line_length`, `style_doc_comments`.
- `ctest --preset tsan`: 73/73. `scripts/check-core-deps.sh`: OK.
- Existing parser tests unchanged (`git diff` touches no file under `tests/parse` or `src/`).

## Links

`features/detailed-search-analysis/planning.md`
