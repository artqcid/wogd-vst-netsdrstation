#include "kiwi_client.h"

#include "kiwi_commands.h"
#include "common/diag.h"        // <windows.h> (WIN32_LEAN_AND_MEAN) before file_logger.h
#include "util/file_logger.h"

#include <chrono>
#include <thread>

namespace netsdr {

namespace {
// Sends a frame when the connection is open; returns false otherwise.
bool sendIfOpen(KiwiConnection& connection, const std::string& frame) {
    if (!connection.isConnected()) {
        return false;
    }
    return connection.sendText(frame);
}
} // namespace

KiwiClient::~KiwiClient() {
    destroying_.store(true);
    disconnect();
    // Wait for reconnect thread to finish
    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }
    // Wait for the keepalive timer to finish
    if (keepaliveThread_.joinable()) {
        keepaliveThread_.join();
    }
}

bool KiwiClient::connect(const KiwiClientConfig& config) {
    config_ = config;
    if (config_.host.empty()) {
        return false;
    }

    connection_.setServer(config_.host, config_.port);
    connection_.setCallbacks(KiwiConnection::Callbacks{
        // Text messages from the server (e.g. MSG sample_rate=...) forwarded.
        [this](const std::string& message) {
            handleTextMessage(message);
            if (onTextMessage_) {
                onTextMessage_(message);
            }
        },
        // Binary messages are audio frames; forwarded to the DSP bridge.
        [this](const std::string& message) {
            sendKeepaliveThrottled(); // Throttled to 1 Hz (matches kiwirecorder.py)
            if (onBinaryMessage_) {
                onBinaryMessage_(message);
            }
        },
        // On open: run the handshake sequence (Phase 1 only), then fire the callback.
        [this]() {
            handshakePhase2Done_.store(false);
            reconnectAttempts_.store(0); // Reset reconnect counter on successful connection
            keepaliveLogged_.store(false); // Log keepalive once per (re)connect
            // Phase 1: mark the connection as external, then authenticate
            // (anonymous). `SET options` must precede `SET auth` (kiwiclient ref).
            connection_.sendText(kiwiOptionsCommand());
            connection_.sendText(kiwiAuthCommand());
            if (onOpen_) {
                onOpen_();
            }
        },
        // On error: fire the callback if installed.
        [this]() {
            if (onError_) {
                onError_();
            }
        },
        // On close: schedule reconnect with exponential backoff (max 3 attempts).
        [this]() {
            scheduleReconnect();
        },
    });

    const bool started = connection_.connect();

    // Start the background keepalive timer if not already running. It sends
    // `SET keepalive` at 1 Hz independent of audio-frame delivery so the server
    // never hits its inactivity timeout during short SND pauses (FIX-40).
    if (started && !keepaliveRunning_.exchange(true)) {
        if (keepaliveThread_.joinable()) {
            keepaliveThread_.join();
        }
        keepaliveThread_ = std::thread([this]() { keepaliveLoop(); });
    }

    return started;
}

