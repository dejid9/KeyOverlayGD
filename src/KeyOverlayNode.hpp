#pragma once
#include <Geode/Geode.hpp>
#include "Config.hpp"
#include <vector>
#include <chrono>

using namespace geode::prelude;

struct KeyBar {
    float startY = 0.0f;
    float length = 0.0f;
    bool active = true;
};

struct KeyColumnState {
    bool isPressed = false;
    std::vector<KeyBar> bars;
    std::vector<std::chrono::steady_clock::time_point> clickTimestamps;
    CCLabelBMFont* labelNode = nullptr;
    CCLabelBMFont* counterNode = nullptr;
};

class KeyOverlayNode : public CCNode {
protected:
    CCDrawNode* m_keyShadowDraw = nullptr;
    CCDrawNode* m_bgDraw = nullptr;
    CCClippingNode* m_clipNode = nullptr;
    CCDrawNode* m_barShadowDraw = nullptr;
    CCDrawNode* m_barsDraw = nullptr;
    CCDrawNode* m_clipStencil = nullptr;
    CCDrawNode* m_boundsDraw = nullptr;

    std::vector<KeyColumnState> m_columns;
    bool m_isPreview = false;
    bool m_overrideShowBounds = false;
    float m_totalWidth = 0.0f;
    float m_totalHeight = 0.0f;
    float m_accumulatedDt = 0.0f;

    bool init(bool isPreview);
    void update(float dt) override;

    void drawRectWithBorder(CCDrawNode* draw, CCPoint origin, CCPoint size, float radius, ccColor4B fill, float borderWidth, ccColor4B border);
    void drawBlurredRect(CCDrawNode* draw, CCPoint origin, CCPoint size, float radius, ccColor4B color, float blurRadius);

public:
    static KeyOverlayNode* create(bool isPreview = false);
    static KeyOverlayNode* getActive();

    void updateLayout();
    void redraw();
    void setKeyPressed(size_t index, bool pressed);
    void startNewBar(size_t index);
    void setKeyColumnPressedState(size_t index, bool pressed);
    void updateCounterDisplay(size_t index);
    size_t getColumnCount() const { return m_columns.size(); }

    void handleRawKey(int keyCode, bool down);
    void handleMouseButton(int button, bool down);
    void handleGDAction(int action, bool down, int player = 1);
    void handlePlayerButton(PlayerButton button, bool down, bool isSecondPlayer = false);
    void releaseAllKeys();
    void syncHoldOnReset();
    void syncKeyStates();

    void setOverrideShowBounds(bool show) {
        m_overrideShowBounds = show;
        redraw();
    }

    void resetCounters();
    bool isPreview() const { return m_isPreview; }

    float getTotalWidth() const { return m_totalWidth; }
    float getTotalHeight() const { return m_totalHeight; }
};
