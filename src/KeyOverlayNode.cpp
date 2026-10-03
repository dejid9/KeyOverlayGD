#include "KeyOverlayNode.hpp"
#include "InputPoller.hpp"
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/UILayer.hpp>
#include <fmt/format.h>
#include <cmath>
#include <algorithm>

#ifdef GEODE_IS_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

KeyOverlayNode* KeyOverlayNode::getActive() {
    auto g = GJBaseGameLayer::get();
    if (!g) return nullptr;
    if (g->m_uiLayer) {
        if (auto node = g->m_uiLayer->getChildByID("key-overlay-node")) {
            return typeinfo_cast<KeyOverlayNode*>(node);
        }
    }
    if (auto node = g->getChildByID("key-overlay-node")) {
        return typeinfo_cast<KeyOverlayNode*>(node);
    }
    return nullptr;
}

KeyOverlayNode* KeyOverlayNode::create(bool isPreview) {
    auto ret = new KeyOverlayNode();
    if (ret && ret->init(isPreview)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool KeyOverlayNode::init(bool isPreview) {
    if (!CCNode::init()) return false;

    m_isPreview = isPreview;

    ccBlendFunc blendFunc = { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

    m_keyShadowDraw = CCDrawNode::create();
    m_keyShadowDraw->setBlendFunc(blendFunc);
    this->addChild(m_keyShadowDraw, 0);

    m_bgDraw = CCDrawNode::create();
    m_bgDraw->setBlendFunc(blendFunc);
    this->addChild(m_bgDraw, 1);

    m_clipStencil = CCDrawNode::create();
    m_clipNode = CCClippingNode::create(m_clipStencil);
    this->addChild(m_clipNode, 2);

    m_barShadowDraw = CCDrawNode::create();
    m_barShadowDraw->setBlendFunc(blendFunc);
    m_clipNode->addChild(m_barShadowDraw, 0);

    m_barsDraw = CCDrawNode::create();
    m_barsDraw->setBlendFunc(blendFunc);
    m_clipNode->addChild(m_barsDraw, 1);

    m_boundsDraw = CCDrawNode::create();
    m_boundsDraw->setBlendFunc(blendFunc);
    this->addChild(m_boundsDraw, 10);

    updateLayout();
    this->scheduleUpdate();

    return true;
}

void KeyOverlayNode::drawRectWithBorder(CCDrawNode* draw, CCPoint origin, CCPoint size, float radius, ccColor4B fill, float borderWidth, ccColor4B border) {
    if (size.x <= 0.0f || size.y <= 0.0f) return;
    if (fill.a == 0 && (borderWidth <= 0.0f || border.a == 0)) return;

    ccColor4F fillF = to4F(fill);
    ccColor4F borderF = (borderWidth > 0.0f && border.a > 0) ? to4F(border) : ccc4f(0, 0, 0, 0);

    if (radius <= 0.5f) {
        CCPoint verts[4] = {
            origin,
            { origin.x + size.x, origin.y },
            { origin.x + size.x, origin.y + size.y },
            { origin.x, origin.y + size.y }
        };
        draw->drawPolygon(verts, 4, fillF, borderWidth, borderF);
        return;
    }

    radius = std::min(radius, std::min(size.x, size.y) * 0.5f);
    std::vector<CCPoint> points;
    const int segmentsPerCorner = 4;

    auto addCorner = [&](float cx, float cy, float startAngle) {
        for (int i = 0; i <= segmentsPerCorner; ++i) {
            float angle = startAngle + (M_PI * 0.5f) * (static_cast<float>(i) / segmentsPerCorner);
            points.push_back({ cx + radius * std::cos(angle), cy + radius * std::sin(angle) });
        }
    };

    addCorner(origin.x + size.x - radius, origin.y + radius, -M_PI * 0.5f);
    addCorner(origin.x + size.x - radius, origin.y + size.y - radius, 0.0f);
    addCorner(origin.x + radius, origin.y + size.y - radius, M_PI * 0.5f);
    addCorner(origin.x + radius, origin.y + radius, M_PI);

    draw->drawPolygon(points.data(), static_cast<unsigned int>(points.size()), fillF, borderWidth, borderF);
}

void KeyOverlayNode::drawBlurredRect(CCDrawNode* draw, CCPoint origin, CCPoint size, float radius, ccColor4B color, float blurRadius) {
    if (size.x <= 0.0f || size.y <= 0.0f || color.a == 0) return;

    if (blurRadius <= 0.5f) {
        drawRectWithBorder(draw, origin, size, radius, color, 0.0f, { 0, 0, 0, 0 });
        return;
    }

    const int passes = 6;
    float baseA = static_cast<float>(color.a) / 255.0f;

    for (int p = passes; p >= 1; --p) {
        float frac = static_cast<float>(p) / passes;
        float expand = blurRadius * frac;
        float t = 1.0f - frac;
        float alpha = baseA * (t * t) * (1.0f / passes) * 2.8f;
        alpha = std::clamp(alpha, 0.0f, 1.0f);

        ccColor4B passColor = {
            color.r,
            color.g,
            color.b,
            static_cast<GLubyte>(alpha * 255.0f)
        };

        CCPoint pOrigin = { origin.x - expand, origin.y - expand };
        CCPoint pSize = { size.x + expand * 2.0f, size.y + expand * 2.0f };
        float pRadius = radius + expand;

        drawRectWithBorder(draw, pOrigin, pSize, pRadius, passColor, 0.0f, { 0, 0, 0, 0 });
    }
}

void KeyOverlayNode::updateLayout() {
    auto& cfg = OverlayConfig::get();
    size_t keyCount = cfg.keys.size();

    // Recreate labels and counters
    for (auto& col : m_columns) {
        if (col.labelNode) col.labelNode->removeFromParent();
        if (col.counterNode) col.counterNode->removeFromParent();
    }
    m_columns.clear();
    m_columns.resize(keyCount);

    for (size_t i = 0; i < keyCount; ++i) {
        std::string text = cfg.keys[i].customLabel.empty() ? cfg.keys[i].label : cfg.keys[i].customLabel;
        auto label = CCLabelBMFont::create(text.c_str(), cfg.keyFont.c_str());
        float scale = cfg.labelScale * (text.length() > 2 ? 0.75f : 1.0f);
        label->setScale(scale);
        this->addChild(label, 3);
        m_columns[i].labelNode = label;

        auto counter = CCLabelBMFont::create("0", cfg.counterFont.c_str());
        counter->setScale(cfg.counterScale);
        this->addChild(counter, 3);
        m_columns[i].counterNode = counter;
    }

    float padding = 4.0f;
    float counterAreaHeight = cfg.showCounters ? (22.0f * (cfg.counterScale / 0.33f)) : 0.0f;
    float keysWidth = keyCount * cfg.keyWidth + (keyCount > 1 ? (keyCount - 1) * cfg.keySpacing : 0.0f);

    m_totalWidth = keysWidth + padding * 2.0f;
    m_totalHeight = cfg.keyHeight + cfg.streamLength + counterAreaHeight + padding * 2.0f;

    this->setContentSize({ m_totalWidth, m_totalHeight });
    this->setScale(cfg.scale);

    if (!m_isPreview) {
        this->setPosition(cfg.position);
    }

    // Configure clipping stencil
    m_clipStencil->clear();
    float streamX = padding - 4.0f;
    float streamW = keysWidth + 8.0f;
    float streamY = padding + (cfg.scrollUpwards ? counterAreaHeight + cfg.keyHeight : counterAreaHeight);
    CCPoint stencilVerts[4] = {
        { streamX, streamY },
        { streamX + streamW, streamY },
        { streamX + streamW, streamY + cfg.streamLength },
        { streamX, streamY + cfg.streamLength }
    };
    m_clipStencil->drawPolygon(stencilVerts, 4, ccc4f(1, 1, 1, 1), 0, ccc4f(0, 0, 0, 0));

    // Update labels text and visibility
    for (size_t i = 0; i < keyCount; ++i) {
        std::string text = cfg.keys[i].customLabel.empty() ? cfg.keys[i].label : cfg.keys[i].customLabel;
        if (m_columns[i].labelNode) {
            m_columns[i].labelNode->setString(text.c_str());
            float scale = cfg.labelScale * (text.length() > 2 ? 0.75f : 1.0f);
            m_columns[i].labelNode->setScale(scale);
            m_columns[i].labelNode->setVisible(cfg.showLabels);
        }
        if (m_columns[i].counterNode) {
            updateCounterDisplay(i);
            m_columns[i].counterNode->setScale(cfg.counterScale);
            m_columns[i].counterNode->setVisible(cfg.showCounters);
        }
    }

    // Force instant redraw of all draw nodes!
    redraw();
}

void KeyOverlayNode::update(float dt) {
    auto& cfg = OverlayConfig::get();
    size_t keyCount = m_columns.size();
    if (keyCount == 0) return;

    if (!m_isPreview) {
        // High-frequency polling synchronization
        if (cfg.pollingRate > 0) {
            InputPoller::get().syncToOverlay(this);
        }

        // CPS tracking for GDAction keys or when polling is disabled
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::milliseconds(1000);
        for (size_t i = 0; i < keyCount; ++i) {
            auto& col = m_columns[i];
            if (i < cfg.keys.size() && (cfg.keys[i].isGDAction || cfg.pollingRate <= 0)) {
                col.clickTimestamps.erase(
                    std::remove_if(col.clickTimestamps.begin(), col.clickTimestamps.end(),
                        [&](auto const& t) { return t < cutoff; }),
                    col.clickTimestamps.end()
                );
                cfg.keys[i].currentCPS = static_cast<int>(col.clickTimestamps.size());
                updateCounterDisplay(i);
            }
        }

#ifdef GEODE_IS_WINDOWS
    if (cfg.pollingRate <= 0) {
        // Fallback for hooks mode: verify physical key state so keys never get stuck
        auto pl = PlayLayer::get();
        for (size_t i = 0; i < cfg.keys.size() && i < keyCount; ++i) {
            auto const& k = cfg.keys[i];
            if (m_columns[i].isPressed) {
                bool physicallyDown = false;
                if (k.isMouse) {
                    int vk = (k.mouseButton == 1) ? VK_RBUTTON : VK_LBUTTON;
                    physicallyDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
                } else if (k.keyCode != 0) {
                    physicallyDown = (GetAsyncKeyState(k.keyCode) & 0x8000) != 0;
                } else if (k.isGDAction) {
                    if (k.gdPlayer == 2) {
                        if (k.gdButton == 1 && pl && pl->m_player2) {
                            auto it = pl->m_player2->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                            if (it != pl->m_player2->m_holdingButtons.end()) physicallyDown = it->second;
                            if (pl->m_uiLayer && pl->m_uiLayer->m_p2Jumping) physicallyDown = true;
                        } else if (k.gdButton == 2 && pl && pl->m_player2) {
                            physicallyDown = pl->m_player2->m_holdingLeft;
                        } else if (k.gdButton == 3 && pl && pl->m_player2) {
                            physicallyDown = pl->m_player2->m_holdingRight;
                        }
                    } else {
                        if (k.gdButton == 1) {
                            physicallyDown = ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) ||
                                             ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) ||
                                             ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                            if (pl && pl->m_player1) {
                                auto it = pl->m_player1->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                                if (it != pl->m_player1->m_holdingButtons.end() && it->second) physicallyDown = true;
                            }
                            if (pl && pl->m_uiLayer && pl->m_uiLayer->m_p1Jumping) physicallyDown = true;
                        } else if (k.gdButton == 2) {
                            physicallyDown = ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) ||
                                             ((GetAsyncKeyState('A') & 0x8000) != 0);
                        } else if (k.gdButton == 3) {
                            physicallyDown = ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) ||
                                             ((GetAsyncKeyState('D') & 0x8000) != 0);
                        }
                    }
                }
                if (!physicallyDown) {
                    setKeyPressed(i, false);
                }
            }
        }
    }
#endif
    } else {
        // Preview mode: update CPS display for preview clicks
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::milliseconds(1000);
        for (size_t i = 0; i < keyCount; ++i) {
            auto& col = m_columns[i];
            col.clickTimestamps.erase(
                std::remove_if(col.clickTimestamps.begin(), col.clickTimestamps.end(),
                    [&](auto const& t) { return t < cutoff; }),
                col.clickTimestamps.end()
            );
            if (i < cfg.keys.size()) {
                cfg.keys[i].currentCPS = static_cast<int>(col.clickTimestamps.size());
            }
            updateCounterDisplay(i);
        }
    }

    // Optional Overlay Render FPS limiter (0 = Uncapped)
    if (cfg.overlayFPS > 0.0f) {
        m_accumulatedDt += dt;
        float targetInterval = 1.0f / cfg.overlayFPS;
        if (m_accumulatedDt < targetInterval) {
            return;
        }
        dt = m_accumulatedDt;
        m_accumulatedDt = 0.0f;
    }

    // Advance bars
    for (size_t i = 0; i < keyCount; ++i) {
        auto& col = m_columns[i];
        for (auto& bar : col.bars) {
            if (bar.active) {
                bar.length += cfg.barSpeed * dt;
            } else {
                bar.startY += cfg.barSpeed * dt;
            }
        }

        // Remove expired bars
        col.bars.erase(
            std::remove_if(col.bars.begin(), col.bars.end(), [&](KeyBar const& b) {
                return b.startY > cfg.streamLength;
            }),
            col.bars.end()
        );
    }

    redraw();
}

