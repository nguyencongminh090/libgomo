---
id: detailed-search-analysis
type: planning
size: L
status: draft
owner: codex
created: 2026-11-22
supersedes: []
links: [engine-protocol-port]
---
# Giữ dữ liệu phân tích chi tiết từ Gomocup và GTP

## Goal

Cho ứng dụng truy cập hạng MultiPV của Gomocup và các ứng viên bổ sung trong một dòng
phân tích GTP, đồng thời tiếp tục nhận được kết quả tóm tắt khi engine chỉ gửi dữ liệu đơn giản.

Probe của đợt review xác nhận `MESSAGE multipv 1 ev 7 pv 1,1` và
`MESSAGE multipv 2 ev 7 pv 1,1` cho cùng một `InfoEvent`. Với GTP,
`info move H8 visits 120 pv H8` cũng cho cùng sự kiện khi nối thêm
`info move G8 visits 50 pv G8`. Sau khi chuyển thành `InfoEvent`, ứng dụng không thể
lấy lại phần dữ liệu đã bị bỏ.

## Non-goals

- Thêm giao thức, adapter hoặc quy tắc mới cho bàn cờ.
- Thay đổi cách bắt đầu, dừng hoặc kết thúc một search.

## Constraints

- Giữ core trung lập về giao thức và độc lập với I/O, thread và adapter.
- Parser tiếp tục chạy được trên các bản ghi đầu ra mà không cần khởi động engine.
- Đánh giá tính tương thích với người dùng hiện tại của `InfoEvent` và `SearchEvent`.
  Thêm kiểu vào `SearchEvent` có thể làm hỏng các `std::visit` xử lý đủ mọi kiểu hiện có.
- Các ca chấp nhận cần chứng minh hai cặp đầu vào ở trên cho kết quả phân biệt được qua
  API đã chọn. Kiểm thử cả đầu ra đơn giản và dòng chưa nhận dạng.

## Options

1. Thêm dữ liệu ứng viên có thứ hạng vào báo cáo phân tích. Ứng dụng có thể hiển thị
   nhiều ứng viên bằng một mô hình chung. Cần xác định ý nghĩa của hạng khi engine thiếu
   thông tin và cách giữ các trường tóm tắt hiện có.
2. Cho ứng dụng đăng ký nhận dòng gốc của báo cáo đã nhận dạng. Cách này giữ chi tiết cho
   chẩn đoán và xử lý riêng. Ứng dụng muốn đọc ứng viên vẫn cần hiểu cú pháp từng engine.

## Decision

Chờ chủ repo chốt yêu cầu và phương án API. Sau khi planning được duyệt, tách việc thành
các todo tối đa M và dùng fixture của từng giao thức để kiểm chứng.

## Open questions

- Ứng dụng cần danh sách ứng viên có kiểu, dòng gốc để chẩn đoán, hay cả hai?
- Nếu dùng mô hình có kiểu, trường nào ngoài hạng và PV cần hỗ trợ ở lần đầu, và biểu diễn
  thế nào khi engine không cung cấp hạng?

## References

[InfoEvent](../../include/pygomo/core/search_event.hpp) ·
[Parser Gomocup](../../src/parse/gomocup_parser.cpp) ·
[Parser GTP](../../src/parse/gtp_parser.cpp)
