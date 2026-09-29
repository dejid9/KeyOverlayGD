#include "SettingsPopup.hpp"
#include "InputPoller.hpp"
#include <Geode/ui/TextInput.hpp>
#include <fmt/format.h>

static const std::vector<std::pair<std::string, std::string>> s_fontList = {
    { "bigFont.fnt", "Big Font" },
    { "goldFont.fnt", "Gold Font" },
    { "chatFont.fnt", "Chat Font" },
    { "gjFont01.fnt", "Font 01" },
    { "gjFont02.fnt", "Font 02" },
    { "gjFont03.fnt", "Font 03" },
    { "gjFont04.fnt", "Font 04" },
    { "gjFont05.fnt", "Font 05" },
    { "gjFont06.fnt", "Font 06" },
    { "gjFont07.fnt", "Font 07" },
    { "gjFont08.fnt", "Font 08" },
    { "gjFont09.fnt", "Font 09" },
    { "gjFont10.fnt", "Font 10" },
    { "gjFont11.fnt", "Font 11" },
    { "gjFont12.fnt", "Font 12" },
    { "gjFont13.fnt", "Font 13" },
    { "gjFont14.fnt", "Font 14" },
    { "gjFont15.fnt", "Font 15" }
};

static std::string getKeyName(int keyCode) {
    if (keyCode >= KEY_A && keyCode <= KEY_Z) {
        return std::string(1, static_cast<char>('A' + (keyCode - KEY_A)));
    }
    if (keyCode >= KEY_Zero && keyCode <= KEY_Nine) {
        return std::string(1, static_cast<char>('0' + (keyCode - KEY_Zero)));
    }
    switch (keyCode) {
        case KEY_Space: return "Sp";
        case KEY_Up: return "Up";
        case KEY_Down: return "Down";
        case KEY_Left: return "Left";
        case KEY_Right: return "Right";
        case KEY_Shift: return "Shift";
        case KEY_Control: return "Ctrl";
        case KEY_Alt: return "Alt";
        case KEY_Tab: return "Tab";
        case KEY_Enter: return "Enter";
        case KEY_Escape: return "Esc";
        default: return fmt::format("#{}", keyCode);
    }
}

// ---------------- INTERACTIVE POSITION EDITOR LAYER ----------------
class OverlayPositionEditor : public CCLayer {
protected:
    CCPoint m_lastTouch;
    bool m_dragging = false;
    CCLabelBMFont* m_statusLabel = nullptr;
    SettingsPopup* m_parentPopup = nullptr;

    bool init(SettingsPopup* popup) {
        if (!CCLayer::init()) return false;
        m_parentPopup = popup;

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-500);

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Top Banner HUD
        auto topBar = CCScale9Sprite::create("square02_001.png");
        topBar->setContentSize({ winSize.width - 40.0f, 44.0f });
        topBar->setColor(ccc3(15, 15, 20));
        topBar->setOpacity(220);
        topBar->setPosition({ winSize.width * 0.5f, winSize.height - 28.0f });
        this->addChild(topBar);

        auto title = CCLabelBMFont::create("Drag overlay to move • Snap or scale below", "goldFont.fnt");
        title->setScale(0.42f);
        title->setPosition({ topBar->getContentSize().width * 0.5f, 30.0f });
        topBar->addChild(title);

        auto& cfg = OverlayConfig::get();
        m_statusLabel = CCLabelBMFont::create(
            fmt::format("Pos: ({:.0f}, {:.0f}) | Scale: {:.2f}x", cfg.position.x, cfg.position.y, cfg.scale).c_str(),
            "chatFont.fnt"
        );
        m_statusLabel->setScale(0.55f);
        m_statusLabel->setPosition({ topBar->getContentSize().width * 0.5f, 12.0f });
        topBar->addChild(m_statusLabel);

        // Control Menu
        auto menu = CCMenu::create();
        menu->setPosition({ winSize.width * 0.5f, winSize.height - 62.0f });
        this->addChild(menu);

        auto makeBtn = [&](char const* text, SEL_MenuHandler handler, int tag, char const* sprName) {
            auto spr = ButtonSprite::create(text, "goldFont.fnt", sprName, 0.6f);
            spr->setScale(0.52f);
            auto btn = CCMenuItemSpriteExtra::create(spr, this, handler);
            btn->setTag(tag);
            menu->addChild(btn);
            return btn;
        };

        makeBtn("Scale -", menu_selector(OverlayPositionEditor::onAdjustScale), -1, "GJ_button_04.png");
        makeBtn("Scale +", menu_selector(OverlayPositionEditor::onAdjustScale), 1, "GJ_button_04.png");
        makeBtn("BL", menu_selector(OverlayPositionEditor::onSnapCorner), 1, "GJ_button_05.png");
        makeBtn("BR", menu_selector(OverlayPositionEditor::onSnapCorner), 2, "GJ_button_05.png");
        makeBtn("TL", menu_selector(OverlayPositionEditor::onSnapCorner), 3, "GJ_button_05.png");
        makeBtn("TR", menu_selector(OverlayPositionEditor::onSnapCorner), 4, "GJ_button_05.png");
        makeBtn("Done", menu_selector(OverlayPositionEditor::onDone), 0, "GJ_button_01.png");

        menu->setLayout(RowLayout::create()->setGap(5.0f));

        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->setOverrideShowBounds(true);
        }

        return true;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        m_lastTouch = touch->getLocation();
        m_dragging = true;
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        CCPoint cur = touch->getLocation();
        CCPoint delta = cur - m_lastTouch;
        m_lastTouch = cur;

        auto& cfg = OverlayConfig::get();
        cfg.position = cfg.position + delta;

        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->setPosition(cfg.position);
            overlay->updateLayout();
        }

        if (m_statusLabel) {
            m_statusLabel->setString(fmt::format("Pos: ({:.0f}, {:.0f}) | Scale: {:.2f}x", cfg.position.x, cfg.position.y, cfg.scale).c_str());
        }
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_dragging = false;
        OverlayConfig::get().save();
    }

    void onAdjustScale(CCObject* sender) {
        int dir = static_cast<CCNode*>(sender)->getTag();
        auto& cfg = OverlayConfig::get();
        cfg.scale = std::clamp(cfg.scale + dir * 0.05f, 0.4f, 2.0f);

        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->setScale(cfg.scale);
            overlay->updateLayout();
        }

        if (m_statusLabel) {
            m_statusLabel->setString(fmt::format("Pos: ({:.0f}, {:.0f}) | Scale: {:.2f}x", cfg.position.x, cfg.position.y, cfg.scale).c_str());
        }
        cfg.save();
    }

    void onSnapCorner(CCObject* sender) {
        int tag = static_cast<CCNode*>(sender)->getTag();
        auto& cfg = OverlayConfig::get();
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        float w = 180.0f * cfg.scale;
        float h = (cfg.keyHeight + cfg.streamLength + 20.0f) * cfg.scale;
        float margin = 20.0f;

        switch (tag) {
            case 1: cfg.position = CCPoint(margin, margin + 40.0f); break; // BL
            case 2: cfg.position = CCPoint(winSize.width - w - margin, margin + 40.0f); break; // BR
            case 3: cfg.position = CCPoint(margin, winSize.height - h - margin); break; // TL
            case 4: cfg.position = CCPoint(winSize.width - w - margin, winSize.height - h - margin); break; // TR
        }

        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->setPosition(cfg.position);
            overlay->updateLayout();
        }

        if (m_statusLabel) {
            m_statusLabel->setString(fmt::format("Pos: ({:.0f}, {:.0f}) | Scale: {:.2f}x", cfg.position.x, cfg.position.y, cfg.scale).c_str());
        }
        cfg.save();
    }

    void onDone(CCObject*) {
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->setOverrideShowBounds(false);
        }
        OverlayConfig::get().save();
        if (m_parentPopup) {
            m_parentPopup->setVisible(true);
            m_parentPopup->switchTab(3);
            m_parentPopup->updatePreview();
        }
        this->removeFromParentAndCleanup(true);
    }

