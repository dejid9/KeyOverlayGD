#pragma once
#include <Geode/Geode.hpp>
#include "Config.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>

#ifdef GEODE_IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

class KeyOverlayNode;

struct KeyPollState {
    int vk = 0;
    bool isMouse = false;
    int mouseButton = 0; // 0=L, 1=R
    bool isGDAction = false;
    int gdButton = 1;
    int gdPlayer = 1;

    bool isDown = false;
    int pendingClicks = 0;
    bool isPressed = false;
    int currentCPS = 0;

    std::chrono::steady_clock::time_point lastPressTime{};
    std::vector<std::chrono::steady_clock::time_point> clickTimestamps;
};

class InputPoller {
private:
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::vector<KeyPollState> m_keys;
    std::mutex m_keysMutex;
    std::atomic<int> m_pollingRate{1000}; // Hz

    InputPoller() = default;
    void run();

public:
    static InputPoller& get();

    ~InputPoller() {
        stop();
    }

    void start();
    void stop();
    void setPollingRate(int hz);
    void updateKeysFromConfig();

    void clearPendingClicks();
    void syncToOverlay(KeyOverlayNode* overlay);
};
