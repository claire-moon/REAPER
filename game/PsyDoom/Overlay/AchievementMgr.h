#pragma once

#include <string>
#include <vector>
#include <queue>
#include <string_view>

struct Achievement {
    std::string id;
    std::string title;
    std::string description;
    bool unlocked;
};

class AchievementManager {
public:
    static AchievementManager& Get() {
        static AchievementManager instance;
        return instance;
    }

    void Init();
    
    // Call this when a game event happens
    // Usage: AchievementManager::Get().Unlock("CYBER_KILL");
    void Unlock(const std::string_view id);

    // Call this every frame inside ReaperOverlay::Render()
    void UpdateAndRender(float deltaTime);

    // Helper to get formatted notification text
    const std::string& GetCurrentNotificationTitle() const;

private:
    AchievementManager() = default;

    std::vector<Achievement> m_achievements;
    std::queue<std::string> m_notificationQueue;
    
    // Notification State
    bool m_showingNotification = false;
    float m_notificationTimer = 0.0f;
    std::string m_currentNotificationId;
    std::string m_currentNotificationTitle; // Cached title for rendering
    
    // Constants
    static constexpr float NOTIFICATION_DURATION = 5.0f; // Seconds
};