// Sends `SET keepalive` at 1 Hz while the client is connected, gated by the
// same second-timestamp throttle as the SND callback so it never double-sends.
//
// IMPORTANT: keepalive is only sent AFTER the handshake (phase 2) has completed
// (`handshakePhase2Done_`). Sending `SET keepalive` before the audio channel is
// initialised makes the KiwiSDR server place the connection in "monitor" mode
// and never start the SND audio stream (observed against kphsdr.com:8073,
// firmware v1.900). The reference client only sends keepalive after
// `sample_rate`/`audio_rate` and in the SND binary callback.
void KiwiClient::keepaliveLoop() {
    while (!destroying_.load() && keepaliveRunning_.load()) {
        if (connection_.isConnected() && handshakePhase2Done_.load()) {
            sendKeepaliveThrottled();
        }
        // Wake often so shutdown is responsive, but only send once per second.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void KiwiClient::disconnect() {
    handshakePhase2Done_.store(false);
    reconnectRunning_.store(false);
    // Stop the keepalive timer thread (idempotent).
    keepaliveRunning_.store(false);
    if (keepaliveThread_.joinable()) {
        keepaliveThread_.join();
    }
    // Don't join - thread may be detached or running
    // The thread will exit when reconnectRunning_ is false
    connection_.disconnect();
}

bool KiwiClient::isConnected() const {
    return connection_.isConnected();
}

bool KiwiClient::setFrequency(double freqKhz) {
    config_.freqKhz = freqKhz;
    return sendIfOpen(connection_, kiwiSetModFreqCommand(
                                       config_.mode, config_.lowCut,
                                       config_.highCut, freqKhz));
}

bool KiwiClient::setTuning(const std::string& mode, int lowCut, int highCut,
                           double freqKhz) {
    config_.mode = mode;
    config_.lowCut = lowCut;
    config_.highCut = highCut;
    config_.freqKhz = freqKhz;
    return sendIfOpen(connection_, kiwiSetModFreqCommand(mode, lowCut, highCut, freqKhz));
}

bool KiwiClient::setAgc(bool on, bool hang, int thresh, int slope, int decay,
                        int manGain) {
    config_.agcOn = on;
    return sendIfOpen(connection_, kiwiAgcCommand(on, hang, thresh, slope, decay, manGain));
}

bool KiwiClient::setSquelch(bool on, double threshold) {
    return sendIfOpen(connection_, kiwiSquelchCommand(on, threshold));
}

bool KiwiClient::setNb(bool on, double threshold) {
    return sendIfOpen(connection_, kiwiNbCommand(on, threshold));
}

bool KiwiClient::setNr(bool on) {
    return sendIfOpen(connection_, kiwiNrCommand(on));
}

bool KiwiClient::setDeemp(bool on) {
    return sendIfOpen(connection_, kiwiDeempCommand(on));
}

bool KiwiClient::setComp(bool on) {
    return sendIfOpen(connection_, kiwiCompCommand(on));
}

void KiwiClient::setOnTextMessage(TextMessageCallback callback) {
    onTextMessage_ = std::move(callback);
}

void KiwiClient::setOnBinaryMessage(BinaryMessageCallback callback) {
    onBinaryMessage_ = std::move(callback);
}

void KiwiClient::setOnOpen(StateCallback cb) {
    onOpen_ = std::move(cb);
}

void KiwiClient::setOnError(StateCallback cb) {
    onError_ = std::move(cb);
}

void KiwiClient::setOnClose(StateCallback cb) {
    onClose_ = std::move(cb);
}

void KiwiClient::handleTextMessage(const std::string& message) {
    if (handshakePhase2Done_.load()) {
        return;
    }
    // Trigger: server sends "MSG ... audio_rate=..." after successful auth.
    // This arrives as a binary MSG frame routed to the text handler by kiwi_connection.
    if (message.find("audio_rate=") == std::string::npos) {
        return;
    }
    handshakePhase2Done_.store(true);

    // Parse the server's actual audio rate (e.g. "audio_rate=12000"). Some Kiwis
    // use a different rate, so hard-coding 12000 would ack the wrong rate and the
    // server never activates the SND audio stream. Fall back to the configured
    // rate if the value cannot be parsed.
    int audioRate = static_cast<int>(config_.sampleRate);
    const auto pos = message.find("audio_rate=");
    if (pos != std::string::npos) {
        try {
            const double rate = std::stod(message.substr(pos + 11));
            if (rate > 0.0 && rate < 1000000.0) {
                audioRate = static_cast<int>(rate);
            }
        } catch (...) {
        }
    }

    // Acknowledge the audio rate (this is what activates the SND stream), then
    // initialise the receive path, mirroring the kiwiclient reference sequence:
    //   audio_rate -> SET AR OK in=<rate> out=<rate>
    //   then squelch/genattn/gen/mod/agc + keepalive.
    connection_.sendText(kiwiArOkCommand(audioRate, audioRate));
    if (!config_.userName.empty()) {
        connection_.sendText(kiwiIdentUserCommand(config_.userName));
    }
    connection_.sendText(kiwiSquelchMaxCommand(false, 0));
    connection_.sendText(kiwiGenAttnCommand(0));
    connection_.sendText(kiwiGenCommand(0, -1));
    connection_.sendText(kiwiSetModFreqCommand(
        config_.mode, config_.lowCut, config_.highCut, config_.freqKhz));
    connection_.sendText(kiwiAgcCommand(
        config_.agcOn, false, -130, 6, 1000, 20));
    connection_.sendText(kiwiKeepaliveCommand());
}

void KiwiClient::scheduleReconnect() {
    // Don't schedule reconnect if we're being destroyed
    if (destroying_.load()) {
        return;
    }

    const int attempts = reconnectAttempts_.load();
    if (attempts >= kMaxReconnectAttempts) {
        // Max attempts reached, give up and notify
        if (onClose_) {
            onClose_();
        }
        return;
    }

    // Notify the UI that the connection dropped and we are reconnecting. This
    // makes the previously-silent auto-reconnect visible to the user (FIX-40):
    // the GUI shows a transient disconnected/reconnecting state instead of
    // staying "Connected" the whole time.
    if (onClose_) {
        onClose_();
    }

    // Stop any existing reconnect thread (non-blocking)
    reconnectRunning_.store(false);
    // Don't join here - this callback runs on the WebSocket thread
    // and joining could deadlock. The thread will be joined in the destructor.

    // Detach the old thread if it's still running
    if (reconnectThread_.joinable()) {
        reconnectThread_.detach();
    }

    // Start new reconnect thread
    reconnectRunning_.store(true);
    reconnectThread_ = std::thread([this]() {
        reconnectLoop();
    });
}

void KiwiClient::sendKeepaliveThrottled() {
    // Send keepalive at most once per second (matches kiwirecorder.py behavior).
    // The server may interpret more frequent keepalives as spam and disconnect.
    const auto now = std::chrono::system_clock::now();
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    const auto lastSecs = lastKeepaliveSecs_.load();
    if (secs != lastSecs) {
        lastKeepaliveSecs_.store(secs);
        if (connection_.isConnected()) {
            connection_.sendText(kiwiKeepaliveCommand());
            if (!keepaliveLogged_.exchange(true)) {
                NETSDR_LOG_DEBUG("keepalive started");
            }
        }
    }
}

void KiwiClient::reconnectLoop() {
    const int attempt = reconnectAttempts_.load();
    // Exponential backoff: 1s, 2s, 4s
    const int delayMs = 1000 * (1 << attempt);

    // Sleep in 100ms steps for responsive shutdown
    for (int i = 0; i < delayMs / 100 && reconnectRunning_.load() && !destroying_.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!reconnectRunning_.load() || destroying_.load()) {
        return; // disconnect() was called or we're being destroyed
    }

    // Re-set callbacks before reconnect (disconnect() clears them)
    connection_.setCallbacks(KiwiConnection::Callbacks{
        [this](const std::string& message) {
            handleTextMessage(message);
            if (onTextMessage_) {
                onTextMessage_(message);
            }
        },
        [this](const std::string& message) {
            sendKeepaliveThrottled();
            if (onBinaryMessage_) {
                onBinaryMessage_(message);
            }
        },
        [this]() {
            handshakePhase2Done_.store(false);
            reconnectAttempts_.store(0);
            keepaliveLogged_.store(false); // Log keepalive once per (re)connect
            connection_.sendText(kiwiOptionsCommand());
            connection_.sendText(kiwiAuthCommand());
            if (onOpen_) {
                onOpen_();
            }
        },
        [this]() {
            if (onError_) {
                onError_();
            }
        },
        [this]() {
            scheduleReconnect();
        },
    });

    // Attempt to reconnect
    if (connection_.connect()) {
        reconnectAttempts_.store(attempt + 1);
    } else {
        reconnectAttempts_.store(attempt + 1);
        if (onClose_ && !destroying_.load()) {
            onClose_();
        }
    }
}



} // namespace netsdr