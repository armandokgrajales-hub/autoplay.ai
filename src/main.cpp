// ============================================================
//  AutoPlay AI - Mod para Geometry Dash 2.2074 (Android)
//  Geode SDK | Desarrollado con C++20
// ============================================================

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/cocos/include/cocos2d.h>

using namespace geode::prelude;
using namespace cocos2d;

// ============================================================
//  ESTADO GLOBAL DE LA IA
// ============================================================

static bool g_aiEnabled = false;       // ¿Está la IA activada?
static bool g_buttonHeld = false;      // ¿Está el botón presionado actualmente?

// ============================================================
//  PARÁMETROS DE DETECCIÓN DE OBSTÁCULOS
// ============================================================

// Distancia horizontal (en unidades de juego) para buscar obstáculos
static constexpr float DETECTION_RANGE_X = 120.0f;

// Distancia vertical máxima para considerar un obstáculo relevante
static constexpr float DETECTION_RANGE_Y = 80.0f;

// IDs de objetos que se consideran peligrosos (spikes, saws, etc.)
// Geometry Dash usa IDs internos para cada tipo de objeto
static const std::vector<int> HAZARD_IDS = {
    // Spikes básicos
    8, 39, 40, 49,
    // Saws (sierras)
    148, 149, 150,
    // Sawblade giratorias
    184, 185,
    // Bloques peligrosos con hitbox mortal
    1, 2, 3, 4,
    // Spikes invertidos
    395, 396,
    // Obstáculos de portales peligrosos (no portales normales)
    // Nota: ajusta según necesidades
    1331, 1332,
};

// ============================================================
//  UTILIDAD: ¿Es un objeto peligroso?
// ============================================================

bool isHazardObject(GameObject* obj) {
    if (!obj) return false;

    int objID = obj->m_objectID;

    // Verificar si el ID está en la lista de peligros
    for (int id : HAZARD_IDS) {
        if (objID == id) return true;
    }

    // Verificar por tipo de objeto usando flags de GD
    // m_objectType == 2 suele indicar objetos sólidos/peligrosos
    // Ignoramos decoraciones (objectType == 4 en muchos casos)
    if (obj->m_objectType == GameObjectType::Decoration) {
        return false;
    }

    // Objetos con hitbox mortal activada
    if (obj->m_isDead) return false; // objeto ya destruido

    return false;
}

// ============================================================
//  LÓGICA PRINCIPAL: HOOK EN PlayLayer::update
// ============================================================

class $modify(AIPlayLayer, PlayLayer) {

    // Se ejecuta cada frame del juego
    void update(float dt) {
        // Primero ejecutamos el update original (NO omitirlo)
        PlayLayer::update(dt);

        // Si la IA está desactivada, no hacemos nada
        if (!g_aiEnabled) return;

        // Verificamos que el jugador exista y esté vivo
        if (!m_player1 || !m_isAlive) return;

        // Obtenemos la posición del jugador
        CCPoint playerPos = m_player1->getPosition();

        // --------------------------------------------------------
        //  DETECCIÓN DE OBSTÁCULOS
        // --------------------------------------------------------
        bool shouldPress = false;

        // Recorremos todos los objetos del nivel
        // m_objects contiene todos los GameObjects cargados
        if (m_objects) {
            for (int i = 0; i < m_objects->count(); i++) {
                auto* obj = static_cast<GameObject*>(m_objects->objectAtIndex(i));
                if (!obj) continue;

                // Ignorar decoraciones explícitamente
                if (obj->m_objectType == GameObjectType::Decoration) continue;

                // Posición del objeto
                CCPoint objPos = obj->getPosition();

                // Distancia X: el objeto debe estar ADELANTE del jugador
                float deltaX = objPos.x - playerPos.x;
                if (deltaX < 0 || deltaX > DETECTION_RANGE_X) continue;

                // Distancia Y: el objeto debe estar en rango vertical
                float deltaY = std::abs(objPos.y - playerPos.y);
                if (deltaY > DETECTION_RANGE_Y) continue;

                // Verificar si es un obstáculo peligroso
                if (isHazardObject(obj)) {
                    shouldPress = true;
                    break; // Ya encontramos un peligro, no seguimos
                }

                // Detección adicional: si hay un bloque sólido y el jugador
                // está en modo vuelo (ship, UFO, etc.), saltar sobre él
                if (obj->m_objectType == GameObjectType::Solid) {
                    // Si el obstáculo está a la misma altura o encima
                    if (objPos.y >= playerPos.y - 20.0f && deltaX < 80.0f) {
                        shouldPress = true;
                        break;
                    }
                }
            }
        }

        // --------------------------------------------------------
        //  CONTROL DEL BOTÓN (simular tap/click)
        // --------------------------------------------------------
        if (shouldPress && !g_buttonHeld) {
            // Presionar el botón del jugador 1
            this->pushButton(0, true);
            g_buttonHeld = true;
        } else if (!shouldPress && g_buttonHeld) {
            // Soltar el botón
            this->releaseButton(0, true);
            g_buttonHeld = false;
        }
    }

