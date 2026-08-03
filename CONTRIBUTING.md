# Contributing

Bug reports and compatibility updates are welcome. Include the exact commits of both the Playerbots AzerothCore fork and
`mod-playerbots`, relevant configuration, reproduction steps, and the smallest useful log excerpt.

Before opening a pull request:

1. Run both patch installers in check mode against a clean upstream checkout.
2. Build the worldserver with warnings enabled.
3. Verify start, stop, restart persistence, owner logout, and empty-realm behaviour.
4. Run `git diff --check` and keep C++ formatting consistent with AzerothCore.
5. Update `CHANGELOG.md` when behaviour or compatibility changes.

Do not commit client files, extracted map data, database dumps, credentials, active server configuration, or copyrighted
game assets.