public:
    static OverlayPositionEditor* create(SettingsPopup* popup) {
        auto ret = new OverlayPositionEditor();
        if (ret && ret->init(popup)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ---------------- RENAME KEY POPUP ----------------
class RenameKeyPopup : public geode::Popup {
protected:
    int m_keyIndex = 0;
    TextInput* m_input = nullptr;

    bool init(int keyIndex) {
        if (!Popup::init(260.0f, 150.0f)) return false;
        m_keyIndex = keyIndex;
        this->setTitle("Rename Key");

        auto& cfg = OverlayConfig::get();
        std::string cur = "";
        if (keyIndex >= 0 && keyIndex < static_cast<int>(cfg.keys.size())) {
            cur = cfg.keys[keyIndex].customLabel.empty() ? cfg.keys[keyIndex].label : cfg.keys[keyIndex].customLabel;
        }

        m_input = TextInput::create(160.0f, "Custom Label");
        m_input->setString(cur);
        m_input->setPosition({ m_mainLayer->getContentSize().width * 0.5f, m_mainLayer->getContentSize().height * 0.5f + 12.0f });
        m_mainLayer->addChild(m_input);

        auto btnSpr = ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png", 0.7f);
        btnSpr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(RenameKeyPopup::onApply));
        auto menu = CCMenu::create();
        menu->addChild(btn);
        menu->setPosition({ m_mainLayer->getContentSize().width * 0.5f, m_mainLayer->getContentSize().height * 0.5f - 30.0f });
        m_mainLayer->addChild(menu);

        return true;
    }

    void onApply(CCObject*) {
        if (m_input) {
            auto& cfg = OverlayConfig::get();
            if (m_keyIndex >= 0 && m_keyIndex < static_cast<int>(cfg.keys.size())) {
                cfg.keys[m_keyIndex].customLabel = m_input->getString();
            }
        }
        if (SettingsPopup::s_activeInstance) {
            SettingsPopup::s_activeInstance->setupKeysTab();
            SettingsPopup::s_activeInstance->updatePreview();
        }
        this->onClose(nullptr);
    }

public:
    static RenameKeyPopup* create(int keyIndex) {
        auto ret = new RenameKeyPopup();
        if (ret && ret->init(keyIndex)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ---------------- SETTINGS POPUP ----------------
SettingsPopup* SettingsPopup::s_activeInstance = nullptr;

SettingsPopup* SettingsPopup::create() {
    auto ret = new SettingsPopup();
    if (ret && ret->init(500.0f, 300.0f)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool SettingsPopup::init(float width, float height) {
    if (!Popup::init(width, height)) return false;

    s_activeInstance = this;
    this->setTitle("KeyOverlay Settings");
    if (m_bgSprite) {
        m_bgSprite->setOpacity(225);
    }

    auto winSize = m_mainLayer->getContentSize();

    // 1. Right Side: Live Preview Panel
    m_previewBg = CCLayerColor::create(ccc4(20, 20, 25, 230), 140.0f, 215.0f);
    m_previewBg->ignoreAnchorPointForPosition(false);
    m_previewBg->setAnchorPoint({ 0.5f, 0.5f });
    m_previewBg->setPosition({ winSize.width - 85.0f, winSize.height * 0.5f - 12.0f });
    m_mainLayer->addChild(m_previewBg);

    auto previewTitle = CCLabelBMFont::create("Live Preview", "goldFont.fnt");
    previewTitle->setScale(0.48f);
    previewTitle->setPosition({ m_previewBg->getPositionX(), m_previewBg->getPositionY() + 96.0f });
    m_mainLayer->addChild(previewTitle);

    // Live KeyOverlayNode inside preview box
    m_previewNode = KeyOverlayNode::create(true);
    m_previewNode->setScale(0.68f);
    m_mainLayer->addChild(m_previewNode);

    // Preview Background Color Switcher buttons right under preview title
    auto bgMenu = CCMenu::create();
    bgMenu->setPosition({ m_previewBg->getPositionX(), m_previewBg->getPositionY() + 80.0f });
    m_mainLayer->addChild(bgMenu);

    auto makeBgBtn = [&](ccColor3B col, int tag) {
        auto spr = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
        spr->setScale(0.32f);
        spr->setColor(col);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onPreviewBgColor));
        btn->setTag(tag);
        bgMenu->addChild(btn);
    };

    makeBgBtn(ccc3(20, 20, 25), 1);    // Dark
    makeBgBtn(ccc3(245, 245, 250), 2); // Light
    makeBgBtn(ccc3(0, 100, 200), 3);   // Blue
    makeBgBtn(ccc3(120, 30, 160), 4);  // Purple
    bgMenu->setLayout(RowLayout::create()->setGap(3.0f));

    // 2. Left Side: Tab Buttons Menu
    m_tabButtonsMenu = CCMenu::create();
    m_tabButtonsMenu->setPosition({ 150.0f, winSize.height - 45.0f });
    m_mainLayer->addChild(m_tabButtonsMenu);

    auto makeTabBtn = [&](char const* text, int tab) {
        auto spr = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_01.png", 0.7f);
        spr->setScale(0.44f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onTabClicked));
        btn->setTag(tab);
        m_tabButtonsMenu->addChild(btn);
        return btn;
    };

    makeTabBtn("Keys", 0);
    makeTabBtn("Fonts", 1);
    makeTabBtn("Style", 2);
    makeTabBtn("Layout", 3);
    makeTabBtn("Engine", 4);
    m_tabButtonsMenu->setLayout(RowLayout::create()->setGap(3.0f));

    // 3. Tab Content Node
    m_tabContent = CCNode::create();
    m_tabContent->setPosition({ 15.0f, 42.0f });
    m_tabContent->setContentSize({ 310.0f, 205.0f });
    m_mainLayer->addChild(m_tabContent);

    // 4. Bottom Action Buttons
    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({ 155.0f, 22.0f });
    m_mainLayer->addChild(bottomMenu);

    auto saveSpr = ButtonSprite::create("Save & Apply", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    saveSpr->setScale(0.62f);
    auto saveBtn = CCMenuItemSpriteExtra::create(saveSpr, this, menu_selector(SettingsPopup::onSave));
    bottomMenu->addChild(saveBtn);

    auto resetSpr = ButtonSprite::create("Defaults", "goldFont.fnt", "GJ_button_06.png", 0.8f);
    resetSpr->setScale(0.62f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SettingsPopup::onReset));
    bottomMenu->addChild(resetBtn);

    bottomMenu->setLayout(RowLayout::create()->setGap(12.0f));

    switchTab(0);
    updatePreview();

    return true;
}

void SettingsPopup::onPreviewBgColor(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    if (!m_previewBg) return;

    switch (tag) {
        case 1: m_previewBg->setColor(ccc3(20, 20, 25)); break;
        case 2: m_previewBg->setColor(ccc3(245, 245, 250)); break;
        case 3: m_previewBg->setColor(ccc3(0, 100, 200)); break;
        case 4: m_previewBg->setColor(ccc3(120, 30, 160)); break;
    }
}

void SettingsPopup::switchTab(int tab) {
    if (m_currentTab != tab) {
        m_listeningKeyIndex = -1;
    }
    m_currentTab = tab;
    m_tabContent->removeAllChildren();

    if (tab == 0) setupKeysTab();
    else if (tab == 1) setupFontsTab();
    else if (tab == 2) setupColorsTab();
    else if (tab == 3) setupLayoutTab();
    else if (tab == 4) setupEngineTab();
}

void SettingsPopup::onTabClicked(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    m_listeningKeyIndex = -1;
    switchTab(btn->getTag());
}

// ---------------- TAB 0: KEYS ----------------
void SettingsPopup::setupKeysTab() {
    m_tabContent->removeAllChildren();
    auto& cfg = OverlayConfig::get();
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_tabContent->addChild(menu);

    auto title = CCLabelBMFont::create("Configured Keys (Max 5)", "goldFont.fnt");
    title->setScale(0.42f);
    title->setAnchorPoint({ 0.0f, 0.5f });
    title->setPosition({ 10.0f, 185.0f });
    m_tabContent->addChild(title);

    float startY = 158.0f;
    float rowGap = 28.0f;

    for (size_t i = 0; i < cfg.keys.size() && i < 5; ++i) {
        float y = startY - i * rowGap;

        auto label = CCLabelBMFont::create(fmt::format("Key {}:", i + 1).c_str(), "bigFont.fnt");
        label->setScale(0.30f);
        label->setAnchorPoint({ 0.0f, 0.5f });
        label->setPosition({ 8.0f, y });
        m_tabContent->addChild(label);

        bool isListening = (m_listeningKeyIndex == static_cast<int>(i));
        std::string dispText = isListening ? "Press..." : (cfg.keys[i].customLabel.empty() ? cfg.keys[i].label : cfg.keys[i].customLabel);
        char const* btnSprite = isListening ? "GJ_button_02.png" : "GJ_button_04.png";

        auto spr = ButtonSprite::create(dispText.c_str(), "goldFont.fnt", btnSprite, 0.65f);
        spr->setScale(0.50f);
        auto rebindBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onRebindKey));
        rebindBtn->setTag(static_cast<int>(i));
        rebindBtn->setPosition({ 80.0f, y });
        menu->addChild(rebindBtn);

        // Action/Key Mode Toggle
        std::string modeText = "RawKey";
        char const* modeBtnSpr = "GJ_button_05.png";
        if (cfg.keys[i].isGDAction) {
            int p = cfg.keys[i].gdPlayer;
            int b = cfg.keys[i].gdButton;
            if (b == 1) {
                modeText = (p == 2) ? "P2 Jump" : "P1 Jump";
                modeBtnSpr = (p == 2) ? "GJ_button_02.png" : "GJ_button_01.png";
            } else if (b == 2) {
                modeText = (p == 2) ? "P2 Left" : "P1 Left";
                modeBtnSpr = (p == 2) ? "GJ_button_04.png" : "GJ_button_03.png";
            } else if (b == 3) {
                modeText = (p == 2) ? "P2 Right" : "P1 Right";
                modeBtnSpr = (p == 2) ? "GJ_button_04.png" : "GJ_button_03.png";
            }
        }
        auto modeSpr = ButtonSprite::create(modeText.c_str(), "goldFont.fnt", modeBtnSpr, 0.6f);
        modeSpr->setScale(0.40f);
        auto modeBtn = CCMenuItemSpriteExtra::create(modeSpr, this, menu_selector(SettingsPopup::onToggleJumpBind));
        modeBtn->setTag(static_cast<int>(i));
        modeBtn->setPosition({ 145.0f, y });
        menu->addChild(modeBtn);

        // Rename Label Button
        auto renSpr = ButtonSprite::create("Text", "goldFont.fnt", "GJ_button_03.png", 0.6f);
        renSpr->setScale(0.42f);
        auto renBtn = CCMenuItemSpriteExtra::create(renSpr, this, menu_selector(SettingsPopup::onRenameKey));
        renBtn->setTag(static_cast<int>(i));
        renBtn->setPosition({ 205.0f, y });
        menu->addChild(renBtn);

        // Delete button
        if (cfg.keys.size() > 1) {
            auto delSpr = ButtonSprite::create("X", "goldFont.fnt", "GJ_button_06.png", 0.6f);
            delSpr->setScale(0.42f);
            auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(SettingsPopup::onDeleteKeyIndex));
            delBtn->setTag(static_cast<int>(i));
            delBtn->setPosition({ 260.0f, y });
            menu->addChild(delBtn);
        }
    }

    // Add / Remove / Clear buttons at bottom
    float btnY = 14.0f;
    if (cfg.keys.size() < 5) {
        auto addSpr = ButtonSprite::create("+ Add Key", "goldFont.fnt", "GJ_button_02.png", 0.65f);
        addSpr->setScale(0.48f);
        auto addBtn = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(SettingsPopup::onAddKey));
        addBtn->setPosition({ 55.0f, btnY });
        menu->addChild(addBtn);
    }
    if (cfg.keys.size() > 1) {
        auto remSpr = ButtonSprite::create("- Remove", "goldFont.fnt", "GJ_button_06.png", 0.65f);
        remSpr->setScale(0.48f);
        auto remBtn = CCMenuItemSpriteExtra::create(remSpr, this, menu_selector(SettingsPopup::onRemoveKey));
        remBtn->setPosition({ 150.0f, btnY });
        menu->addChild(remBtn);
    }
    auto clrSpr = ButtonSprite::create("Clear Counts", "goldFont.fnt", "GJ_button_05.png", 0.65f);
    clrSpr->setScale(0.48f);
    auto clrBtn = CCMenuItemSpriteExtra::create(clrSpr, this, menu_selector(SettingsPopup::onResetCounters));
    clrBtn->setPosition({ 245.0f, btnY });
    menu->addChild(clrBtn);
}

