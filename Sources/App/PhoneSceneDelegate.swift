import UIKit
import SwiftUI

/// Màn hình trên iPhone: hiển thị dashboard toàn màn hình, nằm ngang.
final class PhoneSceneDelegate: UIResponder, UIWindowSceneDelegate {
    var window: UIWindow?

    func scene(_ scene: UIScene,
               willConnectTo session: UISceneSession,
               options connectionOptions: UIScene.ConnectionOptions) {
        guard let windowScene = scene as? UIWindowScene else { return }
        let window = UIWindow(windowScene: windowScene)
        window.rootViewController = DashboardHostingController(rootView: DashboardScreen(layout: .phone))
        window.makeKeyAndVisible()
        self.window = window
        UIApplication.shared.isIdleTimerDisabled = true   // không tắt màn hình khi đang chạy
    }

    func sceneDidBecomeActive(_ scene: UIScene) {
        // Live Activity chỉ được bắt đầu khi app đang mở
        LiveActivityManager.shared.start()
    }
}

final class DashboardHostingController: UIHostingController<DashboardScreen> {
    override var prefersStatusBarHidden: Bool { true }
    override var prefersHomeIndicatorAutoHidden: Bool { true }
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask { .landscape }
}
