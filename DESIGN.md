# IRONWAKE design notes: world size, controls, POV, mechanics, balance (v1, 2026-11-08)

## 1. World size
Engine limits are not the constraint. UE5 has double-precision Large World Coordinates and World
Partition streaming, so the world could be thousands of km. The real limits are travel time,
content density, and Mac frame rate.

Travel-time maths: a 15-knot ironclad does about 28 km/h. Crossing a 100 km sea at true speed takes
over 3 hours of real time. Too slow, so the game compresses:
- Ships move at ~3x real speed in open water (an ironclad crosses 10 km in ~7 min).
  Combat drops to ~1.5x so gunnery stays readable.
- Long legs between known ports use a "make way" fast-sail (time skip with weather and encounter
  rolls; an encounter drops you back into real time).

Recommended world: about 120 x 120 km (14,400 km2), built as:
- Open ocean (procedural UE Water + weather): costs almost nothing per km.
- 30-40 islands in clusters; content density goes into islands, ports and coasts, not open water.
- 4 sea zones: calm trade sea (centre), stormy northern straits, pirate reef belt, outer ocean
  (unexplored, wrecks, anomalies) at the edges.
- Visibility ~15-20 km in clear weather (horizon from a bridge), so islands appear on the skyline.
For scale: World of Warships battle maps are ~30-50 km square; this is a few of those joined,
with the empty water doing the work.
Expansion is cheap: more zones can be added by streaming new World Partition cells.

## 2. POV and camera
- Main: third-person orbit camera behind the ship (WoWs style). Mouse moves the camera freely;
  the ship keeps its course.
- Gunnery view: hold right mouse (or Shift) for a binocular/rangefinder zoom (2x-16x) locked to
  the camera direction, with a reticle and range readout.
- Bridge view: first-person on the bridge/wheelhouse for atmosphere, sailing in storms and photo
  mode. Not needed for combat.
- Airship view (when flying your own later): chase camera, plus a bombsight view looking down.
- Free/photo camera: detach, slow time, depth of field.
- Port view: the camera swings to a fixed harbour shot for docking, market and refit screens.

## 3. Mouse and keyboard
Ships are throttle-and-rudder, not WASD-direct, so they feel heavy:
- W/S: engine telegraph steps (Full astern, Half astern, Stop, Slow, Half, Full, Flank). It holds.
- A/D: rudder steps (hard port to hard starboard); Q/E centre rudder / auto-follow course.
- Mouse: aim turrets where the camera points (turrets traverse at their real rate, so you see them
  catching up). LMB fire the selected battery, RMB zoom.
- 1-4: select battery (main guns, secondaries, torpedoes, flak/special). Middle mouse: lock target.
- Map (M): click to set a waypoint course; the ship autopilots and you take over at any time.
- Tab: fleet orders (follow, engage my target, screen, retreat) once escorts exist.
- F: searchlight; R: damage control (put out fires/flooding, cooldown); Space: ram warning/brace.
- Full gamepad mapping as well (sticks = rudder/throttle + camera).

## 4. Core mechanics
- Movement physics: mass, acceleration, turning circle and rudder lag per hull. Big ships take
  ~30-60 s to reach full speed and turn wide. Wind affects airships strongly, ships slightly.
  Waves pitch the ship, which moves the guns (firing on the roll matters in storms).
- Gunnery: ballistic shells with drag and flight time (2-20 s by range). Lead the target by eye,
  with a subtle lead indicator that can be turned off. Dispersion grows with range, own speed and
  turning. Spotting: you can only aim at what's visible (line of sight, fog, night, smoke,
  airship spotting).
- Armor and damage: zones (belt, deck, turrets, superstructure, bow/stern). Shell type (AP pierces
  thick belt at the right angle, HE sets fires and wrecks superstructure). Angling the hull bounces
  shells. Effects you see: fires, flooding and list, disabled turrets, lost rudder, funnel shot
  away (speed drop), ammo magazine detonation (rare, cinematic).
- Crew: a simple pool that repairs, fights fires and mans guns. Losses slow everything.
- Ramming and boarding: close-range options for desperate or pirate play; boarding a crippled ship
  can capture it (adds to fleet or sells as a prize).
- Airships: slow, fragile, high. Bomb runs against slow ships, spotting extends gun range.
  Counters: flak slot, evasive turning, smoke, sailing under storm cover.
