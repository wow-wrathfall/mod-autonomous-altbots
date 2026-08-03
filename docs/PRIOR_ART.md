# Prior-art and duplicate-project search

Search performed on 2026-08-03 across the AzerothCore module catalogue/web results, GitHub repository and code-oriented
queries, the current `mod-playerbots` repository/configuration/wiki, and the CMaNGOS Playerbots lineage.

## Findings

### Current AzerothCore Playerbots

The maintained [`mod-playerbots/mod-playerbots`](https://github.com/mod-playerbots/mod-playerbots) supports ordinary
altbots, including manual `.playerbots bot add` login and `AiPlayerbot.BotAutologin`, which logs account alts when their
owner logs in. Its own documentation distinguishes altbots from randombots: randombots roam and progress autonomously,
while altbots are owner-oriented.

No current configuration option, command, or separately indexed AzerothCore module was found that provides all of:

1. a persistent per-character allowlist;
2. login without an online owner;
3. full off-screen activity;
4. independent randombot-style quest/grind behaviour; and
5. explicit persistent start/stop commands.

Relevant upstream references:

- [mod-playerbots repository](https://github.com/mod-playerbots/mod-playerbots)
- [Playerbot Commands wiki](https://github.com/mod-playerbots/mod-playerbots/wiki/Playerbot-Commands)
- [playerbots.conf.dist](https://github.com/mod-playerbots/mod-playerbots/blob/master/conf/playerbots.conf.dist)

### CMaNGOS Playerbots

The separate [`cmangos/playerbots`](https://github.com/cmangos/playerbots) lineage contains an always-online/selfbot
concept exposed in some configurations as `ToggleAlwaysOnlineChars` and `ToggleAlwaysOnlineAccounts`. That is the closest
match found and should be credited as prior art.

It is not a drop-in AzerothCore module, uses a different core/module architecture, and the equivalent options are absent
from the current AzerothCore Playerbots source and active configuration reviewed for this release.

### Name collision

No repository or indexed code result was found using `mod-autonomous-altbots`, `mod_autonomous_altbots`, or the
`.autonomousalt` command namespace at the time of the search.

## Conclusion

The public project is reasonably distinct for the current AzerothCore Playerbots ecosystem, but its README intentionally
describes it as an AzerothCore port/extension of an established always-online Playerbots idea rather than a novel concept.
