#include "OverlayMain.h"
#include "AchievementMgr.h"

// Access global frame duration for smooth animations
extern double gPrevFrameDuration;

namespace ReaperOverlay {

    static bool g_isInitialized = false;

    void Init() {
        if (g_isInitialized) return;
        AchievementManager::Get().Init();
        g_isInitialized = true;
    }

    void Render() {
        // Lazy initialization to ensure system is up when first rendered
        if (!g_isInitialized) {
            Init();
        }

        float dt = (float)gPrevFrameDuration;
        // Fallback if delta time is zero (e.g. first frame or paused logic weirdness)
        if (dt <= 0.0001f) {
            dt = 1.0f / 30.0f; 
        }

        AchievementManager::Get().UpdateAndRender(dt);
    }

    void Shutdown() {
        g_isInitialized = false;
    }
}
