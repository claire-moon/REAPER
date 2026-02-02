#include "i_modern_input.h"

#include "Doom/doomdef.h"    // For TickInputs and fixed_t conversions
#include "PsyDoom/Input.h"   // For PsyDoom::Input functions
#include <cmath>
#include <algorithm>
#include <SDL.h>

namespace Modern
{
    // Helper for float to fixed (defined before use)
    static constexpr fixed_t d_float_to_fixed(float f) {
        return (fixed_t)(f * 65536.0f);
    }

    // Singleton instance
    InputManager& InputManager::GetInstance()
    {
        static InputManager instance;
        return instance;
    }

    void InputManager::Init()
    {
        // Default Key Bindings (WASD + Mouse)
        BindKey(GameAction::MoveForward, SDL_SCANCODE_W);
        BindKey(GameAction::MoveBackward, SDL_SCANCODE_S);
        BindKey(GameAction::StrafeLeft,   SDL_SCANCODE_A);
        BindKey(GameAction::StrafeRight,  SDL_SCANCODE_D);
        BindKey(GameAction::Use,          SDL_SCANCODE_E);
        BindKey(GameAction::Attack,       SDL_SCANCODE_LCTRL); // Default fallback
        BindKey(GameAction::Menu,         SDL_SCANCODE_ESCAPE);
        BindKey(GameAction::ToggleMap,    SDL_SCANCODE_TAB);
        
        // Default Mouse Bindings
        BindMouseButton(GameAction::Attack, SDL_BUTTON_LEFT);
        // BindMouseButton(GameAction::Use,    SDL_BUTTON_RIGHT); // Optional: Right click use

        // Default Gamepad Bindings (Modern Shooter Layout)
        BindGamepadButton(GameAction::Use,    GamepadInput::BTN_X); // reload/use usually
        BindGamepadButton(GameAction::Attack, GamepadInput::AXIS_TRIG_RIGHT); // RT to shoot
        BindGamepadButton(GameAction::Menu,   GamepadInput::BTN_START);
        
        // Note: Analog Sticks are handled specially in GenerateTickInputs for smoother control
    }

    void InputManager::Shutdown()
    {
        m_keyBindings.clear();
        m_mouseBindings.clear();
        m_gamepadBindings.clear();
    }

    void InputManager::Update()
    {
        // Allow PsyDoom's core Input system to pump events
        // In a full integration, we might take ownership here, but for now we piggyback
        // PsyDoom::Input::update() is called by the main loop usually.
        
        // Accumulate mouse deltas for this frame
        m_accumMouseX += PsyDoom::Input::getMouseXMovement();
        m_accumMouseY += PsyDoom::Input::getMouseYMovement();
    }

    void InputManager::BindKey(GameAction action, uint32_t sdlScancode)
    {
        m_keyBindings[sdlScancode] = action;
    }

    void InputManager::BindMouseButton(GameAction action, int buttonIndex)
    {
        m_mouseBindings[buttonIndex] = action;
    }

    void InputManager::BindGamepadButton(GameAction action, GamepadInput button)
    {
        m_gamepadBindings[button] = action;
    }

    void InputManager::SetAnalogConfig(const AnalogConfig& config)
    {
        m_analogConfig = config;
    }

    void InputManager::SetRumbleConfig(const RumbleConfig& config)
    {
        m_rumbleConfig = config;
    }

    bool InputManager::IsActionHeld(GameAction action) const
    {
        // Check Keyboard
        for (const auto& binding : m_keyBindings)
        {
            if (binding.second == action)
            {
                if (PsyDoom::Input::isKeyboardKeyPressed((uint16_t)binding.first))
                    return true;
            }
        }

        // Check Mouse
        for (const auto& binding : m_mouseBindings)
        {
            if (binding.second == action)
            {
                // PsyDoom Input uses MouseButton enum which is 1-based usually
                if (PsyDoom::Input::isMouseButtonPressed((MouseButton)binding.first))
                    return true;
            }
        }

        // Check Gamepad
        for (const auto& binding : m_gamepadBindings)
        {
            if (binding.second == action)
            {
                if (PsyDoom::Input::isGamepadInputPressed(binding.first))
                    return true;
            }
        }
        
        return false;
    }

