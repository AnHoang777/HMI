import CarPlay
import SwiftUI

/// Điểm vào CarPlay.
/// - Bản HMI (mặc định): dùng template thông tin chuẩn của Apple (danh mục Driving Task).
/// - Bản HMIDemo (cờ CARPLAY_FULL_DASHBOARD): vẽ nguyên dashboard lên màn hình xe
///   bằng cơ chế của app dẫn đường. Chỉ dùng để demo, Apple sẽ không duyệt lên App Store.
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
    private var interfaceController: CPInterfaceController?

#if CARPLAY_FULL_DASHBOARD

    private var carWindow: CPWindow?

    func templateApplicationScene(_ templateApplicationScene: CPTemplateApplicationScene,
                                  didConnect interfaceController: CPInterfaceController,
                                  to window: CPWindow) {
        self.interfaceController = interfaceController
        self.carWindow = window

        let host = UIHostingController(rootView: DashboardScreen(layout: .carPlay))
        host.view.backgroundColor = .black
        window.rootViewController = host

        let mapTemplate = CPMapTemplate()
        mapTemplate.automaticallyHidesNavigationBar = true
        interfaceController.setRootTemplate(mapTemplate, animated: false, completion: nil)

        VehicleStore.shared.carPlayConnected = true
    }

    func templateApplicationScene(_ templateApplicationScene: CPTemplateApplicationScene,
                                  didDisconnect interfaceController: CPInterfaceController,
                                  from window: CPWindow) {
        self.interfaceController = nil
        self.carWindow = nil
        VehicleStore.shared.carPlayConnected = false
    }

#else

    private var infoController: CarPlayInfoController?

    func templateApplicationScene(_ templateApplicationScene: CPTemplateApplicationScene,
                                  didConnect interfaceController: CPInterfaceController) {
        self.interfaceController = interfaceController
        let controller = CarPlayInfoController(store: .shared)
        infoController = controller
        interfaceController.setRootTemplate(controller.template, animated: false, completion: nil)
        VehicleStore.shared.carPlayConnected = true
    }

    func templateApplicationScene(_ templateApplicationScene: CPTemplateApplicationScene,
                                  didDisconnectInterfaceController interfaceController: CPInterfaceController) {
        self.interfaceController = nil
        infoController = nil
        VehicleStore.shared.carPlayConnected = false
    }

#endif
}
