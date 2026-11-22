---
id: CORE-03-document-architecture-lifecycle
type: todo
size: M
status: active
owner: codex
created: 2026-11-25
supersedes: []
links: []
planning:
---
# Bổ sung phụ thuộc và vòng đời phiên vào sơ đồ kiến trúc

## Scope

- Bổ sung cạnh `parse → core` trong sơ đồ Draw.io dựa trên `target_link_libraries`.
- Thể hiện trình tự ứng dụng ghép transport, adapter và `DeadlineEngine`, gọi `open()`,
  bắt đầu search và đóng phiên. Minh họa bằng `ProcessTransport`. Chỉ rõ bên nào sở hữu
  từng đối tượng và thứ tự đóng transport từ luồng điều khiển trước khi hủy decorator.
- Đồng bộ bản PNG xuất từ Draw.io và sơ đồ Mermaid đồng hành. Bổ sung frontmatter cho
  `docs/design/architecture.md` theo mẫu design để xử lý lỗi `FM-MISSING`.

## Done-when

Tiêu chí đề xuất cho `docs/features.json`, chờ chủ repo duyệt.

- Khi mở bản Draw.io và PNG, cạnh `parse → core` và các bước mở, tìm kiếm, đóng phiên
  đều đọc được. Đối chiếu với CMake, header của adapter và hợp đồng của `DeadlineEngine`.
- Sơ đồ Mermaid đồng hành mô tả cùng phụ thuộc và trình tự. Chạy
  `node scripts/harness/harness-check.mjs` và xác nhận hết lỗi `FM-MISSING`
  của `docs/design/architecture.md`.

## Evidence

- `ctest --preset default -R smoke` đạt. Bộ `ctest --preset default` có 62 test đạt và
  bỏ qua 4 test Rapfi chưa cấu hình `PYGOMO_RAPFI`.
- Trước khi sửa, bản Draw.io và PNG thiếu cạnh `parse → core`. Harness báo
  `ERROR FM-MISSING docs/design/architecture.md: frontmatter missing`.
- Draw.io Desktop mở và xuất sơ đồ sang PNG 1604 × 2892. Kiểm tra trực quan cạnh
  `parse → core`, `open()` trên ba adapter, và trình tự mở, tìm kiếm, đóng. XML nhúng
  trong PNG trùng với nguồn sau khi chuẩn hóa bằng `xmllint`.
- Đối chiếu cạnh phụ thuộc với `target_link_libraries(pygomo_parse PUBLIC pygomo::core)`.
  Đối chiếu lời gọi và vòng đời với header và mã nguồn của các adapter,
  `ProcessTransport` và `DeadlineEngine`. Một chương trình chạy thử với tiến trình con
  giả lập Gomocup nhận đúng một nước đi khi tìm kiếm hoàn tất và đúng một `ErrorEvent`
  khi đóng phiên đang tìm kiếm. Trong cả hai trường hợp, `close()` đợi tiến trình kết thúc.
- Mermaid CLI kết xuất thành công cả ba sơ đồ. Bản kết xuất có cạnh `parse → core`,
  `open()` trên ba adapter và đúng thứ tự vòng đời. `harness-check.mjs` hết lỗi `FM-MISSING`.

## Links

[Draw.io](../design/architecture.drawio) · [PNG](../design/architecture.drawio.png) ·
[Mermaid](../design/architecture.md) ·
[CMake](../../CMakeLists.txt) ·
[Hợp đồng vòng đời của DeadlineEngine](../../include/pygomo/engine/deadline_engine.hpp)
