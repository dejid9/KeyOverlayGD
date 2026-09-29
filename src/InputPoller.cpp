#include "InputPoller.hpp"
#include "KeyOverlayNode.hpp"
#include "SettingsPopup.hpp"
#include <algorithm>

InputPoller& InputPoller::get() {
    static InputPoller instance;
    return instance;
}

void InputPoller::start() {
#ifndef GEODE_IS_WINDOWS
    return;
#else
    if (m_running.load()) return;
    updateKeysFromConfig();
    m_running.store(true);
    m_thread = std::thread(&InputPoller::run, this);
#endif
}

void InputPoller::stop() {
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void InputPoller::setPollingRate(int hz) {
    m_pollingRate.store(hz);
}

void InputPoller::updateKeysFromConfig() {
    std::lock_guard<std::mutex> lock(m_keysMutex);
    auto& cfg = OverlayConfig::get();
    m_pollingRate.store(cfg.pollingRate);
    m_keys.clear();

    for (auto const& k : cfg.keys) {
        KeyPollState s;
        s.vk = k.keyCode;
        s.isMouse = k.isMouse;
        s.mouseButton = k.mouseButton;
        s.isGDAction = k.isGDAction;
        s.gdButton = k.gdButton;
        s.gdPlayer = k.gdPlayer;
        s.isDown = false;
        s.pendingClicks = 0;
        s.isPressed = false;
        s.currentCPS = 0;
        s.lastPressTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        m_keys.push_back(s);
    }
}

void InputPoller::run() {
    while (m_running.load()) {
        int rate = m_pollingRate.load();
        if (rate <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

#ifdef GEODE_IS_WINDOWS
        // Check if GD window is the foreground process
        DWORD fgProc = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &fgProc);
        bool isGameFocused = (fgProc == GetCurrentProcessId());

        if (isGameFocused) {
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(m_keysMutex);

            for (auto& k : m_keys) {
                if (k.isGDAction) {
                    continue;
                }
                bool down = false;
                if (k.isMouse) {
                    int vk = (k.mouseButton == 1) ? VK_RBUTTON : VK_LBUTTON;
                    down = (GetAsyncKeyState(vk) & 0x8000) != 0;
                } else if (k.vk != 0) {
                    down = (GetAsyncKeyState(k.vk) & 0x8000) != 0;
                }

                // Rising edge with 8ms hardware debounce filter
                if (down && !k.isDown) {
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - k.lastPressTime).count();
                    if (elapsedMs >= 8) {
                        k.lastPressTime = now;
                        k.pendingClicks++;
                        k.clickTimestamps.push_back(now);
                    }
                }
                k.isDown = down;
                k.isPressed = down;

                // Prune clicks older than 1000ms
                auto cutoff = now - std::chrono::milliseconds(1000);
                k.clickTimestamps.erase(
                    std::remove_if(k.clickTimestamps.begin(), k.clickTimestamps.end(),
                        [&](auto const& t) { return t < cutoff; }),
                    k.clickTimestamps.end()
                );
                k.currentCPS = static_cast<int>(k.clickTimestamps.size());
            }
        } else {
            std::lock_guard<std::mutex> lock(m_keysMutex);
            for (auto& k : m_keys) {
                k.isDown = false;
                k.isPressed = false;
            }
        }
#endif

        int sleepMicros = 1000000 / std::max(1, rate);
        if (sleepMicros > 1500) {
            std::this_thread::sleep_for(std::chrono::microseconds(sleepMicros));
        } else {
            auto target = std::chrono::steady_clock::now() + std::chrono::microseconds(sleepMicros);
            while (std::chrono::steady_clock::now() < target) {
                std::this_thread::yield();
            }
        }
    }
}

void InputPoller::clearPendingClicks() {
    std::lock_guard<std::mutex> lock(m_keysMutex);
    for (auto& k : m_keys) {
        k.pendingClicks = 0;
        k.clickTimestamps.clear();
        k.currentCPS = 0;
    }
}

void InputPoller::syncToOverlay(KeyOverlayNode* overlay) {
    if (!overlay) return;
    auto& cfg = OverlayConfig::get();
    std::lock_guard<std::mutex> lock(m_keysMutex);

    for (size_t i = 0; i < m_keys.size() && i < overlay->getColumnCount(); ++i) {
        auto& k = m_keys[i];
        if (k.isGDAction) {
            continue;
        }
        int clicks = k.pendingClicks;
        k.pendingClicks = 0;
        if (clicks > 0) {
            if (i < cfg.keys.size()) {
                cfg.keys[i].clickCount += clicks;
            }
            for (int c = 0; c < clicks; ++c) {
                overlay->startNewBar(i);
            }
        }

        bool down = k.isPressed;
        overlay->setKeyColumnPressedState(i, down);

        int cps = k.currentCPS;
        if (i < cfg.keys.size()) {
            cfg.keys[i].currentCPS = cps;
        }
        overlay->updateCounterDisplay(i);
    }
}
