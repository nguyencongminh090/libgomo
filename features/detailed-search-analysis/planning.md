---
id: detailed-search-analysis
type: planning
size: L
status: draft
owner: claude
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
- Dòng gốc của báo cáo trong `InfoEvent` (chủ repo chọn chỉ dữ liệu có kiểu; muốn chẩn đoán
  bằng dòng gốc thì mở planning riêng).
- Trường `prior`, `lcb`, `utility`, `seldepth`, `stdev` (để todo sau nếu cần).
- Dạng `MESSAGE Visits N Winrate P% ...` của KataGomo `gom` (hiện là `UnparsedEvent`).
- Gộp các dòng Gomocup thành snapshot: framework không đoán ranh giới giữa các lần lặp.

## Constraints

- Giữ core trung lập về giao thức và độc lập với I/O, thread và adapter.
- Parser tiếp tục chạy được trên các bản ghi đầu ra mà không cần khởi động engine.
- Không thêm kiểu vào `SearchEvent`. Ba chỗ coi mọi kiểu khác Info/Message/Unparsed là sự kiện
  kết thúc search: `src/engine/blocking_search.cpp:15`, `src/engine/deadline_engine.cpp:70`,
  `src/katagomo/katagomo_engine.cpp:106`. Một kiểu mới vẫn biên dịch được nhưng sẽ kết thúc
  search ở dòng ứng viên đầu tiên. Thêm thành viên ở cuối `InfoEvent` giữ được `operator==`
  mặc định và khởi tạo aggregate hiện có.
- Giữ chữ ký public `parseGomocupLine` và `parseGtpAnalyzeLine` (một dòng cho một sự kiện).
- Hai giao thức có hình dạng khác nhau, theo đầu ra thật:
  - Rapfi (`search/searchoutput.cpp:129-209`) in mỗi ứng viên một dòng:
    `(N) v | d-s | pv`, `(N) v (W w, D d, S s) | V n | SD sd | pv`,
    `depth d-s multipv N ev v n … tm t pv …` và `multipv N ev v w W d D stdev s v n seldepth sd
    n N … prior p pv …`. Hạng N bắt đầu từ 1. `V`/`v` là visits của ứng viên; `n`, `Node` và
    `Visit` trong dòng `Speed …` là tổng node của search.
  - KataGomo (`Gom2026@9a0e65c`, `cpp/command/gtp.cpp:560-672`) in mọi ứng viên trên một dòng.
    lz-analyze: `info move M visits n winrate W·10000 prior p lcb l order k pv …`.
    kata-analyze: thêm `utility`, `winrate` trong 0..1, `scoreMean`/`scoreLead` (KataGomo đặt
    giá trị drawrate ×100 vào đó, không theo nghĩa của KataGo). `order` bắt đầu từ 0.
    Phối cảnh winrate theo `reportAnalysisWinratesAs` (mặc định bên đi), framework không biết
    cấu hình của engine.
- Các ca chấp nhận chứng minh hai cặp đầu vào ở Goal cho kết quả phân biệt được qua API đã chọn.
  Kiểm thử cả đầu ra đơn giản và dòng chưa nhận dạng.

## Options

1. **`InfoEvent` thêm `std::vector<Candidate> candidates`** (chọn). GTP: một dòng cho một
   `InfoEvent` chứa đủ N ứng viên, giữ được ranh giới snapshot. Gomocup: một dòng có hạng cho
   vector một phần tử. Không đổi chữ ký parser.
2. `InfoEvent` thêm `std::optional<int> multipv`, mỗi ứng viên một `InfoEvent`. Gọn hơn, nhưng
   `parseGtpAnalyzeLine` phải trả nhiều sự kiện (đổi API public) và ứng dụng mất ranh giới
   snapshot của GTP.
3. Gắn dòng gốc vào báo cáo. Ứng dụng muốn đọc ứng viên vẫn phải hiểu cú pháp từng engine.
4. Kiểu mới trong `SearchEvent`. Loại: xem ràng buộc về sự kiện kết thúc.

## Decision

Chủ repo chọn (2026-11-22): chỉ dữ liệu có kiểu; phương án 1; trường tối thiểu cộng winrate và
drawrate. Chủ repo giao cho người lập planning quyết nghĩa của các trường tóm tắt; quyết định
bên dưới cần chủ repo xác nhận khi duyệt.

Kiểu mới trong core (`search_event.hpp`):

