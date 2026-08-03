# Integration patches

The AzerothCore module API does not currently expose two lifecycle decisions needed by this project: forcing full
Playerbots activity for a selected character, and distinguishing that masterless selected alt from a real player inside
`RandomPlayerbotMgr`. The module also needs one prepared statement to enumerate enabled per-character settings.

The release therefore carries two narrow patches:

- `azerothcore-playerbot.patch`
  - adds a prepared `character_settings` query by source;
  - adds a non-mutating player-setting lookup.
- `mod-playerbots.patch`
  - forces full AI activity only for marked autonomous alts;
  - skips random guild/fixed-level processing for them;
  - prevents them from being added to the random manager's real-player list.

## Supported baseline for v1.0.0

- AzerothCore Playerbots fork: `ceeb3116ebedf4b35f12b75e5481b6ddd0de7a89` (`Playerbot`)
- mod-playerbots: `3fa1c1e49f8f1324b72461e576bce7c89b0a6521` (`master`)

Nearby revisions may apply cleanly. Always run the installer in check mode after pulling upstream.

## Check, apply, or reverse

From the AzerothCore root:

```bash
bash modules/mod-autonomous-altbots/tools/install-patches.sh --check
bash modules/mod-autonomous-altbots/tools/install-patches.sh
bash modules/mod-autonomous-altbots/tools/install-patches.sh --reverse
```

PowerShell equivalents:

```powershell
.\modules\mod-autonomous-altbots\tools\Install-Patches.ps1 -CheckOnly
.\modules\mod-autonomous-altbots\tools\Install-Patches.ps1
.\modules\mod-autonomous-altbots\tools\Install-Patches.ps1 -Reverse
```

Before reversing, remove or move this module out of the core's `modules` directory. Otherwise its references to the added
prepared statement will intentionally fail the next build.

Never use `git apply --reject` or force a partially applied patch. Refresh the patch against the new upstream revision and
run a full compile.