// ---------------- TAB 1: FONTS & TEXT ----------------
void SettingsPopup::setupFontsTab() {
    m_tabContent->removeAllChildren();
    auto& cfg = OverlayConfig::get();
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_tabContent->addChild(menu);

    auto makeFontSelector = [&](CCPoint pos, char const* title, std::string const& curFont, SEL_MenuHandler cycleHandler) {
        auto lbl = CCLabelBMFont::create(title, "bigFont.fnt");
        lbl->setScale(0.28f);
        lbl->setAnchorPoint({ 0.0f, 0.5f });
        lbl->setPosition(pos);
        m_tabContent->addChild(lbl);

        std::string fontName = curFont;
        for (auto const& p : s_fontList) {
            if (p.first == curFont) {
                fontName = p.second;
                break;
            }
        }

        auto prevSpr = ButtonSprite::create("<", "goldFont.fnt", "GJ_button_04.png", 0.6f);
        prevSpr->setScale(0.46f);
        auto prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, cycleHandler);
        prevBtn->setTag(-1);
        prevBtn->setPosition({ pos.x + 115.0f, pos.y });
        menu->addChild(prevBtn);

        auto fontLbl = CCLabelBMFont::create(fontName.c_str(), "goldFont.fnt");
        fontLbl->setScale(0.40f);
        fontLbl->setPosition({ pos.x + 175.0f, pos.y });
        m_tabContent->addChild(fontLbl);

        auto nextSpr = ButtonSprite::create(">", "goldFont.fnt", "GJ_button_04.png", 0.6f);
        nextSpr->setScale(0.46f);
        auto nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, cycleHandler);
        nextBtn->setTag(1);
        nextBtn->setPosition({ pos.x + 235.0f, pos.y });
        menu->addChild(nextBtn);
    };

    makeFontSelector({ 10.0f, 180.0f }, "Key Label Font:", cfg.keyFont, menu_selector(SettingsPopup::onCycleKeyFont));
    makeFontSelector({ 10.0f, 152.0f }, "Counter Font:", cfg.counterFont, menu_selector(SettingsPopup::onCycleCounterFont));

    CCLabelBMFont* lbl = nullptr;
    createSliderRow(m_tabContent, { 10.0f, 124.0f }, "Key Font Size", cfg.labelScale, 0.15f, 1.20f, 401, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 10.0f, 98.0f }, "Counter Size", cfg.counterScale, 0.15f, 1.20f, 402, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 10.0f, 72.0f }, "Key Text Y", cfg.labelOffsetY, -25.0f, 25.0f, 403, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 10.0f, 46.0f }, "Counter Y", cfg.counterOffsetY, -30.0f, 30.0f, 404, menu_selector(SettingsPopup::onSlider), lbl);

    // Text Colors - 3 separate clean columns with NO OVERLAPS!
    createColorButtonRow(m_tabContent, { 8.0f, 16.0f }, "Idle:", cfg.labelColor, 221, menu_selector(SettingsPopup::onColorPick), 50.0f);
    createColorButtonRow(m_tabContent, { 105.0f, 16.0f }, "Press:", cfg.labelPressedColor, 222, menu_selector(SettingsPopup::onColorPick), 55.0f);
    createColorButtonRow(m_tabContent, { 205.0f, 16.0f }, "Count:", cfg.counterColor, 223, menu_selector(SettingsPopup::onColorPick), 57.0f);
}

