import Foundation
import Combine

enum DataSourceKind: String, CaseIterable, Identifiable {
    case simulated, esp32

    var id: String { rawValue }
    var title: String { self == .simulated ? "Giả lập" : "ESP32 (BLE)" }
    var shortTitle: String { self == .simulated ? "SIM" : "BLE" }
}

enum DataSourceStatus: Equatable {
    case idle
    case searching
    case connected(String)
    case disconnected
    case error(String)

    var text: String {
        switch self {
        case .idle: return "Chưa kết nối"
        case .searching: return "Đang tìm ESP32…"
        case .connected(let name): return name
        case .disconnected: return "Mất kết nối – đang thử lại"
        case .error(let message): return message
        }
    }

    var isConnected: Bool {
        if case .connected = self { return true }
        return false
    }
}

/// Mọi nguồn dữ liệu (giả lập, ESP32…) đều tuân theo giao thức này.
/// Các callback luôn được gọi trên main thread.
protocol VehicleDataSource: AnyObject {
    var onUpdate: ((VehicleState) -> Void)? { get set }
    var onStatus: ((DataSourceStatus) -> Void)? { get set }
    func start()
    func stop()
    func setMode(_ mode: DriveMode)
}

/// Kho dữ liệu dùng chung cho màn hình iPhone và CarPlay.
final class VehicleStore: ObservableObject {
    static let shared = VehicleStore()

    @Published private(set) var state = VehicleState.demo
    @Published private(set) var status: DataSourceStatus = .idle
    @Published private(set) var sourceKind: DataSourceKind
    @Published var carPlayConnected = false

    private var source: VehicleDataSource?
    private var isStarted = false
    private static let sourceKey = "dataSource"

    private init() {
        let saved = UserDefaults.standard.string(forKey: Self.sourceKey) ?? ""
        sourceKind = DataSourceKind(rawValue: saved) ?? .simulated
    }

    func start() {
        guard !isStarted else { return }
        isStarted = true
        activate(sourceKind)
    }

    func setSource(_ kind: DataSourceKind) {
        guard kind != sourceKind else { return }
        UserDefaults.standard.set(kind.rawValue, forKey: Self.sourceKey)
        sourceKind = kind
        activate(kind)
    }

    func reconnect() {
        activate(sourceKind)
    }

    func setMode(_ mode: DriveMode) {
        state.mode = mode
        source?.setMode(mode)
    }

    func cycleMode() {
        setMode(state.mode.next)
    }

    private func activate(_ kind: DataSourceKind) {
        if let old = source {
            old.onUpdate = nil
            old.onStatus = nil
            old.stop()
        }
        let newSource: VehicleDataSource
        switch kind {
        case .simulated: newSource = SimulatedDataSource(initial: state)
        case .esp32: newSource = ESP32BLEDataSource(initial: state)
        }
        newSource.onUpdate = { [weak self] newState in self?.state = newState }
        newSource.onStatus = { [weak self] newStatus in self?.status = newStatus }
        source = newSource
        newSource.start()
    }
}