void KeyOverlayNode::redraw() {
    auto& cfg = OverlayConfig::get();
    size_t keyCount = m_columns.size();
    if (keyCount == 0) return;

    float padding = 4.0f;
    float counterHeight = cfg.showCounters ? (22.0f * (cfg.counterScale / 0.33f)) : 0.0f;
    float keysY = cfg.scrollUpwards ? padding + counterHeight : m_totalHeight - padding - cfg.keyHeight;
    float streamBaseY = cfg.scrollUpwards ? keysY + cfg.keyHeight : keysY;

    m_keyShadowDraw->clear();
    m_barShadowDraw->clear();
    m_bgDraw->clear();
    m_barsDraw->clear();
    m_boundsDraw->clear();

    CCPoint shadowOffset = cfg.getShadowOffset();

    // 1. Container Background & Shadow (optional)
    if (cfg.showContainerBg) {
        if (cfg.shadowEnabled) {
            drawBlurredRect(m_keyShadowDraw, shadowOffset, { m_totalWidth, m_totalHeight }, cfg.cornerRadius * 1.5f, cfg.shadowColor, cfg.shadowBlur);
        }
        drawRectWithBorder(m_bgDraw, { 0, 0 }, { m_totalWidth, m_totalHeight }, cfg.cornerRadius * 1.5f, cfg.containerBgColor, cfg.borderWidth, cfg.keyBorderColor);
    }

    // 2. Draw Keys & Stream Bars
    for (size_t i = 0; i < keyCount; ++i) {
        auto& col = m_columns[i];
        float kx = padding + i * (cfg.keyWidth + cfg.keySpacing);

        // Key Box
        CCPoint keyPos = { kx, keysY };
        CCPoint keySize = { cfg.keyWidth, cfg.keyHeight };

        // Soft Blurred Key Shadow
        if (cfg.shadowEnabled) {
            drawBlurredRect(m_keyShadowDraw, keyPos + shadowOffset, keySize, cfg.cornerRadius, cfg.shadowColor, cfg.shadowBlur);
        }

        // Key Fill & Border
        ccColor4B currentKeyFill = col.isPressed ? cfg.keyPressedColor : cfg.keyBgColor;
        drawRectWithBorder(m_bgDraw, keyPos, keySize, cfg.cornerRadius, currentKeyFill, cfg.borderWidth, cfg.keyBorderColor);

        // Key Label
        if (col.labelNode && cfg.showLabels) {
            col.labelNode->setPosition(keyPos.x + cfg.keyWidth * 0.5f, keyPos.y + cfg.keyHeight * 0.5f + cfg.labelOffsetY);
            col.labelNode->setColor(col.isPressed ? ccc3(cfg.labelPressedColor.r, cfg.labelPressedColor.g, cfg.labelPressedColor.b) : ccc3(cfg.labelColor.r, cfg.labelColor.g, cfg.labelColor.b));
            col.labelNode->setOpacity(col.isPressed ? cfg.labelPressedColor.a : cfg.labelColor.a);
        }

        // Counter Label (Directly below key box)
        if (col.counterNode && cfg.showCounters) {
            float cy = cfg.scrollUpwards ? padding + counterHeight * 0.45f : keysY - 10.0f;
            col.counterNode->setPosition(keyPos.x + cfg.keyWidth * 0.5f, cy + cfg.counterOffsetY);
            col.counterNode->setColor(ccc3(cfg.counterColor.r, cfg.counterColor.g, cfg.counterColor.b));
            col.counterNode->setOpacity(cfg.counterColor.a);
        }

        // Waterfall Bars
        for (auto const& bar : col.bars) {
            float by = 0.0f;
            float bh = bar.length;

            if (cfg.scrollUpwards) {
                by = streamBaseY + bar.startY;
            } else {
                by = streamBaseY - bar.startY - bar.length;
            }

            CCPoint barPos = { kx, by };
            CCPoint barSize = { cfg.keyWidth, bh };

            if (cfg.shadowEnabled) {
                if (cfg.fastBarShadow) {
                    ccColor4B sCol = cfg.shadowColor;
                    sCol.a = static_cast<GLubyte>(sCol.a * 0.55f);
                    float expand = cfg.shadowBlur * 0.35f;
                    CCPoint pOrigin = { barPos.x + shadowOffset.x - expand, barPos.y + shadowOffset.y - expand };
                    CCPoint pSize = { barSize.x + expand * 2.0f, barSize.y + expand * 2.0f };
                    drawRectWithBorder(m_barShadowDraw, pOrigin, pSize, 0.0f, sCol, 0.0f, { 0, 0, 0, 0 });
                } else {
                    drawBlurredRect(m_barShadowDraw, barPos + shadowOffset, barSize, 0.0f, cfg.shadowColor, cfg.shadowBlur * 0.7f);
                }
            }
            drawRectWithBorder(m_barsDraw, barPos, barSize, 0.0f, cfg.barColor, 0, { 0, 0, 0, 0 });
        }
    }

    // 3. Element Bounds Visualization
    if (cfg.showBounds || m_overrideShowBounds) {
        // Outer bounds rectangle in cyan
        drawRectWithBorder(m_boundsDraw, { 0.0f, 0.0f }, { m_totalWidth, m_totalHeight }, 0.0f, { 0, 0, 0, 0 }, 1.5f, { 0, 255, 255, 240 });

        // Stream bounds in yellow
        float streamY = padding + (cfg.scrollUpwards ? counterHeight + cfg.keyHeight : counterHeight);
        float keysWidth = keyCount * cfg.keyWidth + (keyCount > 1 ? (keyCount - 1) * cfg.keySpacing : 0.0f);
        drawRectWithBorder(m_boundsDraw, { padding, streamY }, { keysWidth, cfg.streamLength }, 0.0f, { 0, 0, 0, 0 }, 1.0f, { 255, 255, 0, 190 });

        // Key outlines in bright green
        for (size_t i = 0; i < keyCount; ++i) {
            float kx = padding + i * (cfg.keyWidth + cfg.keySpacing);
            drawRectWithBorder(m_boundsDraw, { kx, keysY }, { cfg.keyWidth, cfg.keyHeight }, 0.0f, { 0, 0, 0, 0 }, 1.0f, { 0, 255, 120, 200 });
        }
    }
}

