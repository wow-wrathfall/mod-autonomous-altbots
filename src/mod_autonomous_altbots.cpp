/*
 * Copyright (C) 2026 mod-autonomous-altbots contributors
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 2 of the License, or any later version.
 */

#include "CharacterCache.h"
#include "Chat.h"
#include "CommandScript.h"
#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerSettings.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotMgr.h"
#include "Playerbots.h"
#include "RBAC.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"
#include "World.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace Acore::ChatCommands;

namespace
{
constexpr char SETTING_SOURCE[] = "mod_autonomous_altbots";
constexpr char MODULE_VERSION[] = "1.0.1";
constexpr uint32 SETTING_ENABLED = 0;
constexpr uint32 STOP_WATCH_DURATION_MS = 30000;

struct AutonomousAltConfig
{
    bool enabled = true;
    uint32 startupDelayMs = 30000;
    uint32 checkIntervalMs = 10000;
    bool quest = true;
    bool grind = true;
    bool newRpg = true;
    bool independent = true;
    bool verboseLogging = true;
};

AutonomousAltConfig g_Config;
std::unordered_set<ObjectGuid::LowType> g_DesiredBots;
std::unordered_map<ObjectGuid::LowType, uint32> g_StopWatchElapsed;
uint32 g_StartupElapsed = 0;
uint32 g_CheckElapsed = 0;
bool g_StartupComplete = false;

bool PlayerSettingsEnabled()
{
    return sWorld->getBoolConfig(CONFIG_PLAYER_SETTINGS_ENABLED);
}

void LoadConfig()
{
    g_Config.enabled = sConfigMgr->GetOption<bool>("AutonomousAltBots.Enable", true);
    g_Config.startupDelayMs =
        sConfigMgr->GetOption<uint32>("AutonomousAltBots.StartupDelaySeconds", 30) * IN_MILLISECONDS;
    g_Config.checkIntervalMs = std::max<uint32>(
        IN_MILLISECONDS,
        sConfigMgr->GetOption<uint32>("AutonomousAltBots.CheckIntervalSeconds", 10) * IN_MILLISECONDS);
    g_Config.quest = sConfigMgr->GetOption<bool>("AutonomousAltBots.Strategy.Quest", true);
    g_Config.grind = sConfigMgr->GetOption<bool>("AutonomousAltBots.Strategy.Grind", true);
    g_Config.newRpg = sConfigMgr->GetOption<bool>("AutonomousAltBots.Strategy.NewRpg", true);
    g_Config.independent = sConfigMgr->GetOption<bool>("AutonomousAltBots.Independent", true);
    g_Config.verboseLogging = sConfigMgr->GetOption<bool>("AutonomousAltBots.VerboseLogging", true);
}

void LoadDesiredBots()
{
    g_DesiredBots.clear();

    if (!PlayerSettingsEnabled())
    {
        LOG_ERROR(
            "module",
            "mod-autonomous-altbots: EnablePlayerSettings must be set to 1 in worldserver.conf; no alts will be loaded");
        return;
    }

    CharacterDatabasePreparedStatement* stmt =
        CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_SETTINGS_BY_SOURCE);
    stmt->SetData(0, SETTING_SOURCE);

    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();
            ObjectGuid::LowType guid = fields[0].Get<ObjectGuid::LowType>();
            PlayerSettingVector settings = PlayerSettingsStore::ParseSettingsData(fields[1].Get<std::string>());
            if (!settings.empty() && settings[SETTING_ENABLED].IsEnabled())
                g_DesiredBots.insert(guid);
        } while (result->NextRow());
    }

    LOG_INFO("module", "mod-autonomous-altbots: loaded {} persistent character selection(s)", g_DesiredBots.size());
}

Player* FindConnectedCharacter(ObjectGuid guid)
{
    return ObjectAccessor::FindConnectedPlayer(guid);
}

bool IsPlayerbot(Player* player)
{
    if (!player)
        return false;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
    return botAI && !botAI->IsRealPlayer();
}