    bool InputManager::IsActionJustPressed(GameAction action) const
    {
        // Check Keyboard
        for (const auto& binding : m_keyBindings)
        {
            if (binding.second == action)
            {
                if (PsyDoom::Input::isKeyboardKeyJustPressed((uint16_t)binding.first))
                    return true;
            }
        }
        
        // Check Mouse
        for (const auto& binding : m_mouseBindings)
        {
            if (binding.second == action)
            {
                if (PsyDoom::Input::isMouseButtonJustPressed((MouseButton)binding.first))
                    return true;
            }
        }

        // Check Gamepad
        for (const auto& binding : m_gamepadBindings)
        {
            if (binding.second == action)
            {
                if (PsyDoom::Input::isGamepadInputJustPressed(binding.first))
                    return true;
            }
        }

        return false;
    }

    bool InputManager::CheckUseInteraction() const
    {
        return IsActionJustPressed(GameAction::Use) || IsActionHeld(GameAction::Use);
    }

    // Helper to get a scalar value (0.0 to 1.0) for a given action
    // Useful for merging button presses with analog axes
    float InputManager::GetRawActionValue(GameAction action) const
    {
        if (IsActionHeld(action))
            return 1.0f;
            
        // Map specific analog axes to actions if not explicitly bound effectively?
        // In this implementation, we handle explicit axis binding via BindGamepadButton
        // if BindGamepadButton was called with an AXIS enum.
        
        // Check if any bound axis is active
        for (const auto& binding : m_gamepadBindings)
        {
            if (binding.second == action)
            {
                if (GamepadInputUtils::isAxis(binding.first))
                {
                    float val = PsyDoom::Input::getAdjustedGamepadInputValue(binding.first, m_analogConfig.deadzone);
                    if (val > 0.0f) return val;
                }
            }
        }

        return 0.0f;
    }

    void InputManager::GetMouseLookDeltas(float& outDx, float& outDy)
    {
        // Calculate raw deltas for this frame
        const float rawDx = m_accumMouseX * m_analogConfig.sensitivityX;
        const float rawDy = m_accumMouseY * m_analogConfig.sensitivityY;

        // Add to smoothing history
        m_mouseHistory.push_back({ rawDx, rawDy });
        while (m_mouseHistory.size() > kMouseSmoothFrames)
        {
            m_mouseHistory.pop_front();
        }

        // Average the history to smooth out jitter
        float avgDx = 0.0f;
        float avgDy = 0.0f;

        for (const auto& delta : m_mouseHistory)
        {
            avgDx += delta.x;
            avgDy += delta.y;
        }

        if (!m_mouseHistory.empty())
        {
            avgDx /= m_mouseHistory.size();
            avgDy /= m_mouseHistory.size();
        }

        outDx = avgDx;
        outDy = avgDy;
        
        // Clear accumulators
        m_accumMouseX = 0.0f;
        m_accumMouseY = 0.0f;
    }

    void InputManager::TriggerRumble(float lowFreq, float highFreq, uint32_t durationMs)
    {
        if (!m_rumbleConfig.enabled)
            return;
            
        float scale = m_rumbleConfig.intensityScale;
        PsyDoom::Input::rumble(lowFreq * scale, highFreq * scale, durationMs);
    }

