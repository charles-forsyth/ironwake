# Chapter 13: Build, test and tooling

Scope: how Ironwake is built (editor target, packaged Mac app), every script under `build/tools/` (one row each: purpose, arguments, key facts), every `-iw*` command-line flag and trigger file the C++ reads (with file:line), the test directors and their modes (`-iws2`, `-iww2`, `-iwp8`, `-iwp9`, `-iwp11`, `-iwp12`, `-iwa1`, `-iwe1`, `-iwi1`, `-iww1`, `-iwh1`, P1/S1 trigger files), the offscreen / `-nosound` / CrashReportClient rules from `build/BRIEF.md`, the editor Python scripts, the Ursa cluster scripts in `~/Projects/game-concepts/ironwake/cluster/`, the Kanban workflow and a summary of the BRIEF rules. Nothing here was executed: no editor or game was launched for this chapter. Citation: `value` [src path:line]. Paths are relative to `[REDACTED]/Projects/unreal/IronwakeTest` unless absolute.

## 13.1 Environment

| Item | Value | Source |
|---|---|---|
| Engine association | `5.8` [src IronwakeTest.uproject:3] | IronwakeTest.uproject |
| Engine install | `[REDACTED]/Epic Games/UE_5.8` | `[REDACTED]/Epic Games/UE_5.8` [src build/tools/package.sh:7] |
| Engine version / hardware (BRIEF) | UE `5.8.4`, Apple M5 Mac, Metal, 32 GB | `UE 5.8.4` [src build/BRIEF.md:19], `Apple M5 Mac, Metal, 32 GB` [src build/BRIEF.md:20] |
| Game default map | `/Game/Maps/Ironwake_Sea` | `GameDefaultMap=/Game/Maps/Ironwake_Sea.Ironwake_Sea` [src Config/DefaultEngine.ini:32] |
| Editor startup map | same map | `EditorStartupMap=/Game/Maps/Ironwake_Sea.Ironwake_Sea` [src Config/DefaultEngine.ini:33] |
| C++ module | `IronwakeTest`; unity builds **off** (files share anonymous-namespace names) | `bUseUnity = false` [src Source/IronwakeTest/IronwakeTest.Build.cs:8] |
| Module dependencies | Core, CoreUObject, Engine, InputCore, EnhancedInput, Water, Niagara, UMG, Slate, SlateCore, ProceduralMeshComponent | `PublicDependencyModuleNames` [src Source/IronwakeTest/IronwakeTest.Build.cs:9] |
| Packaged bundle ids seen | `com.omstead.ironwake` (P5 and later builds) and `com.YourCompany.IronwakeTest` (older builds / default) | `com.omstead.ironwake` [src build/README_P5.txt:1], `com.YourCompany.IronwakeTest` [src build/tools/h1openbench.sh:5] |
| Live editor MCP | Unreal MCP server at `http://127.0.0.1:8001/mcp` (server name `unreal-ironwake`); [REDACTED] uses port `8000` and must never be touched | `http://127.0.0.1:8001/mcp` [src build/BRIEF.md:21], `port 8000` [src build/BRIEF.md:41] |
| Standalone test MCP ports | `8011` (p9/p10/p11/p12/p3/p7/s2/fxshot/p5 scripts), `8018` (a1), `8019` (h1, r1, w1, w2), `8021` (i1) | `8011` [src build/tools/p11run.sh:7], `8018` [src build/tools/a1run.sh:11], `8019` [src build/tools/h1run.sh:9], `8021` [src build/tools/i1run.sh:12] |
| Headless build port | `8009` (once held by a stale CrashReportClient) | `port 8009` [src build/BRIEF.md:63] |

## 13.2 Build commands

| What | Command (verbatim essentials) | Source |
|---|---|---|
| Editor build (C++) | `"$UE/Engine/Build/BatchFiles/Mac/Build.sh" IronwakeTestEditor Mac Development -Project=$P/IronwakeTest.uproject > Saved/s1build.log` | `Build.sh` [src build/tools/s1build.sh:13] |
| Fix the editor module manifest | after a build, newest `libUnrealEditor-IronwakeTest-*.dylib` is patched into `Binaries/Mac/UnrealEditor.modules` with `sed -i ''` | `UnrealEditor.modules` [src build/tools/s1build.sh:19] |
| Package (cook + stage) | `RunUAT.sh BuildCookRun -project=... -platform=Mac -clientconfig=Development -build -cook -stage -pak -iostore -unattended -utf8output -nop4 -nocompileeditor -skipbuildeditor > Saved/package.log` ; editor must be closed | `BuildCookRun` [src build/tools/package.sh:8], `-skipbuildeditor` [src build/tools/package.sh:9] |
| Package output | `Saved/StagedBuilds/Mac/IronwakeTest.app` | `Saved/StagedBuilds/Mac/IronwakeTest.app` [src build/tools/package.sh:5] |
| Promotion | the staged `.app` is copied with `ditto` to `~/Games/Ironwake/Ironwake.app` (or `Ironwake_<card>.app`); `-archive` produced a bare binary that crashes on a missing `libtbb.12.dylib` | `ditto` [src build/logs/D.md:88] |
| Headless editor Python | `IWTEST_SCRIPT=<module> UnrealEditor <uproject> -unattended -nullrhi -abslog=...` (`Content/Python/init_unreal.py` imports `$IWTEST_SCRIPT`) | `-nullrhi` [src build/tools/pyrun.sh:7], `IWTEST_SCRIPT` [src Content/Python/init_unreal.py:2] |
| Standalone game run from the editor build | `UnrealEditor <uproject> /Game/Maps/Ironwake_Sea -game -windowed -ResX= -ResY= -iwnomenu ...` | `-game -windowed` [src build/tools/s2run.sh:12] |
| Live editor relaunch (BRIEF) | `UnrealEditor IronwakeTest.uproject /Game/Maps/Ironwake_Sea -log`, background mode, never nohup/disown; delete `Saved/Autosaves/*` first after an unclean exit ("Restore Packages" dialog blocks MCP) | `Restore Packages` [src build/BRIEF.md:85], `rm -rf ~/Projects/unreal/IronwakeTest/Saved/Autosaves/*` [src build/BRIEF.md:86] |
| Only one IronwakeTest editor at a time | two instances fight over the same files | `Only one IronwakeTest` [src build/BRIEF.md:38] |
| Packaged-app launch (benchmarks) | `open -n <app> --args ...` (launching the binary directly gets frame-cap sleeps throttled by macOS, capped runs read ~18 fps) | `open -n` [src build/tools/h1openbench.sh:7], `~18 fps` [src build/tools/h1openbench.sh:3] |

Stale launcher note: `s1build.sh` relaunches the editor with `nohup` [src build/tools/s1build.sh:22] which **conflicts** with the BRIEF rule "never nohup/disown" [src build/BRIEF.md:38]; the BRIEF is the later rule.

## 13.3 Offscreen, sound and crash-reporter rules (BRIEF)

| Rule | Text / value | Source |
|---|---|---|
| CrashReportClient trap (HARD RULE) | every script that launches UnrealEditor or a packaged Ironwake app must END with `pkill -x CrashReportClient 2>/dev/null; true`, also on failure/timeout (put it in a trap); before completing a card `pgrep -x CrashReportClient` must print nothing | `pkill -x CrashReportClient` [src build/BRIEF.md:4], `pgrep -x CrashReportClient` [src build/BRIEF.md:5] |
| Cleanup after every run | kill any `CrashReportClient` for IronwakeTest with `pkill -f 'CrashReportClient.*IronwakeTest'` and confirm none left (two stayed open 6-11 h) | `pkill -f 'CrashReportClient.*IronwakeTest'` [src build/BRIEF.md:63] |
| No focus stealing (HARD RULE, Chuck 2026-11-11 08:52) | use `-RenderOffScreen` for `-game` capture runs; if that breaks captures, batch every shot of a step into ONE launch, max one launch per ~20 min | `-RenderOffScreen` [src build/BRIEF.md:8], `max one launch per ~20 min` [src build/BRIEF.md:9] |
| Always `-nosound -unattended -nosplash` | offscreen runs still play audio he cannot dismiss; only a spectrum-recording audio check may run with sound and only when the orchestrator says Chuck is away | `-nosound` [src build/BRIEF.md:10], `Only an audio check` [src build/BRIEF.md:11] |
| Batch launches | "batch PIE/-game test runs (several checks per launch)" | `batch PIE/-game test runs` [src build/BRIEF.md:188] |
| No LLM/GUI side effects | tests strip input so stray typing cannot open the menu (`ClearAllMappings` once a second in the S2 director; same idea in W2) | `ClearAllMappings` [src Source/IronwakeTest/IronwakeS2Test.cpp:67], `Unattended: strip input` [src Source/IronwakeTest/IronwakeW2Test.cpp:63] |
| Never touch [REDACTED] | Chuck's own session, port `8000` | `NEVER touch` [src build/BRIEF.md:40] |

