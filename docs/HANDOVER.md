# Xbox 360 Blades launcher — Codex handover

Prepared 30 October 2026. This document supersedes earlier progress summaries where they conflict with the current v5 scope. Inspect the actual source and latest artifact manifest before making changes.

## Objective and user preferences

Recreate the old Xbox 360 Blades experience as a convincing 2008 time capsule, on a Peer Pressure-modded Jasper. This is a standalone native launcher, with Aurora retained as the fallback. The user wants faithful graphics, transitions, sound, menus and Guide, real installed games and their own profile. They dislike a screenshot/slideshow imitation and want interactive behavior.

The user explicitly selected this **five-blade 16:9 walkthrough** as the main reference:
https://www.youtube.com/watch?v=5JPMeN1SkUE

Use the supplied reference folder and frame-by-frame footage to identify mismatches independently. Earlier 4:3 references may be adapted to 16:9 where needed, but do not mix different dashboard versions silently. The long-term target includes physical Xbox-button activation of the authentic Guide, including eventual in-game behavior; that routing has not been implemented.

Mostly offline operation is acceptable. Marketplace may honestly show offline. Desired time-capsule features include locally archived period ads, trailers, themes and store presentation. Direct ReLive service integration has no verified API in this project. Peer Pressure compatibility must be researched before proposing plugins; earlier research says DashLaunch plugins are unsupported.

The user prefers action, concise updates, and economical use of time/usage. Preserve their existing files, reference assets and console setup.

## Workspace and architecture

Repository/workspace: `/Users/diego/Desktop/Blades`.

The Mac preview and Xbox executable share a 1280×720 software renderer in C. The Xbox adapter uploads the framebuffer as a texture and presents through xsdk. This is not an XDK project.

- `blades/src/app.c`: navigation, page hierarchy, blade transitions, My Games, profile cards and Guide-background cache.
- `blades/src/library.c`, `library.h`: portable, read-only GOD discovery and shared catalog model.
- `blades/src/draw.c`: software drawing and exact lookup-table dimming.
- `blades/src/font.c`: embedded Segoe font, cached atlases rasterized at actual display sizes.
- `blades/src/guide.c`: local Guide state, input isolation, page hierarchy, slide animation and cached panel rendering.
- `blades/src/reference_assets.c`, `reference_asset_data.c`, `asset_codec.c`: lossless reference components and decoding.
- `blades/src/guide_art_data.c`: generated Guide surfaces and traced scanline edges.
- `blades/src/stick.h`: shared analog-stick dead zone/hysteresis and dominant-axis conversion.
- `blades/platform/mac/main.m`: native Cocoa preview, keyboard/controller input, capture mode and sound.
- `blades/platform/xsdk/main.c`: Xbox initialization, profile/time polling, GOD scan, input, rendering and timing logs.
- `blades/platform/xsdk/screen.vert`, `screen_layout.h`: framebuffer shader and matching vertex declaration.
- `blades/platform/xsdk/ui_audio_xsdk.c`: threaded audio feeder.
- `blades/platform/xsdk/boot_probe.c`: separate minimal blue/green GPU/presentation diagnostic.

Toolchain: `/Users/diego/xsdk-gcc`; xsdk source/library under `blades-project/research/xsdk`. CMake configuration is in `blades/CMakeLists.txt`; local build cache is `blades/build/xsdk`.

The workspace originally arrived with project files untracked. No clean committed baseline was established. Do not discard files merely because Git calls them untracked. Check current status before editing; no commit or remote publication is implied.

## Current artifact

Latest prepared build: **v5 — read-only GOD discovery and signed-in dashboard profile**.

- Main executable: `/Users/diego/Desktop/Blades/blades/build/xsdk/blades.xex`
- USB-ready copy: `/Users/diego/Desktop/Blades/blades/build/usb-boot-test/blades.xex`
- Manifest: `blades/build/xsdk/artifact.json`
- Size: **15,286,272 bytes**
- Header: `XEX2`
- SHA-256: `d955546f652462c4defc0469811da82e9e252c3178b806ec25c088faa8eb8c10`
- **This v5 artifact has not been tested on the console.**

Any new rebuild supersedes these values. Refresh the manifest and USB-ready copy before handing the user another XEX. Keep builds clearly identified to prevent testing stale USB copies.

## What is actually verified

### User-reported console results