    void InputManager::GenerateTickInputs(TickInputs& outInputs)
    {
        // 1. Digital/Button Mappings
        if (IsActionHeld(GameAction::MoveForward))  outInputs._flags1.fMoveForward = true;
        if (IsActionHeld(GameAction::MoveBackward)) outInputs._flags1.fMoveBackward = true;
        if (IsActionHeld(GameAction::StrafeLeft))   outInputs._flags1.fStrafeLeft = true;
        if (IsActionHeld(GameAction::StrafeRight))  outInputs._flags1.fStrafeRight = true;
        if (IsActionHeld(GameAction::TurnLeft))     outInputs._flags1.fTurnLeft = true;
        if (IsActionHeld(GameAction::TurnRight))    outInputs._flags1.fTurnRight = true;
        if (IsActionHeld(GameAction::Attack))       outInputs._flags1.fAttack = true;
        if (IsActionHeld(GameAction::Use))          outInputs._flags1.fUse = true;
        
        if (IsActionJustPressed(GameAction::WeaponNext)) outInputs._flags2.fNextWeapon = true;
        if (IsActionJustPressed(GameAction::WeaponPrev)) outInputs._flags2.fPrevWeapon = true;
        if (IsActionJustPressed(GameAction::ToggleMap))  outInputs._flags2.fToggleMap = true;
        
        // 2. Analog Movement (Left Stick)
        // Hardcoded Modern Standard: Left Stick = Move/Strafe
        float fwd = PsyDoom::Input::getAdjustedGamepadInputValue(GamepadInput::AXIS_LEFT_Y, m_analogConfig.deadzone);
        float side = PsyDoom::Input::getAdjustedGamepadInputValue(GamepadInput::AXIS_LEFT_X, m_analogConfig.deadzone);
        
        // Invert Y for forward movement if needed (usually Up is -1 on some joysticks, check SDL)
        // SDL GameController: Axis 1 (LeftY) - Negative is Up, Positive is Down
        // Doom: Positive is Forward
        // So we negate LeftY
        fwd = -fwd; 
        
        if (std::abs(fwd) > 0.01f)
        {
            // Convert float (-1.0 to 1.0) to fixed_t (FRACUNIT = 100% speed)
             outInputs.setAnalogForwardMove(d_float_to_fixed(fwd)); 
        }
        
        if (std::abs(side) > 0.01f)
        {
             outInputs.setAnalogSideMove(d_float_to_fixed(side));
        }
        
        // 3. Analog Turning / Mouse Look (Right Stick + Mouse)
        float turn = 0.0f;
        
        // Right Stick X
        float rx = PsyDoom::Input::getAdjustedGamepadInputValue(GamepadInput::AXIS_RIGHT_X, m_analogConfig.deadzone);
        if (std::abs(rx) > 0.01f)
        {
            if (m_analogConfig.exponentialCurve)
                rx = (rx >= 0 ? 1.0f : -1.0f) * std::pow(std::abs(rx), 2.2f); // Power curve
            turn += rx * m_analogConfig.sensitivityX; // Apply sensitivity
        }

        // Mouse Look
        float mdx, mdy;
        GetMouseLookDeltas(mdx, mdy);
        turn += mdx * 0.05f; // Mouse scale

        // Apply turning
        if (std::abs(turn) > 0.001f)
        {
             // 1 pixel ~= 0.5 degrees
             constexpr float kAngleScale = 100000000.0f;
             angle_t angleDelta = (angle_t)(turn * kAngleScale);
             outInputs.setAnalogTurn(angleDelta);
        }

        // Apply Pitch (Up/Down)
        outInputs.lookPitch = 0;
        if (std::abs(mdy) > 0.001f)
        {
            // Scale mdy (pixels * sensitivity) to INT16 pitch units
            // Map +/- 90 degrees to +/- 16384 approx
            // 1 pixel ~= 0.5 degrees ~= 100 units
            outInputs.lookPitch = (int16_t)(mdy * 100.0f);
        }
    }

    // Helper for float to fixed (simple)
    static constexpr fixed_t d_float_to_fixed(float f) {
        return (fixed_t)(f * 65536.0f);
    }
}
