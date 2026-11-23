---
id: extensible-engine-commands
type: planning
size: L
status: approved
owner: claude
created: 2026-11-23
supersedes: []
links: [engine-protocol-port, detailed-search-analysis]
---
# Cho ứng dụng mở rộng lệnh, luật, cấu trúc bàn và đầu ra của engine

## Goal

Một ứng dụng có engine riêng (ví dụ Rapfi V2/V3 với `YXANALZ`, `INFO VCTHREAD`, WALL, Torus,
Portal, luật Caro) dùng được PyGomo mà không phải sửa thư viện: gửi lệnh và tuỳ chọn mới, nhận
kết quả có kiểu, khai báo luật và cấu trúc bàn ngoài những gì có sẵn, và tiếp tục dùng
`ProcessTransport`, `DeadlineEngine`, `blocking_search`.

Hôm nay có năm chỗ chặn (đọc từ `include/` ngày 2026-11-23):

1. Lệnh đi ra chỉ có `EnginePort::sendRaw(line)`: không có kiểu trả về, không gắn trả lời với
   lệnh, "nothing is promised".
2. Parser là hàm tự do `parseGomocupLine`, `GomocupEngine` gọi cứng nó (`final`, `onLine`
   private). Muốn đọc đầu ra riêng phải viết lại cả adapter.
3. `Rules` dùng hai enum đóng (`BoardRule`, `OpeningRule`). Caro không biểu diễn được:
   `supports()` trả `false`.
4. `SearchEvent` là `std::variant` đóng. Đầu ra lạ chỉ còn là `MessageEvent` / `UnparsedEvent`
   (text thô), ứng dụng tự parse.
5. `Position` chỉ biết chiều rộng, chiều cao và danh sách nước đi. Không có ô bị vô hiệu (WALL),
   cạnh nối vòng (Torus) hay cặp ô nối nhau (Portal). Chủ repo xác nhận (2026-11-23): WALL biến
   một ô thành ô không đặt được quân; WALL/Torus đổi cấu trúc bàn; danh sách nước đi của
   `Position` không đổi.

## Findings từ mã Rapfi (2026-11-23)

Đọc `command/gomocup.cpp` của hai bản. Thay phần lớn spike EXT-00. Số dòng theo bản đã đọc.

| Cái tên | Thực chất ở engine | Hệ quả cho thiết kế |
|---|---|---|
| `VC2` | Không phải lệnh: là giá trị của tuỳ chọn `INFO VCTHREAD 0..3` (V2 `:271-290`). Engine trả `MESSAGE VCN Mode enabled: ...`. | Là một lệnh không chờ trả lời, tham số số nguyên. |
| `YXANALZ` | Lệnh nhiều dòng: `YXANALZ`, các `x,y`, `DONE` (V2 `:705-760`). Sau đó engine chạy một search thật trên danh sách nước, in các dòng MultiPV, rồi in nước đi cuối. | Là một search (kết thúc bằng sự kiện kết thúc thường), không phải lệnh độc lập. Cần Busy và sink như `startSearch`. |
| Caro | Id luật 8: `INFO RULE 8` (V3 `:235`). Chỉ khai báo được với khai mở tự do; `SWAP2BOARD` trả `ERROR` (V3 `:981`). | Một luật ở `Rules::extensions`; encoder đổi sang id. |
| WALL | `INFO WALL x,y`, chỉ sau `START` (V3 `:449-470`); `START` xoá tường. Khối `BOARD`/`YXBOARD` có thể mang mục WALL (`side 3`), không tính là một nước (`:767-845`). | Tường đi cùng vị trí, không phải luật. Encoder gửi sau `START`. |
| Torus | `INFO TORUS 0\|1`, một giá trị bật cả hai trục (V3 `:535-580`). Từ chối nếu bàn nhỏ, Renju, hoặc portal quá gần. | `Wrap` ba giá trị là thừa: dùng `bool torus`. Việc từ chối là của engine. |
| Portal | `INFO YXPORTAL x,y x,y` (cặp ô); không hỗ trợ bàn chữ nhật (V3 `:495-530`). | Cặp toạ độ có kiểu: không là chuỗi hay số. |
| Bàn chữ nhật | `RECTSTART w,h` (V3 `:678-720`), lệnh của giao thức Piskvork. Chế độ tọa độ `FLIPY_X` ảnh hưởng w,h. | `GomocupEngine` gửi `RECTSTART` thay vì `START n`. |
| `BOARD`, `YXBOARD`, `SWAP2BOARD`, `TURN`, `YXBLOCK` | Họ lệnh "gửi vị trí", mỗi lệnh một khối `x,y,c` kết thúc bằng `DONE` (V3 `:748-870`). `c`: 1 = bên đang đi, 2 = bên kia, 3 = tường. `BOARD` bắt đầu search, `YXBOARD` chỉ đặt vị trí, `SWAP2BOARD` dùng cho khai mở swap2, `TURN x,y` thêm một nước. Khối là **nguồn tin duy nhất về tường**: khối không có `c=3` xoá hết tường. `GomocupEngine` hôm nay cứng một kịch bản (`START`, `INFO rule`, `INFO ...`, `BEGIN` hoặc `BOARD..DONE`, `gomocup_engine.cpp:50-72`). | Cách gửi một yêu cầu là việc của engine, không phải của thư viện. Cần một hook ghi đè toàn bộ kịch bản (không chỉ thêm dòng), cùng bộ dựng khối dùng lại được. |
| Dừng | `STOP`/`YXSTOP` được đọc kể cả khi đang nghĩ; mọi lệnh khác bị **bỏ im lặng** khi `thinking` (V3 `:1127-1128`). Engine in nước hiện có rồi kết thúc. | `force` phải đợi sự kiện kết thúc rồi mới gửi lệnh mới; gửi sớm sẽ mất lệnh. |