// ---------------- TAB 2: COLORS & SHADOW ----------------
void SettingsPopup::setupColorsTab() {
    m_tabContent->removeAllChildren();
    auto& cfg = OverlayConfig::get();

    // Key & Bar Colors
    createColorButtonRow(m_tabContent, { 8.0f, 182.0f }, "Key Idle", cfg.keyBgColor, 202, menu_selector(SettingsPopup::onColorPick), 74.0f);
    createColorButtonRow(m_tabContent, { 155.0f, 182.0f }, "Key Pressed", cfg.keyPressedColor, 203, menu_selector(SettingsPopup::onColorPick), 85.0f);

    createColorButtonRow(m_tabContent, { 8.0f, 156.0f }, "Waterfall", cfg.barColor, 204, menu_selector(SettingsPopup::onColorPick), 74.0f);
    createColorButtonRow(m_tabContent, { 155.0f, 156.0f }, "Key Border", cfg.keyBorderColor, 205, menu_selector(SettingsPopup::onColorPick), 85.0f);

    CCLabelBMFont* lbl = nullptr;
    createSliderRow(m_tabContent, { 8.0f, 128.0f }, "Key Opacity", static_cast<float>(cfg.keyBgColor.a) / 255.0f, 0.0f, 1.0f, 210, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 8.0f, 102.0f }, "Waterf Opacity", static_cast<float>(cfg.barColor.a) / 255.0f, 0.0f, 1.0f, 213, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 8.0f, 76.0f }, "Border Width", cfg.borderWidth, 0.0f, 4.0f, 211, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 8.0f, 50.0f }, "Corner Radius", cfg.cornerRadius, 0.0f, 12.0f, 212, menu_selector(SettingsPopup::onSlider), lbl);
    createSliderRow(m_tabContent, { 8.0f, 24.0f }, "Shadow Blur", cfg.shadowBlur, 0.0f, 15.0f, 214, menu_selector(SettingsPopup::onSlider), lbl);

    // Bottom Row: Shadow Toggle, Shadow Color, Container Box
    createToggleRow(m_tabContent, { 8.0f, 2.0f }, "Shadow", cfg.shadowEnabled, 305, menu_selector(SettingsPopup::onToggle), 52.0f);
    createColorButtonRow(m_tabContent, { 100.0f, 2.0f }, "Color", cfg.shadowColor, 206, menu_selector(SettingsPopup::onColorPick), 45.0f);
    createToggleRow(m_tabContent, { 190.0f, 2.0f }, "Container", cfg.showContainerBg, 307, menu_selector(SettingsPopup::onToggle), 68.0f);
}

// ---------------- TAB 3: POSITION & LAYOUT ----------------
void SettingsPopup::setupLayoutTab() {
    m_tabContent->removeAllChildren();
    auto& cfg = OverlayConfig::get();
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_tabContent->addChild(menu);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // Top: Drag on Screen & Reset Pos buttons + Presets (all on top row without colliding!)
    auto editSpr = ButtonSprite::create("Drag on Screen", "goldFont.fnt", "GJ_button_02.png", 0.65f);
    editSpr->setScale(0.50f);
    auto editBtn = CCMenuItemSpriteExtra::create(editSpr, this, menu_selector(SettingsPopup::onOpenInteractiveEditor));
    editBtn->setPosition({ 52.0f, 188.0f });
    menu->addChild(editBtn);

    auto resetPosSpr = ButtonSprite::create("Reset Pos", "goldFont.fnt", "GJ_button_06.png", 0.65f);
    resetPosSpr->setScale(0.50f);
    auto resetPosBtn = CCMenuItemSpriteExtra::create(resetPosSpr, this, menu_selector(SettingsPopup::onResetPos));
    resetPosBtn->setPosition({ 128.0f, 188.0f });
    menu->addChild(resetPosBtn);

    auto snapMenu = CCMenu::create();
    snapMenu->setPosition({ 238.0f, 188.0f });
    m_tabContent->addChild(snapMenu);

    auto makePresetBtn = [&](char const* text, int tag) {
        auto spr = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_04.png", 0.5f);
        spr->setScale(0.36f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onPresetPos));
        btn->setTag(tag);
        snapMenu->addChild(btn);
    };

    makePresetBtn("BL", 1);
    makePresetBtn("BR", 2);
    makePresetBtn("TL", 3);
    makePresetBtn("TR", 4);
    makePresetBtn("Ctr", 5);
    snapMenu->setLayout(RowLayout::create()->setGap(2.0f));

    // Numbered sliders with direct TextInput!
    createFloatInputRow(m_tabContent, { 8.0f, 160.0f }, "Position X", cfg.position.x, 0.0f, winSize.width, 311);
    createFloatInputRow(m_tabContent, { 8.0f, 134.0f }, "Position Y", cfg.position.y, 0.0f, winSize.height, 312);
    createFloatInputRow(m_tabContent, { 8.0f, 108.0f }, "Scale", cfg.scale, 0.3f, 2.0f, 301);
    createFloatInputRow(m_tabContent, { 8.0f, 82.0f }, "Key Size", cfg.keyWidth, 24.0f, 64.0f, 303);
    createFloatInputRow(m_tabContent, { 8.0f, 56.0f }, "Stream Len", cfg.streamLength, 40.0f, 300.0f, 304);
    createFloatInputRow(m_tabContent, { 8.0f, 30.0f }, "Speed", cfg.barSpeed, 80.0f, 900.0f, 302);

    // Bottom toggles - 4 evenly spaced toggles across 300px with NO OVERLAPS!
    createToggleRow(m_tabContent, { 6.0f, 4.0f }, "Reset/Att", cfg.resetCounterOnAttempt, 103, menu_selector(SettingsPopup::onToggle), 52.0f);
    createToggleRow(m_tabContent, { 80.0f, 4.0f }, "Upwards", cfg.scrollUpwards, 102, menu_selector(SettingsPopup::onToggle), 50.0f);
    createToggleRow(m_tabContent, { 152.0f, 4.0f }, "Counts", cfg.showCounters, 101, menu_selector(SettingsPopup::onToggle), 44.0f);
    createToggleRow(m_tabContent, { 218.0f, 4.0f }, "Bounds", cfg.showBounds, 310, menu_selector(SettingsPopup::onToggle), 44.0f);
}

