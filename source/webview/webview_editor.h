#pragma once
// A minimal, platform-agnostic wrapper around webview/webview (MIT).
//
// Hides the webview/webview C++ API behind a tiny interface so the rest of the
// plugin (IPlugView editor, controller) never needs to include webview.h.
// WebViewHost owns a webview instance, binds the "vstHost" JS namespace and
// forwards JS messages to a user-supplied callback.

#include <cstdint>
#include <memory>
#include <string>

namespace netsdr {

class WebViewHost {
public:
    // onMessage is invoked (on the webview/UI thread) whenever the JS side
    // calls a bound "vstHost.*" function. The argument is the JSON payload.
    using MessageHandler = void (*)(const char* message, void* userData);

    WebViewHost();
    ~WebViewHost();

    WebViewHost(const WebViewHost&) = delete;
    WebViewHost& operator=(const WebViewHost&) = delete;

    // Attaches the webview to an existing OS window handle (HWND/NSView/gtk).
    // Returns false on failure.
    bool attach(void* parentHandle);

    // Detaches and destroys the webview.
    void detach();

    // Navigates the webview to the given URL.
    bool navigate(const std::string& url);

    // Executes JavaScript in the webview (C++ -> JS).
    bool eval(const std::string& js);

    // Sizes the embedded widget to fill the parent window's client area.
    // (FIX-22: the webview library's embedded mode never sizes its child
    // "widget" window itself, so we do it explicitly after attach and on every
    // resize.)
    void resizeToParent();

    // Registers the callback invoked on JS -> C++ messages.
    void setMessageHandler(MessageHandler handler, void* userData);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace netsdr
