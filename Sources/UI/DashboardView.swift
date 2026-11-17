import SwiftUI

enum DashboardLayout {
    case phone     // có thanh app bên trái
    case carPlay   // chỉ phần đồng hồ (CarPlay đã có thanh app riêng)
}

/// Kích thước "bản vẽ" gốc. Toàn bộ giao diện được vẽ theo toạ độ cố định này
/// rồi phóng to/thu nhỏ cho vừa mọi màn hình (iPhone, CarPlay…).
enum DashCanvas {
    static let sidebarWidth: CGFloat = 110
    static let gap: CGFloat = 14
    static let panelWidth: CGFloat = 1176
    static let height: CGFloat = 620
    static let centerX: CGFloat = panelWidth / 2
}

struct DashboardScreen: View {
    let layout: DashboardLayout
    @ObservedObject var store: VehicleStore
    @State var showSettings = false

    init(layout: DashboardLayout, store: VehicleStore = .shared) {
        self.layout = layout
        self.store = store
    }

    private var canvasWidth: CGFloat {
        layout == .phone ? DashCanvas.sidebarWidth + DashCanvas.gap + DashCanvas.panelWidth : DashCanvas.panelWidth
    }

    var body: some View {
        GeometryReader { geo in
            let scale = min(geo.size.width / canvasWidth, geo.size.height / DashCanvas.height)
            HStack(spacing: DashCanvas.gap) {
                if layout == .phone {
                    SideBar(store: store, onMenu: { showSettings = true })
                        .frame(width: DashCanvas.sidebarWidth, height: DashCanvas.height)
                }
                MainPanel(store: store)
                    .frame(width: DashCanvas.panelWidth, height: DashCanvas.height)
            }
            .frame(width: canvasWidth, height: DashCanvas.height)
            .scaleEffect(scale)
            .frame(width: geo.size.width, height: geo.size.height)
        }
        .padding(layout == .phone ? 8 : 0)
        .background(Theme.screenBackground.ignoresSafeArea())
        .preferredColorScheme(.dark)
        .statusBarHidden(true)
        .persistentSystemOverlays(.hidden)
        .sheet(isPresented: $showSettings) {
            SettingsView(store: store)
        }
    }
}

// MARK: - Thanh app bên trái

struct SideBar: View {
    @ObservedObject var store: VehicleStore
    let onMenu: () -> Void
    @Environment(\.openURL) var openURL

    var body: some View {
        VStack(spacing: 0) {
            TimelineView(.periodic(from: .now, by: 1)) { context in
                Text(context.date, format: .dateTime.hour(.twoDigits(amPM: .omitted)).minute(.twoDigits))
                    .font(.dash(34, .semibold))
                    .monospacedDigit()
            }
            .padding(.top, 26)

            HStack(spacing: 6) {
                Image(systemName: "cellularbars")
                Text(store.sourceKind.shortTitle)
            }
            .font(.dash(24, .semibold))
            .foregroundStyle(store.status.isConnected ? Color.white : Color.gray)
            .padding(.top, 6)

            Spacer()

            VStack(spacing: 26) {
                AppIcon(symbol: "location.north.circle.fill",
                        colors: [Color(hex: 0x4CD964), Color(hex: 0x1E90FF)]) {
                    if let url = URL(string: "maps://") { openURL(url) }
                }
                AppIcon(symbol: "music.note", colors: [Color(hex: 0xFF5E6C), Color(hex: 0xE5243B)]) {
                    if let url = URL(string: "music://") { openURL(url) }
                }
                AppIcon(symbol: "phone.fill", colors: [Color(hex: 0x5BE36B), Color(hex: 0x28C840)]) { }
            }

            Spacer()

            Button(action: onMenu) {
                Image(systemName: "square.grid.3x3.fill")
                    .font(.system(size: 44, weight: .bold))
                    .foregroundStyle(.white)
            }
            .padding(.bottom, 34)
        }
        .foregroundStyle(.white)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(RoundedRectangle(cornerRadius: 26).fill(Color.white.opacity(0.07)))
    }
}

