#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_set>

using namespace geode::prelude;

struct KeyBindInfo {
    std::string id;
    std::string label;
    std::string customLabel;
    int keyCode = 0;          // cocos2d::enumKeyCodes
    bool isMouse = false;     // true for mouse buttons (0 = Left, 1 = Right)
    int mouseButton = 0;
    bool isGDAction = false;  // true for native GD actions (Jump P1, Left P1, Right P1)
    int gdButton = 1;         // 1 = Jump, 2 = Left, 3 = Right
    int gdPlayer = 1;         // 1 = Player 1, 2 = Player 2
    int clickCount = 0;
    int currentCPS = 0;
};

struct TouchTracker {
    static inline std::unordered_set<int> activeTouches;
    static bool hasActiveTouches() {
        return !activeTouches.empty();
    }
};

class OverlayConfig {
public:
    static OverlayConfig& get();

    // Key list
    std::vector<KeyBindInfo> keys;

    // Dimensions & Geometry
    CCPoint position = { 30.0f, 60.0f };
    float scale = 1.0f;
    float keyWidth = 44.0f;
    float keyHeight = 44.0f;
    float keySpacing = 6.0f;
    float streamLength = 160.0f;
    float barSpeed = 340.0f;
    bool scrollUpwards = true;
    bool showCounters = true;
    bool showLabels = true;
    bool showContainerBg = false;
    float cornerRadius = 2.0f;
    float borderWidth = 1.8f;
    bool showBounds = false;

    // Fonts & Text Positioning
    std::string keyFont = "bigFont.fnt";
    std::string counterFont = "bigFont.fnt";
    float labelScale = 0.38f;
    float counterScale = 0.33f;
    float labelOffsetY = 0.0f;
    float counterOffsetY = 0.0f;

    // Colors & Opacity (ccColor4B: r, g, b, a)
    ccColor4B containerBgColor = { 15, 15, 20, 150 };
    ccColor4B keyBgColor = { 20, 20, 25, 130 };
    ccColor4B keyPressedColor = { 255, 255, 255, 240 };
    ccColor4B barColor = { 255, 255, 255, 230 };
    ccColor4B keyBorderColor = { 255, 255, 255, 220 };
    ccColor4B labelColor = { 255, 255, 255, 240 };
    ccColor4B labelPressedColor = { 20, 20, 25, 255 };
    ccColor4B counterColor = { 255, 255, 255, 240 };

    // Soft Blurred Shadows
    bool shadowEnabled = true;
    ccColor4B shadowColor = { 0, 0, 0, 140 };
    float shadowDistance = 0.0f;
    float shadowAngle = 270.0f;
    float shadowBlur = 6.0f;

    CCPoint getShadowOffset() const {
        if (shadowDistance <= 0.001f) return CCPoint(0.0f, 0.0f);
        float rad = shadowAngle * 3.14159265f / 180.0f;
        return CCPoint(shadowDistance * std::cos(rad), shadowDistance * std::sin(rad));
    }

    // Performance & Input Polling
    float overlayFPS = 0.0f;        // 0 = Uncapped (Sync with Game FPS), or 60, 120, 144, 240, 360
    int pollingRate = 1000;         // Click Polling Frequency in Hz (0 = Off/Hook, 120, 250, 500, 1000, 2000)
    int counterMode = 0;            // 0 = Total Clicks, 1 = CPS, 2 = CPS | Total
    bool fastBarShadow = true;      // Fast single-pass bar shadow to eliminate 30 FPS stutter

    // Behavior
    bool enabled = true;
    bool resetCounterOnAttempt = false;
    bool hideInEditor = true;

    void load();
    void save();
    void resetDefaults();
};
