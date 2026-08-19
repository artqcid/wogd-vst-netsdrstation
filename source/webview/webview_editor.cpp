#include "webview_editor.h"

#include "webview/webview.h"

#include <memory>

namespace netsdr {

class WebViewHost::Impl {
public:
    Impl() = default;
    ~Impl() { detach(); }

    bool attach(void* parentHandle) {
        if (w_) {
            return true; // already attached
        }
        try {
            // webview::webview(debug, window). Passing a parent HWND embeds the
            // webview as a child of the host window instead of creating a
            // top-level window of its own.
            w_ = std::make_unique<webview::webview>(/*debug=*/true, parentHandle);
        } catch (const std::exception&) {
            w_.reset();
            return false;
        }

        // Bridge: expose a "vstHost" namespace to the JS side. Every bound
        // function forwards its JSON payload to the message handler.
        w_->bind("vstHostSetParameter", [this](const std::string& req) {
            dispatchMessage("setParameter", req);
            return std::string{};
        });
        w_->bind("vstHostGetParameters", [this](const std::string& /*req*/) {
            dispatchMessage("getParameters", "");
            return std::string{};
        });
        w_->bind("vstHostResize", [this](const std::string& req) {
            dispatchMessage("resize", req);
            return std::string{};
        });

        // Expose a stable `window.vstHost` object that wraps the flat bindings.
        // This matches the contract expected by ui/src/services/pluginService.ts.
        const std::string bridgeJs = R"js(
window.vstHost = {
  setParameter: function (id, value) {
    window.vstHostSetParameter(JSON.stringify({ id: id, value: value }));
  },
  getParameters: function () {
    window.vstHostGetParameters();
  }
};
)js";
        w_->init(bridgeJs);
        return true;
    }

    void detach() {
        if (w_) {
            w_.reset();
        }
    }

    bool navigate(const std::string& url) {
        if (!w_) {
            return false;
        }
        const auto err = w_->navigate(url);
        return !err.has_error();
    }

    bool eval(const std::string& js) {
        if (!w_) {
            return false;
        }
        const auto err = w_->eval(js);
        return !err.has_error();
    }

    void setSize(int width, int height) {
        if (w_) {
            w_->set_size(width, height, WEBVIEW_HINT_NONE);
        }
    }

    void setMessageHandler(MessageHandler handler, void* userData) {
        handler_ = handler;
        userData_ = userData;
    }

private:
    void dispatchMessage(const std::string& type, const std::string& payload) {
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
    void* userData_ = nullptr;
};

WebViewHost::WebViewHost() : impl_(new Impl()) {}
WebViewHost::~WebViewHost() { delete impl_; }

bool WebViewHost::attach(void* parentHandle) { return impl_->attach(parentHandle); }
void WebViewHost::detach() { impl_->detach(); }
bool WebViewHost::navigate(const std::string& url) { return impl_->navigate(url); }
bool WebViewHost::eval(const std::string& js) { return impl_->eval(js); }
void WebViewHost::setSize(int width, int height) { impl_->setSize(width, height); }
void WebViewHost::setMessageHandler(MessageHandler handler, void* userData) {
    impl_->setMessageHandler(handler, userData);
}

} // namespace netsdr