The initial USB launch returned immediately to Aurora. The user supplied this diagnostic:

```text
shader: the vertex layout lacks an input the shader fetches
blades: setup failed: 0xC000000D
exit: status 1
```

Cause: the shader used GLSL location 1 for UV, which this SDK maps to COLOR0; the runtime supplied TEXCOORD0. Changing UV to location 2 fixed that mismatch.

After receiving the corrected v2, the user confirmed **it launches on the Jasper**. They reported good left/right blade movement and a working Start-operated local Guide, with slight animation lag. They later supplied six photographs and reported rough Xbox animation and broken-looking text. The exact hashes of the versions in those photographs were not supplied.

The photos are preserved in `References/hardware/console-feedback-1.jpg` through `console-feedback-6.jpg`. They expose uneven text and clear rectangular patches around icons/background replacements. Static photos do not measure frame rate.

No user confirmation has been received for the latest v4 font/performance fixes or v5 real catalog/profile-card changes. Actual Aurora exit destination, real system-Guide coexistence, sound quality and frame rate remain incompletely verified.

### Local checks completed

- Mac preview/app builds and Xbox XEX packaging pass.
- ASan/UBSan navigation tests pass: blade/page hierarchy, selection, repeats, Guide hierarchy/confirmation, frozen transitions, input isolation, analog conversion, simulated system-UI takeover/release.
- Font regression checks pass: identical glyph pixels after translating text one screen pixel at 27/31/37/38px. The preserved old renderer fails this check.
- Synthetic GOD parser tests pass: metadata/name/path extraction, truncated package rejection and incorrect content type rejection.
- Installed-catalog tests pass: scrolling beyond eight entries, empty filters and unreadable-HDD UI.
- Compiled Xbox vertex shader inputs match the actual runtime layout.
- Raw Mac captures were visually inspected; they are not proof of Xbox rendering/performance.

The SDK post-link scanner still reports three libc `rldic` instructions Xenia skips. No emulator pass is claimed. Do not confuse this warning with the resolved shader-layout failure.

## Implemented interaction and limits

Five blades: Marketplace, Xbox Live, Games, Media and System. Implemented pages include Games Library/My Games, Console Settings/Display/Audio, Media Center information and local Guide subpages. Many actions still show unavailable/offline notices.

Mac: arrows navigate, Return selects, Escape backs out, Tab or G toggles Guide. Controller: D-pad and now left stick navigate, A selects, B goes back, Start toggles local Guide, X/Y provide Guide shortcuts. Back ends the Xbox title outside system UI; its return destination needs confirmation. Stick engages at 12000 and releases below 8000 on signed Xbox axes; strongest axis wins. Mac normalizes its stick into that range.

### My Games in v5

The user confirmed games are **GOD format**, in the normal HDD Content tree:

```text
Content/0000000000000000/<8-digit title ID>/00007000/<package>
```

Xbox startup scans `hdd:/Content/0000000000000000` once. The SDK maps `hdd:` to the internal HDD device. It reads package headers only; validates signature magic, declared header size, content type 0x7000 and title ID; reads the English UTF-16BE display name; and sorts up to 256 discovered packages. It skips `.data` directories. It does not read game data fragments, modify packages, mount them or validate licenses/full integrity.

My Games uses real detected names/counts and eight visible rows with scrolling. GOD entries appear under All Games. Arcade/Demos/Recently Downloaded are currently empty rather than using invented metadata. Game artwork is a generic controller icon. Non-ASCII names currently show question marks because the font renderer is ASCII-only. Multi-disc packages are separate entries; there is no deduplication or disc grouping.

The Mac still uses eight sample reference games by default. Shared tests inject a synthetic installed catalog. A populated Mac sample list is not evidence of Xbox scanning.

**GOD launching is NOT implemented.** Selecting a discovered game shows an explicit message to launch through Aurora. Package discovery must not be presented as full Aurora integration. The supported mounting/launch/handoff API and return path are still unknown and require investigation.

Scan logs look like:

```text
blades: GOD scan hdd:/Content/0000000000000000 returned <count>
```

`-1` means the root could not be opened; `0` means no accepted headers were found. Some nested read failures currently just skip candidates. The scan is synchronous before first GPU setup; large libraries may delay startup. A later pass should make scan progress/errors clearer without adding frame-loop disk access.

Package metadata reference: https://github.com/xenia-project/xenia/blob/master/src/xenia/vfs/devices/stfs_xbox.h