Với KataGomo và GTP: `stop` không đọc được trong lúc search (AUD-02, `Gom2026@9a0e65c`), nên
`force` ở hai adapter này đợi search kết thúc tự nhiên.

## Non-goals

- Thêm giao thức mới (đã có Gomocup, GTP, KataGomo). Tài liệu hoá cách viết adapter riêng là
  việc của todo cuối.
- Cài sẵn lệnh của Rapfi trong thư viện. Thư viện cung cấp chỗ cắm; lệnh Rapfi nằm ở ứng dụng
  hoặc ví dụ.
- Core tính đường thắng, láng giềng hay hợp lệ của luật. Core chỉ lưu cấu trúc bàn như dữ liệu;
  engine mới hiểu Torus hay WALL có nghĩa gì với đường năm, và tự từ chối khi không hợp lệ.
- Đổi chữ ký `parseGomocupLine`, `parseGtpAnalyzeLine`, `EnginePort` đang có.

## Constraints

- Core giữ trung lập giao thức và không include I/O, thread, process, protocol
  (`scripts/check-core-deps.sh`). Mọi kiểu mới trong core là dữ liệu thuần.
- Không phá API public: mã hiện có biên dịch và chạy y nguyên (thành viên mới ở cuối struct,
  tham số mới có mặc định, enumerator mới ở cuối enum).
- ADR-01: bốn chỗ coi "mọi kiểu khác Info/Message/Unparsed" là sự kiện kết thúc search
  (`src/engine/blocking_search.cpp:20`, `src/engine/deadline_engine.cpp:71`,
  `src/katagomo/katagomo_engine.cpp:108`, `src/gomocup/gomocup_engine.cpp:121`). Thêm kiểu vào
  `SearchEvent` mà chưa sửa các chỗ này sẽ kết thúc search ở dòng đầu tiên. Hướng D phải đi
  sau một bước gom điều kiện này về một hàm.
- Thêm `PositionError::Blocked` ở cuối enum: mọi `switch` trên `PositionError` trong repo phải
  được rà (EXT-06 liệt kê chúng).
- Rapfi bỏ im lặng lệnh gửi khi đang nghĩ (xem Findings): không bao giờ gửi lệnh mới trước sự
  kiện kết thúc của search đang chạy.
- Tọa độ với `coord_conversion_mode` khác `none` chưa có fixture nào (handoff); `RECTSTART` kế
  thừa khoảng trống đó.
- Mỗi todo tách ra tối đa cỡ M và để build xanh sau từng todo (`module-architecture.md`).

## Options

Năm hướng bổ trợ nhau. Mỗi hướng có phương án chọn và phương án bị loại.

### A. Lệnh có kiểu (`CommandPort`)

