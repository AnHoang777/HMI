import ActivityKit
import SwiftUI

/// Dữ liệu Live Activity – dùng chung giữa app và widget extension.
/// File này chỉ được dùng ActivityKit/SwiftUI (không tham chiếu kiểu nào khác của app).
struct DashboardAttributes: ActivityAttributes {
    struct ContentState: Codable, Hashable {
        var speed: Int          // km/h
        var soc: Int            // %
        var range: Int          // km
        var mode: String        // ECO / NORMAL / SPORT
        var voltage: Double     // V
        var current: Double     // A
        var batteryTemp: Int    // °C
        var source: String      // SIM / BLE
        var connected: Bool
    }

    var vehicleName: String
}

/// Bảng màu neon dùng trong Live Activity
enum Neon {
    static let cyan = Color(red: 0.098, green: 0.878, blue: 1.0)
    static let blue = Color(red: 0.118, green: 0.420, blue: 1.0)
    static let green = Color(red: 0.239, green: 0.949, blue: 0.416)
    static let orange = Color(red: 1.0, green: 0.624, blue: 0.110)
    static let red = Color(red: 1.0, green: 0.231, blue: 0.231)
    static let background = Color(red: 0.035, green: 0.067, blue: 0.125)

    static func socColor(_ soc: Int) -> Color {
        soc > 30 ? green : (soc > 15 ? orange : red)
    }

    static func modeColor(_ mode: String) -> Color {
        switch mode {
        case "SPORT": return orange
        case "NORMAL": return cyan
        default: return green
        }
    }
}
