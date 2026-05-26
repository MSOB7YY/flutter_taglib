import Flutter
import UIKit
import UniformTypeIdentifiers

public class FlutterTaglibPlugin: NSObject, FlutterPlugin, UIDocumentPickerDelegate {
  private var channel: FlutterMethodChannel?
  private var pendingResult: FlutterResult?
  private var activeUrls = [String: URL]()

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "flutter_taglib", binaryMessenger: registrar.messenger())
    let instance = FlutterTaglibPlugin()
    instance.channel = channel
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "pickAndAuthorizeDirectory":
      self.pendingResult = result
      let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.folder], asCopy: false)
      picker.delegate = self
      picker.allowsMultipleSelection = false
      if let rootVC = UIApplication.shared.windows.first(where: { $0.isKeyWindow })?.rootViewController {
        var topVC = rootVC
        while let presented = topVC.presentedViewController {
          topVC = presented
        }
        topVC.present(picker, animated: true)
      } else {
        result(FlutterError(code: "NO_VIEW_CONTROLLER", message: "Failed to find key window root view controller", details: nil))
        self.pendingResult = nil
      }

    case "startAccessingDirectory":
      guard let args = call.arguments as? [String: Any],
            let bookmarkBase64 = args["bookmark"] as? String else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Missing bookmark argument", details: nil))
        return
      }
      
      guard let bookmarkData = Data(base64Encoded: bookmarkBase64) else {
        result(FlutterError(code: "INVALID_BOOKMARK", message: "Failed to decode Base64 bookmark", details: nil))
        return
      }
      
      do {
        var isStale = false
        let url = try URL(resolvingBookmarkData: bookmarkData, options: .withSecurityScope, relativeTo: nil, bookmarkDataIsStale: &isStale)
        let success = url.startAccessingSecurityScopedResource()
        if success {
          activeUrls[url.path] = url
          result(["path": url.path, "isStale": isStale])
        } else {
          result(FlutterError(code: "ACCESS_DENIED", message: "Failed to access security-scoped resource", details: nil))
        }
      } catch {
        result(FlutterError(code: "RESOLVE_FAILED", message: error.localizedDescription, details: nil))
      }

    case "stopAccessingDirectory":
      guard let args = call.arguments as? [String: Any],
            let path = args["path"] as? String else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Missing path argument", details: nil))
        return
      }
      
      if let url = activeUrls.removeValue(forKey: path) {
        url.stopAccessingSecurityScopedResource()
      }
      result(nil)

    default:
      result(FlutterMethodNotImplemented)
    }
  }

  // MARK: - UIDocumentPickerDelegate
  public func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
    guard let url = urls.first else {
      pendingResult?(nil)
      pendingResult = nil
      return
    }

    do {
      let success = url.startAccessingSecurityScopedResource()
      defer {
        if success {
          url.stopAccessingSecurityScopedResource()
        }
      }
      
      let bookmarkData = try url.bookmarkData(options: .withSecurityScope, includingResourceValuesForKeys: nil, relativeTo: nil)
      let bookmarkBase64 = bookmarkData.base64EncodedString()
      
      pendingResult?([
        "path": url.path,
        "bookmark": bookmarkBase64
      ])
    } catch {
      pendingResult?(FlutterError(code: "BOOKMARK_FAILED", message: error.localizedDescription, details: nil))
    }
    pendingResult = nil
  }

  public func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
    pendingResult?(nil)
    pendingResult = nil
  }
}