void KeyOverlayNode::updateCounterDisplay(size_t index) {
    if (index >= m_columns.size()) return;
    auto& cfg = OverlayConfig::get();
    if (index >= cfg.keys.size()) return;
    auto counter = m_columns[index].counterNode;
    if (!counter) return;

    std::string text;
    if (cfg.counterMode == 1) { // CPS
        text = fmt::format("{}", cfg.keys[index].currentCPS);
    } else if (cfg.counterMode == 2) { // CPS | Total
        text = fmt::format("{}|{}", cfg.keys[index].currentCPS, cfg.keys[index].clickCount);
    } else { // Total
        text = std::to_string(cfg.keys[index].clickCount);
    }
    counter->setString(text.c_str());
}

void KeyOverlayNode::startNewBar(size_t index) {
    if (index >= m_columns.size()) return;
    auto& col = m_columns[index];
    // Deactivate any previous active bar so it detaches properly
    for (auto& b : col.bars) {
        if (b.active) {
            b.active = false;
            if (b.length < 12.0f) {
                b.length = 12.0f;
            }
        }
    }
    KeyBar newBar;
    newBar.startY = 0.0f;
    newBar.length = 0.0f;
    newBar.active = true;
    col.bars.push_back(newBar);
}

void KeyOverlayNode::setKeyColumnPressedState(size_t index, bool pressed) {
    if (index >= m_columns.size()) return;
    auto& col = m_columns[index];
    col.isPressed = pressed;

    if (!pressed) {
        for (auto& bar : col.bars) {
            if (bar.active) {
                bar.active = false;
                if (bar.length < 12.0f) {
                    bar.length = 12.0f;
                }
            }
        }
    }
}