void ApplyAutonomousStrategies(Player* bot)
{
    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI || botAI->IsRealPlayer())
        return;

    bot->UpdatePlayerSetting(SETTING_SOURCE, SETTING_ENABLED, 1);

    if (g_Config.independent && botAI->GetMaster())
        botAI->SetMaster(nullptr);

    if (g_Config.quest && !botAI->HasStrategy("quest", BOT_STATE_NON_COMBAT))
        botAI->ChangeStrategy("+quest", BOT_STATE_NON_COMBAT);

    if (g_Config.grind && !botAI->HasStrategy("grind", BOT_STATE_NON_COMBAT))
        botAI->ChangeStrategy("+grind", BOT_STATE_NON_COMBAT);

    if (g_Config.newRpg && !botAI->HasStrategy("new rpg", BOT_STATE_NON_COMBAT))
        botAI->ChangeStrategy("+new rpg", BOT_STATE_NON_COMBAT);

    if (g_Config.independent)
    {
        if (botAI->HasStrategy("follow", BOT_STATE_NON_COMBAT))
            botAI->ChangeStrategy("-follow", BOT_STATE_NON_COMBAT);
        if (botAI->HasStrategy("stay", BOT_STATE_NON_COMBAT))
            botAI->ChangeStrategy("-stay", BOT_STATE_NON_COMBAT);
    }

    botAI->AllowActivity(ALL_ACTIVITY, true);
}

void SetDesired(ObjectGuid guid, bool enabled)
{
    PlayerSettingsStore::UpdateSetting(guid.GetCounter(), SETTING_SOURCE, SETTING_ENABLED, enabled ? 1 : 0);

    if (Player* player = FindConnectedCharacter(guid))
        player->UpdatePlayerSetting(SETTING_SOURCE, SETTING_ENABLED, enabled ? 1 : 0);

    if (enabled)
    {
        g_DesiredBots.insert(guid.GetCounter());
        g_StopWatchElapsed.erase(guid.GetCounter());
    }
    else
    {
        g_DesiredBots.erase(guid.GetCounter());
        g_StopWatchElapsed[guid.GetCounter()] = 0;
        if (sRandomPlayerbotMgr.GetPlayerBot(guid))
            sRandomPlayerbotMgr.LogoutPlayerBot(guid);
    }
}

bool ResolveCharacter(ChatHandler* handler, std::string name, ObjectGuid& guid)
{
    if (!normalizePlayerName(name))
    {
        handler->PSendSysMessage("Invalid character name '{}'.", name);
        return false;
    }

    guid = sCharacterCache->GetCharacterGuidByName(name);
    if (!guid)
    {
        handler->PSendSysMessage("Character '{}' was not found.", name);
        return false;
    }

    return true;
}

void SendStatus(ChatHandler* handler, ObjectGuid guid)
{
    std::string name;
    sCharacterCache->GetCharacterNameByGuid(guid, name);
    bool desired = g_DesiredBots.contains(guid.GetCounter());
    Player* connected = FindConnectedCharacter(guid);
    char const* state = !connected ? "offline" : (IsPlayerbot(connected) ? "online as bot" : "online as player");

    handler->PSendSysMessage(
        "Autonomous alt {}: {}, {}, level {}.",
        name,
        desired ? "enabled" : "disabled",
        state,
        sCharacterCache->GetCharacterLevelByGuid(guid));
}

class AutonomousAltBotsWorldScript : public WorldScript
{
public:
    AutonomousAltBotsWorldScript()
        : WorldScript(
              "AutonomousAltBotsWorldScript",
              { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP, WORLDHOOK_ON_UPDATE })
    {
    }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
        LOG_INFO(
            "module",
            "mod-autonomous-altbots v{}: {} (startup delay={}s, check interval={}s, independent={})",
            MODULE_VERSION,
            g_Config.enabled ? "enabled" : "disabled",
            g_Config.startupDelayMs / IN_MILLISECONDS,
            g_Config.checkIntervalMs / IN_MILLISECONDS,
            g_Config.independent);
    }

    void OnStartup() override
    {
        LoadDesiredBots();
        g_StartupElapsed = 0;
        g_CheckElapsed = 0;
        g_StartupComplete = g_Config.startupDelayMs == 0;
    }

    void OnUpdate(uint32 diff) override
    {
        ProcessStopWatch(diff);

        if (!g_Config.enabled || !PlayerSettingsEnabled())
            return;

        if (!g_StartupComplete)
        {
            g_StartupElapsed += diff;
            if (g_StartupElapsed < g_Config.startupDelayMs)
                return;
            g_StartupComplete = true;
        }

        g_CheckElapsed += diff;
        if (g_CheckElapsed < g_Config.checkIntervalMs)
            return;
        g_CheckElapsed = 0;

        for (ObjectGuid::LowType lowGuid : g_DesiredBots)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(lowGuid);
            Player* connected = FindConnectedCharacter(guid);
            if (!connected)
            {
                sRandomPlayerbotMgr.AddPlayerBot(guid, 0);
                continue;
            }

            if (IsPlayerbot(connected))
            {
                ApplyAutonomousStrategies(connected);
            }
            else if (g_Config.verboseLogging)
            {
                LOG_DEBUG(
                    "module",
                    "mod-autonomous-altbots: {} is logged in as a real player; autonomous login deferred",
                    connected->GetName());
            }
        }
    }