void SettingsPopup::createFloatInputRow(CCNode* parent, CCPoint pos, char const* title, float currentVal, float minVal, float maxVal, int tag) {
    auto label = CCLabelBMFont::create(title, "bigFont.fnt");
    label->setScale(0.24f);
    label->setAnchorPoint({ 0.0f, 0.5f });
    label->setPosition(pos);
    parent->addChild(label);

    auto slider = Slider::create(this, menu_selector(SettingsPopup::onSlider), 0.40f);
    float pct = (currentVal - minVal) / (maxVal - minVal);
    pct = std::max(0.0f, std::min(1.0f, pct));
    slider->setValue(pct);
    slider->setTag(tag);
    if (slider->getThumb()) {
        slider->getThumb()->setTag(tag);
    }
    slider->setPosition(pos.x + 130.0f, pos.y);
    parent->addChild(slider);

    auto input = TextInput::create(55.0f, "0.0");
    input->setScale(0.50f);
    input->setPosition(pos.x + 220.0f, pos.y);
    input->setCommonFilter(CommonFilter::Float);
    input->setString(fmt::format("{:.1f}", currentVal));
    input->setTag(tag + 1000);
    input->setCallback([this, tag, minVal, maxVal, slider](std::string const& str) {
        auto res = numFromString<float>(str);
        if (auto val = res.ok()) {
            float clamped = std::clamp(*val, minVal, maxVal);
            slider->setValue((clamped - minVal) / (maxVal - minVal));
            auto& cfg = OverlayConfig::get();
            switch (tag) {
                case 301: cfg.scale = clamped; break;
                case 302: cfg.barSpeed = clamped; break;
                case 303: cfg.keyWidth = cfg.keyHeight = clamped; break;
                case 304: cfg.streamLength = clamped; break;
                case 311: cfg.position.x = clamped; break;
                case 312: cfg.position.y = clamped; break;
                case 501: cfg.overlayFPS = clamped; break;
                case 502: {
                    cfg.pollingRate = static_cast<int>(clamped);
                    InputPoller::get().setPollingRate(cfg.pollingRate);
                    break;
                }
            }
            updatePreview();
        }
    });
    parent->addChild(input);
}

// ---------------- HELPER CONTROLS ----------------
void SettingsPopup::createSliderRow(CCNode* parent, CCPoint pos, char const* title, float currentVal, float minVal, float maxVal, int tag, SEL_MenuHandler handler, CCLabelBMFont*& outValLabel) {
    auto label = CCLabelBMFont::create(title, "bigFont.fnt");
    label->setScale(0.28f);
    label->setAnchorPoint({ 0.0f, 0.5f });
    label->setPosition(pos);
    parent->addChild(label);

    auto slider = Slider::create(this, handler, 0.50f);
    float pct = (currentVal - minVal) / (maxVal - minVal);
    pct = std::max(0.0f, std::min(1.0f, pct));
    slider->setValue(pct);
    slider->setTag(tag);
    if (slider->getThumb()) {
        slider->getThumb()->setTag(tag);
    }
    slider->setPosition(pos.x + 148.0f, pos.y);
    parent->addChild(slider);

    std::string valStr = (tag == 210 || tag == 213) ? fmt::format("{:.0f}%", currentVal * 100.0f) : fmt::format("{:.1f}", currentVal);
    auto valLabel = CCLabelBMFont::create(valStr.c_str(), "chatFont.fnt");
    valLabel->setScale(0.46f);
    valLabel->setAnchorPoint({ 0.0f, 0.5f });
    valLabel->setPosition(pos.x + 225.0f, pos.y);
    valLabel->setTag(tag + 1000);
    parent->addChild(valLabel);
    outValLabel = valLabel;
}

void SettingsPopup::createToggleRow(CCNode* parent, CCPoint pos, char const* title, bool isToggled, int tag, SEL_MenuHandler handler, float toggleOffsetX) {
    auto label = CCLabelBMFont::create(title, "bigFont.fnt");
    label->setScale(0.24f);
    label->setAnchorPoint({ 0.0f, 0.5f });
    label->setPosition(pos);
    parent->addChild(label);

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    parent->addChild(menu);

    auto toggler = CCMenuItemToggler::createWithStandardSprites(this, handler, 0.44f);
    toggler->toggle(isToggled);
    toggler->setTag(tag);
    toggler->setPosition(pos.x + toggleOffsetX, pos.y);
    menu->addChild(toggler);
}

void SettingsPopup::createColorButtonRow(CCNode* parent, CCPoint pos, char const* title, ccColor4B const& color, int tag, SEL_MenuHandler handler, float buttonOffsetX) {
    auto label = CCLabelBMFont::create(title, "bigFont.fnt");
    label->setScale(0.25f);
    label->setAnchorPoint({ 0.0f, 0.5f });
    label->setPosition(pos);
    parent->addChild(label);

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    parent->addChild(menu);

    auto spr = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
    spr->setScale(0.38f);
    spr->setColor(ccc3(color.r, color.g, color.b));

    auto btn = CCMenuItemSpriteExtra::create(spr, this, handler);
    btn->setTag(tag);
    btn->setPosition(pos.x + buttonOffsetX, pos.y);
    menu->addChild(btn);
}