void KeyOverlayNode::setKeyPressed(size_t index, bool pressed) {
    if (index >= m_columns.size()) return;

    auto& col = m_columns[index];
    if (col.isPressed == pressed) return;

    col.isPressed = pressed;

    if (pressed) {
        auto& cfg = OverlayConfig::get();
        if (index < cfg.keys.size()) {
            cfg.keys[index].clickCount++;
            col.clickTimestamps.push_back(std::chrono::steady_clock::now());
            updateCounterDisplay(index);
        }

        startNewBar(index);
    } else {
        for (auto& bar : col.bars) {
            if (bar.active) {
                bar.active = false;
                if (bar.length < 12.0f) {
                    bar.length = 12.0f;
                }
            }
        }
    }

    redraw();
}

void KeyOverlayNode::releaseAllKeys() {
    for (size_t i = 0; i < m_columns.size(); ++i) {
        auto& col = m_columns[i];
        col.isPressed = false;
        for (auto& bar : col.bars) {
            if (bar.active) {
                bar.active = false;
                if (bar.length < 12.0f) {
                    bar.length = 12.0f;
                }
            }
        }
    }
    redraw();
}

void KeyOverlayNode::syncHoldOnReset() {
    auto& cfg = OverlayConfig::get();
    auto pl = PlayLayer::get();

    for (size_t i = 0; i < cfg.keys.size() && i < m_columns.size(); ++i) {
        auto const& k = cfg.keys[i];
        bool isHeld = false;

#ifdef GEODE_IS_WINDOWS
        if (k.isMouse) {
            int vk = (k.mouseButton == 1) ? VK_RBUTTON : VK_LBUTTON;
            isHeld = (GetAsyncKeyState(vk) & 0x8000) != 0;
        } else if (k.keyCode != 0) {
            isHeld = (GetAsyncKeyState(k.keyCode) & 0x8000) != 0;
        }

        if (k.isGDAction) {
            if (k.gdPlayer == 2) {
                if (k.gdButton == 1) {
                    if (pl && pl->m_player2) {
                        auto it = pl->m_player2->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                        if (it != pl->m_player2->m_holdingButtons.end() && it->second) isHeld = true;
                    }
                    if (pl && pl->m_uiLayer && pl->m_uiLayer->m_p2Jumping) isHeld = true;
                } else if (k.gdButton == 2) {
                    if (pl && pl->m_player2 && pl->m_player2->m_holdingLeft) isHeld = true;
                } else if (k.gdButton == 3) {
                    if (pl && pl->m_player2 && pl->m_player2->m_holdingRight) isHeld = true;
                }
            } else {
                if (k.gdButton == 1) {
                    if (((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)) {
                        isHeld = true;
                    }
                    if (pl && pl->m_player1) {
                        auto it = pl->m_player1->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                        if (it != pl->m_player1->m_holdingButtons.end() && it->second) isHeld = true;
                    }
                    if (pl && pl->m_uiLayer && pl->m_uiLayer->m_p1Jumping) isHeld = true;
                } else if (k.gdButton == 2) {
                    if (((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) ||
                        ((GetAsyncKeyState('A') & 0x8000) != 0)) {
                        isHeld = true;
                    }
                    if (pl && pl->m_player1 && pl->m_player1->m_holdingLeft) isHeld = true;
                } else if (k.gdButton == 3) {
                    if (((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) ||
                        ((GetAsyncKeyState('D') & 0x8000) != 0)) {
                        isHeld = true;
                    }
                    if (pl && pl->m_player1 && pl->m_player1->m_holdingRight) isHeld = true;
                }
            }
        }
#else
        if (k.isGDAction) {
            PlayerObject* player = (k.gdPlayer == 2 && pl) ? pl->m_player2 : (pl ? pl->m_player1 : nullptr);
            if (player) {
                if (k.gdButton == 1) {
                    auto it = player->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                    if (it != player->m_holdingButtons.end() && it->second) isHeld = true;
                    if (pl->m_uiLayer) {
                        if (k.gdPlayer == 2 ? pl->m_uiLayer->m_p2Jumping : pl->m_uiLayer->m_p1Jumping) isHeld = true;
                    }
                } else if (k.gdButton == 2) {
                    if (player->m_holdingLeft) isHeld = true;
                } else if (k.gdButton == 3) {
                    if (player->m_holdingRight) isHeld = true;
                }
            }
            if (TouchTracker::hasActiveTouches()) {
                isHeld = true;
            }
        } else if (k.isMouse && k.mouseButton == 0) {
            if (TouchTracker::hasActiveTouches()) {
                isHeld = true;
            }
        }
#endif

        auto& col = m_columns[i];
        if (isHeld) {
            col.isPressed = true;
            bool hasActiveBar = false;
            for (auto& bar : col.bars) {
                if (bar.active) {
                    hasActiveBar = true;
                    break;
                }
            }
            if (!hasActiveBar) {
                KeyBar newBar;
                newBar.startY = 0.0f;
                newBar.length = 0.0f;
                newBar.active = true;
                col.bars.push_back(newBar);
            }
        } else {
            col.isPressed = false;
            for (auto& bar : col.bars) {
                if (bar.active) {
                    bar.active = false;
                    if (bar.length < 12.0f) {
                        bar.length = 12.0f;
                    }
                }
            }
        }
    }

    redraw();
}

void KeyOverlayNode::syncKeyStates() {
#ifdef GEODE_IS_WINDOWS
    auto& cfg = OverlayConfig::get();
    auto pl = PlayLayer::get();
    for (size_t i = 0; i < cfg.keys.size() && i < m_columns.size(); ++i) {
        auto const& k = cfg.keys[i];
        bool physicallyDown = false;
        if (k.isMouse) {
            int vk = (k.mouseButton == 1) ? VK_RBUTTON : VK_LBUTTON;
            physicallyDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
        } else if (k.keyCode != 0) {
            physicallyDown = (GetAsyncKeyState(k.keyCode) & 0x8000) != 0;
        } else if (k.isGDAction) {
            if (k.gdPlayer == 2) {
                if (k.gdButton == 1 && pl && pl->m_player2) {
                    auto it = pl->m_player2->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                    if (it != pl->m_player2->m_holdingButtons.end()) physicallyDown = it->second;
                    if (pl->m_uiLayer && pl->m_uiLayer->m_p2Jumping) physicallyDown = true;
                } else if (k.gdButton == 2 && pl && pl->m_player2) {
                    physicallyDown = pl->m_player2->m_holdingLeft;
                } else if (k.gdButton == 3 && pl && pl->m_player2) {
                    physicallyDown = pl->m_player2->m_holdingRight;
                }
            } else {
                if (k.gdButton == 1) {
                    physicallyDown = ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) ||
                                     ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) ||
                                     ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                    if (pl && pl->m_player1) {
                        auto it = pl->m_player1->m_holdingButtons.find(static_cast<int>(PlayerButton::Jump));
                        if (it != pl->m_player1->m_holdingButtons.end()) physicallyDown = it->second;
                    }
                    if (pl && pl->m_uiLayer && pl->m_uiLayer->m_p1Jumping) physicallyDown = true;
                } else if (k.gdButton == 2) {
                    physicallyDown = ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) ||
                                     ((GetAsyncKeyState('A') & 0x8000) != 0);
                } else if (k.gdButton == 3) {
                    physicallyDown = ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) ||
                                     ((GetAsyncKeyState('D') & 0x8000) != 0);
                }
            }
        }
        if (physicallyDown != m_columns[i].isPressed) {
            setKeyPressed(i, physicallyDown);
        }
    }
