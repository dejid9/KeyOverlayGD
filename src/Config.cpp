#include "Config.hpp"

OverlayConfig& OverlayConfig::get() {
    static OverlayConfig instance;
    return instance;
}

static matjson::Value colorToJson(ccColor4B const& c) {
    auto arr = matjson::Value::array();
    arr.push(static_cast<int>(c.r));
    arr.push(static_cast<int>(c.g));
    arr.push(static_cast<int>(c.b));
    arr.push(static_cast<int>(c.a));
    return arr;
}

static ccColor4B jsonToColor(matjson::Value const& v, ccColor4B def) {
    if (v.isArray() && v.size() >= 4) {
        return ccc4(
            static_cast<GLubyte>(v[0].asInt().unwrapOr(def.r)),
            static_cast<GLubyte>(v[1].asInt().unwrapOr(def.g)),
            static_cast<GLubyte>(v[2].asInt().unwrapOr(def.b)),
            static_cast<GLubyte>(v[3].asInt().unwrapOr(def.a))
        );
    }
    return def;
}

void OverlayConfig::resetDefaults() {
    keys.clear();

    KeyBindInfo k1;
    k1.id = "key_1";
    k1.label = "M1";
    k1.customLabel = "";
    k1.keyCode = 0;
    k1.isMouse = true;
    k1.mouseButton = 0;
    k1.isGDAction = true;
    k1.gdButton = 1;
    k1.gdPlayer = 1;
    k1.clickCount = 0;
    keys.push_back(k1);

    KeyBindInfo k2;
    k2.id = "key_2";
    k2.label = "Sp";
    k2.customLabel = "";
    k2.keyCode = KEY_Space;
    k2.isMouse = false;
    k2.isGDAction = false;
    k2.gdButton = 1;
    k2.clickCount = 0;
    keys.push_back(k2);

    KeyBindInfo k3;
    k3.id = "key_3";
    k3.label = "Up";
    k3.customLabel = "";
    k3.keyCode = KEY_Up;
    k3.isMouse = false;
    k3.isGDAction = false;
    k3.gdButton = 1;
    k3.clickCount = 0;
    keys.push_back(k3);

    KeyBindInfo k4;
    k4.id = "key_4";
    k4.label = "R";
    k4.customLabel = "";
    k4.keyCode = KEY_R;
    k4.isMouse = false;
    k4.isGDAction = false;
    k4.gdButton = 1;
    k4.clickCount = 0;
    keys.push_back(k4);

    position = CCPoint(30.0f, 60.0f);
    scale = 1.0f;
    keyWidth = 44.0f;
    keyHeight = 44.0f;
    keySpacing = 6.0f;
    streamLength = 160.0f;
    barSpeed = 340.0f;
    scrollUpwards = true;
    showCounters = true;
    showLabels = true;
    showContainerBg = false;
    cornerRadius = 2.0f;
    borderWidth = 1.8f;
    showBounds = false;

    keyFont = "bigFont.fnt";
    counterFont = "bigFont.fnt";
    labelScale = 0.38f;
    counterScale = 0.33f;
    labelOffsetY = 0.0f;
    counterOffsetY = 0.0f;

    containerBgColor = { 15, 15, 20, 150 };
    keyBgColor = { 20, 20, 25, 130 };
    keyPressedColor = { 255, 255, 255, 240 };
    barColor = { 255, 255, 255, 230 };
    keyBorderColor = { 255, 255, 255, 220 };
    labelColor = { 255, 255, 255, 240 };
    labelPressedColor = { 20, 20, 25, 255 };
    counterColor = { 255, 255, 255, 240 };

    shadowEnabled = true;
    shadowColor = { 0, 0, 0, 140 };
    shadowDistance = 0.0f;
    shadowAngle = 270.0f;
    shadowBlur = 6.0f;

    overlayFPS = 0.0f;
    pollingRate = 1000;
    counterMode = 0;
    fastBarShadow = true;

    enabled = true;
    resetCounterOnAttempt = false;
    hideInEditor = true;
}

