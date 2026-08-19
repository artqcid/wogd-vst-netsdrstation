#include "plugin_editor.h"

#include "vst/common/paramids.h"

#include "base/source/fobject.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

#include <cstdlib>
#include <cstring>

namespace netsdr {

using namespace Steinberg;
using namespace Steinberg::Vst;

IMPLEMENT_FUNKNOWN_METHODS(PluginEditor, IPlugView, IPlugView::iid)

namespace {
const ViewRect kDefaultSize(0, 0, 640, 400);
const int32 kMinWidth = 320;
const int32 kMinHeight = 200;

// Vite dev server used for hot reload in debug builds (workspace-workflow.md).
#ifndef NDEBUG
constexpr const char* kUiUrl = "http://localhost:5173";
#else
constexpr const char* kUiUrl = "http://localhost:5173"; // TODO: serve bundled dist in release
#endif
} // namespace

PluginEditor::PluginEditor(Vst::IEditController* controller)
    : controller_(controller)
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
    // Parse the JSON envelope and forward parameter changes to the controller.
    // The bridge sends messages of the form:
    //   {"type":"setParameter","data":{"id":"freq","value":440.0}}
    // For the sine-synth proof we forward setParameter to the host so it can
    // automate the parameter and echo the change back to the DSP.
    if (message == nullptr) {
        return;
    }
    if (std::strstr(message, "\"type\":\"setParameter\"") != nullptr) {
        // Determine the parameter id and value from the payload.
        ParamID tag = kParamVolume;
        double normalized = 0.0;
        const char* idMark = std::strstr(message, "\"id\":\"");
        const char* valueMark = std::strstr(message, "\"value\":");
        if (idMark != nullptr && valueMark != nullptr) {
            const char* idStart = idMark + 7;
            const char* valueStart = valueMark + 8;
            if (std::strncmp(idStart, "freq", 4) == 0) {
                tag = kParamFreq;
            } else if (std::strncmp(idStart, "mute", 4) == 0) {
                tag = kParamMute;
            }
            normalized = std::strtod(valueStart, nullptr);
        }
        controller_->setParamNormalized(tag, normalized);
    }
}

} // namespace netsdr
