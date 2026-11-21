# HMI – Bảng đồng hồ xe máy điện (iOS + CarPlay)

## Cấu trúc

| Thư mục | Nội dung |
|---|---|
| `Sources/App` | AppDelegate, scene iPhone |
| `Sources/UI` | Dashboard SwiftUI (vòng tốc độ neon, pin, ODO/TRIP…), màn hình cài đặt |
| `Sources/CarPlay` | Scene CarPlay (template chuẩn hoặc vẽ nguyên dashboard) |
| `Sources/Model` | `VehicleState`, `VehicleStore` (dữ liệu dùng chung iPhone + CarPlay) |
| `Sources/Data` | Nguồn dữ liệu: `SimulatedDataSource` (giả lập), `ESP32BLEDataSource` (BLE) |
| `project.yml` | Cấu hình XcodeGen (sinh ra `HMI.xcodeproj`) |

Hai target:

- **HMI** – CarPlay dùng template thông tin chuẩn của Apple (entitlement `carplay-driving-task`). Có thể xin Apple duyệt.
- **HMIDemo** – CarPlay vẽ nguyên dashboard (entitlement `carplay-maps`, cờ `CARPLAY_FULL_DASHBOARD`). Chỉ để demo.

## Build bằng GitHub Actions (không cần Mac)

Mỗi lần push lên nhánh `main`, workflow `.github/workflows/build.yml` sẽ build và xuất artifact `HMI-builds`, gồm:

- `HMI-unsigned.ipa`, `HMIDemo-unsigned.ipa`: cài lên iPhone bằng Sideloadly hoặc AltStore với Apple ID của bạn.
- `HMI-simulator.zip`: bản chạy trên iOS Simulator (cần Mac).

## Chạy trên Mac (nếu có)

```bash
brew install xcodegen
xcodegen generate
open HMI.xcodeproj
```

Để xem CarPlay: chạy app trên Simulator, sau đó vào menu **I/O → External Displays → CarPlay**.

## Giao thức ESP32 (BLE – Nordic UART Service)

- Service `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- TX (notify, ESP32 → iPhone) `6E400003-…`: mỗi dòng là một JSON, kết thúc bằng `\n`:

```json
{"spd":32.5,"soc":91,"v":66.9,"a":12.3,"odo":12450.2,"trip":36.8,"t":28,"soh":100,"rng":72,"mode":"eco"}
```

- RX (write, iPhone → ESP32) `6E400002-…`: `{"cmd":"mode","value":"sport"}`

Trường nào không gửi thì app giữ giá trị cũ. Chọn nguồn dữ liệu trong app bằng nút lưới ở góc dưới bên trái.

## Đồng hồ ESP32 (Waveshare ESP32-S3-Touch-LCD-4.3)

Thư mục `esp32/` chứa firmware PlatformIO (Arduino + LVGL 8.3) vẽ cùng giao diện neon trên màn 800×480.

- `src/board.cpp`: driver màn RGB, cảm ứng GT911, IO expander CH422G
- `src/ui.cpp`: giao diện (thanh pin 5 vạch nhiều màu, không có CarPlay)
- `src/bms.cpp`: đọc BMS qua CAN 250 kbps (GPIO15 TX / GPIO16 RX, chỉ nghe, không gửi): SOC, điện áp, dòng điện, nhiệt độ pin – cùng bảng giải mã với firmware sạc `code_sac`
- `src/vcu.cpp`: đọc CAN của xe – 0x102 (P/READY, ECO/SPORT, chân chống), 0x201 (tốc độ /10 km/h, ODO /10 km, phanh). TRIP = ODO − mốc reset (lưu NVS), giữ lâu ô TRIP để reset.
- `src/vehicle.cpp`: dữ liệu giả lập, chỉ dùng khi `DEMO_SIMULATION = true` trong `main.cpp`

Chạm ô hình xe để bật/tắt ô số raw (0x102, 0x201) – dùng khi dò mã CAN.

Đấu dây CAN: CANH/CANL của BMS vào cổng CAN trên board (chung GND). Log BMS in ra Serial 115200 mỗi 2 giây.
- `tools/gen_fonts.sh`: tạo font tiếng Việt (Be Vietnam Pro) cho LVGL

GitHub Actions (`.github/workflows/esp32.yml`) build ra artifact `HMI-esp32`:

- `HMI-esp32-full.bin`: nạp ở địa chỉ **0x0** (bootloader + phân vùng + app)
- `HMI-esp32-app.bin`: chỉ phần app, nạp ở **0x10000** khi cập nhật

Nạp bằng trình duyệt Chrome/Edge: https://espressif.github.io/esptool-js/ → Connect → chọn file `HMI-esp32-full.bin`, địa chỉ `0x0` → Program.

Nguồn: Type-C 5V khi nạp/chạy thử, cổng DC 7–36V khi gắn lên xe (lấy 12V sau khoá, **không** đấu vào pin chính).
