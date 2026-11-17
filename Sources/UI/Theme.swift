import SwiftUI

enum Brand {
    /// Tên hiển thị ở đầu màn hình
    static let name = "VINFAST"
}

enum Theme {
    static let cyan = Color(hex: 0x19E0FF)
    static let blue = Color(hex: 0x1E6BFF)
    static let green = Color(hex: 0x3DF26A)
    static let orange = Color(hex: 0xFF9F1C)
    static let red = Color(hex: 0xFF3B3B)
    static let screenBackground = Color(hex: 0x020409)

    static let neonGradient = LinearGradient(
        stops: [
            .init(color: .clear, location: 0),
            .init(color: blue, location: 0.15),
            .init(color: cyan, location: 0.4),
            .init(color: cyan, location: 0.6),
            .init(color: blue, location: 0.85),
            .init(color: .clear, location: 1),
        ],
        startPoint: .leading, endPoint: .trailing)

    static let iconGradient = LinearGradient(colors: [cyan, blue], startPoint: .top, endPoint: .bottom)
}

extension Color {
    init(hex: UInt32, opacity: Double = 1) {
        self.init(.sRGB,
                  red: Double((hex >> 16) & 0xFF) / 255,
                  green: Double((hex >> 8) & 0xFF) / 255,
                  blue: Double(hex & 0xFF) / 255,
                  opacity: opacity)
    }
}

extension Font {
    static func dash(_ size: CGFloat, _ weight: Font.Weight = .semibold) -> Font {
        .system(size: size, weight: weight, design: .rounded)
    }
}
