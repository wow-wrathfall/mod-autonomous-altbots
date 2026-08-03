# Design notes

## State model

The desired state is a single per-character flag in AzerothCore's existing `character_settings` table:

- source: `mod_autonomous_altbots`
- index `0`: `1` enabled, `0` disabled

The in-memory GUID set is rebuilt at world startup and can be refreshed with `.autonomousalt reload`.

## Login lifecycle

Every check interval, an enabled offline character is passed to `RandomPlayerbotMgr::AddPlayerBot(guid, 0)`. Using a zero
master account creates a genuine masterless Playerbot session. The module then restores the configured autonomous
strategies and, in independent mode, clears any real-player master.

The module does not duplicate character state or simulate progression in SQL. AzerothCore saves the live character using
its ordinary persistence path.

## Logout lifecycle

Disabling a character removes its GUID from the desired set, persists `0`, and requests an ordinary Playerbots logout.
A 30-second stop watch catches an asynchronous login that was already in flight when the command was issued.

## Why integration patches are required

Masterless normal-account characters enter the random manager's holder, but they are not members of its random-account
fleet. Without the patch, they can be treated as real players for activity bookkeeping and can receive random-manager
guild/XP handling. The Playerbots activity scheduler also has no module hook for a per-character force-active decision.

The patch is deliberately narrow and keyed off the same persistent setting as the module.
