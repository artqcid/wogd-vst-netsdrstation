#include "webview_editor.h"

#include "common/diag.h"
#include "webview/webview.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <memory>
#include <string>

// DOS header of this DLL; used as HMODULE to resolve the plugin's own path
// (GetModuleFileNameW(nullptr, ...) would return the HOST executable's path).
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace netsdr {

class WebViewHost::Impl {
public:
  Impl() = default;
  ~Impl() { detach(); }

  bool attach(void *parentHandle) {
    if (w_) {
      return true; // already attached
    }
    diagLog("webview attach: parent=%p", parentHandle);
    std::wstring moduleDir;
#ifdef _WIN32
    {
      wchar_t path[MAX_PATH] = {};
      // NOTE: use the plugin DLL's own image base, not nullptr (host exe).
      if (GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path,
                             MAX_PATH) > 0) {
        std::wstring full(path);
        auto pos = full.rfind(L'\\');
        if (pos != std::wstring::npos) {
          moduleDir = full.substr(0, pos);
          SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER",
                                  (moduleDir + L"\\FixedRuntime").c_str());
        }
      }
      diagLog("webview attach: moduleDir=%ls", moduleDir.c_str());
    }
#endif
    try {
      // webview::webview(debug, window). Passing a parent HWND embeds the
      // webview as a child of the host window instead of creating a
      // top-level window of its own.
      // Use the local WebView2 Fixed Version from the VST3 bundle
#ifndef NDEBUG
      constexpr bool kWebViewDebug = true;
#else
      constexpr bool kWebViewDebug = false;
#endif
      w_ = std::make_unique<webview::webview>(kWebViewDebug, parentHandle);
    } catch (const std::exception &e) {
      diagLog("webview attach: EXCEPTION %s", e.what());
      w_.reset();
      return false;
    }
    diagLog("webview attach: webview created OK");

    // Bridge: expose a "vstHost" namespace to the JS side. Every bound
    // function forwards its JSON payload to the message handler.
    w_->bind("vstHostSetParameter", [this](const std::string &req) {
      dispatchMessage("setParameter", req);
      return std::string{};
    });
    w_->bind("vstHostGetParameters", [this](const std::string & /*req*/) {
      dispatchMessage("getParameters", "");
      return std::string{};
    });
    w_->bind("vstHostSetStation", [this](const std::string &req) {
      dispatchMessage("setStation", req);
      return std::string{};
    });
    w_->bind("vstHostDisconnect", [this](const std::string & /*req*/) {
      dispatchMessage("disconnect", "");
      return std::string{};
    });
    w_->bind("vstHostResize", [this](const std::string &req) {
      dispatchMessage("resize", req);
      return std::string{};
    });

    // Expose a stable `window.vstHost` object that wraps the flat bindings.
    // This matches the contract expected by ui/src/services/pluginService.ts.
    //
    // NOTE: the id/value are passed as separate native arguments (not a
    // pre-stringified object) so webview serializes them into a single JSON
    // array ["<id>",<value>], which the C++ bridge parses (see
    // vst/common/bridge_protocol.h).
    const std::string bridgeJs = R"js(
window.vstHost = {
  setParameter: function (id, value) {
    window.vstHostSetParameter(id, value);
  },
  getParameters: function () {
    window.vstHostGetParameters();
  },
  setStation: function (hostPort) {
    window.vstHostSetStation(hostPort);
  },
  disconnect: function () {
    window.vstHostDisconnect();
  }
};
)js";
    w_->init(bridgeJs);
#ifdef _WIN32
    SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", nullptr);
#endif
    resizeToParent();
    return true;
  }

  void detach() {
    if (w_) {
      w_.reset();
    }
  }

  bool navigate(const std::string &url) {
    if (!w_) {
      diagLog("webview navigate: FAIL (no webview)");
      return false;
    }
    const auto err = w_->navigate(url);
    const bool ok = !err.has_error();
    diagLog("webview navigate: %s url=%s", ok ? "ok" : "FAIL", url.c_str());
    return ok;
  }

  bool eval(const std::string &js) {
    if (!w_) {
      return false;
    }
    const auto err = w_->eval(js);
    return !err.has_error();
  }

  void resizeToParent() {
    // The webview library's embedded mode creates the child "widget" window
    // at 0x0 and only sizes it from the *parent's* own WM_SIZE handler,
    // which is never wired up when we embed into a host window. So we size
    // the widget to the parent's client area explicitly (FIX-22).
    if (!w_) {
      return;
    }
    auto parentRes = w_->window();
    auto widgetRes = w_->widget();
    if (!parentRes.ok() || !widgetRes.ok()) {
      diagLog("webview resize: window/widget not available");
      return;
    }
#if defined(_WIN32)
    const HWND parent = static_cast<HWND>(parentRes.value());
    const HWND widget = static_cast<HWND>(widgetRes.value());
    RECT r{};
    if (GetClientRect(parent, &r)) {
      const int w = r.right - r.left;
      const int h = r.bottom - r.top;
      MoveWindow(widget, 0, 0, w, h, TRUE);
      diagLog("webview resize: parent=%p widget=%p size=%dx%d", parent,
              widget, w, h);
    } else {
      diagLog("webview resize: GetClientRect FAILED parent=%p", parent);
    }
#else
    // macOS (NSView) / Linux (GtkWidget): resize the native widget here.
    // Not exercised by M1 (Windows-only); kept as a TODO for the port.
    (void)parentRes;
    (void)widgetRes;
#endif
  }

  void setMessageHandler(MessageHandler handler, void *userData) {
    handler_ = handler;
    userData_ = userData;
  }

private:
  void dispatchMessage(const std::string &type, const std::string &payload) {
    diagLog("webview dispatchMessage: type=%s payload=%s", type.c_str(),
            payload.substr(0, 128).c_str());
    if (handler_) {
      // Compose a tiny JSON envelope: {"type":"...","data":<payload>}.
      std::string message = "{\"type\":\"" + type + "\",\"data\":";
      message += payload.empty() ? "null" : payload;
      message += "}";
      handler_(message.c_str(), userData_);
    }
  }

  std::unique_ptr<webview::webview> w_;
  MessageHandler handler_ = nullptr;
  void *userData_ = nullptr;
};

WebViewHost::WebViewHost() : impl_(std::make_unique<Impl>()) {}
WebViewHost::~WebViewHost() = default;

bool WebViewHost::attach(void *parentHandle) {
  return impl_->attach(parentHandle);
}
void WebViewHost::detach() { impl_->detach(); }
bool WebViewHost::navigate(const std::string &url) {
  return impl_->navigate(url);
}
bool WebViewHost::eval(const std::string &js) { return impl_->eval(js); }
void WebViewHost::setMessageHandler(MessageHandler handler, void *userData) {
  impl_->setMessageHandler(handler, userData);
}

void WebViewHost::resizeToParent() { impl_->resizeToParent(); }

} // namespace netsdr
