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
