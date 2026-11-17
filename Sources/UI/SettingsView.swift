import SwiftUI

struct SettingsView: View {
    @ObservedObject var store: VehicleStore
    @Environment(\.dismiss) var dismiss
    @AppStorage(LiveActivityManager.enabledKey) var liveActivityEnabled = true

    var body: some View {
        NavigationStack {
            Form {
                Section("Nguồn dữ liệu") {
                    Picker("Nguồn", selection: Binding(get: { store.sourceKind },
                                                       set: { store.setSource($0) })) {
                        ForEach(DataSourceKind.allCases) { kind in
                            Text(kind.title).tag(kind)
                        }
                    }
                    .pickerStyle(.segmented)

                    LabeledContent("Trạng thái", value: store.status.text)

                    Button("Kết nối lại") { store.reconnect() }
                }

                Section("Chế độ lái") {
                    Picker("Chế độ", selection: Binding(get: { store.state.mode },
                                                        set: { store.setMode($0) })) {
                        ForEach(DriveMode.allCases) { mode in
                            Text(mode.title).tag(mode)
                        }
                    }
                    .pickerStyle(.segmented)
                }

                Section {
                    LabeledContent("Trạng thái", value: store.carPlayConnected ? "Đã kết nối" : "Chưa kết nối")
                    Toggle("Live Activity (CarPlay, màn hình khoá)", isOn: $liveActivityEnabled)
                } header: {
                    Text("CarPlay")
                } footer: {
                    Text("Trên iOS 26, thẻ tốc độ / pin / quãng đường hiện trên CarPlay kể cả khi khoá iPhone. Bật: Cài đặt → Cài đặt chung → CarPlay → xe của bạn → Hiện widget.")
                }

                Section {
                    Text("Service: Nordic UART (6E400001-…)\nESP32 gửi mỗi dòng một JSON, kết thúc bằng \\n:")
                    Text(#"{"spd":32,"soc":91,"v":66.9,"a":0.0,"odo":12450,"trip":36.8,"t":28,"soh":100,"rng":72,"mode":"eco"}"#)
                        .font(.system(.footnote, design: .monospaced))
                } header: {
                    Text("Giao thức ESP32")
                }
            }
            .onChange(of: liveActivityEnabled) { _, enabled in
                if enabled {
                    LiveActivityManager.shared.start()
                } else {
                    LiveActivityManager.shared.stop()
                }
            }
            .navigationTitle("Cài đặt")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Xong") { dismiss() }
                }
            }
        }
    }
}