**CONFLICT (scripts vs rule):** only `p11run.sh` (and `p14run.sh`, which `exec`s it) pass `-nosound -unattended -nosplash -RenderOffScreen` [src build/tools/p11run.sh:7]; every other `-game` script (a1run, h1run, i1run, p10run, p12run, p9run, r1run, s2run, w1run, w2run, p3circle, p3fx, p5b/p5c, p7circle) predates the 08:52 rule and omits them [src build/tools/p9run.sh:6]; `e1openbench.sh` passes them [src build/tools/e1openbench.sh:7]. The BRIEF rule applies; add the flags (or use `p11run.sh`) for any new run. Only `p10run.sh`, `p11run.sh`, `p12run.sh` use an `EXIT` trap [src build/tools/p10run.sh:6]; the others `pkill` at the end of the script and a script killed by timeout skips that line.

## 13.4 `build/tools/` scripts (one row each)

Conventions: `P=$HOME/Projects/unreal/IronwakeTest`; `UE` = `UnrealEditor` inside `[REDACTED]/Epic Games/UE_5.8`; `gtimeout` bounds each run; logs go to `Saved/*.log` via `-abslog`.

### Run scripts (launch the editor build `-game` or the packaged app)

| Script | Purpose / args | Key facts | Source |
|---|---|---|---|
| `a1run.sh` | `a1run.sh <hold\|route\|tour\|fire> [extra]`: A1 autopilot director | `-iwnomenu -iwa1=$M -iwa1quit`, timeout `A1TIMEOUT:-1800` [src build/tools/a1run.sh:10] s, MCP port `8018`, log `Saved/a1_$M.log` copied to `build/logs/A1/` | build/tools/a1run.sh |
| `h1run.sh` | `h1run.sh [extra]`: H1 handling/camera test | `-iwh1 -iwh1quit`, `gtimeout 400`, port `8019`, log `Saved/h1test.log` | `gtimeout 400` [src build/tools/h1run.sh:8] |
| `i1run.sh` | `i1run.sh [mode=shots] [extra]`: I1 archipelago director (modes shots, bench) | `-iwi1=$M -iwi1quit`, timeout `I1TIMEOUT:-600` [src build/tools/i1run.sh:11], port `8021`; also `pkill -f "IronwakeTest.uproject.*-game"` first | build/tools/i1run.sh |
| `p10run.sh` | `p10run.sh <mode> [extra]`: P10 shots via the P8 director | `-iwp8=$M -iwp8quit -iwunlockall`, 1600x900, `TMO` default `600`, EXIT trap | `TMO:-600` [src build/tools/p10run.sh:7] |
| `p11run.sh` | `p11run.sh <mode> [ResX ResY] [extra]`: P11 director (day, wx, harbour, help; also tw, p14, bench) | the only run script with `-nosound -unattended -nosplash -RenderOffScreen`; port `8011`; log `Saved/p11_$M.log`; default 1600x900 | `RX=${2:-1600}` [src build/tools/p11run.sh:8] |
| `p12run.sh` | `p12run.sh <mode> [extra]`: P12 (shots, day, night, bench) | `-iwp12=$M -iwp12quit -iwunlockall`, EXIT trap | `-iwp12=$M` [src build/tools/p12run.sh:7] |
| `p14run.sh` | `p14run.sh <mode>`: P14 = `-iwp11` modes offscreen | `exec` of `p11run.sh` | `exec` [src build/tools/p14run.sh:3] |
| `p1run.sh` | `p1run.sh <mode> <wait_s> [keep]`: P1 loop test through the **live editor** | writes `Saved/iw_p1.txt`, StartPIE over MCP, greps IWP1/IWGM/IWAI/IWEND, StopPIE | `iw_p1.txt` [src build/tools/p1run.sh:6] |
| `p3circle.sh` | `p3circle.sh <hull>`: full-circle timing at flank + hard rudder | `-iws2=circle -iws2quit -iwship=$H -iwunlockall`, `gtimeout 400`, log `Saved/s2_circle_<hull>.log` | `gtimeout 400` [src build/tools/p3circle.sh:10] |
| `p3fx.sh` | `p3fx.sh <hull> <tag> [flags]`: hull-side photo at speed | `-iws2=fxshot`, `gtimeout 300`, flags `-iwnowake -iwnobow -iwnosheet -iwnospume`, shot `Saved/shots/P3fx_<hull>_<tag>.png` | `gtimeout 300` [src build/tools/p3fx.sh:9] |
| `p5b_ladder.sh` | 3 hulls (cutter, frigate, ironclad) x {`4`,`8`,`13`} kn x sea {`0.25`,`1.0`}: side + chase shots | `-iws2=p5b`, `gtimeout 300`, 1600x900, `-iwhold=70` | `for H in cutter frigate ironclad` [src build/tools/p5b_ladder.sh:8] |
| `p5c_count.sh` | slam counts for the cutter at 13 kn, sea `0.25`,`1.0`,`1.6` | `-iwcount`, `gtimeout 260`, 1280x720, counts `IWP5B slam` lines | `IWP5B slam` [src build/tools/p5c_count.sh:10] |
| `p5c_ladder.sh` | slam counts then the 18-cell ladder via `p5c_run.sh` | `-iwhold=60`, `gtimeout 260` | `p5c_run.sh` [src build/tools/p5c_ladder.sh:14] |
| `p5c_run.sh` | `p5c_run.sh <hull> <sea> <kn> [hold]`: one ladder cell | `gtimeout 200`, copies shots to `build/logs/P5c/shots/` | `gtimeout 200` [src build/tools/p5c_run.sh:9] |
| `p7bench.sh` | `p7bench.sh <suffix[:..]>...`: bench packaged apps `Ironwake_<suffix>.app` | bundle id `com.omstead.ironwake`, `FRAMES=600`, uses `e1openbench.sh` with `-iwtour` | `FRAMES=600` [src build/tools/p7bench.sh:3] |
| `p7circle.sh` | `p7circle.sh <hull> <fps>`: circle time at a fixed game delta | `-benchmark -fps=$F`, `gtimeout 500`, 1280x720 | `-benchmark -fps=$F` [src build/tools/p7circle.sh:10] |
| `p9run.sh` | `p9run.sh <hull> <mode> [extra]`: P9 collision director | `-iwp9=$M -iwp9quit -iwship=$H -iwunlockall`, `TMO` default `900`, 1280x720 | `TMO:-900` [src build/tools/p9run.sh:9] |
| `r1run.sh` | `r1run.sh [extra]`: R1 capture director | `-iwr1`, `gtimeout 300`, deletes `Saved/shots/R_*.png` first; `IWX` env adds ExecCmds | `gtimeout 300` [src build/tools/r1run.sh:9] |
| `s2run.sh` | `s2run.sh <mode> [extra]`: S2 director (modes in 13.6) | `-iws2=$MODE -iws2quit`, **no gtimeout** (runs until the director quits), kills older `-game` runs first | `-iws2=$MODE -iws2quit` [src build/tools/s2run.sh:12] |
| `s2batch.sh` | `s2batch.sh mode[:ship] ...`: runs `s2run.sh` per spec (default ship frigate), summary `Saved/s2batch.txt` | `gtimeout 1200` per mode | `gtimeout 1200` [src build/tools/s2batch.sh:7] |
| `w1run.sh` | `w1run.sh [extra]`: W1 test director | `-iww1 -iww1quit`, `W1TIMEOUT` default `700`, port `8019` | `W1TIMEOUT:-700` [src build/tools/w1run.sh:8] |
| `w2run.sh` | `w2run.sh [mode=all] [extra]`: W2 storm/sea director | `-iww2=$M -iww2quit`, `W2TIMEOUT` default `600` | `W2TIMEOUT:-600` [src build/tools/w2run.sh:9] |
| `s1run.sh` | `s1run.sh <mode> <max_wait_s> [keep]`: S1 voyage test through the live editor | writes `Saved/iw_s1.txt`, PIE over MCP, waits for `IWS1 tour done` | `iw_s1.txt` [src build/tools/s1run.sh:6] |
| `s1build.sh` | `s1build.sh [nolaunch]`: stop editor, build editor target, fix modules, relaunch on MCP `8001`, wait up to `120*4` s for MCP | see 13.2 | `ModelContextProtocolPort=8001` [src build/tools/s1build.sh:22] |
| `package.sh` | cook + stage Mac Development build | see 13.2; prints `package rc=` | `package rc=` [src build/tools/package.sh:12] |
| `pyrun.sh` | `pyrun.sh <module> [timeout=500]`: headless editor Python with `-nullrhi` | log `Saved/py_<module>.log`, greps `IWP10|LogPython: Error|Traceback|Fatal` | `${2:-500}` [src build/tools/pyrun.sh:7] |
| `h1bench.sh` | `h1bench.sh <app> <label> "<ExecCmds>" [resx resy frames]`: bench a packaged binary directly, `-iwtour`, csv profile | defaults 1600x900, `450` frames; env `IWFRONT` brings the app to front via osascript | `N=${6:-450}` [src build/tools/h1bench.sh:4] |
| `h1openbench.sh` | `h1openbench.sh <label> [args]`: like above via `open -n` (correct capped fps) | `-csvCaptureFrames=450`, kills the app by pattern, copies csv to `build/logs/$BENCHDIR/` | `csvCaptureFrames=450` [src build/tools/h1openbench.sh:7] |
| `r1openbench.sh` | copy of `h1openbench.sh` (header still says h1) that also matches `Ironwake_v1.app` | `pkill -f "Ironwake(_v1)?.app/..."` | `Ironwake(_v1)?.app` [src build/tools/r1openbench.sh:10] |
| `e1openbench.sh` | `e1openbench.sh <label> [args]`: no automatic `-iwtour`; `FRAMES`, `IWID`, `IWAPP`, `BENCHDIR` env | adds `-nosound -unattended -nosplash -RenderOffScreen` | `-nosound -unattended -nosplash -RenderOffScreen` [src build/tools/e1openbench.sh:7] |