void OverlayConfig::save() {
    auto json = matjson::Value::object();
    json["enabled"] = enabled;
    json["resetCounterOnAttempt"] = resetCounterOnAttempt;
    json["hideInEditor"] = hideInEditor;

    json["overlayFPS"] = overlayFPS;
    json["pollingRate"] = pollingRate;
    json["counterMode"] = counterMode;
    json["fastBarShadow"] = fastBarShadow;

    json["posX"] = position.x;
    json["posY"] = position.y;
    json["scale"] = scale;
    json["keyWidth"] = keyWidth;
    json["keyHeight"] = keyHeight;
    json["keySpacing"] = keySpacing;
    json["streamLength"] = streamLength;
    json["barSpeed"] = barSpeed;
    json["scrollUpwards"] = scrollUpwards;
    json["showCounters"] = showCounters;
    json["showLabels"] = showLabels;
    json["showContainerBg"] = showContainerBg;
    json["cornerRadius"] = cornerRadius;
    json["borderWidth"] = borderWidth;
    json["showBounds"] = showBounds;

    json["keyFont"] = keyFont;
    json["counterFont"] = counterFont;
    json["labelScale"] = labelScale;
    json["counterScale"] = counterScale;
    json["labelOffsetY"] = labelOffsetY;
    json["counterOffsetY"] = counterOffsetY;

    json["containerBgColor"] = colorToJson(containerBgColor);
    json["keyBgColor"] = colorToJson(keyBgColor);
    json["keyPressedColor"] = colorToJson(keyPressedColor);
    json["barColor"] = colorToJson(barColor);
    json["keyBorderColor"] = colorToJson(keyBorderColor);
    json["labelColor"] = colorToJson(labelColor);
    json["labelPressedColor"] = colorToJson(labelPressedColor);
    json["counterColor"] = colorToJson(counterColor);

    json["shadowEnabled"] = shadowEnabled;
    json["shadowColor"] = colorToJson(shadowColor);
    json["shadowDistance"] = shadowDistance;
    json["shadowAngle"] = shadowAngle;
    json["shadowBlur"] = shadowBlur;

    auto keysArr = matjson::Value::array();
    for (auto const& k : keys) {
        auto kObj = matjson::Value::object();
        kObj["id"] = k.id;
        kObj["label"] = k.label;
        kObj["customLabel"] = k.customLabel;
        kObj["keyCode"] = k.keyCode;
        kObj["isMouse"] = k.isMouse;
        kObj["mouseButton"] = k.mouseButton;
        kObj["isGDAction"] = k.isGDAction;
        kObj["gdButton"] = k.gdButton;
        kObj["gdPlayer"] = k.gdPlayer;
        keysArr.push(kObj);
    }
    json["keys"] = keysArr;

    Mod::get()->setSavedValue("overlay_config", json);
}

