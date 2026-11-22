---
id: DOC-04-candidates-adr-guide
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-22
supersedes: []
links: [CORE-06-info-event-candidates, GOMO-07-multipv-candidates, GTP-07-analyze-all-candidates]
planning: detailed-search-analysis
---
# ADR cho hình dạng `InfoEvent` và cập nhật guide

## Scope

- Viết ADR trong `docs/adr/` (template `docs/templates/adr.md`) cho quyết định của planning:
  `InfoEvent::candidates` thay vì kiểu mới trong `SearchEvent` hay trường `multipv`; nghĩa các
  trường tóm tắt; lý do (ba chỗ phân loại terminal, giữ chữ ký parser).
- Cập nhật `docs/guide/developer-guide.tex`: cách đọc ứng viên, nghĩa mới của `nodes` và tóm
  tắt, giới hạn đã biết (phối cảnh winrate GTP theo cấu hình engine, GTP không có drawrate).
- Làm sau khi CORE-06, GOMO-07, GTP-07 xong.

## Don't

- Không đổi mã nguồn.

## Done-when

Features `DOC-04-*` trong `docs/features.json`.

## Evidence

## Links

`features/detailed-search-analysis/planning.md`
