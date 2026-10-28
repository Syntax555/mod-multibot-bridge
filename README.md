# MultiBot Bridge for Playerbots Portable

Server-side support for the
[MultiBot Chatless fork](https://github.com/Syntax555/MultiBot-Chatless),
[Questie bot progress](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/questie-playerbots.md)
and [Zygor Individual Progression](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/zygor-progression.md)
in [AzerothCore Playerbots Portable](https://github.com/Syntax555/mod-playerbots-portable).
The module exchanges bounded `MBOT` addon messages and validates requests against
server-side character, group and Playerbots state.

## Install

This is a **compiled server module**, with no standalone addon or module ZIP.
Use the portable [Latest server bundle](https://github.com/Syntax555/mod-playerbots-portable/releases/latest)
or its [source-build workflow](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/building.md).
The portable [manifest](https://github.com/Syntax555/mod-playerbots-portable/blob/main/versions.lock.json)
pins this fork with the compatible core, Playerbots, Individual Progression,
Era Talents and client addon revisions. A source change requires rebuilding and
installing the worldserver.

For the UI, close WoW and install `MultiBot-Chatless-latest.zip` from
[the addon fork's Latest release](https://github.com/Syntax555/MultiBot-Chatless/releases/latest)
so `Interface/AddOns/MultiBot/MultiBot.toc` exists. Use client 3.3.5a, build 12340.
Historical talents additionally need the server release's matching
`EraTalents-client-latest.zip`, including its addon and `Data/patch-V.mpq`.
Optional Questie and Zygor packages come from their maintained forks. See
[client addon installation](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/addons.md).
Client updates are installed manually with WoW closed.

## Supported behavior

The bridge supplies rosters, lifecycle state, strategy and role controls,
spellbooks, inventory and equipment, quest interactions, trainers, professions,
banking and group orders. Endpoint availability does not override the portable
realm's earned-play rules:

- Bots train, repair, buy and bank through ordinary nearby NPC interactions,
  using their own money and items. Class training follows their own earned era
  and talent/quest prerequisites.
- Natural progression refuses generated autogear, free maintenance and preset
  or bulk talent application. Respecs use paid trainers. For historical trees,
  the matching EraTalents addon lets owners manually spend grouped altbots'
  earned points within normal inspect range.
- Requests reuse server authorization and native Playerbots restrictions.
  Summon, resurrection, movement and pet controls do not authorize free grants
  or travel bypasses. Humans and SelfBot characters retain human control rights;
  group membership alone does not turn a connected player into a bot.
- Combat profiles can be automatic or chosen explicitly without spending talent
  points; see [profiles and owned equipment](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/combat-profiles.md).

The module does not expose an arbitrary remote Playerbots command executor.
Some companion-addon features retain narrow compatibility chat handling; manual
Playerbots chat commands remain available subject to server restrictions.

## Read-only quest and progression services

Questie uses `QUEST_PROGRESS_V1` and `QUEST_PROGRESS_PUSH_V1` to display eligible
group bots' quest state and objective counters. The server rejects real-player
and SelfBot targets and checks group membership and Playerbots security.
Snapshots carry the request token and authoritative bot GUID; Questie's strict
GUID consumer requires this bridge support and a matching addon. Losing access
or removing a bot clears the watched view. These reads never accept, complete
or reward quests.

Zygor uses `IP_PROGRESS_V1` for the requesting character's personal progression,
quest availability and instance access. `IP_TARGET_V1` also permits on-demand
reads for an owned, grouped altbot in inspect range. These answers do not copy
another character's tier or grant access. Watches track the sender's own state;
bot-target reads are requested on demand.

Token, target, field and packet limits are checked by the server. The addon
must match responses to the active request and current subject. Quest and
instance answers describe eligibility checks; they do not guarantee that an
NPC, route, scripted objective or encounter works. Full in-game coverage remains
[open validation work](https://github.com/Syntax555/mod-playerbots-portable/blob/main/docs/todo.md#gameplay-validation).

## Configuration and troubleshooting

`conf/MultiBotBridge.conf.dist` contains one module option:

```ini
MultiBotBridge.EnableConsoleLogs = 0
```

The portable installation places it at `configs/modules/MultiBotBridge.conf`.
Set it to `1` for bridge startup/RX/TX diagnostics. Gameplay rules are configured
in Playerbots, Individual Progression and Era Talents; preserve the portable
profiles when updating.

If the addon reports the bridge unavailable, confirm that the running
worldserver was built with this module and that the compatible MultiBot addon
is enabled. For refused actions, check control rights, group/range, earned era,
trainer requirements, owned money/items and the returned reason. Include the
server revision, addon version, relevant configuration and reproduction in a
[problem report](https://github.com/Syntax555/mod-playerbots-portable/issues).

## Sources and contributing

Work on `main` under [AGENTS.md](AGENTS.md) and the portable
[contributing guide](https://github.com/Syntax555/mod-playerbots-portable/blob/main/CONTRIBUTING.md).
The **Portable integration** workflow tests this fork against the assembly's
pinned dependencies; complete native builds run in the portable repository.
Source fixtures do not certify every UI interaction or gameplay path.

This fork retains
[Wishmaster117/mod-multibot-bridge](https://github.com/Wishmaster117/mod-multibot-bridge)
at [`ebfa01a129dd`](https://github.com/Wishmaster117/mod-multibot-bridge/commit/ebfa01a129dd4c441b2527e42dcc37b6da95e6c2).
The [imported README](https://github.com/Wishmaster117/mod-multibot-bridge/blob/ebfa01a129dd4c441b2527e42dcc37b6da95e6c2/README.md)
provides the original project's broader protocol and development background,
including credits to AzerothCore, Playerbots and historical Jellypowered bridge
contributions. Its upstream feature and runtime claims do not define this
fork's earned-play behavior.

The imported revision has no explicit repository license declaration. Preserve
its authorship and notices; the portable project's MIT license does not license
this dependency. See [third-party notices](https://github.com/Syntax555/mod-playerbots-portable/blob/main/THIRD_PARTY_NOTICES.md).