struct AppIcon: View {
    let symbol: String
    let colors: [Color]
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            RoundedRectangle(cornerRadius: 18)
                .fill(LinearGradient(colors: colors, startPoint: .top, endPoint: .bottom))
                .frame(width: 80, height: 80)
                .overlay(
                    Image(systemName: symbol)
                        .font(.system(size: 40, weight: .bold))
                        .foregroundStyle(.white)
                )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Bảng đồng hồ chính

struct MainPanel: View {
    @ObservedObject var store: VehicleStore

    var body: some View {
        let s = store.state
        let cx = DashCanvas.centerX

        ZStack {
            PanelBackground()

            // Viền neon
            TopNeonShape()
                .stroke(Theme.neonGradient, style: StrokeStyle(lineWidth: 3, lineCap: .round))
                .shadow(color: Theme.cyan.opacity(0.9), radius: 6)
            SideNeonShape()
                .stroke(Theme.neonGradient, style: StrokeStyle(lineWidth: 2, lineCap: .round))
                .opacity(0.7)
            RangePanelShape()
                .fill(LinearGradient(colors: [Theme.blue.opacity(0.25), Theme.blue.opacity(0.03)],
                                     startPoint: .top, endPoint: .bottom))
            RangePanelShape()
                .stroke(Theme.cyan.opacity(0.9), lineWidth: 2.5)
                .shadow(color: Theme.cyan, radius: 8)

            // Logo
            BrandHeader().position(x: cx, y: 50)

            // Cột trái: pin
            BatteryColumn(state: s)
                .frame(width: 300, alignment: .topLeading)
                .position(x: 50 + 150, y: 265)

            Image(systemName: "scooter")
                .font(.system(size: 92, weight: .regular))
                .foregroundStyle(LinearGradient(colors: [.white, Theme.blue], startPoint: .top, endPoint: .bottom))
                .shadow(color: Theme.blue.opacity(0.8), radius: 10)
                .position(x: 185, y: 545)

            // Giữa: đồng hồ tốc độ
            SpeedGauge(speed: s.speed, radius: 195)
                .position(x: cx, y: 290)

            Text(s.speedText)
                .font(.system(size: 172, weight: .heavy, design: .rounded))
                .monospacedDigit()
                .foregroundStyle(.white)
                .shadow(color: .white.opacity(0.35), radius: 6)
                .contentTransition(.numericText())
                .animation(.easeOut(duration: 0.2), value: s.speedText)
                .position(x: cx, y: 272)

            Text("km/h")
                .font(.dash(34, .semibold))
                .foregroundStyle(.white)
                .position(x: cx, y: 372)

            ModeBadge(mode: s.mode) { store.cycleMode() }
                .position(x: cx, y: 436)

            // Dưới giữa: quãng đường còn lại
            VStack(spacing: 2) {
                HStack(spacing: 10) {
                    Image(systemName: "mappin.circle")
                        .foregroundStyle(Theme.cyan)
                    Text("Ước tính còn đi được")
                }
                .font(.dash(26, .medium))
                .foregroundStyle(.white.opacity(0.92))

                HStack(alignment: .firstTextBaseline, spacing: 10) {
                    Text(s.rangeText)
                        .font(.system(size: 66, weight: .heavy, design: .rounded))
                        .monospacedDigit()
                    Text("km")
                        .font(.dash(38, .bold))
                }
                .foregroundStyle(Theme.green)
                .shadow(color: Theme.green.opacity(0.6), radius: 8)
            }
            .position(x: cx, y: 548)

            // Cột phải: thông tin
            InfoColumn(state: s)
                .frame(width: 300)
                .position(x: 860 + 150, y: 265)

            // Dưới phải: trạng thái kết nối
            StatusCluster(store: store)
                .position(x: 1010, y: 552)
        }
        .frame(width: DashCanvas.panelWidth, height: DashCanvas.height)
        .clipShape(RoundedRectangle(cornerRadius: 28))
    }
}

// MARK: - Thành phần

struct PanelBackground: View {
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 28)
                .fill(LinearGradient(colors: [Color(hex: 0x0B1830), Color(hex: 0x040912)],
                                     startPoint: .top, endPoint: .bottom))
            RadialGradient(colors: [Theme.blue.opacity(0.28), .clear],
                           center: UnitPoint(x: 0.5, y: 0.45), startRadius: 10, endRadius: 430)
            RoundedRectangle(cornerRadius: 28)
                .stroke(Theme.blue.opacity(0.25), lineWidth: 1.5)
        }
    }
}

