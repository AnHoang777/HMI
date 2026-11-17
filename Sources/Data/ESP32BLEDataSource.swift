import Foundation
import CoreBluetooth

/// Nhận dữ liệu từ ESP32 qua BLE, dùng Nordic UART Service (NUS).
///
/// ESP32 gửi (notify trên characteristic TX) từng dòng JSON kết thúc bằng '\n', ví dụ:
///   {"spd":32.5,"soc":91,"v":66.9,"a":12.3,"odo":12450.2,"trip":36.8,"t":28,"soh":100,"rng":72,"mode":"eco"}
/// Mọi trường đều không bắt buộc – trường nào thiếu sẽ giữ giá trị cũ.
///
/// App gửi lệnh xuống ESP32 (write trên characteristic RX), ví dụ:
///   {"cmd":"mode","value":"sport"}
final class ESP32BLEDataSource: NSObject, VehicleDataSource {
    static let serviceUUID = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    static let rxUUID      = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E") // iPhone -> ESP32
    static let txUUID      = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E") // ESP32 -> iPhone

    var onUpdate: ((VehicleState) -> Void)?
    var onStatus: ((DataSourceStatus) -> Void)?

    private var central: CBCentralManager?
    private var peripheral: CBPeripheral?
    private var rxCharacteristic: CBCharacteristic?
    private var buffer = Data()
    private var state: VehicleState
    private var isRunning = false

    init(initial: VehicleState) {
        state = initial
        super.init()
    }

    func start() {
        isRunning = true
        if let central = central {
            if central.state == .poweredOn { scan() }
        } else {
            central = CBCentralManager(delegate: self, queue: .main)
        }
    }

    func stop() {
        isRunning = false
        central?.stopScan()
        if let peripheral = peripheral {
            central?.cancelPeripheralConnection(peripheral)
        }
        peripheral = nil
        rxCharacteristic = nil
        buffer.removeAll()
    }

    func setMode(_ mode: DriveMode) {
        state.mode = mode
        send("{\"cmd\":\"mode\",\"value\":\"\(mode.rawValue)\"}\n")
    }

    // MARK: - Private

    private func scan() {
        guard isRunning, let central = central, central.state == .poweredOn else { return }
        onStatus?(.searching)
        central.scanForPeripherals(withServices: [Self.serviceUUID], options: nil)
    }

    private func send(_ text: String) {
        guard let peripheral = peripheral, let rx = rxCharacteristic,
              let data = text.data(using: .utf8) else { return }
        let type: CBCharacteristicWriteType = rx.properties.contains(.writeWithoutResponse)
            ? .withoutResponse : .withResponse
        peripheral.writeValue(data, for: rx, type: type)
    }

    private func handle(line: Data) {
        guard !line.isEmpty,
              let packet = try? JSONDecoder().decode(BLEPacket.self, from: line) else { return }
        if let v = packet.spd { state.speed = v }
        if let v = packet.soc { state.soc = v }
        if let v = packet.v { state.voltage = v }
        if let v = packet.a { state.current = v }
        if let v = packet.odo { state.odo = v }
        if let v = packet.trip { state.trip = v }
        if let v = packet.t { state.batteryTemp = v }
        if let v = packet.soh { state.soh = v }
        if let v = packet.rng { state.range = v }
        if let raw = packet.mode, let mode = DriveMode(rawValue: raw.lowercased()) { state.mode = mode }
        onUpdate?(state)
    }
}

private struct BLEPacket: Decodable {
    let spd: Double?
    let soc: Double?
    let v: Double?
    let a: Double?
    let odo: Double?
    let trip: Double?
    let t: Double?
    let soh: Double?
    let rng: Double?
    let mode: String?
}

// MARK: - CBCentralManagerDelegate

extension ESP32BLEDataSource: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn: scan()
        case .poweredOff: onStatus?(.error("Bluetooth đang tắt"))
        case .unauthorized: onStatus?(.error("Chưa cấp quyền Bluetooth"))
        case .unsupported: onStatus?(.error("Thiết bị không hỗ trợ BLE"))
        default: onStatus?(.idle)
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        central.stopScan()
        self.peripheral = peripheral
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices([Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        onStatus?(.disconnected)
        retryLater()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        self.peripheral = nil
        rxCharacteristic = nil
        buffer.removeAll()
        guard isRunning else { return }
        onStatus?(.disconnected)
        retryLater()
    }

    private func retryLater() {
        DispatchQueue.main.asyncAfter(deadline: .now() + 2) { [weak self] in self?.scan() }
    }
}

// MARK: - CBPeripheralDelegate

extension ESP32BLEDataSource: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == Self.serviceUUID }) else { return }
        peripheral.discoverCharacteristics([Self.txUUID, Self.rxUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        for characteristic in service.characteristics ?? [] {
            if characteristic.uuid == Self.txUUID {
                peripheral.setNotifyValue(true, for: characteristic)
            } else if characteristic.uuid == Self.rxUUID {
                rxCharacteristic = characteristic
            }
        }
        onStatus?(.connected(peripheral.name ?? "ESP32"))
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic.uuid == Self.txUUID, let data = characteristic.value else { return }
        buffer.append(data)
        while let newline = buffer.firstIndex(of: 0x0A) {
            let line = buffer.subdata(in: buffer.startIndex..<newline)
            buffer.removeSubrange(buffer.startIndex...newline)
            handle(line: line)
        }
        if buffer.count > 4096 { buffer.removeAll() }   // chống tràn nếu ESP32 quên gửi '\n'
    }
}
