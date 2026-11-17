import ActivityKit
import Combine
import Foundation

/// Bật/cập nhật Live Activity "Bảng đồng hồ" (hiện trên CarPlay iOS 26, màn hình khoá, Dynamic Island).
/// iOS chỉ cho phép *bắt đầu* Live Activity khi app đang mở; sau đó app có thể cập nhật cả khi chạy nền.
final class LiveActivityManager {
    static let shared = LiveActivityManager()
    static let enabledKey = "liveActivityEnabled"

    private var activity: Activity<DashboardAttributes>?
    private var cancellable: AnyCancellable?
    private var lastState: DashboardAttributes.ContentState?

    var isEnabled: Bool {
        UserDefaults.standard.object(forKey: Self.enabledKey) as? Bool ?? true
    }

    func start(store: VehicleStore = .shared) {
        guard isEnabled, ActivityAuthorizationInfo().areActivitiesEnabled else { return }

        // Activity cũ đã bị người dùng tắt hoặc hết hạn → bỏ
        if let current = activity, current.activityState != .active {
            activity = nil
        }
        // Dùng lại activity còn sống từ lần chạy trước (nếu có)
        if activity == nil {
            activity = Activity<DashboardAttributes>.activities.first { $0.activityState == .active }
        }
        if activity == nil {
            let state = Self.contentState(store.state, status: store.status, kind: store.sourceKind)
            do {
                activity = try Activity.request(
                    attributes: DashboardAttributes(vehicleName: "HMI"),
                    content: ActivityContent(state: state, staleDate: nil),
                    pushType: nil)
                lastState = state
            } catch {
                print("Không bật được Live Activity: \(error)")
            }
        }

        if cancellable == nil {
            cancellable = store.$state
                .combineLatest(store.$status, store.$sourceKind)
                .throttle(for: .seconds(1), scheduler: RunLoop.main, latest: true)
                .sink { [weak self] state, status, kind in
                    self?.push(Self.contentState(state, status: status, kind: kind))
                }
        }
    }

    func stop() {
        cancellable = nil
        activity = nil
        lastState = nil
        let running = Activity<DashboardAttributes>.activities
        Task {
            for item in running {
                await item.end(nil, dismissalPolicy: .immediate)
            }
        }
    }

    private func push(_ state: DashboardAttributes.ContentState) {
        guard let activity = activity, state != lastState else { return }
        lastState = state
        let content = ActivityContent(state: state, staleDate: Date().addingTimeInterval(120))
        Task {
            await activity.update(content)
        }
    }

    private static func contentState(_ s: VehicleState,
                                     status: DataSourceStatus,
                                     kind: DataSourceKind) -> DashboardAttributes.ContentState {
        DashboardAttributes.ContentState(
            speed: Int(s.speed.rounded()),
            soc: Int(s.soc.rounded()),
            range: Int(s.range.rounded()),
            mode: s.mode.title,
            voltage: (s.voltage * 10).rounded() / 10,
            current: (s.current * 10).rounded() / 10,
            batteryTemp: Int(s.batteryTemp.rounded()),
            source: kind.shortTitle,
            connected: kind == .esp32 && status.isConnected)
    }
}
