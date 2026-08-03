# Changelog

All notable changes to this project are documented here. The format follows Keep a Changelog and the project uses
Semantic Versioning.

## [1.0.1] - 2026-08-03

### Fixed

- Corrected the clean-build workflow's AzerothCore `APPS_BUILD` value from `worldserver` to `world-only`.
- Updated the official checkout and compiler-cache actions to their current Node.js 24 releases.

## [1.0.0] - 2026-08-03

### Added

- Persistent per-character autonomous-alt selection.
- Masterless login and automatic relogin after worldserver restarts.
- GM/console start, stop, enable, disable, status, list, and reload commands.
- Quest, grind, New RPG, and independent-mode strategy management.
- Full off-screen activity override for selected alts.
- Protection from random-bot guild assignment and fixed-level XP flags.
- In-flight login cancellation guard.
- Versioned AzerothCore and mod-playerbots integration patches.
- Cross-platform patch installers and GitHub Actions compile workflow.
