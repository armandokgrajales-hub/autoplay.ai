#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Esto crea una alerta en el menú principal para comprobar que el mod funciona
        FLAlertLayer::create(
            "Autoplay.Ai",
            "¡Compilacion exitosa! El mod está listo para programar la IA.",
            "OK"
        )->show();

        return true;
    }
};