private:
    void ProcessStopWatch(uint32 diff)
    {
        for (auto itr = g_StopWatchElapsed.begin(); itr != g_StopWatchElapsed.end();)
        {
            ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(itr->first);
            if (sRandomPlayerbotMgr.GetPlayerBot(guid))
            {
                sRandomPlayerbotMgr.LogoutPlayerBot(guid);
                itr->second = 0;
            }
            else
            {
                itr->second += diff;
            }

            if (itr->second >= STOP_WATCH_DURATION_MS)
                itr = g_StopWatchElapsed.erase(itr);
            else
                ++itr;
        }
    }
};

class AutonomousAltBotsCommandScript : public CommandScript
{
public:
    AutonomousAltBotsCommandScript() : CommandScript("AutonomousAltBotsCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable autonomousAltCommandTable =
        {
            { "enable", HandleEnable, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "disable", HandleDisable, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "start", HandleEnable, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "stop", HandleDisable, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "status", HandleStatus, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "list", HandleList, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
            { "reload", HandleReload, rbac::RBAC_PERM_COMMAND_GM, Console::Yes },
        };
        static ChatCommandTable commandTable =
        {
            { "autonomousalt", autonomousAltCommandTable },
        };
        return commandTable;
    }

    static bool HandleEnable(ChatHandler* handler, std::string name)
    {
        if (!PlayerSettingsEnabled())
        {
            handler->SendSysMessage(
                "Autonomous altbots require EnablePlayerSettings = 1 in worldserver.conf, followed by a server restart.");
            return false;
        }

        ObjectGuid guid;
        if (!ResolveCharacter(handler, name, guid))
            return false;

        uint32 accountId = sCharacterCache->GetCharacterAccountIdByGuid(guid);
        if (sPlayerbotAIConfig.IsInRandomAccountList(accountId) || sRandomPlayerbotMgr.IsAddclassBot(guid.GetCounter()))
        {
            handler->PSendSysMessage("{} belongs to a managed random-bot account and cannot be enrolled as an alt.", name);
            return false;
        }

        if (Player* connected = FindConnectedCharacter(guid); connected && !IsPlayerbot(connected))
        {
            handler->PSendSysMessage("{} is logged in as a player. Log that character out before enabling it.", name);
            return false;
        }

        SetDesired(guid, true);
        handler->PSendSysMessage("{} is enabled and will remain autonomously online.", name);
        return true;
    }

    static bool HandleDisable(ChatHandler* handler, std::string name)
    {
        ObjectGuid guid;
        if (!ResolveCharacter(handler, name, guid))
            return false;

        SetDesired(guid, false);
        handler->PSendSysMessage("{} is disabled and will remain offline after logout.", name);
        return true;
    }

    static bool HandleStatus(ChatHandler* handler, std::string name)
    {
        ObjectGuid guid;
        if (!ResolveCharacter(handler, name, guid))
            return false;

        SendStatus(handler, guid);
        return true;
    }

    static bool HandleList(ChatHandler* handler)
    {
        if (g_DesiredBots.empty())
        {
            handler->SendSysMessage("No autonomous alts are enabled.");
            return true;
        }

        handler->PSendSysMessage("Autonomous alts ({}):", g_DesiredBots.size());
        for (ObjectGuid::LowType lowGuid : g_DesiredBots)
            SendStatus(handler, ObjectGuid::Create<HighGuid::Player>(lowGuid));
        return true;
    }

    static bool HandleReload(ChatHandler* handler)
    {
        LoadDesiredBots();
        handler->PSendSysMessage("Reloaded {} autonomous alt selection(s).", g_DesiredBots.size());
        return true;
    }
};
} // namespace

void Addmod_autonomous_altbotsScripts()
{
    new AutonomousAltBotsWorldScript();
    new AutonomousAltBotsCommandScript();
}
