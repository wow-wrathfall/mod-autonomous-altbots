# Compatibility

## Supported software

`mod-autonomous-altbots` targets only the actively maintained AzerothCore Playerbots fork and its matching
`mod-playerbots` module. It does not target stock AzerothCore, the retired `azerothcore/mod-playerbots` repository,
CMaNGOS, TrinityCore, or NPCBots.

The exact v1.0.0 baselines are recorded in [`patches/README.md`](../patches/README.md).

## Upgrade procedure

1. Stop the worldserver and back up the character database.
2. Pull both upstream repositories.
3. Run the patch installer with `--check` or `-CheckOnly`.
4. If the check fails, do not force the patch. Refresh it against the new source.
5. Reconfigure CMake and compile with warnings enabled.
6. Start with no selected autonomous alts and inspect startup logs.
7. Test one disposable character through start, progression, stop, and restart persistence.
8. Re-enable production characters after the smoke test.

## Configuration interactions

- `EnablePlayerSettings` must be enabled before login data is loaded.
- `AiPlayerbot.DisabledWithoutRealPlayer` must be disabled to avoid empty-realm logout/relogin churn.
- `BotActiveAlone` and SmartScale do not throttle selected autonomous alts.
- Selected alts are normal account characters, not random bots, and are outside random level-bracket reset selection.
- Autonomous sessions count in `RandomPlayerbotMgr`'s online holder and can reduce the number of random sessions at the
  configured cap.

## Known limitations

- The module requires source patches until equivalent upstream lifecycle hooks exist.
- It does not guarantee that every quest is solvable by Playerbots AI.
- It does not add offline simulation; characters are genuine logged-in Playerbot sessions consuming normal server CPU.
- It does not automatically train, repair, or sell beyond behaviour already supplied by the enabled Playerbots strategies
  and other installed modules.