#else
    syncHoldOnReset();
#endif
}

void KeyOverlayNode::handlePlayerButton(PlayerButton button, bool down, bool isSecondPlayer) {
    auto& cfg = OverlayConfig::get();
    int btnInt = static_cast<int>(button);
    int playerNum = isSecondPlayer ? 2 : 1;

    bool matchedAction = false;
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        auto const& k = cfg.keys[i];
        if (k.isGDAction && k.gdButton == btnInt && k.gdPlayer == playerNum) {
            setKeyPressed(i, down);
            matchedAction = true;
        }
    }

    if (!matchedAction && !isSecondPlayer) {
#ifndef GEODE_IS_WINDOWS
        if (button == PlayerButton::Jump) {
            for (size_t i = 0; i < cfg.keys.size(); ++i) {
                auto const& k = cfg.keys[i];
                if (!k.isGDAction && (k.isMouse || k.keyCode == KEY_Space || k.keyCode == KEY_Up)) {
                    setKeyPressed(i, down);
                    break;
                }
            }
        }
#endif
    }
}

void KeyOverlayNode::handleRawKey(int keyCode, bool down) {
    auto& cfg = OverlayConfig::get();
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        auto const& k = cfg.keys[i];
        if (k.keyCode == keyCode && !k.isGDAction) {
            setKeyPressed(i, down);
        }
    }
}

void KeyOverlayNode::handleMouseButton(int button, bool down) {
    auto& cfg = OverlayConfig::get();
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        auto const& k = cfg.keys[i];
        if (k.isMouse && k.mouseButton == button && !k.isGDAction) {
            setKeyPressed(i, down);
        }
    }
}

void KeyOverlayNode::handleGDAction(int action, bool down, int player) {
    auto& cfg = OverlayConfig::get();
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        auto const& k = cfg.keys[i];
        if (k.isGDAction && k.gdButton == action && k.gdPlayer == player) {
            setKeyPressed(i, down);
        }
    }
}

void KeyOverlayNode::resetCounters() {
    auto& cfg = OverlayConfig::get();
    for (size_t i = 0; i < cfg.keys.size(); ++i) {
        cfg.keys[i].clickCount = 0;
        cfg.keys[i].currentCPS = 0;
        if (i < m_columns.size()) {
            m_columns[i].clickTimestamps.clear();
            updateCounterDisplay(i);
        }
    }
    redraw();
}
