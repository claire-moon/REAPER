#include "AchievementMgr.h"
#include "../../Doom/Base/i_misc.h"
#include "../../Doom/Base/i_main.h"
#include "../../Doom/Base/s_sound.h"
#include "../../Doom/Base/sounds.h"
#include "../Game.h"
#include "FileUtils.h"
#include "JsonUtils.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cstdio>
#include <vector>

static const char* ACHIEVEMENTS_FILE = "achievements.json";
static const char* SAVE_FILE = "saved_achievements.json";

void AchievementManager::Init() {
    LoadAchievementsData();
    LoadProgress();
}

void AchievementManager::LoadAchievementsData() {
    m_achievements.clear();

    if (!FileUtils::fileExists(ACHIEVEMENTS_FILE)) {
        // Fallback defaults
        m_achievements.push_back({ "CYBER_KILL", "Cyberbully", "Defeated the Cyberdemon", "STKEYS2", false });
        m_achievements.push_back({ "FIRST_BLOOD", "First Blood", "Killed your first enemy", "STKEYS0", false });
        return;
    }

    FileData fileData = FileUtils::getContentsOfFile(ACHIEVEMENTS_FILE);
    if (fileData.size == 0) return;

    std::string jsonStr((const char*)fileData.bytes.get(), fileData.size);

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError() || !doc.IsArray()) {
        I_Error("Failed to parse achievements.json");
        return;
    }

    for (const auto& val : doc.GetArray()) {
        Achievement ach;
        ach.id = JsonUtils::getOrDefault<const char*>(val, "id", "UNKNOWN");
        ach.title = JsonUtils::getOrDefault<const char*>(val, "title", "Untitled");
        ach.description = JsonUtils::getOrDefault<const char*>(val, "description", "...");
        ach.icon = JsonUtils::getOrDefault<const char*>(val, "icon", "");
        ach.unlocked = false;
        m_achievements.push_back(ach);
    }
}

void AchievementManager::LoadProgress() {
    if (!FileUtils::fileExists(SAVE_FILE)) return;

    FileData fileData = FileUtils::getContentsOfFile(SAVE_FILE);
    if (fileData.size == 0) return;

    std::string jsonStr((const char*)fileData.bytes.get(), fileData.size);

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError() || !doc.IsArray()) return;

    for (const auto& val : doc.GetArray()) {
        if (val.IsString()) {
            std::string id = val.GetString();
            for (auto& ach : m_achievements) {
                if (ach.id == id) {
                    ach.unlocked = true;
                    break;
                }
            }
        }
    }
}

void AchievementManager::SaveProgress() {
    rapidjson::Document doc;
    doc.SetArray();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    for (const auto& ach : m_achievements) {
        if (ach.unlocked) {
            rapidjson::Value v;
            v.SetString(ach.id.c_str(), allocator);
            doc.PushBack(v, allocator);
        }
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    FileUtils::writeDataToFile(SAVE_FILE, buffer.GetString(), buffer.GetSize());
}

void AchievementManager::Unlock(const std::string_view id) {
    for (auto& ach : m_achievements) {
        if (ach.id == id) {
            if (!ach.unlocked) {
                ach.unlocked = true;
                m_notificationQueue.push(ach.id);
                // Play achievement sound
                S_StartSound(nullptr, sfx_getpow);
                SaveProgress();
            }
            return;
        }
    }
}

const std::string& AchievementManager::GetCurrentNotificationTitle() const {
    return m_currentNotificationTitle;
}

const std::string& AchievementManager::GetCurrentNotificationIcon() const {
    return m_currentNotificationIcon;
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
                m_currentNotificationIcon = a.icon;
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
            // TODO: Use a proper box background if possible
            
            int32_t xPos = 20;
            int32_t yPos = 20;

            I_DrawString(xPos, yPos, "ACHIEVEMENT UNLOCKED!");
            I_DrawString(xPos + 10, yPos + 20, m_currentNotificationTitle.c_str());
        }
    }
}