### Analysis and helper scripts

| Script | Purpose / args | Source |
|---|---|---|
| `csvstat.py` | `csvstat.py <csvprofile.csv[.gz]> [skip=60]`: medians/p90 of FrameTime, GameThread, RenderThread, RHIThread, GPUTime, median FPS and the top `25` GPU/* columns above `0.05` ms (UE csv header is at the END) | `skip = int(sys.argv[2]) if len(sys.argv) > 2 else 60` [src build/tools/csvstat.py:6] |
| `csvfps.py` | `csvfps.py <csv>...`: avg fps, 1 % low fps, mean GPU ms, first `120` rows skipped | `[120:]` [src build/tools/csvfps.py:7] |
| `luma.sh` | `luma.sh <png...>`: mean luma and spread via ffmpeg signalstats | `signalstats` [src build/tools/luma.sh:4] |
| `px.py` | `px.py img.png x y ...`: mean RGB of a `40x20` box via ffmpeg (no PIL) | `crop=40:20` [src build/tools/px.py:7] |
| `whitestat.py` | `whitestat.py <png...>`: rows with > `50` % near-white pixels and near-white fraction (pure-Python PNG decode) | `0.5` [src build/tools/whitestat.py:26] |
| `gaugeread.py` | `gaugeread.py [project dir]`: reads `Saved/w2gauge/g*.png` against `IWW2G` log rows to compare rendered vs query water height at the gauge post | `IWW2G` [src build/tools/gaugeread.py:6] |
| `w4cov.py` | `w4cov.py <shots dir>`: foam coverage from `W4C_*_m7.png` (foam mask) vs `_m8` (no foam); foam = luma diff `> 25`; needs PIL + numpy | `> 25` [src build/tools/w4cov.py:12] |
| `look.sh` | `look.sh <tag>`: two viewport captures (`cap.py`) at (80000, 60000, 1000) pitch `-2` and `20`, composite with ffmpeg | `80000 60000 1000` [src build/tools/look.sh:4] |
| `cloudtry.sh` | `cloudtry.sh out.png '<scalar json>' '<vector json>'`: set `MI_StormDeck2` parameters and swap the volumetric cloud material over MCP | `MI_StormDeck2` [src build/tools/cloudtry.sh:5] |
| `spec_check.py` | verifies cited values in `docs/spec/[0-9]*.md` against sources: tag `` `value` [src path:line] ``, value within +/-`2` lines (case-insensitive literal, also tolerates trailing `f`, commas, spaces, trailing zeros); flags `--sample N --seed S --all --files f... --fails`; exit 1 on any failure | `Usage: spec_check.py` [src build/tools/spec_check.py:7] |

### Editor-control helpers (talk to the live editor)

| Script | Purpose / args | Source |
|---|---|---|
| `mcp.py` | tiny MCP client: `list`, `describe <toolset>`, `call <toolset> <tool> '<json>' [out.png]`; URL from env `UE_MCP` (default `http://127.0.0.1:8001/mcp`), timeout `600` s; `MAXC` env truncates output (default `20000`) | `UE_MCP` [src build/tools/mcp.py:3] |
| `uepy.py` | run a Python file inside the live editor via PythonScriptPlugin remote execution; only targets the node whose `project_name` is `IronwakeTest`, never [REDACTED] | `project_name` [src build/tools/uepy.py:14] |
| `script.py` | `script.py file.py`: run a ProgrammaticToolset script (`run()`) over MCP | `execute_tool_script` [src build/tools/script.py:4] |
| `cap.py` | `cap.py out.png x y z pitch yaw`: MCP `CaptureViewport` to a PNG (UI hidden) | `CaptureViewport` [src build/tools/cap.py:7] |
| `capasset.py` | `capasset.py out.png [asset]`: MCP `CaptureAssetImage` thumbnail (default `/Game/Ironwake/Ships/Ironclad/SM_Ironclad`) | `CaptureAssetImage` [src build/tools/capasset.py:4] |
| `inv.py` | actor inventory of the open level (name, label, class, location) through `find_actors` | `find_actors` [src build/tools/inv.py:6] |
| `props.py` | `props.py <refPath> [list\|get a,b]`: list/get object properties via MCP | `list_properties` [src build/tools/props.py:6] |
| `setp.py` | `setp.py <refPath> '<json>'`: `set_properties` via MCP | `set_properties` [src build/tools/setp.py:3] |
| `keys.py` | `keys.py <refPath>...`: print sorted property names of objects | `list_properties` [src build/tools/keys.py:4] |
| `nset.py` | Niagara helper library over MCP (`ref`, `call`, `F/I/B/V2/V3/C` value builders, `setin`) imported by the fx scripts | `NiagaraToolsets.NiagaraToolset_System` [src build/tools/nset.py:3] |
| `ndump.py` | `ndump.py <system> <emitter>`: visible module inputs with compact values | `GetEmitterTopology` [src build/tools/ndump.py:6] |
| `niag.py` | `niag.py <systemPath> [verbose]`: dump all emitters' module inputs | `GetSystemSummary` [src build/tools/niag.py:6] |
| `fxsetup.py` | sets Niagara parameters of `NS_IW_Splash`, `NS_IW_Explosion2`, `NS_IW_Fire`, `NS_IW_Smoke` (e.g. splash burst `260` particles) and saves them | `NS_IW_Splash` [src build/tools/fxsetup.py:46] |
| `p5b_smoke.py`, `p5c_smoke.py` | retune `NS_IW_FunnelSmoke` (P5b: spawn rate `9`; P5c: rate `14`, lifetime `4`-`7`) | `SpawnRate` [src build/tools/p5b_smoke.py:6], `SpawnRate` [src build/tools/p5c_smoke.py:5] |
| `w3fx.py` | W3 retune of `NS_IW_Rain` (`24000` /s) and `NS_IW_Spume` | `24000.0` [src build/tools/w3fx.py:14] |
| `w4fx.py` | W4 soft spray: `NS_IW_Spume` and `NS_IW_BowSheet` material and spawn retune; run `env -u PYTHONPATH python3 build/tools/w4fx.py [spume\|all]` | `M_IW_Spray` [src build/tools/w4fx.py:8] |
| `alltools.txt` | 830-line text list of every MCP tool name (`Toolset.Class.Tool`), a reference dump, not a script | `EditorToolset.EditorAppToolset.StartPIE` [src build/tools/alltools.txt:12] |

Other `build/` items: `build/watch_cards.sh` (13.9), `build/p10_*.py` (Blender helpers for the NASA star map: crop, flip, probe, sim; they read `build/assets/p10/starmap_2020_8k.exr`) [src build/p10_crop.py:2], and the asset-prep folders `build/r1`, `build/e1`, `build/p8` (asset regeneration, chapter 12).

All `env -u PYTHONPATH python3 ...` invocations unset `PYTHONPATH` so the editor's Python stub path does not leak into system Python [src build/tools/p1run.sh:8].

## 13.5 Command-line flags (`-iw*`) and trigger files

Parsing is by `FParse::Param` (bare switch) or `FParse::Value` (`name=value`) on `FCommandLine::Get()`. Each row: flag, where it is read, effect. "Director" flags only arm a test; the director then runs from `Voyage.Tick`.

### Mode / flow

| Flag | Read at | Effect | Source |
|---|---|---|---|
| `-iwnomenu` | GameMode StartPlay | skip the title menu (automation) | `iwnomenu` [src Source/IronwakeTest/IronwakeGameMode.cpp:47] |
| `-iwtour` | GameMode, LivingSea | start the cinematic tour (also via `Saved/iw_tour.txt`); LivingSea appends `quit` so the app exits at the end | `iwtour` [src Source/IronwakeTest/IronwakeGameMode.cpp:48], `iwtour` [src Source/IronwakeTest/IronwakeLivingSea.cpp:358] |
| `-iwtourshots` | LivingSea | tour saves screenshots | `iwtourshots` [src Source/IronwakeTest/IronwakeLivingSea.cpp:359] |
| `-iwcredits` | PlayerController BeginPlay | title menu with credits card, screenshot `R_credits`, quit after `11` s | `iwcredits` [src Source/IronwakeTest/IronwakePlayerController.cpp:685] |
| `-iwstorm` | Voyage / GameMode | start with the storm at sea (no enemy) | `iwstorm` [src Source/IronwakeTest/IronwakeVoyage.cpp:331], `iwstorm` [src Source/IronwakeTest/IronwakeGameMode.cpp:51] |
| `-iwlegacywx` | Voyage | use the old preset weather instead of the living clock/weather (`StartLiving` skipped) | `iwlegacywx` [src Source/IronwakeTest/IronwakeVoyage.cpp:343] |
| `-iwship=<id>` | Fleet `GetSelected` | force the ship (cutter, frigate, ironclad...); also the player's saved choice fallback | `iwship=` [src Source/IronwakeTest/IronwakeFleet.cpp:144] |
| `-iwunlockall` | Fleet `IsUnlocked` | every roster ship unlocked | `iwunlockall` [src Source/IronwakeTest/IronwakeFleet.cpp:164] |
| `-iwgfx=N` | Settings `GetGraphics` | override saved graphics level (0..2) for the run | `iwgfx=` [src Source/IronwakeTest/IronwakeSettings.cpp:54] |
| `-iwdebug` | Voyage HUD | adds preset and sea-state debug text to the status chips | `iwdebug` [src Source/IronwakeTest/IronwakeVoyage.cpp:1288] |
| `?iwrestart`, `?iwpeaceful=1` (URL options, not switches) | GameMode StartPlay | restart skips the title menu and keeps the mode | `iwrestart` [src Source/IronwakeTest/IronwakeGameMode.cpp:58], `iwpeaceful` [src Source/IronwakeTest/IronwakeGameMode.cpp:58] |

### Test-director arming

| Flag | Read at | Modes (13.6) / effect | Source |
|---|---|---|---|
| `-iws2=<mode>` / `-iws2quit` | Voyage / S2Test | S2 director; no enemy; `progress` mode makes the voyage tick while paused | `iws2=` [src Source/IronwakeTest/IronwakeVoyage.cpp:332], `iws2quit` [src Source/IronwakeTest/IronwakeS2Test.cpp:70] |
| `-iww2=<mode>` / `-iww2quit` | Voyage / W2Test | W2 storm director; no enemy | `iww2=` [src Source/IronwakeTest/IronwakeVoyage.cpp:330], `iww2quit` [src Source/IronwakeTest/IronwakeW2Test.cpp:61] |
| `-iwp8=<mode>` / `-iwp8quit` | Voyage / P8 | sky/town shot director | `iwp8=` [src Source/IronwakeTest/IronwakeVoyage.cpp:326], `iwp8quit` [src Source/IronwakeTest/IronwakeP8.cpp:380] |
| `-iwp9=<mode>` / `-iwp9quit` | Voyage / P9Test | collision director | `iwp9=` [src Source/IronwakeTest/IronwakeVoyage.cpp:325], `iwp9quit` [src Source/IronwakeTest/IronwakeP9Test.cpp:34] |
| `-iwp11=<mode>` / `-iwp11quit` | Voyage / P11 | living-world shot director | `iwp11=` [src Source/IronwakeTest/IronwakeVoyage.cpp:341], `iwp11quit` [src Source/IronwakeTest/IronwakeP11.cpp:506] |
| `-iwp12=<mode>` / `-iwp12quit` | Voyage / P12 | town camera director | `iwp12=` [src Source/IronwakeTest/IronwakeVoyage.cpp:335], `iwp12quit` [src Source/IronwakeTest/IronwakeP12.cpp:1031] |
| `-iwa1=<mode>` / `-iwa1quit` | Voyage, GameMode / Autopilot | autopilot director; every mode except `fire` is peaceful | `iwa1=` [src Source/IronwakeTest/IronwakeVoyage.cpp:324], `iwa1quit` [src Source/IronwakeTest/IronwakeAutopilot.cpp:899], `!= TEXT("fire")` [src Source/IronwakeTest/IronwakeGameMode.cpp:58] |
| `-iwe1=<mode>` / `-iwe1quit` | Voyage | E1 world-asset shot director (modes incl. `bench`, `bench_island`) | `iwe1=` [src Source/IronwakeTest/IronwakeVoyage.cpp:328], `iwe1quit` [src Source/IronwakeTest/IronwakeVoyage.cpp:1588] |
| `-iwi1=<mode>` / `-iwi1quit` | Archipelago | I1 islands director (`shots`, `bench`) | `iwi1=` [src Source/IronwakeTest/IronwakeArchipelago.cpp:257], `iwi1quit` [src Source/IronwakeTest/IronwakeArchipelago.cpp:1294] |
| `-iww1`, `-iww1clip`, `-iww1quit` | GameMode, ShipPawn, ShipHydro | W1 handling/waves director (`clip` = 300-frame clip at `-benchmark -fps=30`); peaceful | `iww1` [src Source/IronwakeTest/IronwakeGameMode.cpp:50], `iww1clip` [src Source/IronwakeTest/IronwakeShipPawn.cpp:248], `iww1quit` [src Source/IronwakeTest/IronwakeShipHydro.cpp:434] |
| `-iwh1`, `-iwh1quit` | ShipPawn | H1 handling/camera test (also `Saved/iw_h1.txt`) | `iwh1` [src Source/IronwakeTest/IronwakeShipPawn.cpp:255], `iwh1quit` [src Source/IronwakeTest/IronwakeShipPawn.cpp:259] |
| `-iwr1` | LivingSea | R1 capture director (hides on-screen messages) | `iwr1` [src Source/IronwakeTest/IronwakeLivingSea.cpp:367] |

### Test parameters

| Flag | Read at | Effect | Source |
|---|---|---|---|
| `-iwkn=<kn>` | S2Test | target speed for `p5b`/`p5cwide` (default `8`) | `iwkn=` [src Source/IronwakeTest/IronwakeS2Test.cpp:481] |
| `-iwsea=<scale>` | S2Test | sea scale (default `0.25` p5b, `0.9` p5cwide) | `iwsea=` [src Source/IronwakeTest/IronwakeS2Test.cpp:481] |
| `-iwhold=<s>` | S2Test | hold time (default `50`) | `iwhold=` [src Source/IronwakeTest/IronwakeS2Test.cpp:482] |
| `-iwtag=<tag>` | S2Test, W2Test | shot-name tag | `iwtag=` [src Source/IronwakeTest/IronwakeS2Test.cpp:470], `iwtag=` [src Source/IronwakeTest/IronwakeW2Test.cpp:218] |
| `-iwcount` | S2Test, ShipHydro | log slam statistics | `iwcount` [src Source/IronwakeTest/IronwakeS2Test.cpp:483], `iwcount` [src Source/IronwakeTest/IronwakeShipHydro.cpp:206] |
| `-iwfast` | S2Test | `SpeedCompression = 6` in the fxshot run | `iwfast` [src Source/IronwakeTest/IronwakeS2Test.cpp:459] |
| `-iwslow` | S2Test | warm-up cruise `6` kn instead of `8` | `iwslow` [src Source/IronwakeTest/IronwakeS2Test.cpp:586] |
| `-iwturn` | S2Test | p5cwide adds a turn | `iwturn` [src Source/IronwakeTest/IronwakeS2Test.cpp:527] |
| `-iwgaugeat=x,y`, `-iwgaugestate=<s>` | W2Test | gauge location and sea state (default `4`) | `iwgaugeat=` [src Source/IronwakeTest/IronwakeW2Test.cpp:107], `iwgaugestate=` [src Source/IronwakeTest/IronwakeW2Test.cpp:108] |
| `-iwi1seaoff=<cm>` | Archipelago | sea offset for island placement | `iwi1seaoff=` [src Source/IronwakeTest/IronwakeArchipelago.cpp:217] |

### Time, weather, sky (living world)

| Flag | Read at | Effect | Source |
|---|---|---|---|
| `-iwdaymin=<min>` | P11 `StartLiving` | real minutes per game day (min `0.5`; default 48 per P11 log) | `iwdaymin=` [src Source/IronwakeTest/IronwakeP11.cpp:85] |
| `-iwhour=<h>` | P11 | start clock hour | `iwhour=` [src Source/IronwakeTest/IronwakeP11.cpp:86] |
| `-iwskyhour=<h>` | P11, P8 | fixed hour and clock paused (`ClockSpeedIdx = 2`); P8 dome holds the hour | `iwskyhour=` [src Source/IronwakeTest/IronwakeP11.cpp:87], `iwskyhour=` [src Source/IronwakeTest/IronwakeP8.cpp:51] |
| `-iwwx=<0-5>` | P11 | start weather state (clamped; default fair `1`) | `iwwx=` [src Source/IronwakeTest/IronwakeP11.cpp:88] |
| `-iwnodrift` | P11 | weather drift off | `iwnodrift` [src Source/IronwakeTest/IronwakeP11.cpp:90] |
| `-iwday=a,b,c` | P11 | P14 noon-lift triple (sky gain default `2.2`, sun `1.2`, bias `0.5`) | `iwday=` [src Source/IronwakeTest/IronwakeP11.cpp:191] |
| `-iwtintdawn=r,g,b`, `-iwtintdusk=r,g,b` | P11 | atmosphere tints at twilight | `iwtintdawn=` [src Source/IronwakeTest/IronwakeP11.cpp:226], `iwtintdusk=` [src Source/IronwakeTest/IronwakeP11.cpp:227] |
| `-iwozone=<x>` | P11 | ozone absorption multiplier (default `3`) | `iwozone=` [src Source/IronwakeTest/IronwakeP11.cpp:228] |
| `-iwtw=a,b,c,d,e` | P11 | twilight grade constants (5 values) | `iwtw=` [src Source/IronwakeTest/IronwakeP11.cpp:236] |
| `-iwmoon=<x>` | P11 | max moonlight intensity (default `4`) | `iwmoon=` [src Source/IronwakeTest/IronwakeP11.cpp:268] |
| `-iwmoonnoshadow` | P11 | moon light casts no shadows | `iwmoonnoshadow` [src Source/IronwakeTest/IronwakeP11.cpp:265] |
| `-iwwater=<x>` | P11 | night-water gain | `iwwater=` [src Source/IronwakeTest/IronwakeP11.cpp:277] |
| `-iwev=<ev>` | Voyage | clear-night exposure (default `0.4`) | `iwev=` [src Source/IronwakeTest/IronwakeVoyage.cpp:801] |
| `-iwnolamps` | Voyage | night lamps off | `iwnolamps` [src Source/IronwakeTest/IronwakeVoyage.cpp:828] |
| `-iwskyday=<0-364>`, `-iwskyrate=`, `-iwstargain=`, `-iwmoongain=`, `-iwptth=`, `-iwskygamma=`, `-iwptgain=`, `-iwglow=`, `-iwglowmip=` | P8 night sky | sky day, clock rate, star/moon gain, point threshold, gamma, point gain, glow gain/mip | `iwskyday=` [src Source/IronwakeTest/IronwakeP8.cpp:50], `iwskyrate=` [src Source/IronwakeTest/IronwakeP8.cpp:52], `iwstargain=` [src Source/IronwakeTest/IronwakeP8.cpp:53], `iwmoongain=` [src Source/IronwakeTest/IronwakeP8.cpp:54], `iwptth=` [src Source/IronwakeTest/IronwakeP8.cpp:55], `iwskygamma=` [src Source/IronwakeTest/IronwakeP8.cpp:56], `iwptgain=` [src Source/IronwakeTest/IronwakeP8.cpp:57], `iwglow=` [src Source/IronwakeTest/IronwakeP8.cpp:58], `iwglowmip=` [src Source/IronwakeTest/IronwakeP8.cpp:59] |
| `-iwskypo=`, `-iwskyyo=` | P8 test | pitch / yaw offset of the sky aim | `iwskypo=` [src Source/IronwakeTest/IronwakeP8.cpp:345], `iwskyyo=` [src Source/IronwakeTest/IronwakeP8.cpp:345] |

### Sea / FX / collision debugging

| Flag | Read at | Effect | Source |
|---|---|---|---|
| `-iwnobow` / `-iwnosheet` | ShipHydro | disable bow wave / spray sheets and slams | `iwnobow` [src Source/IronwakeTest/IronwakeShipHydro.cpp:159], `iwnosheet` [src Source/IronwakeTest/IronwakeShipHydro.cpp:159] |
| `-iwnowake` | ShipPawn | disable the wake | `iwnowake` [src Source/IronwakeTest/IronwakeShipPawn.cpp:860] |
| `-iwnospume` | Storm | disable spume | `iwnospume` [src Source/IronwakeTest/IronwakeStorm.cpp:206] |
| `-iwsmokedbg` | ShipHydro | log funnel-smoke state every `3` s | `iwsmokedbg` [src Source/IronwakeTest/IronwakeShipHydro.cpp:166] |
| `-iwwakedbg` | Hydro | wake debug log every `5` s | `iwwakedbg` [src Source/IronwakeTest/IronwakeHydro.cpp:673] |
| `-iwcovk=`, `-iwfoamdbg=`, `-iwstreakk=`, `-iwfoammode=`, `-iwlacek=`, `-iwplanebias=` | Hydro | whitecap coverage, whitecap, streak, debug mode, lace, plane-bias multipliers (default `1`, mode `0`, bias `0`) | `iwcovk=` [src Source/IronwakeTest/IronwakeHydro.cpp:114], `iwfoamdbg=` [src Source/IronwakeTest/IronwakeHydro.cpp:181], `iwstreakk=` [src Source/IronwakeTest/IronwakeHydro.cpp:183], `iwfoammode=` [src Source/IronwakeTest/IronwakeHydro.cpp:186], `iwlacek=` [src Source/IronwakeTest/IronwakeHydro.cpp:190], `iwplanebias=` [src Source/IronwakeTest/IronwakeHydro.cpp:197] |
| `-iwnocoll` | ShipCollision | disable ship collision | `iwnocoll` [src Source/IronwakeTest/IronwakeShipCollision.cpp:20] |
| `-iwground=<k>` | P12 | harbour ground tint factor (default `3.0`) | `iwground=` [src Source/IronwakeTest/IronwakeP12.cpp:50] |
| `-iwnoveg` | P12 | skip town vegetation | `iwnoveg` [src Source/IronwakeTest/IronwakeP12.cpp:282] |

Count: 81 distinct `-iw*` switches plus the `?iwrestart`/`?iwpeaceful` URL options. Counted by listing every `TEXT("iw...")` string passed to `FParse`/`Contains` in `Source/IronwakeTest/` (`grep -rno 'TEXT("-\?iw[a-z0-9_]*=\?")'`).

### Trigger files in `Saved/` (read then deleted unless noted)

| File | Read at | Effect | Source |
|---|---|---|---|
| `iw_tour.txt` | GameMode, LivingSea | start the tour; content = args (`quit`, `shots`) | `iw_tour.txt` [src Source/IronwakeTest/IronwakeGameMode.cpp:48], `iw_tour.txt` [src Source/IronwakeTest/IronwakeLivingSea.cpp:350] |
| `iw_autodemo.txt` | GameMode, ShipPawn | scripted voyage; first line is the shot prefix; counts as automation (no title menu) | `iw_autodemo.txt` [src Source/IronwakeTest/IronwakeShipPawn.cpp:277] |
| `iw_combatdemo.txt` | GameMode, Combat | combat demo; counts as automation | `iw_combatdemo.txt` [src Source/IronwakeTest/IronwakeCombat.cpp:158] |
| `iw_p1.txt` | PlayerController | P1 mode `idle`, `title`, `sink`, `sinkmenu`, `afterrestart`, `fight`; deleted after read | `iw_p1.txt` [src Source/IronwakeTest/IronwakePlayerController.cpp:693] |
| `iw_s1.txt` | Voyage | S1 test mode (e.g. `look`, `progress`); deleted after read | `iw_s1.txt` [src Source/IronwakeTest/IronwakeVoyage.cpp:314] |
| `iw_h1.txt` | ShipPawn | arms the H1 test; deleted | `iw_h1.txt` [src Source/IronwakeTest/IronwakeShipPawn.cpp:255] |

## 13.6 Test directors and modes

All run only when armed by their flag; each logs a tagged line family (grep prefix) and, with the matching `*quit` flag, calls `QuitGame` at the end. Screenshots go to `Saved/shots/` through `IWShot`.

| Director | Flag / script | Modes (file:line of the mode test) | Log prefix |
|---|---|---|---|
| S2 (ships, sailing, FX) | `-iws2=` / `s2run.sh` | `sail` `sail` [src Source/IronwakeTest/IronwakeS2Test.cpp:98], `water` `water` [src Source/IronwakeTest/IronwakeS2Test.cpp:170], `zoom` `zoom` [src Source/IronwakeTest/IronwakeS2Test.cpp:270], `beat` `beat` [src Source/IronwakeTest/IronwakeS2Test.cpp:303], `handling` `handling` [src Source/IronwakeTest/IronwakeS2Test.cpp:345], `circle` `circle` [src Source/IronwakeTest/IronwakeS2Test.cpp:401], `fxshot` `fxshot` [src Source/IronwakeTest/IronwakeS2Test.cpp:449], `p5b` `p5b` [src Source/IronwakeTest/IronwakeS2Test.cpp:476], `p5cwide` `p5cwide` [src Source/IronwakeTest/IronwakeS2Test.cpp:523], `clip` `clip` [src Source/IronwakeTest/IronwakeS2Test.cpp:555], `progress` `progress` [src Source/IronwakeTest/IronwakeS2Test.cpp:652] | `IWS2`, `IWP3`, `IWP4`, `IWP5B` |
| W2 (storm sea) | `-iww2=` / `w2run.sh` | `gauge` `gauge` [src Source/IronwakeTest/IronwakeW2Test.cpp:101], `bench`/`clip` `bench` [src Source/IronwakeTest/IronwakeW2Test.cpp:147], `layers` `layers` [src Source/IronwakeTest/IronwakeW2Test.cpp:170], `discs` `discs` [src Source/IronwakeTest/IronwakeW2Test.cpp:213], `w4cov` `w4cov` [src Source/IronwakeTest/IronwakeW2Test.cpp:225], `w3`/`w3storm` `w3storm` [src Source/IronwakeTest/IronwakeW2Test.cpp:255], `storm`/`quick` `quick` [src Source/IronwakeTest/IronwakeW2Test.cpp:329] | `IWW2`, `IWW3` |
| P9 (collision) | `-iwp9=` / `p9run.sh <hull> <mode>` | `all` or one of `beach`, `stack`, `breakwater`, `moored`, `ship`, `autopilot` `beach` [src Source/IronwakeTest/IronwakeP9Test.cpp:37] | `IWP9` |
| P8 (sky/town shots) | `-iwp8=` / `p10run.sh` | `sky` `sky` [src Source/IronwakeTest/IronwakeP8.cpp:314], `skyq` `skyq` [src Source/IronwakeTest/IronwakeP8.cpp:315], `town` `town` [src Source/IronwakeTest/IronwakeP8.cpp:316], `bench` `bench` [src Source/IronwakeTest/IronwakeP8.cpp:317] | `IWP8`, `IWP10` |
| P11 (living world) | `-iwp11=` / `p11run.sh`, `p14run.sh` | `day` (5 shots dawn..small hours), `wx` (6 weather states), `harbour` (4), `help` (hint and help panel), `tw` (4 twilight shots), `p14` (7 shots), `bench`; selection at `P11Mode == TEXT("p14")` [src Source/IronwakeTest/IronwakeP11.cpp:453] | `IWP11`, `IWP13` |
| P12 (town cameras) | `-iwp12=` / `p12run.sh` | `shots`, `day`, `night`, `bench` `iwp12=shots|day|night` [src Source/IronwakeTest/IronwakeP12.cpp:1031], `day` [src Source/IronwakeTest/IronwakeP12.cpp:1031], `night` [src Source/IronwakeTest/IronwakeP12.cpp:1031], `bench` [src Source/IronwakeTest/IronwakeP12.cpp:1111] | `IWP12` |
| A1 (autopilot) | `-iwa1=` / `a1run.sh` | `hold` `hold` [src Source/IronwakeTest/IronwakeAutopilot.cpp:912], `route` `route` [src Source/IronwakeTest/IronwakeAutopilot.cpp:867], `voyage` `voyage` [src Source/IronwakeTest/IronwakeAutopilot.cpp:1059], `tour` `tour` [src Source/IronwakeTest/IronwakeAutopilot.cpp:1034], `waterline` `waterline` [src Source/IronwakeTest/IronwakeAutopilot.cpp:1155], `fire` `fire` [src Source/IronwakeTest/IronwakeAutopilot.cpp:1195] | `IWA1` |
| E1 (world assets) | `-iwe1=` | `bench`, `bench_island` `bench_island` [src Source/IronwakeTest/IronwakeVoyage.cpp:1630] and shot modes | `IWE1` |
| I1 (islands) | `-iwi1=` / `i1run.sh` | `shots`, `bench` (modes beginning `bench` hold the `3+ islands` view) `bench` [src Source/IronwakeTest/IronwakeArchipelago.cpp:1152] | `IWI1` |
| W1 (handling/waves) | `-iww1` / `w1run.sh` | stages 0-9 (rest draft, short chop, long swell, settle, FULL AHEAD, bow shots); `-iww1clip` = clip mode | `IWW1` |
| H1 (handling/camera) | `-iwh1` / `h1run.sh` | scripted handling + camera captures | `IWH1` |
| R1 (ship captures) | `-iwr1` / `r1run.sh` | capture director | `IWR1` |
| P1 (loop) | `iw_p1.txt` / `p1run.sh` | `idle`, `title`, `sink`, `sinkmenu`, `afterrestart`, `fight` `idle` [src Source/IronwakeTest/IronwakePlayerController.cpp:1312], `fight` [src Source/IronwakeTest/IronwakePlayerController.cpp:1312] | `IWP1`, `IWEND` |
| S1 (voyage) | `iw_s1.txt` / `s1run.sh` | `look`, `progress`, tours `look` [src Source/IronwakeTest/IronwakeVoyage.cpp:1436] | `IWS1` |

P11 shot timing: each step waits `12` s (`18` s for rain/squall) before the shot and `1.5` s after [src Source/IronwakeTest/IronwakeP11.cpp:483-490]. P11 `bench` stands at `330` m, hour `12`, drift off [src Source/IronwakeTest/IronwakeP11.cpp:459].

Test-time input rules: the S2 director clears all input mappings about once per second and the W2/P9 directors strip input too, so a window that steals focus cannot open the menu [src Source/IronwakeTest/IronwakeS2Test.cpp:64-67]. The `progress` mode makes the voyage tickable while paused [src Source/IronwakeTest/IronwakeVoyage.cpp:282].

## 13.7 Editor Python (`Content/Python/`)

Run headless with `build/tools/pyrun.sh <module>` (module name without `.py`) or live with `uepy.py`. Asset import and material builders, mostly idempotent, with `unreal.SystemLibrary.quit_editor()` at the end of headless ones.

| Script | Purpose | Source |
|---|---|---|
| `init_unreal.py` | imports the module named in env `IWTEST_SCRIPT` at editor start | `IWTEST_SCRIPT` [src Content/Python/init_unreal.py:2] |
| `iw_import_audio.py` | import every `Audio/*.wav` to `/Game/Ironwake/Audio` | `/Game/Ironwake/Audio` [src Content/Python/iw_import_audio.py:12] |
| `p11_audio.py`, `w3_audiofix.py`, `w1_assets.py`, `w2_assets.py` | audio imports with looping flags (chapter 10) | `looping` [src Content/Python/p11_audio.py:15] |
| `iw_r1_import.py`, `iw_r1_materials.py`, `iw_r1_airab.py`, `iw_r1_airfix.py`, `iw_import_airship.py` | library ships and airship (R1) | `R1` [src Content/Python/iw_r1_import.py:1] |
| `iw_s2_import.py`, `iw_s2_cloth.py` | steam-sail frigate hull and cloth material (S2) | `S2` [src Content/Python/iw_s2_import.py:1] |
| `iw_e1_import.py`, `iw_p8_import.py` | world assets from `build/assets/e1/` and `build/assets/p8/` | `build/assets/e1` [src Content/Python/iw_e1_import.py:1] |
| `iw_i1_*.py` (import, mats, shelf, surfparams, usage, veglod, folfix, water) | island assets, materials and fixes (I1) | `I1` [src Content/Python/iw_i1_import.py:1] |
| `iw_p8_sky.py`, `iw_p10_sky.py`, `iw_p10_mats.py`, `iw_p10_bounds.py`, `iw_p10_chk.py`, `iw_p12_mats.py`, `iw_p12_smoke.py`, `iw_p12_smoke.py`, `iw_p13_probe.py` | night-sky material, town materials and probes | `P8` [src Content/Python/iw_p8_sky.py:1] |
| `w1_*.py`, `w2_*.py`, `w3_*.py`, `w4_*.py` | ship water FX, ocean material chain, foam nodes (edited in place: deleting referenced water assets crashed the editor) | `IN PLACE` [src Content/Python/w3_foammat.py:1] |
| `build_storm_test.py`, `build_storm_v2.py`, `storm_v3.py`, `storm_v4.py`, `ocean_base.py`, `epic_water_sample.py`, `fix_ocean.py` | early ocean/storm map construction from Epic's water sample | `Epic's` [src Content/Python/ocean_base.py:1] |
| `p5b_assets.py`, `p5c_probe.py` | P5 speed-FX assets and probes | |

Never delete or force-replace an asset that is still referenced (hit an ensure in `ObjectTools.cpp:4051` and killed the editor) [src build/BRIEF.md:88-89].

## 13.8 Ursa cluster scripts (`~/Projects/game-concepts/ironwake/cluster/`)

Cluster work is run by the orchestrator through `[REDACTED]`; workers must **not** submit jobs, they append requests to `build/URSA_REQUESTS.md` [src build/BRIEF.md:166-167]. Partitions: `standard` (CPU), `gpul4` (L4 GPUs), `gpu1v1` (V100).

| File | Kind | Purpose | Key facts | Source |
|---|---|---|---|---|
| `fleet2_concepts.sh` | local shell | concept images for new NPC craft (fishing, patrol, airships, harbour craft) with the `lumina` CLI | studio style prompt, `--aspect-ratio 1:1`, skips existing | `lumina` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/fleet2_concepts.sh:10] |
| `town_concepts.sh` | local shell | concept images for port towns (houses, harbour props) | same style as fleet2 | `lumina` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/town_concepts.sh:10] |
| `img3d.sbatch` | Slurm array `0-1` | Hunyuan3D-2 image to textured GLB, one model per `inputs/*_concept.png` | partition `gpul4` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/img3d.sbatch:5], 1 GPU, `02:00:00` | |
| `harbour3d.sbatch` | Slurm array `0-9` | same, 10 harbour models | `gpul4` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/harbour3d.sbatch:5], `02:00:00` | |
| `harbour3d_v100.sbatch` | Slurm single | same on a V100 | `gpu1v1` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/harbour3d_v100.sbatch:4], `03:00:00` | |
| `prep_ships.sbatch` | Slurm array `0-9` | Blender 5.2.3 headless decimation of 10 library ships | `standard` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/prep_ships.sbatch:3], 8 CPUs, `48G`, `01:30:00`; targets `300000 200000 200000 200000 120000 300000 100000 80000 200000 80000` triangles | `TRIS=` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/prep_ships.sbatch:15] |
| `prep_ships.py` | Blender script | decimate to budget, apply transforms, write `<name>_game.glb`, `<name>_report.json`, previews | `blender -b -P prep_ships.py -- in.glb outdir tris` | `blender -b -P prep_ships.py` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/prep_ships.py:2] |
| `prep_island.py` | Blender script | prep one Poly Haven island glTF (decimate, 2K textures, game GLB + preview) | `-- <in.gltf> <outdir> <target_tris>` | `blender -b -P prep_island.py` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/prep_island.py:2] |
| `prep_islands_local.sh` | local shell | batch-run `prep_island.py` over `models/islands`, `xargs -P 4`, timeout `900` s each | budgets `120000` coast/cliff, `60000` trees, `25000` saplings, `15000` shrubs | `gtimeout 900` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/prep_islands_local.sh:10] |
| `remesh_bake.py` | Blender script | remesh a messy triangle-soup model and bake textures from the original | `-- <in.glb> <outdir> <target_faces> <tex_res>` | `blender -b -P remesh_bake.py` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/remesh_bake.py:2] |
| `turntable.py` | Blender script | Cycles turntable stills + key art | `-- <model> <outdir> <frames> <res_x> <res_y> <samples>` | `blender -b -P turntable.py` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/turntable.py:2] |
| `score_job.sh` | local shell | quick turntables for every model of a downloaded hy3d job, comparison sheet (concept, hero render, turntable) for vision scoring | `gtimeout 240` per model | `gtimeout 240` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/score_job.sh:10] |
| `make_sky.py` | Python (Ursa) | night-sky textures: equirect star map (Hipparcos positions/magnitudes/B-V), procedural Milky Way, Moon Kit albedo, `moon_phase_<n>.png` (8), `moon_2027.json` | writes `SKY_USE.md`; skips failed catalogues rather than faking | `Nothing is faked` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/make_sky.py:13] |
| `sky.sbatch` | Slurm | runs `make_sky.py` | `standard` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/sky.sbatch:3], 16 CPUs, `60G`, `01:30:00` | |
| `iw_cluster_watch.sh` | local shell | waits for listed job ids, downloads `out/` to `<dest>/<jobid>/` at the first job that ends, exits (so a background run notifies the orchestrator) | `iw_cluster_watch.sh <dest-root> <jobid>...`, polls every `90` s, up to `3` h | `sleep 90` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/iw_cluster_watch.sh:11] |
| `ursa_wait_fetch.sh` | local shell | wait for a job-name pattern to leave RUNNING/PENDING, then download | `<jobname-pattern> <first-array-jobid> <dest-dir>` | `sleep 120` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/ursa_wait_fetch.sh:6] |
| `ursa_wait_ids.sh` | local shell | `[REDACTED] wait` for ids (timeout `14400`, poll `90`), then merge each job's `out/` into DEST | `ursa_wait_ids.sh DEST id...` | `--timeout 14400` [src [REDACTED]/Projects/game-concepts/ironwake/cluster/ursa_wait_ids.sh:7] |

Also in the same project tree (not under `cluster/`): `~/Projects/game-concepts/ironwake/fetch_islands.py` (fetches island assets) [src [REDACTED]/Projects/game-concepts/ironwake/fetch_islands.py:1] and the audio sources described in chapter 10. Nothing in `cluster/` is invoked by the build or the game; outputs are copied to `~/Projects/game-concepts/ironwake/models/...` by the orchestrator and prepared into `build/assets/` (chapter 12). The orchestrator CLI is `env -u PYTHONPATH [REDACTED]` (`jobs`, `results <id> --prefix out --download`, `job log`, `wait`) [src [REDACTED]/Projects/game-concepts/ironwake/cluster/iw_cluster_watch.sh:16].

## 13.9 Kanban workflow

| Item | Value | Source |
|---|---|---|
| Board slug / name | `ironwake` / "Ironwake overnight build" ("Playable steampunk naval slice in UE 5.8") | `ironwake` [src [REDACTED]/.hermes/kanban/boards/ironwake/board.json:2], `Ironwake overnight build` [src [REDACTED]/.hermes/kanban/boards/ironwake/board.json:3] |
| Board files | `~/.hermes/kanban/boards/ironwake/board.json` and `kanban.db` | `default_workdir` [src [REDACTED]/.hermes/kanban/boards/ironwake/board.json:6] |
| Default workdir | `[REDACTED]/Projects/unreal/IronwakeTest` | `[REDACTED]/Projects/unreal/IronwakeTest` [src [REDACTED]/.hermes/kanban/boards/ironwake/board.json:6] |
| Worker profile | `ironwake-build` (every card file begins "Owner: ironwake-build"); profile directory `~/.hermes/profiles/ironwake-build/` | `Owner: ironwake-build` [src build/cards2/D1_full_spec.md:3] |
| Worker model / provider | `claude-sonnet-5-5` via `custom:ucr-gateway` | `claude-sonnet-5-5` [src [REDACTED]/.hermes/profiles/ironwake-build/config.yaml:24], `custom:ucr-gateway` [src [REDACTED]/.hermes/profiles/ironwake-build/config.yaml:15] |
| Profile kanban setting | `review_dispatch: true` | `review_dispatch: true` [src [REDACTED]/.hermes/profiles/ironwake-build/config.yaml:27] |
| Card source files | `build/cards/` and `build/cards2/*.md`; card id map in `build/cards2/ids.json` (17 entries, e.g. `P1_loop` `t_877230bd`, `W4` `t_ecad708c`, `P6_opus_review` `t_0d533935`) | `P1_loop` [src build/cards2/ids.json:2], `W4` [src build/cards2/ids.json:15] |
| Handoff | each card ends with `kanban_complete` metadata plus `build/logs/<card>.md` (what works, capture paths, what is left, known bugs) | `kanban_complete` [src build/BRIEF.md:73] |
| Watcher | `build/watch_cards.sh <board-slug> <build-dir>`: polls `hermes kanban --board <slug> list` every `60` s; exits (notifying the background run) on any status change, when `<build-dir>/STOP` appears, or after `45` minutes (heartbeat) | `sleep 60` [src build/watch_cards.sh:8], `2700` [src build/watch_cards.sh:6] |
| Stop file | if `build/STOP` exists, write `build/STOP_REASON.md` and stop | `build/STOP` [src build/BRIEF.md:71] |
| Model for the orchestrator | not recorded in the repo: unknown (read the Hermes session config of the default profile) | |

Card order was rewritten several times in BRIEF: current explicit order lines are "S2 -> W2 -> I1 -> P3 -> P4 -> P5" [src build/BRIEF.md:187] and "W3 = readable storm + ship water FX, after I1. Order: I1 -> W3 -> P3 -> P4 -> P5" [src build/BRIEF.md:190]; later cards (P5b-P14, D1) are only in `build/cards2/`. Card history is chapter 14.

## 13.10 BRIEF rules summary

| # | Rule | Source |
|---|---|---|
| 1 | Crash-reporter trap and offscreen/no-sound rules (13.3) | `HARD RULE` [src build/BRIEF.md:3] |
| 2 | Ship water FX scale with speed and wave: small bow spray, big only when the bow drives into a big wave; nothing at a crawl; wake strength/width/length scale with speed | `Ship water FX scale with speed and wave` [src build/BRIEF.md:61] |
| 3 | Open-sea storm look is PARKED (waves, whitecaps, foam, spume, breaking crests); ship water FX still wanted; reviews show calm and moderate seas | `THE OPEN-SEA STORM LOOK IS PARKED` [src build/BRIEF.md:62] |
| 4 | Clean up after every editor/-game run (kill CrashReportClient, confirm none) | `Clean up after every editor or -game run` [src build/BRIEF.md:63] |
| 5 | No secrets in git; commit at every working step; local commits only (no remote) | `No secrets in git` [src build/BRIEF.md:64] |
| 6 | No spending: no cluster jobs from workers, no paid assets, no Fab purchases, no outreach | `No spending` [src build/BRIEF.md:66] |
| 7 | Honest status: "done" = ran in PIE and captured a screenshot that shows it | `Honest status` [src build/BRIEF.md:68] |
| 8 | Respect the art bible: no cartoon kitbash, grey stormy palette, real-metre scale | `Respect the art bible` [src build/BRIEF.md:70] |
| 9 | Stop and write `build/STOP_REASON.md` if `build/STOP` exists | `Stop and write` [src build/BRIEF.md:71] |
| 10 | Leave CLAUDE.md / AGENTS.md alone | `Leave CLAUDE.md` [src build/BRIEF.md:72] |
| 11 | Handoff: `kanban_complete` + `build/logs/<card>.md` | `Handoff` [src build/BRIEF.md:73] |
| 12 | Music OFF by default, SFX on | `Standing rule` [src build/BRIEF.md:148] |
| 13 | Hull never sits below its paint line in calm water | `Never let a hull sit below its paint line` [src build/BRIEF.md:186] |
| 14 | Collision is a hard rule: ships never pass through land, beaches, rocks, piers, buildings or other ships; every placed object gets simple collision | `Collision is a hard rule` [src build/BRIEF.md:192] |
| 15 | Audio: only water + environment sounds | `Audio: ONLY water` [src build/BRIEF.md:184] |
| 16 | Workers must not submit cluster jobs; write requests to `build/URSA_REQUESTS.md` | `do NOT submit cluster jobs` [src build/BRIEF.md:166] |
| 17 | Test windows steal focus: batch runs and relaunch as little as possible | `Test windows steal focus` [src build/BRIEF.md:188] |
| 18 | Re-package the pass build to `~/Games/Ironwake/Ironwake.app`, keep the previous as `Ironwake_v1.app` until verified (PASS 2 rule; later builds are kept as `Ironwake_<card>.app`) | `Ironwake_v1.app` [src build/BRIEF.md:98] |
| 19 | Direction: sailing ships are the game's centre; frigate is the default player ship; close zoom to ~20 m | `the game now centres on SAILING ships` [src build/BRIEF.md:175], `~20 m` [src build/BRIEF.md:179] |

**Stale / superseded BRIEF text:** the "Controls (DESIGN.md section 3)" line (A/D rudder steps, RMB zoom) [src build/BRIEF.md:47] is superseded by the held-wheel rudder and RMB-exits-scope bindings in chapter 9; "Current state (verified 2026-11-09 00:45)" [src build/BRIEF.md:49] describes the pre-build map and is historical.

## 13.12 Public showcase repo and site (2026-11-13)

The public mirror is `charles-forsyth/ironwake` on GitHub; GitHub Pages serves `docs/` at
`https://charles-forsyth.github.io/ironwake/`. It holds the C++ source, Config, the editor Python, the asset pipeline
scripts and these spec chapters. Cooked `.uasset` content and licensed source models stay out of it (see chapter 12 and
`CREDITS.md`).

### Site (`docs/`)
| Piece | What it does | Value |
|---|---|---|
| 3D hangar | `<model-viewer>` over the web GLBs in `docs/models/` (18 models plus `manifest.json`) | `model-viewer/4.0.0` (public repo `docs/index.html:21`) |
| Reel | four cuts; cut 4 is a Blender studio render, labelled as one, the rest are in-game captures | `cut4_articulated_rig_reel` (public repo `docs/site.js:23`) |
| Sound | Web Audio mixer over the 25 MP3s in `docs/audio/` | |
| Wake lab | Canvas Kelvin-wake toy beside the cut-2 footage | |

### Pipeline scripts used for the site
| Script | Job | Key value |
|---|---|---|
| `pipeline/prep_web_models.py` | Blender: assemble articulated GLBs (moving parts kept as separate `PART_*` nodes), decimate to about 26k triangles, write QA thumbnails | `decimate_mesh(tav, 26000)` (public repo `pipeline/prep_web_models.py:845`) |
| weld before decimate | the glTF importer splits vertices on every UV/normal seam; collapse-decimating that tore surfaces into slivers and holes, so seams are merged first | `def weld_mesh(obj, dist=1e-5):` (public repo `pipeline/prep_web_models.py:370`) |
| two-sided materials | exported as glTF `doubleSided`, so back faces never show through | `m.use_backface_culling = False` (public repo `pipeline/prep_web_models.py:499`) |
| depsgraph refresh | read `matrix_world` only after an update, or every part collapses to the origin | `bpy.context.view_layer.update()` (public repo `pipeline/prep_web_models.py:531`) |
| `pipeline/render_rig_reel.py` | Cycles studio vignettes (brig yards, paddle wheels, tender crane and bell, airship propellers) in a lathed cyclorama | `--samples 16` (public repo `pipeline/render_rig_reel.py:7`) |
| camera arc | slow 32 degree arc on the bow quarter, never head-on (bow-on foreshortens and clips) | `200 + 32 * ease` (public repo `pipeline/render_rig_reel.py:232`) |

### Cluster runs (Ursa `standard` partition)
- Web GLBs: one 16-CPU node, 18 Blender slices in parallel, about 40 s per run.
- Reel: an 8-task Slurm array (one task per vignette half, 4 Blender processes x 4 threads), 480 frames at 1280x720 in
  about 7 minutes, then a separate small encode job on the cluster. One node alone runs about 45 minutes (about 23 s per frame).
- Ursa pitfalls: the `ffmpeg/8.1` module lacks libx264 and libmp3lame, so jobs fetch a static ffmpeg into `/tmp`; Mac
  `tar` adds AppleDouble `._*` files (use `COPYFILE_DISABLE=1 tar --no-xattrs` and delete `._*` in the job); UE WAVs are
  WAVE_FORMAT_EXTENSIBLE, which Python's `wave` rejects, so durations come from ffmpeg.

### Release gate
| Check | Value |
|---|---|
| `tools/verify_repo.py`: every model, sound, page reference and link resolves, plus a privacy scan of tracked files and GLB metadata | `VERIFY OK` (public repo `tools/verify_repo.py:144`) |
| CI runs the gate, then every spec citation | `python3 tools/spec_check.py --all --fails` (public repo `.github/workflows/ci.yml:26`) |
| CI job name | `Repo, showcase and privacy gate` (public repo `.github/workflows/ci.yml:13`) |


## 13.11 Gaps / unknown

- No script verifies the built `.app` after packaging (no smoke test); `package.sh` only greps `BUILD SUCCESSFUL|BUILD FAILED|Error:`. A packaged-run smoke check is unknown/absent.
- `-iwe1` mode names beyond `bench`/`bench_island` are not enumerated here: `TickE1Test` mode selection was not read in full (read `Source/IronwakeTest/IronwakeVoyage.cpp` from line 1470).
- `-iwi1` modes beyond `shots` and `bench` (the I1 run script lists only these two) are unknown; `IronwakeArchipelago.cpp` `TickTest` near line 1100-1200 holds the list.
- Exact S1 (`iw_s1.txt`) mode list: only `look` and `progress` found by grep; other modes unknown.
- The orchestrator's model and the `hermes` CLI commands that created the board are not stored in the repo (unknown); the profile config only shows the worker.
- `build/cards/` (first-pass cards) was not read for this chapter; only `build/cards2/` and `ids.json`.
- Whether the 81 flag count is complete for flags read through helper wrappers (e.g. a flag parsed with `HasParam` or by `Contains` on `FCommandLine::Get()` text) is unverified; the `grep` pattern only finds `TEXT("iw...")` literals.
- Packaged apps carry two bundle ids (`com.omstead.ironwake`, `com.YourCompany.IronwakeTest`); which one a given `~/Games/Ironwake*.app` uses must be read from its `Info.plist` (not done; `build/Mac/Resources/Info.template.plist` is the template).
- Test directors for P13 and P14 re-use `-iwp11` modes (`tw`, `p14`); no dedicated `-iwp13`/`-iwp14` flag exists.