- Weather: storms cut visibility and accuracy, damage small ships, hide you. Fog is a tactical tool.
- Economy: 8 goods, port supply/demand, convoys moving goods. Prices respond to war damage.
- Progression: money -> hull purchases and module upgrades; reputation unlocks ports, contracts
  and faction ships. No XP grind, no tech tree spreadsheets.

## 5. Balance
Guiding rule: bigger ships are stronger but not strictly better. Every hull should have a job.

| Hull | Role | Speed | Armor | Firepower | Cargo | Running cost |
|---|---|---|---|---|---|---|
| Cutter | scout, smuggler, fast trader | very high | paper | light | small | very low |
| Gunboat | coastal fighter, pirate hunter | high | light | medium | small | low |
| Steam frigate | all-rounder, early merchant-warship | medium-high | medium | medium | medium | medium |
| Cruiser | raider, convoy escort, long-range | high | medium | heavy secondaries | medium | high |
| Ironclad | line combat, blockade | low | heavy | heavy | low | high |
| Dreadnought | capital, faction flagship | low | very heavy | very heavy | very low | very high |

Balance levers:
- Rock-paper-scissors: cruisers outrun ironclads and kite them; ironclads crush cruisers that close
  in; torpedo gunboats threaten big ships at night and in fog; airships punish slow ships but die
  to flak; small ships evade big guns (turret traverse and dispersion can't track them up close).
- Running costs (coal, crew wages, repairs, port fees) stop a dreadnought from being the default:
  owning one is a business decision.
- Coal range: big ships need coaling stations, so the map shapes strategy.
- NPC difficulty scales by zone, not by player level: the outer ocean is dangerous from day one,
  the trade sea stays safe-ish. No level-matched enemies.
- AI honesty: NPCs use the same physics, spotting and ballistics as the player. No cheating aim.
- Difficulty options: aim assist on/off, damage multiplier, economy harshness, permadeath of ships
  (insurance) on/off.
- Balance testing: a headless simulation harness runs thousands of seeded NPC-vs-NPC battles and
  economy years, then reports win rates per hull matchup and price stability. Tune the numbers from
  data, not guesses (the same approach as earlier headless playtests). This can run as a batch job
  on batch compute nodes if it scales up.

## 6. Decisions (Chuck, 2026-11-08)
- Speed compression: yes (3x open water, 1.5x combat, fast-sail between known ports).
- Lead indicator: ON by default; optional aim assist available in settings.
- Ship loss: chosen per save (insurance/respawn at port, or real loss).

## 7. A living world (NPCs doing things)
The sea should feel busy even when nothing is aimed at you. Everything below is driven by the same
deterministic sim and utility AI as the battles (no LLM), with low-fidelity off-screen updates and
full 3D near the player.
- Traffic: merchant convoys on real trade routes, fishing fleets working banks at dawn, coal tenders
  shuttling to coaling stations, ferries between islands, smugglers running dark at night, salvage
  crews on old wrecks, survey ships charting the outer ocean.
- Navy and pirates: patrols on beats, blockades during wars, pirates shadowing convoys, bounty hunters
  chasing named pirate captains, escorts reacting to distress flares.
- Airships: mail and passenger airships on schedules, scout airships circling contested zones, a
  faction carrier-airship occasionally crossing the sky (an event you hear before you see).
- Ports: cranes loading the ships that are actually docked, smoke from foundries when iron is in
  supply, lights at night, crowds and dockhands, harbour boats, ships queueing for repairs after a
  battle, a port visibly damaged after a siege and rebuilt over the following days.
- Radio and signals: flag signals, signal lamps at night, distress calls, rumours at the harbour
  board ("convoy lost off the Teeth", "pirate Red Mallory sighted north"), news that comes from what
  really happened in the sim.
- Named characters: a few dozen named captains with traits and grudges who persist, rise in rank,
  get sunk, and remember you.
- Wildlife and weather life: gulls following fishing boats, whales in the outer ocean, storm fronts
  that move across the map and reroute traffic, fog banks rolling in at dawn.
- World events: faction wars, port seizures, plague or famine raising grain prices, aether storms in
  the outer ocean, a lost treasure fleet. They emerge from sim state, not timers alone.
- Ambient audio does a lot of the work: distant guns over the horizon, foghorns, ship bells, harbour
  chatter, engine thrum of passing ships.

## 8. Open questions for Chuck
- None right now.