struct BrandHeader: View {
    var body: some View {
        HStack(spacing: 24) {
            ChevronLogo()
                .stroke(LinearGradient(colors: [.white, Color(hex: 0xA9B8CC)], startPoint: .top, endPoint: .bottom),
                        style: StrokeStyle(lineWidth: 4, lineCap: .round, lineJoin: .round))
                .frame(width: 54, height: 42)
                .shadow(color: .white.opacity(0.4), radius: 4)
            Text(Brand.name)
                .font(.dash(34, .bold))
                .kerning(12)
                .foregroundStyle(.white)
        }
    }
}

struct ChevronLogo: Shape {
    func path(in r: CGRect) -> Path {
        var p = Path()
        p.move(to: CGPoint(x: r.minX, y: r.minY))
        p.addLine(to: CGPoint(x: r.midX, y: r.maxY))
        p.addLine(to: CGPoint(x: r.maxX, y: r.minY))
        p.move(to: CGPoint(x: r.minX + r.width * 0.22, y: r.minY))
        p.addLine(to: CGPoint(x: r.midX, y: r.minY + r.height * 0.62))
        p.addLine(to: CGPoint(x: r.maxX - r.width * 0.22, y: r.minY))
        return p
    }
}

struct BatteryColumn: View {
    let state: VehicleState

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("SOC")
                .font(.dash(30, .semibold))
                .foregroundStyle(Theme.cyan)

            HStack(alignment: .firstTextBaseline, spacing: 2) {
                Text(state.socText).font(.system(size: 70, weight: .heavy, design: .rounded)).italic()
                Text("%").font(.system(size: 46, weight: .heavy, design: .rounded)).italic()
            }
            .foregroundStyle(.white)

            BatteryBar(soc: state.soc)
                .frame(width: 236, height: 66)
                .padding(.top, 6)
                .padding(.bottom, 16)

            MetricRow(icon: "bolt.fill", value: state.voltageText, unit: "V")
            NeonDivider()
            MetricRow(icon: "waveform.path.ecg", value: state.currentText, unit: "A")
            NeonDivider()
            MetricRow(icon: "bolt.circle", value: state.powerText, unit: "kW")
        }
    }
}

struct BatteryBar: View {
    let soc: Double

    var body: some View {
        let filled = Int((soc / 10).rounded(.up))
        let color = soc > 30 ? Theme.green : (soc > 15 ? Theme.orange : Theme.red)

        HStack(spacing: 4) {
            HStack(spacing: 5) {
                ForEach(0..<10, id: \.self) { i in
                    RoundedRectangle(cornerRadius: 3)
                        .fill(i < filled ? color : Color.white.opacity(0.07))
                }
            }
            .padding(7)
            .overlay(RoundedRectangle(cornerRadius: 12).stroke(color, lineWidth: 4))
            .shadow(color: color.opacity(0.7), radius: 8)

            RoundedRectangle(cornerRadius: 3)
                .fill(color)
                .frame(width: 8, height: 24)
        }
        .animation(.easeInOut(duration: 0.3), value: filled)
    }
}

struct MetricRow: View {
    let icon: String
    let value: String
    let unit: String

