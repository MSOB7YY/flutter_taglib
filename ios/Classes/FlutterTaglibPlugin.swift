import Flutter
import UIKit
import UniformTypeIdentifiers

public class FlutterTaglibPlugin: NSObject, FlutterPlugin, UIDocumentPickerDelegate {
  private enum PendingPickerAction {
    case directory
    case audioFile
  }

  private var channel: FlutterMethodChannel?
  private var pendingResult: FlutterResult?
  private var pendingPickerAction: PendingPickerAction?
  private var activeUrls = [String: URL]()

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "flutter_taglib", binaryMessenger: registrar.messenger())
    let instance = FlutterTaglibPlugin()
    instance.channel = channel
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "pickAudioFile":
      self.pendingResult = result
      self.pendingPickerAction = .audioFile
      let picker = UIDocumentPickerViewController(
        forOpeningContentTypes: [.audio],
        asCopy: false
      )
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
        self.pendingPickerAction = nil
      }

    case "pickAndAuthorizeDirectory":
      self.pendingResult = result
      self.pendingPickerAction = .directory
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
        self.pendingPickerAction = nil
      }

    case "startAccessingDirectory":
      guard let args = call.arguments as? [String: Any],
            let path = args["path"] as? String else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Missing path argument", details: nil))
        return
      }

      if activeUrls[path] != nil {
        result(["path": path])
      } else {
        result(FlutterError(code: "ACCESS_DENIED", message: "Directory has not been authorized in this session", details: nil))
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

    case "debugInfo":
      result([
        "plugin": "FlutterTaglibPlugin",
        "activeUrlsCount": activeUrls.count,
        "activePaths": Array(activeUrls.keys).sorted(),
        "bundlePath": Bundle.main.bundlePath,
      ])

    default:
      result(FlutterMethodNotImplemented)
    }
  }

  // MARK: - UIDocumentPickerDelegate
  public func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
    guard let url = urls.first else {
      pendingResult?(nil)
      pendingResult = nil
      pendingPickerAction = nil
      return
    }

    let success = url.startAccessingSecurityScopedResource()
    guard success else {
      pendingResult?(FlutterError(code: "ACCESS_DENIED", message: "Failed to access security-scoped resource", details: nil))
      pendingResult = nil
      pendingPickerAction = nil
      return
    }

    switch pendingPickerAction {
    case .audioFile:
      if let existing = activeUrls.removeValue(forKey: url.path) {
        existing.stopAccessingSecurityScopedResource()
      }
      activeUrls[url.path] = url
      pendingResult?([
        "path": url.path,
        "name": url.lastPathComponent,
      ])
    case .directory:
      if let existing = activeUrls.removeValue(forKey: url.path) {
        existing.stopAccessingSecurityScopedResource()
      }
      activeUrls[url.path] = url
      pendingResult?([
        "path": url.path
      ])
    case .none:
      url.stopAccessingSecurityScopedResource()
      pendingResult?(FlutterError(code: "NO_PENDING_PICKER", message: "No pending picker action found", details: nil))
    }
    pendingResult = nil
    pendingPickerAction = nil
  }

  public func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
    pendingResult?(nil)
    pendingResult = nil
    pendingPickerAction = nil
  }
}
