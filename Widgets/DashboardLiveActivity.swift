import ActivityKit
import SwiftUI
import WidgetKit

/// Live Activity "Bảng đồng hồ".
/// - .small  : CarPlay (iOS 26) và Apple Watch Smart Stack
/// - .medium : màn hình khoá iPhone
/// - Dynamic Island
struct DashboardLiveActivity: Widget {
    var body: some WidgetConfiguration {
        ActivityConfiguration(for: DashboardAttributes.self) { context in
            ActivityContentView(state: context.state)
                .activityBackgroundTint(Neon.background)
                .activitySystemActionForegroundColor(.white)
        } dynamicIsland: { context in
            DynamicIsland {
                DynamicIslandExpandedRegion(.leading) {
                    MiniGauge(speed: context.state.speed, size: 64)
                        .padding(.leading, 4)
                }
                DynamicIslandExpandedRegion(.trailing) {
                    VStack(alignment: .trailing, spacing: 4) {
                        Text("\(context.state.soc)%")
                            .font(.system(size: 26, weight: .heavy, design: .rounded))
                            .foregroundStyle(Neon.socColor(context.state.soc))
                        ModePill(mode: context.state.mode, compact: true)
                    }
                    .padding(.trailing, 4)
                }
                DynamicIslandExpandedRegion(.bottom) {
                    RangeLine(state: context.state)
                }
            } compactLeading: {
                Text("\(context.state.speed)")
                    .font(.system(.body, design: .rounded).weight(.heavy))
                    .foregroundStyle(Neon.cyan)
            } compactTrailing: {
                Text("\(context.state.soc)%")
                    .font(.system(.body, design: .rounded).weight(.bold))
                    .foregroundStyle(Neon.socColor(context.state.soc))
            } minimal: {
                Text("\(context.state.speed)")
                    .font(.system(.caption, design: .rounded).weight(.heavy))
                    .foregroundStyle(Neon.cyan)
            }
            .keylineTint(Neon.cyan)
        }
        .supplementalActivityFamilies([.small, .medium])
    }
}

struct ActivityContentView: View {
    let state: DashboardAttributes.ContentState
    @Environment(\.activityFamily) var family

    var body: some View {
        if family == .small {
            SmallActivityView(state: state)
        } else {
            MediumActivityView(state: state)
        }
    }
}

// MARK: - CarPlay / Watch (khung nhỏ)

struct SmallActivityView: View {
    let state: DashboardAttributes.ContentState

    var body: some View {
        HStack(spacing: 10) {
            MiniGauge(speed: state.speed, size: 64)
            VStack(alignment: .leading, spacing: 4) {
                HStack(spacing: 4) {
                    MiniBattery(soc: state.soc)
                        .frame(width: 44, height: 14)
                    Text("\(state.soc)%")
                        .font(.system(size: 15, weight: .heavy, design: .rounded))
                        .foregroundStyle(.white)
                }
                HStack(alignment: .firstTextBaseline, spacing: 2) {
                    Text("\(state.range)")
                        .font(.system(size: 22, weight: .heavy, design: .rounded))
                    Text("km")
                        .font(.system(size: 12, weight: .bold, design: .rounded))
                }
                .foregroundStyle(Neon.green)
                ModePill(mode: state.mode, compact: true)
            }
            Spacer(minLength: 0)
        }
        .padding(8)
    }
}

// MARK: - Màn hình khoá

struct MediumActivityView: View {
    let state: DashboardAttributes.ContentState

