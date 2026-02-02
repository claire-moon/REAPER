#include "OverlayMain.h"
#include "AchievementMgr.h"
#include "../../Doom/Game/p_tick.h" 
#include "../../Doom/Base/i_main.h" 
#include <cstdio>

extern double gPrevFrameDuration;

void OverlayMain::Init() {
    if (m_isInitialized) return;
    AchievementManager::Get().Init();
    m_isInitialized = true;
}

void OverlayMain::Shutdown() {
    m_isInitialized = false;
}

void OverlayMain::Render() {
    if (!m_isInitialized) Init();

    float dt = (float)gPrevFrameDuration;
    if (dt <= 0.0001f) dt = 1.0f / 30.0f; 

    if (m_inputCooldown > 0.0f) m_inputCooldown -= dt;

    AchievementManager::Get().UpdateAndRender(dt);

    if (m_isInteractiveMode) {
        int startX = 60;
        int startY = 40;
        int spacing = 15;

        // Simple background text for now
        I_DrawString(startX, startY, "REAPER OVERLAY PREFS");
        startY += 20;

        const char* menuItems[] = {
            "Toggle Notifications: ON",
            "Sound Volume: 100",
            "Exit Overlay"
        };
        int numItems = 3;

        for (int i = 0; i < numItems; ++i) {
            bool selected = (i == m_selectedMenuItem);
            int y = startY + (i * spacing);
            
            char buffer[64];
            if (selected) {
                // simple arrow
                std::snprintf(buffer, sizeof(buffer), "-> %s", menuItems[i]);
            } else {
                std::snprintf(buffer, sizeof(buffer), "   %s", menuItems[i]);
            }
            I_DrawString(startX, y, buffer);
        }
    }
}

void OverlayMain::ToggleInteractiveMode() {
    m_isInteractiveMode = !m_isInteractiveMode;
    m_inputCooldown = 0.2f;
}

bool OverlayMain::IsInteractiveMode() const {
    return m_isInteractiveMode;
}

void OverlayMain::HandleInput(TickInputs& inputs) {
    if (!m_isInteractiveMode) return;
    if (m_inputCooldown > 0.0f) return;

    // Use accessor methods from TickInputs (macro generated)
    // We assume these return bool& or bool-convertible
    if (inputs.fMenuDown()) {
        m_selectedMenuItem++;
        if (m_selectedMenuItem > 2) m_selectedMenuItem = 0;
        m_inputCooldown = 0.2f;
    }
    else if (inputs.fMenuUp()) {
        m_selectedMenuItem--;
        if (m_selectedMenuItem < 0) m_selectedMenuItem = 2;
        m_inputCooldown = 0.2f;
    }
    else if (inputs.fMenuOk() || inputs.fAttack()) {
        if (m_selectedMenuItem == 2) {
            ToggleInteractiveMode();
        }
        else if (m_selectedMenuItem == 0) {
            // Toggle Notifications Logic (TODO)
        }
        m_inputCooldown = 0.2f;
    }
}
