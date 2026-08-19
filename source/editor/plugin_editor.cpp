#include "plugin_editor.h"

#include "vst/common/bridge_protocol.h"
#include "vst/common/paramids.h"

#include "base/source/fobject.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

IMPLEMENT_FUNKNOWN_METHODS(PluginEditor, IPlugView, IPlugView::iid)

namespace {
const ViewRect kDefaultSize(0, 0, 640, 400);
const int32 kMinWidth = 320;
const int32 kMinHeight = 200;

// Vite dev server used for hot reload in debug builds (workspace-workflow.md).
// Release builds load the pre-built UI bundle (ui/dist) via a file:// URL,
// baked in at configure time (NS_UI_DIST_URL, see source/entry/CMakeLists.txt).
#ifndef NDEBUG
constexpr const char* kUiUrl = "http://localhost:5173";
#else
#ifdef NS_UI_DIST_URL
constexpr const char* kUiUrl = NS_UI_DIST_URL;
#else
constexpr const char* kUiUrl = "http://localhost:5173";
#endif
#endif
} // namespace

PluginEditor::PluginEditor(Vst::IEditController* controller,
                           const ParameterRegistry& registry)
    : controller_(controller)
    , registry_(registry)
    , frame_(nullptr)
    , width_(kDefaultSize.getWidth())
    , height_(kDefaultSize.getHeight())
    , attached_(false) {
    FUNKNOWN_CTOR
    webView_.setMessageHandler(
        [](const char* message, void* userData) {
            static_cast<PluginEditor*>(userData)->onJavaScriptMessage(message);
        },
        this);
}

PluginEditor::~PluginEditor() {
    removed();
}

const char* PluginEditor::uiUrl() {
    return kUiUrl;
}

tresult PLUGIN_API PluginEditor::isPlatformTypeSupported(FIDString type) {
    if (type && (FIDStringsEqual(type, kPlatformTypeHWND) ||
                 FIDStringsEqual(type, kPlatformTypeNSView) ||
                 FIDStringsEqual(type, kPlatformTypeX11EmbedWindowID))) {
        return kResultTrue;
    }
    return kResultFalse;
}

tresult PLUGIN_API PluginEditor::attached(void* parent, FIDString /*type*/) {
    if (attached_) {
        return kResultFalse;
    }
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

tresult PLUGIN_API PluginEditor::onKeyDown(char16 /*key*/, int16 /*keyCode*/, int16 /*modifiers*/) {
    return kResultOk;
}

tresult PLUGIN_API PluginEditor::onKeyUp(char16 /*key*/, int16 /*keyCode*/, int16 /*modifiers*/) {
    return kResultOk;
}

tresult PLUGIN_API PluginEditor::getSize(ViewRect* size) {
    *size = ViewRect(0, 0, width_, height_);
    return kResultTrue;
}

tresult PLUGIN_API PluginEditor::onSize(ViewRect* newSize) {
    width_ = newSize->getWidth();
    height_ = newSize->getHeight();
    webView_.setSize(width_, height_);
    return kResultOk;
}

tresult PLUGIN_API PluginEditor::onFocus(TBool /*state*/) {
    return kResultOk;
}

tresult PLUGIN_API PluginEditor::setFrame(IPlugFrame* frame) {
    if (frame) {
        frame->addRef();
    }
    if (frame_) {
        frame_->release();
    }
    frame_ = frame;
    return kResultOk;
}

tresult PLUGIN_API PluginEditor::canResize() {
    return kResultTrue;
}

tresult PLUGIN_API PluginEditor::checkSizeConstraint(ViewRect* rect) {
    if (rect->getWidth() < kMinWidth) {
        rect->right = rect->left + kMinWidth;
    }
    if (rect->getHeight() < kMinHeight) {
        rect->bottom = rect->top + kMinHeight;
    }
    return kResultOk;
}

void PluginEditor::attachWebView(void* parentHandle) {
    if (!webView_.attach(parentHandle)) {
        return;
    }
    webView_.setSize(width_, height_);
    webView_.navigate(uiUrl());
}

void PluginEditor::onJavaScriptMessage(const char* message) {
    // Parse the bridge envelope and forward parameter changes to the host.
    // The bridge sends messages of the form:
    //   {"type":"setParameter","data":["<id>",<plain value>]}
    // The value is a plain (unnormalized) value in the parameter's own units
    // (Hz for freq, 0..1 for volume, 0/1 for mute); we normalize it via the
    // shared registry before handing it to the controller.
    if (message == nullptr) {
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
}

} // namespace netsdr