### Profile

Existing xsdk signed-in-name polling supplies the Guide. v5 forwards the same gamertag to the Live/Games/Media home cards, replacing the captured demonstration name/count. It does not create, recover or sign in profiles and does not retrieve gamerscore.

**Actual gamerpic retrieval is NOT implemented.** Neutral profile icons are used, including in the Guide. Do not call the placeholder the user's gamerpic. Need a verified way to read the selected user's image and decode/render it on PowerPC. Do not guess XAM ordinals, calling conventions, image formats or profile-package paths.

### Guide

The local signed-out-layout Guide has focus/repeat, X/Y shortcuts, unavailable account/music/settings panes, and dashboard confirmation defaulting to No. Confirming Yes returns to this launcher's Games home; it does not launch Aurora. Signed-in menu variants are unfinished.

**The physical Xbox button remains XAM-owned.** The adapter detects system UI, dismisses our overlay, continues presenting and blocks application input until system UI closes and controls are released. This logic has only simulated local test coverage; actual coexistence needs console confirmation. A global custom Guide across running games is a separate unfinished system integration.

## Performance/font changes to retain

v3 added stick input, frozen dashboard caching during Guide animation and an exact per-channel dimming lookup table.

v4 discovered the Xbox CMake build type was empty and the application had been compiling without optimization. CMake now defaults an unset single-config build to Release; actual current compile rules contain `-O3 -DNDEBUG`. Do not inadvertently revert to an unoptimized build.

v4 replaced resizing of one 40px font atlas with cached atlases rasterized at actual display sizes and direct texel sampling. Common sizes are warmed at initialization. The embedded font remains the supplied Segoe substitute, not a verified original Convection font.

v4 also caches the Guide panel itself, invalidating for page/focus/profile/clock changes, so animation copies curved scanline spans instead of rerasterizing all text each frame. The whole framebuffer is still uploaded each frame. The original animated plasma is not implemented.

Xbox logs aggregate timing every 300 frames:

```text
blades timings us: frame avg=... max=... CPU=... begin-wait=... upload=... (300 frames)
```

Values are microseconds. These measure the CPU loop and API waits/upload, not physical screen refresh or end-to-end input latency. No post-fix console measurements have been received. The periodic log write itself is outside those measurements.

## Reference files and fidelity gaps

- `References/widescreen/source.json`: canonical video frame times, dimensions and scaling provenance.
- `References/widescreen/*.png`: extracted 1280×720 home/page references.
- `References/widescreen/guide/`: main/empty/sign-in/confirmation/error reference frames and source manifest.
- `work/reference-audit/widescreen-5JPMeN1SkUE.mp4`: locally downloaded 1080p60 reference video.
- `work/reference-audit/widescreen-audio.webm`: reference audio.
- `blades-frames/`: older 4:3 frame sequence, historical rather than canonical.
- `References/games tab.png`: earlier single Games screenshot, also historical.
- `blades/tools/bake_widescreen.py`, `bake_guide.py`, `bake_audio.py`: generators. Inspect before regenerating; they may overwrite large generated assets or font data.
- `reference/Default-0.7b.2/Fonts/segoeui_r.ttf`: current embedded font source.

Five substantial visual issues remain: (1) substitute typography and text layout, (2) opaque rectangular icon/background patches, (3) approximate selected-row relief/curvature and focus animation, (4) static rather than animated glass/plasma, and (5) incomplete frame-accurate blade/Guide geometry, easing and fades. Signed-in surfaces, other subpages and sound onset/isolation also need work. These are current observations, not a reconstruction of the truncated earlier conversation's numbered list.

The Games reference at 26.7s catches a focus transition. Compare settled focus against later footage as well, such as 27.5s. A local 390×50 selected-row comparison improved slightly after reversing the highlight gradient and rounding its right edge (mean channel error 42.918 → 41.878 against the 27.5s frame); this is not a whole-screen fidelity score.

Older reported mean error **9.7**, **9.6% far-off pixels**, and **~1px glass-edge alignment** relate to the earlier `games tab.png` reconstruction. They are historical reported metrics, not validation of the current five-blade video-based build. Do not reuse them as current accuracy claims or claim “100% faithful.”

## Build and test commands

These commands have been run successfully in this workspace, from the project root:

