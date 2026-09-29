#include <Geode/Geode.hpp>
#include "Config.hpp"
#include "InputPoller.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    OverlayConfig::get().load();
    InputPoller::get().start();
    log::info("KeyOverlay mod loaded successfully!");
}