// ---------------- EVENT HANDLERS ----------------
void SettingsPopup::onSlider(CCObject* sender) {
    int tag = 0;
    float pct = 0.0f;

    if (auto thumb = typeinfo_cast<SliderThumb*>(sender)) {
        pct = thumb->getValue();
        tag = thumb->getTag();
        if (tag == 0 && thumb->getParent() && thumb->getParent()->getParent()) {
            tag = thumb->getParent()->getParent()->getTag();
        }
    } else if (auto slider = typeinfo_cast<Slider*>(sender)) {
        pct = slider->getValue();
        tag = slider->getTag();
    } else {
        return;
    }

    auto& cfg = OverlayConfig::get();
    float val = 0.0f;

    switch (tag) {
        case 210: { // Key Opacity
            cfg.keyBgColor.a = static_cast<GLubyte>(std::clamp(pct * 255.0f, 0.0f, 255.0f));
            val = pct * 100.0f;
            break;
        }
        case 213: { // Waterfall Opacity
            cfg.barColor.a = static_cast<GLubyte>(std::clamp(pct * 255.0f, 0.0f, 255.0f));
            val = pct * 100.0f;
            break;
        }
        case 211: { // Border Width
            cfg.borderWidth = val = pct * 4.0f;
            break;
        }
        case 212: { // Corner Radius
            cfg.cornerRadius = val = pct * 12.0f;
            break;
        }
        case 214: { // Shadow Blur
            cfg.shadowBlur = val = pct * 15.0f;
            break;
        }
        case 301: { // Overlay Scale
            cfg.scale = val = 0.3f + pct * (2.0f - 0.3f);
            break;
        }
        case 302: { // Scroll Speed
            cfg.barSpeed = val = 80.0f + pct * (900.0f - 80.0f);
            break;
        }
        case 303: { // Key Size
            cfg.keyWidth = cfg.keyHeight = val = 24.0f + pct * (64.0f - 24.0f);
            break;
        }
        case 304: { // Stream Length
            cfg.streamLength = val = 40.0f + pct * (300.0f - 40.0f);
            break;
        }
        case 311: { // Pos X
            auto win = CCDirector::sharedDirector()->getWinSize();
            cfg.position.x = val = pct * win.width;
            break;
        }
        case 312: { // Pos Y
            auto win = CCDirector::sharedDirector()->getWinSize();
            cfg.position.y = val = pct * win.height;
            break;
        }
        case 401: { // Key Text Size
            cfg.labelScale = val = 0.15f + pct * (1.20f - 0.15f);
            break;
        }
        case 402: { // Counter Text Size
            cfg.counterScale = val = 0.15f + pct * (1.20f - 0.15f);
            break;
        }
        case 403: { // Key Text Y Offset
            cfg.labelOffsetY = val = -25.0f + pct * (25.0f - (-25.0f));
            break;
        }
        case 404: { // Counter Y Offset
            cfg.counterOffsetY = val = -30.0f + pct * (30.0f - (-30.0f));
            break;
        }
        case 501: { // Overlay FPS (0 to 360)
            cfg.overlayFPS = val = pct * 360.0f;
            break;
        }
        case 502: { // Polling Rate Hz (0 to 2000)
            int hz = static_cast<int>(pct * 2000.0f);
            cfg.pollingRate = hz;
            val = static_cast<float>(hz);
            InputPoller::get().setPollingRate(hz);
            break;
        }
        default: break;
    }

    if (auto input = typeinfo_cast<TextInput*>(m_tabContent->getChildByTag(tag + 1000))) {
        if (tag == 501 || tag == 502) {
            input->setString(fmt::format("{:.0f}", val));
        } else {
            input->setString(fmt::format("{:.1f}", val));
        }
    }
    if (auto valLabel = typeinfo_cast<CCLabelBMFont*>(m_tabContent->getChildByTag(tag + 1000))) {
        if (tag == 210 || tag == 213) {
            valLabel->setString(fmt::format("{:.0f}%", val).c_str());
        } else if (tag == 501 || tag == 502) {
            valLabel->setString(fmt::format("{:.0f}", val).c_str());
        } else {
            valLabel->setString(fmt::format("{:.1f}", val).c_str());
        }
    }

    updatePreview();
}

void SettingsPopup::onToggle(CCObject* sender) {
    auto toggler = static_cast<CCMenuItemToggler*>(sender);
    bool on = !toggler->isToggled();
    auto& cfg = OverlayConfig::get();

    switch (toggler->getTag()) {
        case 101: cfg.showCounters = on; break;
        case 102: cfg.scrollUpwards = on; break;
        case 103: cfg.resetCounterOnAttempt = on; break;
        case 305: cfg.shadowEnabled = on; break;
        case 307: cfg.showContainerBg = on; break;
        case 308: cfg.fastBarShadow = on; break;
        case 310: cfg.showBounds = on; break;
    }

    updatePreview();
}

void SettingsPopup::onResetCounters(CCObject* sender) {
    auto& cfg = OverlayConfig::get();
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        cfg.keys[i].clickCount = 0;
    }
    if (m_previewNode) {
        m_previewNode->resetCounters();
    }
    if (auto overlay = KeyOverlayNode::getActive()) {
        overlay->resetCounters();
    }
    FLAlertLayer::create("Counters Cleared", "All key counters have been reset to 0.", "OK")->show();
}

void SettingsPopup::onColorPick(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    int tag = btn->getTag();
    auto& cfg = OverlayConfig::get();

    ccColor4B initialColor;
    switch (tag) {
        case 201: initialColor = cfg.containerBgColor; break;
        case 202: initialColor = cfg.keyBgColor; break;
        case 203: initialColor = cfg.keyPressedColor; break;
        case 204: initialColor = cfg.barColor; break;
        case 205: initialColor = cfg.keyBorderColor; break;
        case 206: initialColor = cfg.shadowColor; break;
        case 221: initialColor = cfg.labelColor; break;
        case 222: initialColor = cfg.labelPressedColor; break;
        case 223: initialColor = cfg.counterColor; break;
        default: initialColor = ccc4(255, 255, 255, 255); break;
    }

    auto popup = ColorPickPopup::create(initialColor);
    popup->setCallback([this, tag, btn](ccColor4B const& c) {
        auto& cfg = OverlayConfig::get();
        switch (tag) {
            case 201: cfg.containerBgColor = c; break;
            case 202:
                cfg.keyBgColor.r = c.r;
                cfg.keyBgColor.g = c.g;
                cfg.keyBgColor.b = c.b;
                if (c.a != 255) cfg.keyBgColor.a = c.a;
                break;
            case 203:
                cfg.keyPressedColor.r = c.r;
                cfg.keyPressedColor.g = c.g;
                cfg.keyPressedColor.b = c.b;
                if (c.a != 255) cfg.keyPressedColor.a = c.a;
                break;
            case 204:
                cfg.barColor.r = c.r;
                cfg.barColor.g = c.g;
                cfg.barColor.b = c.b;
                if (c.a != 255) cfg.barColor.a = c.a;
                break;
            case 205:
                cfg.keyBorderColor.r = c.r;
                cfg.keyBorderColor.g = c.g;
                cfg.keyBorderColor.b = c.b;
                if (c.a != 255) cfg.keyBorderColor.a = c.a;
                break;
            case 206:
                cfg.shadowColor = c;
                break;
            case 221: cfg.labelColor = c; break;
            case 222: cfg.labelPressedColor = c; break;
            case 223: cfg.counterColor = c; break;
        }

        if (auto spr = static_cast<CCSprite*>(btn->getNormalImage())) {
            spr->setColor(ccc3(c.r, c.g, c.b));
        }

        updatePreview();
    });
    popup->show();
}

