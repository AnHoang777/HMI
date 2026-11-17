import Foundation

/// Giả lập xe chạy trong phố: tăng tốc, chạy đều, phanh, dừng đèn đỏ.
/// Dòng điện, điện áp, SOC, ODO, nhiệt độ pin và quãng đường còn lại được tính theo mô hình đơn giản.
final class SimulatedDataSource: VehicleDataSource {
    var onUpdate: ((VehicleState) -> Void)?
    var onStatus: ((DataSourceStatus) -> Void)?

    // Pack pin giả lập: 16S Li-ion (~60 V danh định), 40 Ah ≈ 2.4 kWh
    static let cellsInSeries = 16.0
    static let capacityAh = 40.0
    static let nominalVoltage = 59.2
    static let internalResistance = 0.06   // Ω
    static let tick: TimeInterval = 0.1

    private var state: VehicleState
    private var timer: Timer?
    private var elapsed: TimeInterval = 0
    private var targetSpeed: Double = 32
    private var nextTargetChange: TimeInterval = 6

    init(initial: VehicleState) {
        state = initial
    }

    func start() {
        stop()
        onStatus?(.connected("Dữ liệu giả lập"))
        let timer = Timer(timeInterval: Self.tick, repeats: true) { [weak self] _ in
            self?.step()
        }
        RunLoop.main.add(timer, forMode: .common)
        self.timer = timer
    }

    func stop() {
        timer?.invalidate()
        timer = nil
    }

    func setMode(_ mode: DriveMode) {
        state.mode = mode
        targetSpeed = min(targetSpeed, mode.maxSpeed)
    }

    private func step() {
        let dt = Self.tick
        elapsed += dt

        // Chọn tốc độ mục tiêu mới sau mỗi vài giây (20% khả năng dừng)
        if elapsed >= nextTargetChange {
            if Double.random(in: 0..<1) < 0.2 {
                targetSpeed = 0
                nextTargetChange = elapsed + Double.random(in: 3...6)
            } else {
                targetSpeed = Double.random(in: 15...state.mode.maxSpeed)
                nextTargetChange = elapsed + Double.random(in: 5...12)
            }
        }

        // Tốc độ tiến dần về mục tiêu, giới hạn gia tốc
        let accel = min(max((targetSpeed - state.speed) * 0.8, -12), state.mode.maxAccel)
        state.speed = max(0, state.speed + accel * dt)
        let v = state.speed

        // Dòng điện: lực cản lăn + cản gió + tăng tốc; phanh mạnh thì tái sinh (âm)
        let current: Double
        if v < 0.5 && accel <= 0 {
            current = 0
        } else if accel < -1.5 {
            current = accel * 1.2
        } else {
            current = 0.6 + 0.09 * v + 0.0025 * v * v + max(accel, 0) * 3
                + Double.random(in: -0.15...0.15)
        }
        state.current = current

        // SOC theo Ah tiêu thụ
        let usedAh = current * dt / 3600
        state.soc = min(max(state.soc - usedAh / Self.capacityAh * 100, 0), 100)

        // Điện áp = OCV(SOC) - sụt áp nội trở
        let cellOCV = 3.40 + 0.82 * state.soc / 100
        state.voltage = cellOCV * Self.cellsInSeries - current * Self.internalResistance

        // Quãng đường
        let km = v * dt / 3600
        state.odo += km
        state.trip += km

        // Nhiệt độ pin tiến dần về điểm cân bằng phụ thuộc I²
        let equilibrium = 27 + current * current * 0.004
        state.batteryTemp += (equilibrium - state.batteryTemp) * dt / 90

        // Quãng đường còn lại
        let remainingWh = state.soc / 100 * Self.capacityAh * Self.nominalVoltage * state.soh / 100
        state.range = remainingWh / state.mode.consumptionWhPerKm

        onUpdate?(state)
    }
}