1. **Giao diện riêng `CommandPort`** (chọn): `execute(const Command&, EventSink&, CommandOptions)`.
   Adapter nào hỗ trợ thì implement thêm. `EnginePort` không đổi. `Command` là dữ liệu: tên,
   các dòng cần gửi, tham số có kiểu (`Value`, xem phần Decision), tuỳ chọn một khối nhiều
   dòng (`block`, kèm `terminator` như `DONE`, ví dụ khối toạ độ của `YXANALZ`), và **cách hoàn tất** (đã quyết
   từ Findings, vì ba lệnh Rapfi có ba kiểu hoàn tất khác nhau):
   - `None`: xong khi dòng đã được ghi (ví dụ `INFO VCTHREAD 2`).
   - `Search`: là một search, kết thúc bằng sự kiện kết thúc thường (ví dụ `YXANALZ`). Chịu
     cùng luật `Busy` với `startSearch`.
   - `Until(predicate)`: kết thúc ở sự kiện đầu tiên mà `predicate` trả đúng; sự kiện đó vẫn
     đến sink (ví dụ lệnh in vài dòng rồi dừng).
   - Mặc định: đang có search thì trả `Busy`, không gửi gì.
   - `CommandOptions::force` (chủ repo chọn, 2026-11-23): gọi `stop()` (Gomocup gửi `YXSTOP`),
     đợi sự kiện kết thúc của search cũ (nó vẫn đến sink cũ, output không mất), rồi mới gửi
     lệnh mới. `execute` trả ngay: lệnh nằm trong hàng đợi của adapter và được gửi từ luồng đọc
     sau khi sự kiện kết thúc đã được giao. Chỉ một lệnh chờ; lệnh thứ hai trả `Busy`.
   - GTP và KataGomo có `stop()` rỗng: `force` đợi search kết thúc tự nhiên, bị chặn bởi
     `DeadlineEngine` nếu được bọc. Ghi trong doc comment.
2. Thêm `execute` vào `EnginePort`: buộc mọi adapter và engine giả trong test implement, và
   phá `DeadlineEngine`. Loại.
3. Giữ `sendRaw`: đúng là hiện trạng. Loại.

### B. Parser cắm được (`LineParser`)

1. **Adapter nhận `LineParser` qua constructor, mặc định là parser sẵn có** (chọn; chủ repo
   duyệt 2026-11-23), áp dụng cho `GomocupEngine`, `GtpEngine`, `KataGomoEngine`.
   `LineParser = std::function<SearchEvent(std::string_view)>`, kèm `chainParsers(custom,
   fallback)`: **parser của ứng dụng chạy trước**; nếu trả `UnparsedEvent` thì chạy parser mặc
   định. GTP: hook nhận từng dòng của thân trả lời, không thay logic ghép trả lời với lệnh.
2. Bỏ `final` và để ứng dụng kế thừa: lộ `onLine` và khoá nội bộ. Loại.
3. Parser là tham số template của adapter: chuyển cả adapter vào header. Loại.

### C. Luật mở rộng (`Rules::extensions`)

1. **`std::map<std::string, Value> extensions` ở cuối `Rules`** (chọn), cho luật như Caro. Khoá
   lạ làm `supports()` trả `false` ở adapter mặc định (không âm thầm bỏ qua). Enum cũ không đổi.
2. `BoardRule::Custom` kèm tên: lẫn hai tầng, mọi `switch` hiện có phải xử lý nhánh mới. Loại.
3. `Rules` thành `std::variant`: phá mọi chỗ dùng. Loại.

### D. Sự kiện tuỳ biến (`CustomEvent`)

1. **`CustomEvent{tag, fields}` trong `SearchEvent`, không kết thúc search** (chọn), sau khi có
   `isTerminal(const SearchEvent&)` trong core thay cho bốn chỗ lặp điều kiện. Test bảo đảm
   `CustomEvent` không kết thúc search ở cả bốn nơi. `fields` là danh sách cặp khoá-`Value`.
2. Dùng `MessageEvent` mang text: ứng dụng parse lại, không có kiểu. Loại.
3. Sự kiện kết thúc tuỳ biến: nghĩa kết thúc chỉ do adapter quyết định (hoàn tất `Until` là
   cơ chế riêng ở hướng A). Loại.

### E. Cấu trúc bàn (`BoardLayout`)

Chủ repo xác nhận: WALL và Torus là cấu trúc bàn, không phải luật; nước đi không đổi.

