#pragma once
// VST3 editor view backed by a webview/webview WebView (no JUCE).
//
// Implements IPlugView and hosts an OS-native WebView (WebView2 on Windows,
// WKWebView on macOS, WebKitGTK on Linux). The webview loads the Vue UI (Vite
// dev server in debug, bundled dist in release) and bridges JS <-> C++ via
// WebViewHost bindings / eval.

#include "pluginterfaces/gui/iplugview.h"
#include "vst/common/parameter_registry.h"
#include "webview/webview_editor.h"

namespace Steinberg {
namespace Vst {
class IEditController;
}
} // namespace Steinberg

namespace netsdr {

class PluginEditor : public Steinberg::IPlugView {
public:
    PluginEditor(Steinberg::Vst::IEditController* controller,
                 const ParameterRegistry& registry);
    ~PluginEditor();

    DECLARE_FUNKNOWN_METHODS

    // IPlugView
    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API removed() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onWheel(float distance) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode,
                                            Steinberg::int16 modifiers) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode,
                                          Steinberg::int16 modifiers) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* size) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onFocus(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setFrame(Steinberg::IPlugFrame* frame) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canResize() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) SMTG_OVERRIDE;

    // Invoked by WebViewHost when the JS side posts a message.
    void onJavaScriptMessage(const char* message);

    // Returns the UI URL the editor should load (dev server in debug builds).
    static const char* uiUrl();

private:
    void attachWebView(void* parentHandle);

    Steinberg::Vst::IEditController* controller_;
    const ParameterRegistry& registry_;
    Steinberg::IPlugFrame* frame_;
    WebViewHost webView_;

    Steinberg::int32 width_;
    Steinberg::int32 height_;
    bool attached_;
};

} // namespace netsdr
