#pragma once

struct TickInputs;

class OverlayMain {
public:
    static OverlayMain& GetInstance() {
        static OverlayMain instance;
        return instance;
    }

    void Init();
    void Render();
    void Shutdown();

    // Interactive Mode
    void ToggleInteractiveMode();
    bool IsInteractiveMode() const;
    void HandleInput(TickInputs& inputs);

private:
    OverlayMain() = default;

    bool m_isInitialized = false;
    bool m_isInteractiveMode = false;
    int m_selectedMenuItem = 0;
    
    // Simple cooldown to prevent crazy scrolling
    float m_inputCooldown = 0.0f;
};