```sh
make -C blades -j2 check app
/opt/homebrew/bin/cmake --build blades/build/xsdk -j2
make -C blades check-xsdk-layout
```

`check` includes navigation, native font and GOD discovery checks with sanitizers. `check-xsdk-layout` requires the generated Xbox shader header, so run it after the Xbox build. `app` produces `blades/build/Blades.app`; `preview` builds the executable without bundling.

Capture example:

```sh
blades/build/blades-preview --dump /tmp/blades-games.ppm games
```

Other existing states include `market`, `live`, `media`, `system`, `library`, `my-games`, `second-game`, `console`, `display`, `audio`, `guide`, `guide-mid`, `guide-signin`, `guide-confirm`, `guide-info`, `guide-game`. Interactive clocks are real; Guide captures use 4:08 AM for repeatability.

The existing CMake cache already points at the toolchain. A fresh-machine setup/install procedure has not been verified here; consult `docs/toolchain.md` and the SDK before inventing commands. Actual GOD-launch and gamerpic-read commands/APIs are unknown.

Preserved local artifacts include `blades/build/boot-debug/console-booted-v2.xex`, earlier failed/diagnostic binaries and v3/v4 backups. The v2 label denotes the local candidate associated with the successful report, not a console-verified transferred SHA.

## Immediate next steps

1. Have the user test **v5**: Games → Games Library → My Games, compare count/names with Aurora, scroll beyond eight games, and check the home gamertag. Collect the GOD scan log line. Fix path/header assumptions from evidence.
2. Obtain `blades timings us` lines while repeatedly switching blades and opening/closing Guide, plus the user's assessment of v4/v5 text. Use those measurements to target remaining CPU/upload/presentation cost.
3. Implement actual selected-user gamerpic retrieval and native image decoding with verified SDK/XAM interfaces. Keep a clear fallback when unavailable; test account changes and signed-out state.
4. Implement GOD game mounting/launching or a documented Aurora handoff, then prove one known game launches and returns correctly. Do not assume `XamLoaderLaunchTitle` accepts a GOD container as though it were a loose XEX.
5. Continue reference comparison, beginning with the obvious square icon/background patches and native typography. Keep fidelity work independent from claims about console-service integration.

## Console safety and verification

Keep Aurora as the existing startup dashboard. Launch manually from Aurora/USB. Do not replace the startup executable or change Peer Pressure/boot settings. Do not mutate or reorganize the user's Content tree. No external network service is required for discovery or the offline UI.

SDK logging tries beside the XEX (`game:\\`) first, then HDD/USB roots and cache. Look for `blades.log`. Logs may have unused zero-filled trailing space. When investigating a failure, record the tested artifact hash/version and last readable lines. Missing logs alone do not prove loader rejection.

Remaining hardware checks include: stick drift/repeat, rapid direction reversals, Guide latency, physical Xbox/system-Guide takeover and release, sound, screen clipping, Back's actual return destination, real library scan, real gamerpic, and eventual launch/return. A build pass is not a hardware pass.

## Ready-to-use kickoff prompt

> Continue the standalone Xbox 360 Blades launcher in `/Users/diego/Desktop/Blades`. Read `docs/HANDOVER.md`, the latest artifact manifest, and relevant source before changing anything. The user has a Peer Pressure Jasper and keeps Aurora as startup/fallback. The canonical visual reference is the five-blade 16:9 walkthrough `https://www.youtube.com/watch?v=5JPMeN1SkUE`, with local frames/video. Console boot and basic navigation/Guide were reported working on an earlier build; latest v5 is locally built/tested but unverified on hardware. v5 discovers GOD headers under HDD `Content/0000000000000000/<titleID>/00007000`, displays a scrolling My Games catalog and real gamertag on home cards. Actual gamerpic, game artwork, GOD mounting/launching and Xbox-button routing are unfinished. Preserve Release optimization, native-size font atlases, Guide caches, analog input and timing logs. Start with any new console log/results supplied, then implement the next missing integration using verified interfaces. Do not invent launch support, original-font accuracy, tested commands or hardware results. Preserve Aurora boot settings and all reference/user files. Keep updates concise and carry out the authorized work; no sub-agents were requested.

Related detailed notes: `docs/god-library-profile.md`, `font-performance-followup.md`, `analog-guide-followup.md`, `boot-investigation.md`, `guide-verification.md`. Some older sections in README/progress notes retain historical claims; use source, current manifest and this handover to resolve them.