void OverlayConfig::load() {
    auto json = Mod::get()->getSavedValue<matjson::Value>("overlay_config");
    if (!json.isObject()) {
        resetDefaults();
        save();
        return;
    }

    enabled = json["enabled"].asBool().unwrapOr(true);
    resetCounterOnAttempt = json["resetCounterOnAttempt"].asBool().unwrapOr(false);
    hideInEditor = json["hideInEditor"].asBool().unwrapOr(true);

    overlayFPS = static_cast<float>(json["overlayFPS"].asDouble().unwrapOr(0.0));
    pollingRate = static_cast<int>(json["pollingRate"].asInt().unwrapOr(1000));
    counterMode = static_cast<int>(json["counterMode"].asInt().unwrapOr(0));
    fastBarShadow = json["fastBarShadow"].asBool().unwrapOr(true);

    position.x = static_cast<float>(json["posX"].asDouble().unwrapOr(30.0));
    position.y = static_cast<float>(json["posY"].asDouble().unwrapOr(60.0));
    scale = static_cast<float>(json["scale"].asDouble().unwrapOr(1.0));
    keyWidth = static_cast<float>(json["keyWidth"].asDouble().unwrapOr(44.0));
    keyHeight = static_cast<float>(json["keyHeight"].asDouble().unwrapOr(44.0));
    keySpacing = static_cast<float>(json["keySpacing"].asDouble().unwrapOr(6.0));
    streamLength = static_cast<float>(json["streamLength"].asDouble().unwrapOr(160.0));
    barSpeed = static_cast<float>(json["barSpeed"].asDouble().unwrapOr(340.0));
    scrollUpwards = json["scrollUpwards"].asBool().unwrapOr(true);
    showCounters = json["showCounters"].asBool().unwrapOr(true);
    showLabels = json["showLabels"].asBool().unwrapOr(true);
    showContainerBg = json["showContainerBg"].asBool().unwrapOr(false);
    cornerRadius = static_cast<float>(json["cornerRadius"].asDouble().unwrapOr(2.0));
    borderWidth = static_cast<float>(json["borderWidth"].asDouble().unwrapOr(1.8));
    showBounds = json["showBounds"].asBool().unwrapOr(false);

    keyFont = json["keyFont"].asString().unwrapOr("bigFont.fnt");
    counterFont = json["counterFont"].asString().unwrapOr("bigFont.fnt");
    labelScale = static_cast<float>(json["labelScale"].asDouble().unwrapOr(0.38));
    counterScale = static_cast<float>(json["counterScale"].asDouble().unwrapOr(0.33));
    labelOffsetY = static_cast<float>(json["labelOffsetY"].asDouble().unwrapOr(0.0));
    counterOffsetY = static_cast<float>(json["counterOffsetY"].asDouble().unwrapOr(0.0));

    containerBgColor = jsonToColor(json["containerBgColor"], { 15, 15, 20, 150 });
    keyBgColor = jsonToColor(json["keyBgColor"], { 20, 20, 25, 130 });
    keyPressedColor = jsonToColor(json["keyPressedColor"], { 255, 255, 255, 240 });
    barColor = jsonToColor(json["barColor"], { 255, 255, 255, 230 });
    keyBorderColor = jsonToColor(json["keyBorderColor"], { 255, 255, 255, 220 });
    labelColor = jsonToColor(json["labelColor"], { 255, 255, 255, 240 });
    labelPressedColor = jsonToColor(json["labelPressedColor"], { 20, 20, 25, 255 });
    counterColor = jsonToColor(json["counterColor"], { 255, 255, 255, 240 });

    shadowEnabled = json["shadowEnabled"].asBool().unwrapOr(true);
    shadowColor = jsonToColor(json["shadowColor"], { 0, 0, 0, 140 });
    shadowDistance = static_cast<float>(json["shadowDistance"].asDouble().unwrapOr(0.0));
    shadowAngle = static_cast<float>(json["shadowAngle"].asDouble().unwrapOr(270.0));
    shadowBlur = static_cast<float>(json["shadowBlur"].asDouble().unwrapOr(6.0));

    keys.clear();
    if (json["keys"].isArray()) {
        auto arrRes = json["keys"].asArray();
        if (arrRes.isOk()) {
            for (auto const& kVal : arrRes.unwrap()) {
                if (!kVal.isObject()) continue;
                KeyBindInfo k;
                k.id = kVal["id"].asString().unwrapOr("key");
                k.label = kVal["label"].asString().unwrapOr("?");
                k.customLabel = kVal["customLabel"].asString().unwrapOr("");
                k.keyCode = static_cast<int>(kVal["keyCode"].asInt().unwrapOr(0));
                k.isMouse = kVal["isMouse"].asBool().unwrapOr(false);
                k.mouseButton = static_cast<int>(kVal["mouseButton"].asInt().unwrapOr(0));
                k.isGDAction = kVal["isGDAction"].asBool().unwrapOr(false);
                k.gdButton = static_cast<int>(kVal["gdButton"].asInt().unwrapOr(1));
                k.gdPlayer = static_cast<int>(kVal["gdPlayer"].asInt().unwrapOr(1));
                k.clickCount = 0;
                keys.push_back(k);
            }
        }
    }
    if (keys.empty()) {
        resetDefaults();
    }
}
