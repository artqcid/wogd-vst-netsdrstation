#include "plugin_editor.h"

#include "common/diag.h"
#include "vst/common/bridge_protocol.h"
#include "vst/common/paramids.h"
#include "vst/controller/plugin_controller.h"

#include <windows.h>
#include <cstdio>
#include <string>

#include "base/source/fobject.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

// DOS header of this DLL; used as HMODULE to resolve the plugin's own path
// (GetModuleFileNameW(nullptr, ...) would return the host executable's path).
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

IMPLEMENT_FUNKNOWN_METHODS(PluginEditor, IPlugView, IPlugView::iid)

namespace {
const ViewRect kDefaultSize(0, 0, 640, 400);
// Minimum editor size (M4.1): the KiwiSDR UI reflows down to this width/height
// before scrolling instead of clipping. Below it the webview scrolls.
const int32 kMinWidth = 640;
const int32 kMinHeight = 400;

// Vite dev server used for hot reload in debug builds (workspace-workflow.md).
// Release builds load the pre-built UI bundle (ui/dist) via a file:// URL,
// baked in at configure time (NS_UI_DIST_URL, see source/entry/CMakeLists.txt).
#ifndef NDEBUG
constexpr const char *kUiUrl = "http://localhost:5173";
#else
#pragma comment(lib, "kernel32.lib")
std::string percentEncodePath(const std::wstring &path) {
  int len = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0,
                                nullptr, nullptr);
  if (len <= 0) {
    return "";
  }
  std::string utf8(static_cast<size_t>(len - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8.data(), len, nullptr,
                      nullptr);
  static const char *kHex = "0123456789ABCDEF";
  std::string out;
  out.reserve(utf8.size() * 3);
  for (unsigned char c : utf8) {
    const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      (c >= '0' && c <= '9') || c == '/' || c == '-' ||
                      c == '_' || c == '.' || c == '~' || c == '!' ||
                      c == '$' || c == '&' || c == '\'' || c == '(' ||
                      c == ')' || c == '*' || c == '+' || c == ',' ||
                      c == ';' || c == '=' || c == ':' || c == '@';
    if (safe) {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += kHex[c >> 4];
      out += kHex[c & 0x0F];
    }
  }
  return out;
}

std::string buildReleaseUiUrl() {
  wchar_t path[MAX_PATH] = {};
  if (GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path,
                         MAX_PATH) > 0) {
    std::wstring full(path);
    auto pos = full.rfind(L'\\');
    if (pos != std::wstring::npos) {
      std::wstring dir = full.substr(0, pos);
      for (wchar_t &c : dir) {
        if (c == L'\\') {
          c = L'/';
        }
      }
      return "file:///" + percentEncodePath(dir) + "/ui/index.html";
    }
  }
  return "";
}
#endif
} // namespace

PluginEditor::PluginEditor(Vst::EditControllerEx1 *controller,
                           const ParameterRegistry &registry)
    : controller_(controller), registry_(registry), frame_(nullptr),
      width_(kDefaultSize.getWidth()), height_(kDefaultSize.getHeight()),
      attached_(false) {
  FUNKNOWN_CTOR
  pluginController_ = dynamic_cast<PluginController*>(controller_);
  if (pluginController_) {
      pluginController_->setStatusSink([this](const std::string& s) { pushStatus(s); });
      pluginController_->setLevelSink([this](float dbm) { pushLevel(dbm); });
  }
  webView_.setMessageHandler(
      [](const char *message, void *userData) {
        static_cast<PluginEditor *>(userData)->onJavaScriptMessage(message);
      },
      this);
}

PluginEditor::~PluginEditor() {
  if (pluginController_) {
      pluginController_->setStatusSink({});
  }
  removed();
}

const char *PluginEditor::uiUrl() {
#ifndef NDEBUG
  return kUiUrl;
#else
  static const std::string s = buildReleaseUiUrl();
  return s.c_str();
#endif
}

tresult PLUGIN_API PluginEditor::isPlatformTypeSupported(FIDString type) {
  if (type && (FIDStringsEqual(type, kPlatformTypeHWND) ||
               FIDStringsEqual(type, kPlatformTypeNSView) ||
               FIDStringsEqual(type, kPlatformTypeX11EmbedWindowID))) {
    return kResultTrue;
  }
  return kResultFalse;
}