    var body: some View {
        HStack(spacing: 18) {
            Image(systemName: icon)
                .font(.system(size: 32, weight: .semibold))
                .foregroundStyle(Theme.iconGradient)
                .frame(width: 44)
            HStack(alignment: .firstTextBaseline, spacing: 8) {
                Text(value).font(.dash(38, .semibold)).monospacedDigit()
                Text(unit).font(.dash(26, .medium))
            }
            .foregroundStyle(.white)
        }
        .frame(height: 56, alignment: .leading)
    }
}

struct InfoColumn: View {
    let state: VehicleState

    var body: some View {
        VStack(spacing: 0) {
            InfoRow(icon: "road.lanes", label: "ODO", value: state.odoText, unit: "km")
            NeonDivider()
            InfoRow(icon: "road.lanes", label: "TRIP", value: state.tripText, unit: "km")
            NeonDivider()
            InfoRow(icon: "thermometer.medium", label: "Nhiệt độ pin", value: state.tempText, unit: "°C")
            NeonDivider()
            InfoRow(icon: "waveform.path.ecg", label: "SOH", value: state.sohText, unit: "%")
        }
    }
}

struct InfoRow: View {
    let icon: String
    let label: String
    let value: String
    let unit: String

    var body: some View {
        HStack(spacing: 22) {
            Image(systemName: icon)
                .font(.system(size: 40, weight: .light))
                .foregroundStyle(Theme.cyan)
                .shadow(color: Theme.cyan.opacity(0.6), radius: 6)
                .frame(width: 56)
            VStack(alignment: .leading, spacing: 0) {
                Text(label)
                    .font(.dash(26, .medium))
                    .foregroundStyle(Theme.cyan)
                HStack(alignment: .firstTextBaseline, spacing: 8) {
                    Text(value).font(.dash(44, .bold)).monospacedDigit()
                    Text(unit).font(.dash(26, .medium))
                }
                .foregroundStyle(.white)
            }
            Spacer(minLength: 0)
        }
        .frame(height: 92)
    }
}

struct NeonDivider: View {
    var body: some View {
        Rectangle()
            .fill(LinearGradient(colors: [Theme.blue.opacity(0.7), Theme.blue.opacity(0.05)],
                                 startPoint: .leading, endPoint: .trailing))
            .frame(height: 1.5)
    }
}

struct SpeedGauge: View {
    let speed: Double
    let radius: CGFloat
    static let maxSpeed: Double = 80
    static let sweep: Double = 240      // độ
    static let startAngle: Double = 150 // bắt đầu ở góc dưới bên trái

    var body: some View {
        let fraction = min(max(speed / Self.maxSpeed, 0), 1)
        let arc = Self.sweep / 360

        ZStack {
            // vòng mảnh bên ngoài
            Circle()
                .trim(from: 0, to: arc)
                .stroke(Theme.blue.opacity(0.45), style: StrokeStyle(lineWidth: 3, lineCap: .round))
                .rotationEffect(.degrees(Self.startAngle))
                .padding(-24)
                .shadow(color: Theme.blue, radius: 6)

            // rãnh nền
            Circle()
                .trim(from: 0, to: arc)
                .stroke(Color.white.opacity(0.06), style: StrokeStyle(lineWidth: 22, lineCap: .round))
                .rotationEffect(.degrees(Self.startAngle))

            // giá trị
            Circle()
                .trim(from: 0, to: arc * fraction)
                .stroke(AngularGradient(colors: [Theme.blue, Theme.cyan], center: .center,
                                        startAngle: .degrees(0), endAngle: .degrees(max(Self.sweep * fraction, 1))),
                        style: StrokeStyle(lineWidth: 22, lineCap: .round))
                .rotationEffect(.degrees(Self.startAngle))
                .shadow(color: Theme.cyan, radius: 10)
                .shadow(color: Theme.cyan.opacity(0.6), radius: 22)

            // vạch chia
            ForEach(0...16, id: \.self) { i in
                let major = i % 4 == 0
                Capsule()
                    .fill(Color.white.opacity(major ? 0.6 : 0.25))
                    .frame(width: major ? 4 : 2, height: major ? 16 : 10)
                    .offset(y: -(radius - 34))
                    .rotationEffect(.degrees(Self.startAngle + Self.sweep * Double(i) / 16 + 90))
            }
        }
        .frame(width: radius * 2, height: radius * 2)
        .animation(.easeOut(duration: 0.25), value: fraction)
    }
}