1. **Kiểu core `BoardLayout` gắn vào `Position`** (chọn):
   ```cpp
   struct BoardLayout {
       bool                                  torus{false};    ///< Both edges wrap (INFO TORUS 1).
       std::vector<Coord>                    blocked{};       ///< WALL: cells that take no stone.
       std::vector<std::pair<Coord, Coord>>  portals{};       ///< Linked cell pairs.
       std::map<std::string, Value>          extensions{};    ///< Structure the core does not model.
   };
   ```
   `Position::create(w, h, layout = {})`. `play()` trả `PositionError::Blocked` cho ô WALL.
   `moves()` không đổi. `operator==` so cả `layout`. Ô WALL hoặc portal ngoài bàn là
   `PositionError::OutOfBounds` ở `create`. Core không kiểm tra điều engine kiểm tra (Torus cần
   bàn đủ lớn, Portal không dùng trên bàn chữ nhật, Renju không có trên Torus): engine từ chối
   bằng `ERROR`.
2. `Wrap {None, Horizontal, Vertical, Both}`: Rapfi chỉ có một công tắc hai trục. Loại.
3. WALL như một quân trong `moves()`: làm sai `sideToMove()` (đếm theo số nước) và trái yêu
   cầu "danh sách nước đi không đổi". Loại.
4. WALL trong `Rules::extensions`: sai tầng (cấu trúc của một vị trí, không phải luật của ván)
   và `Position` vẫn cho đặt quân lên WALL. Loại.

### F. Dựng yêu cầu do ứng dụng định nghĩa (`RequestEncoder`)

Chủ repo (2026-11-23): `BOARD`, `YXBOARD`... phụ thuộc engine do người dùng định nghĩa; thư viện
phải cho họ tuỳ chỉnh. C và E cũng cần cùng một chỗ ra. Findings cho thấy chỉ "thêm dòng sau
`START`" là không đủ: tường nằm trong khối `BOARD`, `YXBOARD` không bắt đầu search, `SWAP2BOARD`
khác `BOARD`.

1. **`RequestEncoder` thay toàn bộ kịch bản gửi của một lần `startSearch`** (chọn):
   ```cpp
   struct SearchRequest { Rules rules; Position position; Limits limits; };
   using RequestEncoder = std::function<std::expected<std::vector<std::string>, EngineError>(
       const SearchRequest&)>;
   ```
   Adapter nhận `RequestEncoder` qua constructor, mặc định là kịch bản hiện tại của nó, nên
   không đặt gì thì hành vi không đổi. Lỗi trả về là `EngineError::Unsupported` (không gửi gì).
   Để ứng dụng chỉ đổi một phần thay vì viết lại, thư viện công khai các bộ dựng của kịch bản
   mặc định trong namespace `pygomo::encode` (mỗi adapter một nhóm): `defaultGomocupRequest(req)`,
   và các khối nhỏ như `boardBlock(command, moves, extraEntries)` (dòng lệnh, các dòng `x,y,c`,
   `DONE`). Ứng dụng gọi bộ dựng mặc định rồi chèn `INFO WALL`, đổi `BOARD` thành `YXBOARD`,
   hoặc tự viết hết.
   - Không đặt encoder thì adapter từ chối `Rules::extensions` và `BoardLayout` khác mặc định
     bằng `Unsupported`, không gửi gì (không bỏ qua im lặng).
   - Bộ dựng chạy dưới khoá của adapter như phần dựng dòng hiện nay, và không được gọi vào
     adapter (không gọi ngược `startSearch`).
2. `SetupEncoder` chỉ thêm dòng sau `START`: không đặt được tường vào khối `BOARD` và không đổi
   được lệnh gửi vị trí. Loại (đề xuất ở bản trước).
3. Mỗi biến thể (`YXBOARD`, `SWAP2BOARD`) là một cờ trong `Options` của adapter: tăng cờ theo
   mỗi engine mới. Loại.

Lệnh kiểu `BOARD`/`YXBLOCK` đứng riêng (ngoài search) cũng dùng chung bộ dựng khối: `Command` có
`block` (danh sách `Value`) và `terminator` (xem hướng A).

### Bàn chữ nhật

`Position` đã cho chữ nhật; chỉ adapter từ chối. Quyết (chủ repo, 2026-11-23): cho phép.
- `GomocupEngine`: bàn không vuông gửi `RECTSTART w,h` thay cho `START n`.
- `GtpEngine`: đã gửi `boardsize w h`; bỏ kiểm tra `startSearch` nếu có (cần xác nhận khi làm).
- `KataGomoEngine`: giữ `Unsupported` cho bàn không vuông, vì `gom` chỉ có `START n` (AUD-02,
  đọc từ nguồn). Ghi lý do trong doc comment.
- Engine không biết `RECTSTART` chỉ in `ERROR` (GOMO-04: `ERROR` không kết thúc search), search
  treo tới khi `DeadlineEngine` hết hạn; đã nêu trong doc comment.