void SettingsPopup::onRebindKey(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = btn->getTag();
    if (m_listeningKeyIndex == idx) {
        m_listeningKeyIndex = -1;
    } else {
        m_listeningKeyIndex = idx;
    }
    setupKeysTab();
}

void SettingsPopup::onToggleJumpBind(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = btn->getTag();
    auto& cfg = OverlayConfig::get();
    if (idx >= 0 && idx < static_cast<int>(cfg.keys.size())) {
        auto& k = cfg.keys[idx];
        if (!k.isGDAction) {
            k.isGDAction = true;
            k.gdButton = 1;
            k.gdPlayer = 1;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P1";
        } else if (k.gdButton == 1 && k.gdPlayer == 1) {
            k.gdButton = 1;
            k.gdPlayer = 2;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P2";
        } else if (k.gdButton == 1 && k.gdPlayer == 2) {
            k.gdButton = 2;
            k.gdPlayer = 1;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P1<";
        } else if (k.gdButton == 2 && k.gdPlayer == 1) {
            k.gdButton = 3;
            k.gdPlayer = 1;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P1>";
        } else if (k.gdButton == 3 && k.gdPlayer == 1) {
            k.gdButton = 2;
            k.gdPlayer = 2;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P2<";
        } else if (k.gdButton == 2 && k.gdPlayer == 2) {
            k.gdButton = 3;
            k.gdPlayer = 2;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = "P2>";
        } else {
            k.isGDAction = false;
            k.gdButton = 1;
            k.gdPlayer = 1;
            k.isMouse = false;
            if (k.customLabel.empty()) k.label = getKeyName(k.keyCode);
        }
    }
    setupKeysTab();
    updatePreview();
}

void SettingsPopup::onRenameKey(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = btn->getTag();
    auto popup = RenameKeyPopup::create(idx);
    if (popup) {
        popup->show();
    }
}

void SettingsPopup::onDeleteKeyIndex(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    int idx = btn->getTag();
    auto& cfg = OverlayConfig::get();
    if (cfg.keys.size() > 1 && idx >= 0 && idx < static_cast<int>(cfg.keys.size())) {
        cfg.keys.erase(cfg.keys.begin() + idx);
        m_listeningKeyIndex = -1;
        setupKeysTab();
        updatePreview();
    }
}

void SettingsPopup::onAddKey(CCObject* sender) {
    auto& cfg = OverlayConfig::get();
    if (cfg.keys.size() >= 5) return;

    KeyBindInfo newK;
    newK.id = fmt::format("key_{}", cfg.keys.size() + 1);
    newK.label = "C";
    newK.keyCode = KEY_C;
    newK.isMouse = false;
    newK.mouseButton = 0;
    newK.isGDAction = false;
    newK.gdButton = 1;
    newK.gdPlayer = 1;
    newK.clickCount = 0;
    cfg.keys.push_back(newK);

    m_listeningKeyIndex = -1;
    setupKeysTab();
    updatePreview();
}

void SettingsPopup::onRemoveKey(CCObject* sender) {
    auto& cfg = OverlayConfig::get();
    if (cfg.keys.size() <= 1) return;

    cfg.keys.pop_back();
    m_listeningKeyIndex = -1;
    setupKeysTab();
    updatePreview();
}

void SettingsPopup::onCycleKeyFont(CCObject* sender) {
    int dir = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();

    int curIdx = 0;
    for (size_t i = 0; i < s_fontList.size(); ++i) {
        if (s_fontList[i].first == cfg.keyFont) {
            curIdx = static_cast<int>(i);
            break;
        }
    }

    curIdx = (curIdx + dir + static_cast<int>(s_fontList.size())) % static_cast<int>(s_fontList.size());
    cfg.keyFont = s_fontList[curIdx].first;

    setupFontsTab();
    updatePreview();
}

void SettingsPopup::onCycleCounterFont(CCObject* sender) {
    int dir = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();

    int curIdx = 0;
    for (size_t i = 0; i < s_fontList.size(); ++i) {
        if (s_fontList[i].first == cfg.counterFont) {
            curIdx = static_cast<int>(i);
            break;
        }
    }

    curIdx = (curIdx + dir + static_cast<int>(s_fontList.size())) % static_cast<int>(s_fontList.size());
    cfg.counterFont = s_fontList[curIdx].first;

    setupFontsTab();
    updatePreview();
}

void SettingsPopup::onOpenInteractiveEditor(CCObject*) {
    this->setVisible(false);
    auto editor = OverlayPositionEditor::create(this);
    if (editor) {
        CCDirector::sharedDirector()->getRunningScene()->addChild(editor, 1000);
    }
}

void SettingsPopup::onPresetPos(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    float w = 180.0f * cfg.scale;
    float h = (cfg.keyHeight + cfg.streamLength + 20.0f) * cfg.scale;
    float margin = 20.0f;

    switch (tag) {
        case 1: cfg.position = CCPoint(margin, margin + 40.0f); break; // BL
        case 2: cfg.position = CCPoint(winSize.width - w - margin, margin + 40.0f); break; // BR
        case 3: cfg.position = CCPoint(margin, winSize.height - h - margin); break; // TL
        case 4: cfg.position = CCPoint(winSize.width - w - margin, winSize.height - h - margin); break; // TR
        case 5: cfg.position = CCPoint((winSize.width - w) * 0.5f, (winSize.height - h) * 0.5f); break; // Center
    }

    if (m_currentTab == 3) {
        setupLayoutTab();
    }
    updatePreview();
}

void SettingsPopup::onResetPos(CCObject*) {
    auto& cfg = OverlayConfig::get();
    cfg.position = CCPoint(30.0f, 60.0f);
    if (m_currentTab == 3) {
        setupLayoutTab();
    }
    updatePreview();
}

void SettingsPopup::onNudgePos(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();
    float step = 10.0f;

    switch (tag) {
        case 10: cfg.position.x -= step; break;
        case 11: cfg.position.x += step; break;
        case 12: cfg.position.y -= step; break;
        case 13: cfg.position.y += step; break;
    }

    if (m_currentTab == 3) {
        setupLayoutTab();
    }
    updatePreview();
}

bool SettingsPopup::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (m_listeningKeyIndex >= 0) {
        auto& cfg = OverlayConfig::get();
        if (m_listeningKeyIndex < static_cast<int>(cfg.keys.size())) {
            cfg.keys[m_listeningKeyIndex].isMouse = true;
            cfg.keys[m_listeningKeyIndex].mouseButton = 0;
            cfg.keys[m_listeningKeyIndex].isGDAction = true;
            cfg.keys[m_listeningKeyIndex].gdButton = 1;
            cfg.keys[m_listeningKeyIndex].keyCode = 0;
            if (cfg.keys[m_listeningKeyIndex].customLabel.empty()) {
                cfg.keys[m_listeningKeyIndex].label = "M1";
            }
        }
        m_listeningKeyIndex = -1;
        setupKeysTab();
        updatePreview();
        return true;
    }
    return Popup::ccTouchBegan(touch, event);
}