struct ModeBadge: View {
    let mode: DriveMode
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Image(systemName: mode.icon)
                Text(mode.title)
            }
            .font(.dash(30, .bold))
            .foregroundStyle(mode.color)
            .padding(.horizontal, 28)
            .padding(.vertical, 10)
            .background(Capsule().fill(mode.color.opacity(0.12)))
            .overlay(Capsule().stroke(mode.color, lineWidth: 3))
            .shadow(color: mode.color.opacity(0.7), radius: 8)
        }
        .buttonStyle(.plain)
    }
}

struct StatusCluster: View {
    @ObservedObject var store: VehicleStore

    var body: some View {
        let bleActive = store.sourceKind == .esp32 && store.status.isConnected
        VStack(spacing: 8) {
            HStack(spacing: 22) {
                Image(systemName: "antenna.radiowaves.left.and.right")
                    .font(.system(size: 34, weight: .semibold))
                    .foregroundStyle(bleActive ? Theme.cyan : Color.gray)
                    .shadow(color: bleActive ? Theme.cyan : .clear, radius: 6)

                Rectangle()
                    .fill(Color.white.opacity(0.25))
                    .frame(width: 2, height: 46)

                HStack(spacing: 12) {
                    RoundedRectangle(cornerRadius: 12)
                        .fill(store.carPlayConnected ? Theme.green : Color.gray.opacity(0.5))
                        .frame(width: 56, height: 56)
                        .overlay(
                            Image(systemName: "play.circle")
                                .font(.system(size: 32, weight: .bold))
                                .foregroundStyle(.white)
                        )
                    Text("CarPlay")
                        .font(.dash(32, .medium))
                        .foregroundStyle(.white)
                }
            }
            Text(store.status.text)
                .font(.dash(17, .medium))
                .foregroundStyle(.white.opacity(0.55))
                .lineLimit(1)
        }
    }
}

// MARK: - Hình viền neon (toạ độ theo DashCanvas.panelWidth × DashCanvas.height)

struct TopNeonShape: Shape {
    func path(in rect: CGRect) -> Path {
        var p = Path()
        p.move(to: CGPoint(x: 20, y: 30))
        p.addLine(to: CGPoint(x: 330, y: 30))
        p.addCurve(to: CGPoint(x: 420, y: 76), control1: CGPoint(x: 378, y: 30), control2: CGPoint(x: 372, y: 76))
        p.addLine(to: CGPoint(x: 756, y: 76))
        p.addCurve(to: CGPoint(x: 846, y: 30), control1: CGPoint(x: 804, y: 76), control2: CGPoint(x: 798, y: 30))
        p.addLine(to: CGPoint(x: 1156, y: 30))
        return p
    }
}

struct SideNeonShape: Shape {
    func path(in rect: CGRect) -> Path {
        var p = Path()
        p.move(to: CGPoint(x: 20, y: 462))
        p.addLine(to: CGPoint(x: 340, y: 462))
        p.move(to: CGPoint(x: 836, y: 462))
        p.addLine(to: CGPoint(x: 1156, y: 462))
        return p
    }
}

struct RangePanelShape: Shape {
    func path(in rect: CGRect) -> Path {
        var p = Path()
        p.move(to: CGPoint(x: 300, y: 640))
        p.addLine(to: CGPoint(x: 395, y: 505))
        p.addQuadCurve(to: CGPoint(x: 428, y: 482), control: CGPoint(x: 408, y: 482))
        p.addLine(to: CGPoint(x: 748, y: 482))
        p.addQuadCurve(to: CGPoint(x: 781, y: 505), control: CGPoint(x: 768, y: 482))
        p.addLine(to: CGPoint(x: 876, y: 640))
        return p
    }
}