    var body: some View {
        HStack(spacing: 16) {
            MiniGauge(speed: state.speed, size: 88)

            VStack(alignment: .leading, spacing: 7) {
                HStack {
                    Text("SOC")
                        .font(.system(size: 14, weight: .semibold, design: .rounded))
                        .foregroundStyle(Neon.cyan)
                    Text("\(state.soc)%")
                        .font(.system(size: 18, weight: .heavy, design: .rounded))
                        .italic()
                        .foregroundStyle(.white)
                    Spacer()
                    ModePill(mode: state.mode, compact: false)
                }
                MiniBattery(soc: state.soc)
                    .frame(height: 16)
                RangeLine(state: state)
                HStack(spacing: 12) {
                    Label(String(format: "%.1f V", state.voltage), systemImage: "bolt.fill")
                    Label(String(format: "%.1f A", state.current), systemImage: "waveform.path.ecg")
                    Label("\(state.batteryTemp)°C", systemImage: "thermometer.medium")
                }
                .font(.system(size: 12, weight: .medium, design: .rounded))
                .foregroundStyle(.white.opacity(0.7))
                .labelStyle(.titleAndIcon)
            }
        }
        .padding(16)
    }
}

// MARK: - Thành phần

struct MiniGauge: View {
    let speed: Int
    let size: CGFloat
    static let maxSpeed = 80.0

    var body: some View {
        let fraction = min(max(Double(speed) / Self.maxSpeed, 0), 1)
        let line = size * 0.11
        ZStack {
            Circle()
                .trim(from: 0, to: 2.0 / 3.0)
                .stroke(Color.white.opacity(0.1), style: StrokeStyle(lineWidth: line, lineCap: .round))
                .rotationEffect(.degrees(150))
            Circle()
                .trim(from: 0, to: 2.0 / 3.0 * fraction)
                .stroke(LinearGradient(colors: [Neon.blue, Neon.cyan], startPoint: .bottomLeading, endPoint: .topTrailing),
                        style: StrokeStyle(lineWidth: line, lineCap: .round))
                .rotationEffect(.degrees(150))
            VStack(spacing: -2) {
                Text("\(speed)")
                    .font(.system(size: size * 0.42, weight: .heavy, design: .rounded))
                    .foregroundStyle(.white)
                    .minimumScaleFactor(0.6)
                    .lineLimit(1)
                Text("km/h")
                    .font(.system(size: size * 0.14, weight: .semibold, design: .rounded))
                    .foregroundStyle(.white.opacity(0.8))
            }
        }
        .padding(line / 2)
        .frame(width: size, height: size)
    }
}

struct MiniBattery: View {
    let soc: Int

    var body: some View {
        let filled = Int((Double(soc) / 10).rounded(.up))
        let color = Neon.socColor(soc)
        HStack(spacing: 2) {
            ForEach(0..<10, id: \.self) { i in
                RoundedRectangle(cornerRadius: 1.5)
                    .fill(i < filled ? color : Color.white.opacity(0.1))
            }
        }
        .padding(2.5)
        .overlay(RoundedRectangle(cornerRadius: 4).stroke(color, lineWidth: 1.5))
    }
}

struct ModePill: View {
    let mode: String
    let compact: Bool

    var body: some View {
        let color = Neon.modeColor(mode)
        Text(mode)
            .font(.system(size: compact ? 11 : 13, weight: .bold, design: .rounded))
            .foregroundStyle(color)
            .padding(.horizontal, compact ? 7 : 10)
            .padding(.vertical, compact ? 2 : 3)
            .overlay(Capsule().stroke(color, lineWidth: 1.5))
    }
}

struct RangeLine: View {
    let state: DashboardAttributes.ContentState

    var body: some View {
        HStack(alignment: .firstTextBaseline, spacing: 6) {
            Image(systemName: "mappin.circle")
                .foregroundStyle(Neon.cyan)
            Text("Còn đi được")
                .foregroundStyle(.white.opacity(0.85))
            Text("\(state.range) km")
                .font(.system(size: 17, weight: .heavy, design: .rounded))
                .foregroundStyle(Neon.green)
            Spacer(minLength: 0)
            if !state.connected {
                Text(state.source)
                    .foregroundStyle(.white.opacity(0.5))
            }
        }
        .font(.system(size: 13, weight: .medium, design: .rounded))
    }
}
