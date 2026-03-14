# Task features

MAX98357 đã được kết nối ở chân 5, 7, 8 = LRC, DIN, BKC
- [ ] Menu cài đặt sẽ bao gồm những cái sau (config.cpp, config.h)
  - [ ] Tắt bật wifi (wifi nếu đang kết nối thì hiển thị tên ví dụ (Wifi (AP_TEST)))
  - [ ] Tắt bật Bluetooth (hiện đã kết nối nếu đã kết nối chronos)
  - [ ] Điều chỉnh độ sáng màn hình
  - [ ] Điều chỉnh giờ bật / tắt màn hình 
  - [ ] Điều chỉnh thời gian làm mới thời tiết (weatherInterval)
  - [ ] Điều chỉnh ngôn ngữ (Hiện có tiếng anh và tiếng việt)
  - [ ] Các cài đặt cho nhắc lịch uống nước
  - [ ] Điều chỉnh GMT offset
  - [ ] Điều chỉnh chế độ hiển thị (hiện tại đang là chế độ 12h)
  - [ ] Điều chỉnh mochi speed divisor
  - [ ] Điều chỉnh mochiNegative
  - [ ] Điều chỉnh chế độ lật màn
  - [ ] Điều chỉnh negative màn hình

- Các hành động click được xử lý lại hoàn toàn như sau:
  - Route mặc định sẽ là DASAI_MOCHI
  - Khi ở màn hình DASAI_MOCHI hoặc CLOCK:
    - Nhấn 1 click (hiện đang tăng độ sáng) sẽ tắt màn hình
    - Double click sẽ mở đồng hồ nếu đang ở đồng hồ sẽ quay lại DASAI_MOCHI
    - Triple click sẽ mở menu cài đặt
  - Khi ở màn hình cài đặt:
    - Hãy thỏa mái sự dụng các sự kiện để tiện dụng điều chỉnh cài đặt

- [x] Dasai Mochi
- [ ] Hẹn giờ báo thức (có thể cấu hình trong menu hoặc web và cho phép nhiều báo thức)
      _Cài đặt và tính năng bao gồm_
  - [ ] Tùy chỉnh nhạc chuông
  - [ ] Có icon hiện báo thức
  - [ ] Cho phép chọn có lặp lại hay chỉ một lần
- [ ] Nhắc lịch uống nước (có âm thanh mặc định là drink!)
      _Các cài đặt_
  - [ ] Bật / tắt
  - [ ] Độ dài lời nhắc
  - [ ] Khoảng cách giữa mỗi lần nhắc (30p, 45p, 1h, 1.5h, 2h)
  - [ ] Khung giờ hoạt động (Start time, End time) (giờ này có độ ưu cao hơn cài đặt bật tắt màn hình nếu màn hình đang tắt thì bật màn hình và nhắc nhở sau khi người dùng xác nhận đã uống thì lại tắt màn)
  - [ ] Lượng nước tiêu thụ (1.2l, 1.5l, 2l, 2.5l, tùy chỉnh) lượng nước này sẽ làm cơ sở để xem có cần nhắc không nếu đã uống đủ thì thôi đây cũng là cơ sở để hiển thị xem lần nhắc này cần uống bao nhiêu nước
        _Tính năng bao gồm_
  - [ ] Cho màn OLED:
    - [ ] Popup nhỏ trên màn hình
    - [ ] Icon giọt nước xuất hiện ở góc + chớp nhẹ
    - [ ] Toàn màn hình “Drink!”
    - [ ] Hiện lần thứ mấy uống nước và lần này cần uống bao nhiêu hiện cả số lần bỏ lỡ nếu có
    - [ ] Phát âm thanh
  - [ ] Khi thông báo thì sẽ liên tục trong trong "Độ dài lời nhắc" trong thời gian đó nếu người dùng nhấn 1 click thì coi như là đã uống còn không sẽ ghi nhận đó là 1 lần bỏ lỡ và dừng báo chúng ta cũng sẽ có icon giọt nước xuất hiện ở góc + chớp nhẹ nếu có bỏ lỡ người dùng có thể đánh dấu đã hoàn thành lần bỏ lỡ trong menu cài đặt
  - Tính năng này cũng yêu cầu ghi nhật kí theo dạng jsonl
  ```jsonl
  {"ts": "2026-03-12T09:30:00", "type": "reminder_start"}
  {"ts": "2026-03-12T09:30:05", "type": "drink", "amount": 200}
  {"ts": "2026-03-12T10:30:00", "type": "miss"}
  {"ts": "2026-03-12T11:30:00", "type": "manual_fix_miss"}
  ```
- [ ] Xem các API của Chronos và thêm tính năng thông báo và chỉ báo pin

Hàm delay rất có ích khi hiển thị popup chúng ta có thể gọi hàm đó vẽ ra rồi delay sau khi delay loop() chạy sẽ tự clear