void SettingsPopup::handleKeyInput(int keyCode, bool down) {
    if (m_listeningKeyIndex >= 0) {
        if (down) {
            if (keyCode == KEY_Escape) {
                m_listeningKeyIndex = -1;
                setupKeysTab();
                return;
            }
            auto& cfg = OverlayConfig::get();
            if (m_listeningKeyIndex < static_cast<int>(cfg.keys.size())) {
                cfg.keys[m_listeningKeyIndex].keyCode = keyCode;
                cfg.keys[m_listeningKeyIndex].label = getKeyName(keyCode);
                cfg.keys[m_listeningKeyIndex].isGDAction = false;
                cfg.keys[m_listeningKeyIndex].isMouse = false;
                cfg.keys[m_listeningKeyIndex].gdPlayer = 1;
                cfg.keys[m_listeningKeyIndex].gdButton = 1;
            }
            m_listeningKeyIndex = -1;
            setupKeysTab();
            updatePreview();
        }
        return;
    }

    if (m_previewNode) {
        m_previewNode->handleRawKey(keyCode, down);
    }
}

void SettingsPopup::onSave(CCObject* sender) {
    auto& cfg = OverlayConfig::get();
    InputPoller::get().setPollingRate(cfg.pollingRate);
    InputPoller::get().updateKeysFromConfig();
    cfg.save();
    if (auto overlay = KeyOverlayNode::getActive()) {
        overlay->updateLayout();
    }
    FLAlertLayer::create("Saved", "KeyOverlay settings saved successfully!", "OK")->show();
}

// ---------------- TAB 4: ENGINE & PERFORMANCE ----------------
void SettingsPopup::setupEngineTab() {
    m_tabContent->removeAllChildren();
    auto& cfg = OverlayConfig::get();
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_tabContent->addChild(menu);

    auto title = CCLabelBMFont::create("Performance & Smoothness", "goldFont.fnt");
    title->setScale(0.42f);
    title->setAnchorPoint({ 0.0f, 0.5f });
    title->setPosition({ 10.0f, 188.0f });
    m_tabContent->addChild(title);

    // 1. Overlay FPS Slider & TextInput (tag 501)
    createFloatInputRow(m_tabContent, { 8.0f, 162.0f }, "Overlay FPS", cfg.overlayFPS, 0.0f, 360.0f, 501);

    // FPS Presets
    auto fpsMenu = CCMenu::create();
    fpsMenu->setPosition({ 148.0f, 140.0f });
    m_tabContent->addChild(fpsMenu);

    auto makeFpsBtn = [&](char const* text, int fps) {
        auto spr = ButtonSprite::create(text, "goldFont.fnt", (static_cast<int>(cfg.overlayFPS) == fps) ? "GJ_button_01.png" : "GJ_button_04.png", 0.5f);
        spr->setScale(0.36f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onPresetFPS));
        btn->setTag(fps);
        fpsMenu->addChild(btn);
    };
    makeFpsBtn("Max/Game", 0);
    makeFpsBtn("60", 60);
    makeFpsBtn("144", 144);
    makeFpsBtn("240", 240);
    makeFpsBtn("360", 360);
    fpsMenu->setLayout(RowLayout::create()->setGap(3.0f));

    // 2. Click Polling Rate Slider & TextInput (tag 502)
    createFloatInputRow(m_tabContent, { 8.0f, 110.0f }, "Poll Rate Hz", static_cast<float>(cfg.pollingRate), 0.0f, 2000.0f, 502);

    // Polling Presets
    auto pollMenu = CCMenu::create();
    pollMenu->setPosition({ 148.0f, 88.0f });
    m_tabContent->addChild(pollMenu);

    auto makePollBtn = [&](char const* text, int hz) {
        auto spr = ButtonSprite::create(text, "goldFont.fnt", (cfg.pollingRate == hz) ? "GJ_button_01.png" : "GJ_button_04.png", 0.5f);
        spr->setScale(0.36f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SettingsPopup::onPresetPolling));
        btn->setTag(hz);
        pollMenu->addChild(btn);
    };
    makePollBtn("Off/Hooks", 0);
    makePollBtn("250", 250);
    makePollBtn("500", 500);
    makePollBtn("1000", 1000);
    makePollBtn("2000", 2000);
    pollMenu->setLayout(RowLayout::create()->setGap(3.0f));

    // 3. Counter Mode Selector
    auto modeLabel = CCLabelBMFont::create("Counter Display:", "bigFont.fnt");
    modeLabel->setScale(0.24f);
    modeLabel->setAnchorPoint({ 0.0f, 0.5f });
    modeLabel->setPosition({ 8.0f, 54.0f });
    m_tabContent->addChild(modeLabel);

    char const* modeText = "Total Clicks";
    if (cfg.counterMode == 1) modeText = "CPS Only";
    else if (cfg.counterMode == 2) modeText = "CPS | Total";

    auto modeSpr = ButtonSprite::create(modeText, "goldFont.fnt", "GJ_button_05.png", 0.65f);
    modeSpr->setScale(0.48f);
    auto modeBtn = CCMenuItemSpriteExtra::create(modeSpr, this, menu_selector(SettingsPopup::onCycleCounterMode));
    modeBtn->setPosition({ 190.0f, 54.0f });
    menu->addChild(modeBtn);

    // 4. Fast Bar Shadow toggle & Reset / Attempt toggle
    createToggleRow(m_tabContent, { 8.0f, 18.0f }, "Fast Bar Shadow", cfg.fastBarShadow, 308, menu_selector(SettingsPopup::onToggle), 88.0f);
    createToggleRow(m_tabContent, { 170.0f, 18.0f }, "Reset / Attempt", cfg.resetCounterOnAttempt, 103, menu_selector(SettingsPopup::onToggle), 68.0f);
}

void SettingsPopup::onPresetFPS(CCObject* sender) {
    int fps = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();
    cfg.overlayFPS = static_cast<float>(fps);
    setupEngineTab();
    updatePreview();
}

void SettingsPopup::onPresetPolling(CCObject* sender) {
    int hz = static_cast<CCNode*>(sender)->getTag();
    auto& cfg = OverlayConfig::get();
    cfg.pollingRate = hz;
    InputPoller::get().setPollingRate(hz);
    setupEngineTab();
    updatePreview();
}

void SettingsPopup::onCycleCounterMode(CCObject*) {
    auto& cfg = OverlayConfig::get();
    cfg.counterMode = (cfg.counterMode + 1) % 3;
    setupEngineTab();
    updatePreview();
}

void SettingsPopup::onReset(CCObject* sender) {
    OverlayConfig::get().resetDefaults();
    switchTab(m_currentTab);
    updatePreview();
}

void SettingsPopup::onClose(CCObject* sender) {
    s_activeInstance = nullptr;
    Popup::onClose(sender);
}

void SettingsPopup::updatePreview() {
    if (m_previewNode) {
        m_previewNode->updateLayout();
        auto winSize = m_mainLayer->getContentSize();
        float previewCenterX = winSize.width - 85.0f;
        m_previewNode->setPosition({ previewCenterX - m_previewNode->getTotalWidth() * 0.68f * 0.5f, 44.0f });
    }
    if (auto overlay = KeyOverlayNode::getActive()) {
        overlay->updateLayout();
    }
}