```cpp
struct Candidate {
    std::optional<int>          rank{};      ///< 1 = best; empty when the engine prints none.
    std::optional<Coord>        move{};      ///< Candidate move; empty for pass or not sent.
    std::optional<Score>        score{};     ///< Evaluation of this move.
    std::optional<std::int64_t> visits{};    ///< Visits or nodes spent on this move only.
    std::optional<double>       winrate{};   ///< 0..1, perspective as reported by the engine.
    std::optional<double>       drawrate{};  ///< 0..1.
    std::vector<Coord>          principalVariation{};
};
// InfoEvent gets, as its last member:
std::vector<Candidate> candidates{};  ///< Ranked candidates of this report; empty if none.
```

Quy tắc ánh xạ:

- `rank` chỉ lấy từ engine, không suy đoán: Gomocup `(N)` và `multipv N`; GTP `order k` cho
  `k + 1`. Entry GTP không có `order` có `rank` rỗng.
- Gomocup chỉ tạo `Candidate` khi dòng có hạng. Dòng một PV không hạng (`depth … ev … pv`,
  `Bestline`, `Speed …`) giữ `candidates` rỗng như hiện nay.
- GTP: mỗi entry `info` trong dòng thành một `Candidate`, theo thứ tự in. Một entry hỏng làm cả
  dòng thành `UnparsedEvent` như hiện nay.
- `move`: GTP `move`; Gomocup là nước đầu của PV.
- `winrate`/`drawrate`: Rapfi `W`/`D` và `w`/`d` (phần trăm) chia 100. GTP: entry có khóa
  `utility` là thang kata (0..1), không có là thang lz (chia 10000). Giá trị ngoài 0..1 bỏ trống.
  GTP không có drawrate chuẩn: không đọc `scoreLead`/`scoreMean` (nghĩa riêng của KataGomo).

Nghĩa các trường tóm tắt của `InfoEvent` (quyết định của người lập planning):

- `depth`, `elapsed`: của cả search, như hiện nay.
- `nodes`: chỉ là tổng node của search (`n`, `Node`, `Visit` của dòng `Speed`). Visits riêng của
  một ứng viên (`V`, `v`, GTP `visits`) chỉ nằm trong `Candidate::visits`.
- `score`, `principalVariation`: chỉ lấy từ dòng không hạng hoặc ứng viên hạng 1. Dòng hạng 2
  trở đi để trống hai trường này.

Lý do: khi đã có `candidates`, một bản tóm tắt chứa dữ liệu hạng 2 hoặc visits của một nước sẽ
đánh lừa ứng dụng chỉ đọc tóm tắt (tưởng là nước tốt nhất, tưởng là tổng node). Rapfi luôn in
tổng node trong dòng riêng, nên tóm tắt không mất thông tin.

Hệ quả cho test hiện có. Các kiểm tra sau được chuyển sang `candidates` (không xóa, không nới);
duyệt planning này là chấp thuận việc đổi chúng:

- `tests/parse/rapfi_format_test.cpp:51-55` (`(2) 34 | 5-6 | E5 B2`: score và PV sang hạng 2,
  `depth == 5` giữ) và `:57-59` (`V 1K`: `nodes == 1000` thành `visits` của ứng viên).
- `tests/parse/gomocup_parser_test.cpp:67-69` (`multipv 2`: PV sang ứng viên hạng 2).
- `tests/parse/gtp_parser_test.cpp:130` và `:143` (`nodes == 120`, `nodes == 33` thành `visits`).
- `tests/gtp/gtp_engine_test.cpp:271` (`nodes == 120` thành `visits`).

Tách việc sau khi duyệt (mỗi todo tối đa M, `planning: detailed-search-analysis`, feature và
`checked_by` ghi trong `docs/features.json`):

1. Core: `Candidate`, `InfoEvent::candidates`, doc comment, test so sánh và khởi tạo; cập nhật
   `docs/design/architecture.md`. Chưa đổi parser.
2. Gomocup: hạng, ứng viên, winrate/drawrate và quy tắc tóm tắt; fixture cho cả bốn dạng Rapfi
   và cặp probe `multipv 1`/`multipv 2`; chạy thật Rapfi với MultiPV.
3. GTP: mọi entry, `order`, visits, winrate hai thang, quy tắc tóm tắt; cặp probe `H8`/`G8`
   phân biệt được; test của `GtpEngine`.
4. ADR cho hình dạng `InfoEvent` (giao diện public của core) và cập nhật guide.

## Open questions

## References

[InfoEvent](../../include/pygomo/core/search_event.hpp) ·
[Parser Gomocup](../../src/parse/gomocup_parser.cpp) ·
[Parser GTP](../../src/parse/gtp_parser.cpp) ·
Rapfi `search/searchoutput.cpp` (repo ngoài, xem handoff) ·
KataGomo `Gom2026@9a0e65c` `cpp/command/gtp.cpp`, `cpp/configs/gtp_example.cfg:79-81`
