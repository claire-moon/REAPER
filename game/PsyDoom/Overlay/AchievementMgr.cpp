#include "AchievementMgr.h"
#include "../../Doom/Base/i_misc.h"
#include "../../Doom/Base/i_main.h"
#include "../Game.h"
#include <cstdio>

void AchievementManager::Init() {
    // Register hardcoded achievements 
    // In a real system, these might be loaded from a JSON file
    m_achievements.push_back({ "CYBER_KILL", "Cyberbully", "Defeated the Cyberdemon", false });
    m_achievements.push_back({ "FIRST_BLOOD", "First Blood", "Killed your first enemy", false });
    m_achievements.push_back({ "TEST_ACH", "Test Achievement", "This is a test notification", false });
}

void AchievementManager::Unlock(const std::string_view id) {
    for (auto& ach : m_achievements) {
        if (ach.id == id) {
            if (!ach.unlocked) {
                ach.unlocked = true;
                m_notificationQueue.push(ach.id);
            }
            return;
        }
    }
}

const std::string& AchievementManager::GetCurrentNotificationTitle() const {
    return m_currentNotificationTitle;
}

void AchievementManager::UpdateAndRender(float deltaTime) {
    // 1. Manage Queue
    if (!m_showingNotification && !m_notificationQueue.empty()) {
        m_currentNotificationId = m_notificationQueue.front();
        m_notificationQueue.pop();
        
        // Find title
        for (const auto& a : m_achievements) {
            if (a.id == m_currentNotificationId) {
                m_currentNotificationTitle = a.title;
                break;
            }
        }
        
        m_showingNotification = true;
        m_notificationTimer = NOTIFICATION_DURATION;
    }

    // 2. Render if active
    if (m_showingNotification) {
        m_notificationTimer -= deltaTime;
        if (m_notificationTimer <= 0.0f) {
            m_showingNotification = false;
        } else {
            // Draw Notification
            // TODO: Use a proper box background if possible, or just text for now.
            // Text render loop or overlay render loop usually happens at the end of frame.
            
            // X, Y positions. Let's put it at top center. 
            // Screen width is typically 320 for PSX Doom.
            // I_DrawString centers if we calculate width, but simpler to just put it at 10, 10
            
            int32_t xPos = 20;
            int32_t yPos = 20;

            I_DrawString(xPos, yPos, "ACHIEVEMENT UNLOCKED!");
            I_DrawString(xPos + 10, yPos + 20, m_currentNotificationTitle.c_str());
        }
    }
}
