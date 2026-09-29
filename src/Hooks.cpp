#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "Config.hpp"
#include "KeyOverlayNode.hpp"
#include "InputPoller.hpp"
#include "SettingsPopup.hpp"

using namespace geode::prelude;

static bool isSettingsOpen() {
    return (SettingsPopup::s_activeInstance != nullptr &&
            SettingsPopup::s_activeInstance->getParent() != nullptr);
}

class $modify(KeyOverlayPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto& cfg = OverlayConfig::get();
        if (cfg.enabled) {
            if (this->m_uiLayer) {
                if (auto old = this->m_uiLayer->getChildByID("key-overlay-node")) {
                    old->removeFromParent();
                }
                auto overlay = KeyOverlayNode::create(false);
                overlay->setID("key-overlay-node");
                this->m_uiLayer->addChild(overlay, 100);
                overlay->syncHoldOnReset();
            } else {
                if (auto old = this->getChildByID("key-overlay-node")) {
                    old->removeFromParent();
                }
                auto overlay = KeyOverlayNode::create(false);
                overlay->setID("key-overlay-node");
                this->addChild(overlay, 100);
                overlay->syncHoldOnReset();
            }
        }
        InputPoller::get().clearPendingClicks();
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto& cfg = OverlayConfig::get();
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->syncHoldOnReset();
            if (cfg.resetCounterOnAttempt) {
                overlay->resetCounters();
            }
        }
    }

    void resume() {
        PlayLayer::resume();
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->syncHoldOnReset();
        }
    }

    void pauseGame(bool p0) {
        PlayLayer::pauseGame(p0);
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->releaseAllKeys();
        }
    }

    void onExit() {
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->releaseAllKeys();
        }
        TouchTracker::activeTouches.clear();
        InputPoller::get().clearPendingClicks();
        PlayLayer::onExit();
    }
};

class $modify(KeyOverlayPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        auto result = PlayerObject::pushButton(button);
        if (!isSettingsOpen()) {
            if (auto overlay = KeyOverlayNode::getActive()) {
                overlay->handlePlayerButton(button, true, this->m_isSecondPlayer);
            }
        }
        return result;
    }

    bool releaseButton(PlayerButton button) {
        auto result = PlayerObject::releaseButton(button);
        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->handlePlayerButton(button, false, this->m_isSecondPlayer);
        }
        return result;
    }
};

class $modify(KeyOverlayUILayer, UILayer) {
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (touch) {
            TouchTracker::activeTouches.insert(touch->getID());
        }
        auto& cfg = OverlayConfig::get();
#ifdef GEODE_IS_WINDOWS
        if (cfg.pollingRate <= 0 && !isSettingsOpen())
#else
        if (!isSettingsOpen())
#endif
        {
            if (auto overlay = KeyOverlayNode::getActive()) {
                overlay->handleMouseButton(0, true);
            }
        }
        return UILayer::ccTouchBegan(touch, event);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) {
        if (touch) {
            TouchTracker::activeTouches.erase(touch->getID());
        }
        auto& cfg = OverlayConfig::get();
#ifdef GEODE_IS_WINDOWS
        if (cfg.pollingRate <= 0)
#endif
        {
            if (auto overlay = KeyOverlayNode::getActive()) {
                if (!TouchTracker::hasActiveTouches()) {
                    overlay->handleMouseButton(0, false);
                }
            }
        }
        UILayer::ccTouchEnded(touch, event);
    }

    void ccTouchCancelled(CCTouch* touch, CCEvent* event) {
        if (touch) {
            TouchTracker::activeTouches.erase(touch->getID());
        }
        auto& cfg = OverlayConfig::get();
#ifdef GEODE_IS_WINDOWS
        if (cfg.pollingRate <= 0)
#endif
        {
            if (auto overlay = KeyOverlayNode::getActive()) {
                if (!TouchTracker::hasActiveTouches()) {
                    overlay->handleMouseButton(0, false);
                }
            }
        }
        UILayer::ccTouchCancelled(touch, event);
    }
};

class $modify(KeyOverlayPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (auto overlay = KeyOverlayNode::getActive()) {
            overlay->releaseAllKeys();
        }

        auto& cfg = OverlayConfig::get();
        if (!cfg.enabled) return;

        auto spr = ButtonSprite::create("Keys", "goldFont.fnt", "GJ_button_01.png", 0.7f);
        spr->setScale(0.6f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(KeyOverlayPauseLayer::onOpenSettings)
        );

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) {
            menu = this->getChildByID("bottom-button-menu");
        }

        if (menu) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto customMenu = CCMenu::create();
            customMenu->addChild(btn);
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            customMenu->setPosition({ winSize.width - 40.0f, winSize.height - 40.0f });
            this->addChild(customMenu, 10);
        }
    }

    void onOpenSettings(CCObject*) {
        auto popup = SettingsPopup::create();
        if (popup) {
            popup->show();
        }
    }
};

class $modify(KeyOverlayKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        // If settings popup is open and attached to parent
        if (SettingsPopup::s_activeInstance) {
            if (SettingsPopup::s_activeInstance->getParent()) {
                SettingsPopup::s_activeInstance->handleKeyInput(key, isKeyDown);
                return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
            } else {
                SettingsPopup::s_activeInstance = nullptr;
            }
        }

        // If in PlayLayer and overlay is active, route key event
        auto& cfg = OverlayConfig::get();
#ifdef GEODE_IS_WINDOWS
        if (!isKeyRepeat && !isSettingsOpen() && cfg.pollingRate <= 0)
#else
        if (!isKeyRepeat && !isSettingsOpen())
#endif
        {
            if (auto overlay = KeyOverlayNode::getActive()) {
                overlay->handleRawKey(key, isKeyDown);
            }
        }

        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};