## Decision

Chủ repo duyệt 2026-11-23 (chọn `portals` là trường có kiểu riêng của `BoardLayout`). Quyết: phương án đầu của A, B, C, D, E, F và bàn chữ
nhật, theo thứ tự để mỗi bước giữ build xanh. Thứ tự phụ thuộc:
EXT-00 → EXT-01 → EXT-02 → (EXT-03, EXT-05, EXT-06 độc lập nhau) → EXT-04, EXT-07 → EXT-08 →
EXT-09 → EXT-10 → EXT-11.

Đã quyết (chủ repo, 2026-11-23): **`Value` có kiểu**, không dùng chuỗi:
```cpp
using Value = std::variant<bool, std::int64_t, double, std::string, Coord, Move,
                           std::vector<Coord>, std::vector<Move>>;
```
`Coord` và `Move` đã có trong core (`coord.hpp`), nên khối `BOARD` (`x,y,c`) và danh sách
`YXANALZ` biểu diễn được bằng kiểu có sẵn. Tập kiểu đóng, nhưng không là giới hạn cứng: ứng dụng
cần hình dạng khác thì dựng dòng trực tiếp trong `RequestEncoder` hoặc `CommandSpec`, không qua
`Value`. Cách định dạng một `Value` ra dòng (bool là `0/1` hay `true`, số thực mấy chữ số) do
encoder của engine quyết, thư viện chỉ cung cấp `format(Value)` mặc định cho từng kiểu.

| Todo | Nội dung | Cỡ |
|---|---|---|
| EXT-00 | Ghi Findings ở trên thành audit `AUD-08` (một audit bất biến), thêm đọc `ABOUT`/`YXSHOWINFO` nếu cần để chốt `Until`. Không giữ mã. | S |
| EXT-01 | `isTerminal(const SearchEvent&)` trong core; thay bốn chỗ lặp điều kiện. Không đổi hành vi. | S |
| EXT-02 | `Value`, `CustomEvent` vào `SearchEvent`; test không kết thúc search ở bốn nơi. | M |
| EXT-03 | `LineParser` + `chainParsers`; `GomocupEngine` nhận parser. | M |
| EXT-04 | Cùng hook cho `GtpEngine` và `KataGomoEngine`. | M |
| EXT-05 | `Rules::extensions`, `validate`; adapter từ chối khoá lạ. | M |
| EXT-06 | `BoardLayout`, `Position::create(w, h, layout)`, `PositionError::Blocked`; rà mọi `switch` trên `PositionError`; test `play` vào WALL, `operator==`. | M |
| EXT-07 | `RequestEncoder` + `SearchRequest` + `pygomo::encode` (kịch bản mặc định và `boardBlock`) cho `GomocupEngine`; mặc định không đổi hành vi (các test hiện có không sửa); không đặt encoder thì từ chối `extensions` và layout khác mặc định. | M |
| EXT-08 | Cùng `RequestEncoder` cho `GtpEngine` và `KataGomoEngine`. | M |
| EXT-09 | Bàn chữ nhật: `RECTSTART` ở `GomocupEngine`, bỏ kiểm tra vuông ở `GtpEngine`, giữ `KataGomoEngine`; fixture và test live với Rapfi V3. | M |
| EXT-10 | `CommandPort`, `Command` (có `block`), hoàn tất `None`/`Search`/`Until`, `CommandOptions::force`; `GomocupEngine` implement (`YXSTOP` rồi đợi); `DeadlineEngine` chuyển tiếp; GTP/KataGomo implement với `force` đợi hết search. | M |
| EXT-11 | ADR (đổi hợp đồng `SearchEvent`, `Rules`, `Position`), hướng dẫn "viết lệnh, luật, cấu trúc bàn riêng", cập nhật `architecture.md`. | M |

Acceptance chung: trong `tests/`, một adapter giả lập Rapfi khai `INFO VCTHREAD 2`, `YXANALZ`,
Caro (`Rules::extensions`), WALL, Torus, Portal (`BoardLayout`) và bàn chữ nhật, chạy qua
transport giả, không sửa thư viện. Một test live (bỏ qua nếu không có binary) chạy với Rapfi
V2 và V3. Test `force`: search đang chạy, `execute(force)` làm sự kiện kết thúc đến sink cũ
trước, lệnh mới chỉ được gửi sau đó; không có `force` thì trả `Busy` và không gửi gì.

