import CarPlay
import Combine

/// Màn hình CarPlay chuẩn (Driving Task): bảng thông tin 2 cột, cập nhật mỗi giây.
final class CarPlayInfoController {
    let template: CPInformationTemplate
    private var cancellable: AnyCancellable?
    private var lastItems: [String] = []

    init(store: VehicleStore) {
        template = CPInformationTemplate(title: "Bảng đồng hồ",
                                         layout: .twoColumn,
                                         items: Self.items(for: store.state),
                                         actions: [])

        let modeButton = CPTextButton(title: "Đổi chế độ lái", textStyle: .normal) { _ in
            store.cycleMode()
        }
        template.actions = [modeButton]

        // CarPlay không nên cập nhật quá dày → giới hạn 1 lần/giây
        cancellable = store.$state
            .throttle(for: .seconds(1), scheduler: RunLoop.main, latest: true)
            .sink { [weak self] state in self?.update(with: state) }
    }

    private func update(with state: VehicleState) {
        let items = Self.items(for: state)
        let signature = items.map { "\($0.title ?? "")=\($0.detail ?? "")" }
        guard signature != lastItems else { return }
        lastItems = signature
        template.items = items
    }

    static func items(for s: VehicleState) -> [CPInformationItem] {
        [
            CPInformationItem(title: "Tốc độ", detail: "\(s.speedText) km/h"),
            CPInformationItem(title: "Chế độ", detail: s.mode.title),
            CPInformationItem(title: "Pin (SOC)", detail: "\(s.socText) %"),
            CPInformationItem(title: "Còn đi được", detail: "\(s.rangeText) km"),
            CPInformationItem(title: "Điện áp", detail: "\(s.voltageText) V"),
            CPInformationItem(title: "Dòng điện", detail: "\(s.currentText) A"),
            CPInformationItem(title: "Công suất", detail: "\(s.powerText) kW"),
            CPInformationItem(title: "Nhiệt độ pin", detail: "\(s.tempText) °C"),
            CPInformationItem(title: "SOH", detail: "\(s.sohText) %"),
            CPInformationItem(title: "ODO / TRIP", detail: "\(s.odoText) km · \(s.tripText) km"),
        ]
    }
}
