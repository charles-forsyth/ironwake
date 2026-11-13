# Ironwake world plan (Chuck 2026-11-10: "a big world with a bunch of things and ports")

Target (DESIGN.md section 1): about 120 x 120 km, 30-40 islands in clusters, 4 sea zones, many ports and towns,
living harbours (people, carts, ships loading), NPC traffic at sea and in the air. Today's slice is 8.4 km with
18 places and one port: roughly one corner of the calm trade sea.

## How a world this size is built (structure before content)
1. World Partition + Large World Coordinates. The sea is procedural (costs nothing per km); islands and towns stream in
   by cell. The ocean/WaterZone must tile or follow the player instead of one 8.4 km body.
2. Everything procedural from data, nothing hand-placed: a world file (JSON) lists zones, islands, ports, routes;
   seeded builders (the I1 archipelago builder grown up) make landforms, towns, harbours, vegetation from it. So
   adding ports is data plus assets, not a new card each time.
3. Ports as a kit: harbour block (quays, cranes, warehouses, customs house, tavern, chandlery), town builder
   (streets up the slope, houses by size/wealth, gardens, fences, trees), faction/zone style variants.
4. Distance budget: far islands as low-cost silhouettes (HLOD), towns at full detail only within ~2-3 km, crowds and
   props within ~500 m. Keep the 60 FPS target honest with packaged benches at a busy port.
5. Fast sail / "make way" between known ports (time skip with encounter rolls), so 120 km is playable.

## Content counts (first full world)
- Sea zones 4: calm trade sea (centre), stormy northern straits, pirate reef belt, outer ocean.
- Islands 30-40 (14 built), ports 8-12 (1 built), villages/landmarks 15-20.
- Buildings ~25 types x 2-3 variants; harbour/street props ~60; vegetation ~40 species/sizes (Poly Haven CC0 first).
- Ships: player roster 6 hulls; NPC types ~15 (fishing, coastal trade, convoys, patrols, pirates, ferries, tenders);
  airships ~5 types.
- People: harbour and street crowds (Unreal mannequins first; MetaHuman Crowds if Chuck adds it on Fab).
- Economy: 8 goods, port supply/demand, convoys that actually carry cargo (tuned by cluster economy sims).

## Asset pipeline (off the laptop)
- Concepts: lumina on the laptop (cheap, fast), vision-screened before 3D.
- 3D: Hunyuan3D-2 on multi-GPU V100 and L4 batch workers.
- Cleanup: headless Blender on high-memory CPU batch workers (remesh, bake, LODs, collision), one job per asset.
- Library-first where it exists: Poly Haven CC0 (trees, plants, rocks, barrels, crates, lamps, furniture),
  Sketchfab CC-BY ships (21 already downloaded and prepped, most not yet in the game).
- The laptop only imports finished, scored assets; workers get local paths, never cluster access.

## Order
1. Now: P6 review -> P5b (speed-scaled spray/wake, grey funnel smoke) -> P4 clip.
2. Cluster batches running in parallel: NPC ships + airships, town buildings + props, night sky textures, economy sims.
3. W-STRUCT card: World Partition + streamed/tiling ocean + world-data file + builders read it (no new art). Prove 2-3
   zones at ~30-40 km with packaged FPS.
4. PORTS card: port kit + town builder + 3-4 ports populated from the cluster assets; crowds.
5. TRADE card: goods, markets, money, convoys (numbers from the economy sims).
6. SKY card: time of day separate from weather, stars/Milky Way/moon, cloud cover, moving squalls.
7. Then grow outward zone by zone: more islands and ports are data + assets.
