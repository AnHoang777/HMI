import UIKit

@main
final class AppDelegate: UIResponder, UIApplicationDelegate {
    func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Khởi động nguồn dữ liệu (dùng chung cho iPhone và CarPlay)
        VehicleStore.shared.start()
        return true
    }
}