tresult PLUGIN_API PluginEditor::attached(void *parent, FIDString /*type*/) {
  if (attached_) {
    return kResultFalse;
  }
  diagLog("editor attached: parent=%p", parent);
  attachWebView(parent);
  attached_ = true;
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::removed() {
  if (attached_) {
    webView_.detach();
    attached_ = false;
    if (frame_) {
      frame_->release();
      frame_ = nullptr;
    }
  }
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::onWheel(float /*distance*/) {
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::onKeyDown(char16 /*key*/, int16 /*keyCode*/,
                                           int16 /*modifiers*/) {
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::onKeyUp(char16 /*key*/, int16 /*keyCode*/,
                                         int16 /*modifiers*/) {
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::getSize(ViewRect *size) {
  *size = ViewRect(0, 0, width_, height_);
  return kResultTrue;
}

tresult PLUGIN_API PluginEditor::onSize(ViewRect *newSize) {
  width_ = newSize->getWidth();
  height_ = newSize->getHeight();
  webView_.resizeToParent();
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::onFocus(TBool /*state*/) { return kResultOk; }

tresult PLUGIN_API PluginEditor::setFrame(IPlugFrame *frame) {
  if (frame) {
    frame->addRef();
  }
  if (frame_) {
    frame_->release();
  }
  frame_ = frame;
  return kResultOk;
}

tresult PLUGIN_API PluginEditor::canResize() { return kResultTrue; }

tresult PLUGIN_API PluginEditor::checkSizeConstraint(ViewRect *rect) {
  if (rect->getWidth() < kMinWidth) {
    rect->right = rect->left + kMinWidth;
  }
  if (rect->getHeight() < kMinHeight) {
    rect->bottom = rect->top + kMinHeight;
  }
  return kResultOk;
}

void PluginEditor::attachWebView(void *parentHandle) {
  diagLog("editor attachWebView: parent=%p", parentHandle);
  if (!webView_.attach(parentHandle)) {
    diagLog("editor attachWebView: webView_.attach FAILED");
    return;
  }
  webView_.resizeToParent();
  webView_.navigate(uiUrl());
}

void PluginEditor::onJavaScriptMessage(const char *message) {
  // Parse the bridge envelope and forward parameter changes to the host.
  // The bridge sends messages of the form:
  //   {"type":"setParameter","data":["<id>",<plain value>]}
  // The value is a plain (unnormalized) value in the parameter's own units
  // (kHz for freqKhz, 0..1 for volume, 0/1 for mute/bools); we normalize it
  // via the shared registry before handing it to the controller.
  if (message == nullptr) {
    return;
  }
  diagLog("editor onJavaScriptMessage: %s",
          std::string(message).substr(0, 160).c_str());

  // setStation message: {"type":"setStation","data":["host:port"]}
  BridgeSetStation station;
  if (parseSetStationMessage(message, station)) {
    diagLog("editor onJavaScriptMessage: setStation hostPort=%s",
            station.hostPort.c_str());
    if (auto* controller = dynamic_cast<PluginController*>(controller_)) {
      controller->setStation(station.hostPort);
    } else {
      diagLog("editor onJavaScriptMessage: controller_ is NOT PluginController");
    }
    return;
  }

  // disconnect message: {"type":"disconnect","data":null}
  if (parseDisconnectMessage(message)) {
    diagLog("editor onJavaScriptMessage: disconnect");
    if (auto* controller = dynamic_cast<PluginController*>(controller_)) {
      controller->disconnect();
    } else {
      diagLog("editor onJavaScriptMessage: controller_ is NOT PluginController");
    }
    return;
  }

  BridgeSetParameter parsed;
  if (!parseSetParameterMessage(message, parsed)) {
    return; // getParameters / resize / malformed -> nothing to do (M2)
  }

  std::uint32_t tag = 0;
  if (!paramIdFromUiName(parsed.id, tag)) {
    return; // unknown parameter id
  }

  const ParamValue normalized = registry_.toNormalized(tag, parsed.value);
  controller_->setParamNormalized(tag, normalized);
  // Route the change to the processor: EditControllerEx1::performEdit
  // forwards to the host's IComponentHandler (stored by setComponentHandler);
  // the host queues the value into inputParameterChanges for the next
  // process block. Without this the DSP never sees UI edits (constant tone).
  // NOTE: these are plain virtuals on EditController, NOT an interface —
  // queryInterface(IComponentHandler) on the controller returns kNoInterface.
  controller_->beginEdit(tag);
  controller_->performEdit(tag, normalized);
  controller_->endEdit(tag);
}

void PluginEditor::pushStatus(const std::string& status) {
    diagLog("editor pushStatus: %s attached=%d", status.c_str(), (int)attached_);
    if (!attached_) { return; }
    // Escape for a JSON string literal inside the eval'd JS.
    std::string escaped;
    escaped.reserve(status.size());
    for (char c : status) {
        if (c == '\\' || c == '"') { escaped += '\\'; }
        escaped += c;
    }
    const std::string js =
        "window.updateVueState({\"type\":\"status\",\"data\":\"" + escaped + "\"})";
    webView_.eval(js);
}

void PluginEditor::pushLevel(float dbm) {
    if (!attached_) { return; }
    // S-meter level in dBm (UI thread only; eval is not RT-safe).
    char buf[64] = {};
    std::snprintf(buf, sizeof(buf), "window.setLevel(%.1f)", static_cast<double>(dbm));
    webView_.eval(buf);
}

} // namespace netsdr