    // Resetear el estado cuando el nivel se reinicia
    void resetLevel() {
        AIPlayLayer::resetLevel();
        g_buttonHeld = false;

        // Liberar botón al reiniciar para evitar estados corruptos
        if (m_player1) {
            this->releaseButton(0, true);
        }
    }

    // Asegurarse de limpiar al salir del nivel
    void onQuit() {
        g_buttonHeld = false;
        AIPlayLayer::onQuit();
    }
};

// ============================================================
//  GUI: BOTÓN EN PauseLayer PARA ACTIVAR/DESACTIVAR LA IA
// ============================================================

class $modify(AIPauseLayer, PauseLayer) {

    void customSetup() {
        // Ejecutar setup original del PauseLayer
        PauseLayer::customSetup();

        // Obtenemos el tamaño de la pantalla
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // --------------------------------------------------------
        //  Crear el botón ON/OFF para la IA
        // --------------------------------------------------------

        // Texto del botón según estado actual
        std::string btnLabel = g_aiEnabled ? "AI: ON" : "AI: OFF";

        // Crear label para el botón
        auto labelOn  = CCLabelTTF::create("AI: ON",  "bigFont.fnt", 20.0f);
        auto labelOff = CCLabelTTF::create("AI: OFF", "bigFont.fnt", 20.0f);

        // Usar CCMenuItemToggle para un botón de alternancia
        auto itemOn  = CCMenuItemLabel::create(labelOn,  this,
            menu_selector(AIPauseLayer::onAIToggle));
        auto itemOff = CCMenuItemLabel::create(labelOff, this,
            menu_selector(AIPauseLayer::onAIToggle));

        auto toggleBtn = CCMenuItemToggle::createWithTarget(
            this,
            menu_selector(AIPauseLayer::onAIToggle),
            itemOff, itemOn, nullptr
        );

        // Establecer estado inicial del toggle
        toggleBtn->setSelectedIndex(g_aiEnabled ? 1 : 0);
        toggleBtn->setTag(100); // Tag para identificarlo

        // Colorear según estado
        if (g_aiEnabled) {
            labelOn->setColor({0, 255, 100});   // Verde cuando ON
        } else {
            labelOff->setColor({255, 80, 80});  // Rojo cuando OFF
        }

        // --------------------------------------------------------
        //  Posicionar el botón en la esquina inferior derecha
        // --------------------------------------------------------
        auto menu = CCMenu::create();
        menu->addChild(toggleBtn);
        menu->setPosition({winSize.width - 60.0f, 30.0f});
        menu->setZOrder(10);

        // Agregar el menú al PauseLayer
        this->addChild(menu);

        // --------------------------------------------------------
        //  Agregar etiqueta descriptiva
        // --------------------------------------------------------
        auto descLabel = CCLabelTTF::create(
            "AutoPlay AI",
            "chatFont.fnt",
            12.0f
        );
        descLabel->setPosition({winSize.width - 60.0f, 50.0f});
        descLabel->setColor({255, 255, 255});
        descLabel->setOpacity(180);
        this->addChild(descLabel, 10);
    }

    // Callback cuando el usuario toca el botón
    void onAIToggle(CCObject* sender) {
        // Alternar el estado global de la IA
        g_aiEnabled = !g_aiEnabled;

        // Notificación visual usando Geode Notification
        std::string msg = g_aiEnabled
            ? "AutoPlay AI: ACTIVADO ✓"
            : "AutoPlay AI: DESACTIVADO ✗";

        Notification::create(msg, NotificationIcon::Success)->show();

        // Si desactivamos la IA, liberar el botón por seguridad
        if (!g_aiEnabled && g_buttonHeld) {
            auto* pl = PlayLayer::get();
            if (pl) {
                pl->releaseButton(0, true);
            }
            g_buttonHeld = false;
        }

        log::info("AutoPlay AI toggled: {}", g_aiEnabled ? "ON" : "OFF");
    }
};

// ============================================================
//  PUNTO DE ENTRADA DEL MOD
// ============================================================

$on_mod(Loaded) {
    log::info("AutoPlay AI v1.0.0 cargado correctamente.");
    log::info("Usa el boton en PauseLayer para activar/desactivar.");
}

