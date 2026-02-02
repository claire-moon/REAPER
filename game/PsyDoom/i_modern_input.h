#pragma once

#include "Doom/doomdef.h"    // For TickInputs
#include "Input.h"           // For basic types like GamepadInput, MouseButton

#include <cstdint>
#include <functional>
#include <vector>
#include <deque>
#include <map>

namespace Modern
{
    // High-level game actions that can be remapped to physical inputs.
    // Differentiates 'Use' from 'Attack' for interaction complexity.
    enum class GameAction : uint8_t
    {
        None,
        
        // Movement
        MoveForward,
        MoveBackward,
        StrafeLeft,
        StrafeRight,
        
        // View (Digital/Analog fallback)
        TurnLeft,
        TurnRight,
        LookUp,
        LookDown,

        // Actions
        Attack,
        Use,            // Distinct "Use" interaction (Open Door, Flip Switch)
        Jump,           // Reserved for future modernization
        Crouch,         // Reserved for future modernization
        
        // Weaponry
        WeaponNext,
        WeaponPrev,
        
        // Meta
        ToggleMap,
        Menu,
        
        Count
    };

    // Configuration structure for analog stick behavior
    struct AnalogConfig
    {
        float deadzone = 0.2f;
        float sensitivityX = 1.0f;
        float sensitivityY = 1.0f;
        bool  invertY = false;
        bool  exponentialCurve = true; // Apply power curve for finer aiming
    };

    struct RumbleConfig
    {
        bool enabled = true;
        float intensityScale = 1.0f;
    };

    // The InputManager class abstracts hardware polling (SDL) into Game Actions.
    // It acts as the bridge between raw OS events and the game simulation (TickInputs).
    class InputManager
    {
    public:
        static InputManager& GetInstance();

        // --------------------------------------------------------------------------
        // Lifecycle
        // --------------------------------------------------------------------------
        void Init();
        void Shutdown();
        
        // Polls hardware devices (SDL_PumpEvents locally or wraps PsyDoom::Input::update)
        void Update();

        // --------------------------------------------------------------------------
        // Binding & Configuration
        // --------------------------------------------------------------------------
        // Bind a keyboard key to an action
        void BindKey(GameAction action, uint32_t sdlScancode);

        // Bind a mouse button to an action
        void BindMouseButton(GameAction action, int buttonIndex);

        // Bind a physical gamepad button to an action
        void BindGamepadButton(GameAction action, GamepadInput button);

        // Configure Analog Sticks (Hardware abstraction)
        void SetAnalogConfig(const AnalogConfig& config);
        const AnalogConfig& GetAnalogConfig() const { return m_analogConfig; }

        void SetRumbleConfig(const RumbleConfig& config);
        
        // --------------------------------------------------------------------------
        // State Querying
        // --------------------------------------------------------------------------
        
        // Checks if an action is currently held down
        bool IsActionHeld(GameAction action) const;

        // Checks if an action was pressed this exact frame (buffered)
        bool IsActionJustPressed(GameAction action) const;

        // --------------------------------------------------------------------------
        // "Build Engine" Style Interaction Hook
        // --------------------------------------------------------------------------
        // Unlike Doom's original implicit usage, this allows explicit querying for
        // the interaction button. Useful for raycasting interaction logic separate
        // from weapon firing.
        // Returns true if the 'Use' action is active and should trigger interaction protocols.
        bool CheckUseInteraction() const;

        // --------------------------------------------------------------------------
        // Simulation Bridge
        // --------------------------------------------------------------------------
        // Populates the PsyDoom specific TickInputs structure for the simulation tick.
        // Handles proper conversion of analog mouse deltas to turning angles.
        void GenerateTickInputs(TickInputs& outInputs);

        // Consumes accumulated mouse deltas (Freelook)
        // --------------------------------------------------------------------------
        // Feedback
        // --------------------------------------------------------------------------
        void TriggerRumble(float lowFreq, float highFreq, uint32_t durationMs);

        // Returns normalized movement (dx, dy) scaled by sensitivity
        void GetMouseLookDeltas(float& outDx, float& outDy);

    private:
        InputManager() = default;
        ~InputManager() = default;

        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // Internal translation helpers
        float GetRawActionValue(GameAction action) const;
        void  ApplyMouseLook(TickInputs& outInputs);

        // Bindings
        RumbleConfig m_rumbleConfig;
        std::map<uint32_t, GameAction> m_keyBindings;
        std::map<int, GameAction>      m_mouseBindings;
        std::map<GamepadInput, GameAction> m_gamepadBindings;

        AnalogConfig m_analogConfig;
        
        // Accumulators for mouse frame data
        float m_accumMouseX = 0.0f;
        float m_accumMouseY = 0.0f;

        // Mouse Smoothing History
        struct MouseDelta { float x; float y; };
        std::deque<MouseDelta> m_mouseHistory;
        static constexpr size_t kMouseSmoothFrames = 3;
    };

} // namespace Modern
