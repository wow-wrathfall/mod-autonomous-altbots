# mod-autonomous-altbots

Persistent, independently active alt characters for the AzerothCore Playerbots branch.

The module lets an administrator select ordinary player-created characters and keep them logged in as autonomous
Playerbots. Selected characters continue questing, grinding, travelling, looting, equipping upgrades, and gaining normal
character progression without their owner being logged in or nearby.

## Status

Version **1.0.1** is the current public release. It has been compiled successfully against:

- `mod-playerbots/azerothcore-wotlk`, branch `Playerbot`
- `mod-playerbots/mod-playerbots`, branch `master`

The module depends on two small integration patches included in [`patches/`](patches/README.md). It is not compatible
with stock AzerothCore without the Playerbots fork and `mod-playerbots`.

## Features

- Per-character opt-in; it never takes over every character on an account.
- Persistent desired online/offline state across worldserver restarts.
- Full AI activity even with no real player online or nearby.
- Autonomous quest, grind, loot, travel, mount, and New RPG strategies.
- Keeps normal character gear, inventory, spells, quests, guild, position, and XP progression.
- Protects selected alts from random-bot guild assignment and fixed-level/no-XP handling.
- Rejects random-bot and add-class accounts to avoid lifecycle conflicts.
- Handles an in-flight login when an administrator disables a character.
- GM and console commands for starting, stopping, listing, inspecting, and reloading selections.

## Existing related functionality

This project fills a gap in the current AzerothCore port; it does not claim the general idea is new.

- Current AzerothCore `mod-playerbots` can log alt characters in manually and can auto-login account alts when their
  owner logs in. Those alts remain master-oriented and are not a persistent selected autonomous fleet.
- The CMaNGOS Playerbots lineage has an `always online`/`ToggleAlwaysOnlineChars` concept. That implementation is useful
  prior art, but it is not an installable AzerothCore module and is not present in the current AzerothCore Playerbots
  configuration or source paths.

See [`docs/PRIOR_ART.md`](docs/PRIOR_ART.md) for the search scope and links.

## Requirements

1. [AzerothCore Playerbots core fork](https://github.com/mod-playerbots/azerothcore-wotlk), `Playerbot` branch.
2. [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots), `master` branch.
3. Both integration patches supplied with this repository.
4. `EnablePlayerSettings = 1` in the active `worldserver.conf`.
5. `AiPlayerbot.DisabledWithoutRealPlayer = 0` in the active `playerbots.conf`.

The final setting prevents the random-bot holder from repeatedly logging out all masterless bots when the realm is empty.

## Installation

Clone this repository beside `mod-playerbots`:

```bash
cd /path/to/azerothcore-wotlk/modules
git clone https://github.com/Gutwormy/mod-autonomous-altbots.git
```

Apply the integration patches from the AzerothCore root:

```bash
bash modules/mod-autonomous-altbots/tools/install-patches.sh
```

PowerShell users can run:

```powershell
.\modules\mod-autonomous-altbots\tools\Install-Patches.ps1
```

Then set the two required configuration values, re-run CMake, and rebuild the worldserver. The build installs
`mod_autonomous_altbots.conf.dist`; copy it to `mod_autonomous_altbots.conf` when your deployment does not create active
module configs automatically.

Docker example:

```bash
docker compose build ac-worldserver
docker compose up -d
```

The patch installers support `--check`/`-CheckOnly` and `--reverse`/`-Reverse`. Read
[`patches/README.md`](patches/README.md) before upgrading either upstream repository.

## Commands

Commands require AzerothCore's GM command permission.

| Command | Effect |
|---|---|
| `.autoalt start <character>` | Persistently enable the character and log it in. |
| `.autoalt stop <character>` | Log the character out and keep it offline. |
| `.autoalt enable <character>` | Alias for `start`. |
| `.autoalt disable <character>` | Alias for `stop`. |
| `.autoalt status <character>` | Show desired and current online state. |
| `.autoalt list` | List every enabled autonomous alt. |
| `.autoalt reload` | Reload selections from `character_settings`. |

The original `.autonomousalt` namespace remains available as a backward-compatible alias. Use the complete `.autoalt`
name rather than a shorter abbreviation because AzerothCore also provides an `.autobroadcast` command.

A character logged in through the game client must log out before it can be enabled. An already running ordinary altbot
may be enabled; after its current master-owned session ends, the module logs it back in independently.

## Behaviour and resource use

An autonomous alt bypasses `BotActiveAlone` rotation and SmartScale. This is intentional: the selected character is fully
active even in an empty zone. Select a modest number and monitor world update time, CPU, and memory.

The current Playerbots random manager counts these sessions in its online holder. With a 1,500 random-bot target, enabling
five autonomous alts will normally settle near 1,495 random bots plus five autonomous alts rather than 1,505 total.

`AutonomousAltBots.Independent = 1` removes a real-player master and the `follow`/`stay` strategies. Set it to `0` if a
selected alt should resume ordinary master behaviour while grouped with a real player.

## Persistence and data safety

Only the desired enabled state is stored by this module, under source `mod_autonomous_altbots` in AzerothCore's existing
`character_settings` table. Character progression continues through the normal AzerothCore tables. The module does not
create, delete, reset, randomise, relevel, or regear characters.

Back up the character database before testing a new Playerbots/core revision.

## Compatibility and upgrades

The integration patches intentionally fail their `git apply --check` step when upstream code has moved. Do not force them
through a conflict: inspect and refresh the patches for the new revision. See [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md).

## Licence

GPL-2.0-or-later. See [`LICENSE`](LICENSE).

World of Warcraft is a trademark of Blizzard Entertainment. This project is not affiliated with or endorsed by Blizzard
Entertainment, AzerothCore, CMaNGOS, or the mod-playerbots maintainers.
