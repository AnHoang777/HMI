import SwiftUI

enum DriveMode: String, Codable, CaseIterable, Identifiable {
    case eco, normal, sport

    var id: String { rawValue }

    var title: String {
        switch self {
        case .eco: return "ECO"
        case .normal: return "NORMAL"
        case .sport: return "SPORT"
        }
    }

    var icon: String {
        switch self {
        case .eco: return "leaf.fill"
        case .normal: return "gauge.medium"
        case .sport: return "flame.fill"
        }
    }

    var color: Color {
        switch self {
        case .eco: return Theme.green
        case .normal: return Theme.cyan
        case .sport: return Theme.orange
        }
    }

    /// Tốc độ tối đa (km/h) – dùng cho giả lập
    var maxSpeed: Double {
        switch self {
        case .eco: return 45
        case .normal: return 55
        case .sport: return 70
        }
    }

    /// Gia tốc tối đa (km/h mỗi giây) – dùng cho giả lập
    var maxAccel: Double {
        switch self {
        case .eco: return 5
        case .normal: return 8
        case .sport: return 12
        }
    }

    /// Mức tiêu thụ trung bình (Wh/km) – dùng để ước tính quãng đường còn lại
    var consumptionWhPerKm: Double {
        switch self {
        case .eco: return 30
        case .normal: return 34
        case .sport: return 40
        }
    }

    var next: DriveMode {
        let all = DriveMode.allCases
        return all[(all.firstIndex(of: self)! + 1) % all.count]
    }
}

struct VehicleState: Equatable {
    var speed: Double          // km/h
    var soc: Double            // %
    var voltage: Double        // V
    var current: Double        // A (âm = sạc / phanh tái sinh)
    var odo: Double            // km
    var trip: Double           // km
    var batteryTemp: Double    // °C
    var soh: Double            // %
    var range: Double          // km
    var mode: DriveMode

    var power: Double { voltage * current / 1000 }   // kW

    static let demo = VehicleState(speed: 32, soc: 91, voltage: 66.9, current: 0,
                                   odo: 12_450, trip: 36.8, batteryTemp: 28,
                                   soh: 100, range: 72, mode: .eco)
}

// MARK: - Chuỗi hiển thị (dùng chung cho iPhone và CarPlay)

extension VehicleState {
    var speedText: String { "\(Int(speed.rounded()))" }
    var socText: String { "\(Int(soc.rounded()))" }
    var voltageText: String { String(format: "%.1f", voltage) }
    var currentText: String { String(format: "%.1f", current) }
    var powerText: String { String(format: "%.1f", power) }
    var odoText: String {
        Int(odo).formatted(.number.locale(Locale(identifier: "en_US")))
    }
    var tripText: String { String(format: "%.1f", trip) }
    var tempText: String { "\(Int(batteryTemp.rounded()))" }
    var sohText: String { "\(Int(soh.rounded()))" }
    var rangeText: String { "\(Int(range.rounded()))" }
}
