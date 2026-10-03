#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/binding/Slider.hpp>
#include <Geode/binding/SliderThumb.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include "Config.hpp"
#include "KeyOverlayNode.hpp"

using namespace geode::prelude;

class SettingsPopup : public geode::Popup {
protected:
    KeyOverlayNode* m_previewNode = nullptr;
    CCLayerColor* m_previewBg = nullptr;
    CCNode* m_tabContent = nullptr;
    CCMenu* m_tabButtonsMenu = nullptr;
    CCMenuItemToggler* m_masterToggler = nullptr;
    CCLabelBMFont* m_disabledLabel = nullptr;
    int m_previewPressedKeyIndex = -1;

    int m_currentTab = 0;
    int m_listeningKeyIndex = -1;

    bool init(float width, float height);

    void createSliderRow(CCNode* parent, CCPoint pos, char const* title, float currentVal, float minVal, float maxVal, int tag, SEL_MenuHandler handler, CCLabelBMFont*& outValLabel);
    void createFloatInputRow(CCNode* parent, CCPoint pos, char const* title, float currentVal, float minVal, float maxVal, int tag);
    void createToggleRow(CCNode* parent, CCPoint pos, char const* title, bool isToggled, int tag, SEL_MenuHandler handler, float toggleOffsetX = 90.0f);
    void createColorButtonRow(CCNode* parent, CCPoint pos, char const* title, ccColor4B const& color, int tag, SEL_MenuHandler handler, float buttonOffsetX = 80.0f);

    void onTabClicked(CCObject* sender);
    void onSlider(CCObject* sender);
    void onToggle(CCObject* sender);
    void onColorPick(CCObject* sender);

    void onRebindKey(CCObject* sender);
    void onToggleJumpBind(CCObject* sender);
    void onRenameKey(CCObject* sender);
    void onAddKey(CCObject* sender);
    void onRemoveKey(CCObject* sender);
    void onDeleteKeyIndex(CCObject* sender);
    void onResetCounters(CCObject* sender);

    void onCycleKeyFont(CCObject* sender);
    void onCycleCounterFont(CCObject* sender);

    void onPreviewBgColor(CCObject* sender);
    void onOpenInteractiveEditor(CCObject* sender);

    void onPresetPos(CCObject* sender);
    void onResetPos(CCObject* sender);
    void onNudgePos(CCObject* sender);

    void onCycleCounterMode(CCObject* sender);
    void onPresetFPS(CCObject* sender);
    void onPresetPolling(CCObject* sender);

    void onSave(CCObject* sender);
    void onReset(CCObject* sender);
    void onClose(CCObject* sender) override;
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override;

public:
    static SettingsPopup* s_activeInstance;

    ~SettingsPopup() {
        if (s_activeInstance == this) {
            s_activeInstance = nullptr;
        }
    }

    static SettingsPopup* create();
    void setupKeysTab();
    void setupFontsTab();
    void setupColorsTab();
    void setupLayoutTab();
    void setupEngineTab();
    void switchTab(int tab);
    void handleKeyInput(int keyCode, bool down);
    void updatePreview();
};
