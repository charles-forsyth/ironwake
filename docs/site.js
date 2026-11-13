(function () {
  "use strict";

  var $ = function (id) { return document.getElementById(id); };
  var reduceMotion = window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  /* ------------------------------------------------------------------ hero */
  var hero = $("hero-video");
  var heroBtn = $("hero-toggle");
  if (hero && heroBtn) {
    if (reduceMotion) { hero.removeAttribute("autoplay"); hero.pause(); heroBtn.textContent = "Play video"; heroBtn.setAttribute("aria-pressed", "true"); }
    heroBtn.addEventListener("click", function () {
      if (hero.paused) { hero.play(); heroBtn.textContent = "Pause video"; heroBtn.setAttribute("aria-pressed", "false"); }
      else { hero.pause(); heroBtn.textContent = "Play video"; heroBtn.setAttribute("aria-pressed", "true"); }
    });
  }

  /* ------------------------------------------------------------------ reel */
  var CUTS = [
    { file: "cut1_archipelago_voyage", title: "Archipelago voyage", sub: "22 s · all three ports, the brig, the living sea, the market" },
    { file: "cut2_hydrodynamic_wake_lab", title: "Wake lab, 1.5 to 16 knots", sub: "12 s · in-game ironclad beside the solved wake field" },
    { file: "cut3_storm_swell", title: "Heavy swell", sub: "20 s · real vertical sea motion in rough water" },
    { file: "cut4_articulated_rig_reel", title: "Moving parts", sub: "14 s · Blender studio render of the web models: yards, paddles, crane, propellers" }
  ];
  var reelVideo = $("reel-video");
  var reelList = $("reel-list");
  function pickCut(i) {
    var c = CUTS[i];
    reelVideo.poster = "assets/poster_" + c.file + ".jpg";
    reelVideo.src = "video/" + c.file + ".mp4";
    reelList.querySelectorAll(".reel-item").forEach(function (b, j) { b.setAttribute("aria-pressed", String(j === i)); });
  }
  if (reelVideo && reelList) {
    CUTS.forEach(function (c, i) {
      var b = document.createElement("button");
      b.type = "button";
      b.className = "reel-item";
      b.setAttribute("aria-pressed", String(i === 0));
      b.innerHTML = '<img alt="" loading="lazy">' + "<div><strong></strong><small></small></div>";
      b.querySelector("img").src = "assets/poster_" + c.file + ".jpg";
      b.querySelector("strong").textContent = c.title;
      b.querySelector("small").textContent = c.sub;
      b.addEventListener("click", function () { pickCut(i); reelVideo.play().catch(function () {}); });
      reelList.appendChild(b);
    });
  }

  /* ------------------------------------------------------------------ hangar */
  var CAT_LABEL = { ships: "Ship", aircraft: "Airship", architecture: "Waterfront", craft_crew: "Small craft and crew" };
  var mv = $("mv");
  var roster = $("roster");
  var models = [];
  var current = null;

  function fmtBytes(b) { return (b / 1048576).toFixed(1) + " MB"; }
  function selectModel(m) {
    current = m;
    $("insp-cat").textContent = CAT_LABEL[m.category] || m.category;
    $("insp-title").textContent = m.title;
    $("insp-desc").textContent = m.description;
    $("insp-role").textContent = m.unreal_role.replace(/\s*\(\s*/g, " (").replace(/\s*\)/g, ")");
    $("insp-dims").textContent = m.dimensions_m.formatted + " (L x B x H)";
    $("insp-tris").textContent = m.triangles.toLocaleString("en-US") + " triangles";
    $("insp-size").innerHTML = "";
    var a = document.createElement("a");
    a.href = "models/" + m.filename;
    a.textContent = m.filename + " · " + fmtBytes(m.size_bytes);
    a.setAttribute("download", "");
    $("insp-size").appendChild(a);
    $("mv-file").textContent = "models/" + m.filename;
    if (mv) {
      mv.setAttribute("alt", "3D model: " + m.title);
      mv.setAttribute("src", "models/" + m.filename);
      mv.setAttribute("camera-orbit", "32deg 74deg auto");
    }
    roster.querySelectorAll("button").forEach(function (b) { b.setAttribute("aria-pressed", String(b.dataset.id === m.id)); });
  }

  function buildRoster(list) {
    models = list;
    list.forEach(function (m) {
      var b = document.createElement("button");
      b.type = "button";
      b.dataset.id = m.id;
      b.dataset.cat = m.category;
      b.setAttribute("aria-pressed", "false");
      b.innerHTML = '<img alt="" loading="lazy"><span></span>';
      b.querySelector("img").src = "assets/thumbs/" + m.id + ".jpg";
      b.querySelector("span").textContent = m.title;
      b.addEventListener("click", function () { selectModel(m); });
      roster.appendChild(b);
    });
    selectModel(list[0]);
  }

  if (roster) {
    fetch("models/manifest.json")
      .then(function (r) { return r.json(); })
      .then(function (d) { buildRoster(d.models); })
      .catch(function () { roster.textContent = "Could not load the model list."; });
  }

  var filter = $("fleet-filter");
  if (filter) {
    filter.addEventListener("click", function (e) {
      var chip = e.target.closest(".chip");
      if (!chip) return;
      var cat = chip.dataset.cat;
      filter.querySelectorAll(".chip").forEach(function (c) { c.setAttribute("aria-pressed", String(c === chip)); });
      roster.querySelectorAll("button").forEach(function (b) { b.hidden = !(cat === "all" || b.dataset.cat === cat); });
      if (current && cat !== "all" && current.category !== cat) {
        var first = models.filter(function (m) { return m.category === cat; })[0];
        if (first) selectModel(first);
      }
    });
  }

  var rotBtn = $("mv-rotate");
  if (rotBtn && mv) {
    if (reduceMotion) { mv.removeAttribute("auto-rotate"); rotBtn.textContent = "Turntable off"; rotBtn.setAttribute("aria-pressed", "false"); }
    rotBtn.addEventListener("click", function () {
      var on = !mv.hasAttribute("auto-rotate");
      if (on) mv.setAttribute("auto-rotate", ""); else mv.removeAttribute("auto-rotate");
      rotBtn.textContent = on ? "Turntable on" : "Turntable off";
      rotBtn.setAttribute("aria-pressed", String(on));
    });
  }
  var BGS = [["", "harbour", 1.0], ["bg-dusk", "dusk", 0.95], ["bg-paper", "chart paper", 1.1]];
  var bgIdx = 0;
  var bgBtn = $("mv-bg");
  if (bgBtn && mv) {
    bgBtn.addEventListener("click", function () {
      var v = $("viewer");
      v.classList.remove("bg-dusk", "bg-paper");
      bgIdx = (bgIdx + 1) % BGS.length;
      if (BGS[bgIdx][0]) v.classList.add(BGS[bgIdx][0]);
      mv.setAttribute("exposure", String(BGS[bgIdx][2]));
      bgBtn.textContent = "Backdrop: " + BGS[bgIdx][1];
    });
  }
  var resetBtn = $("mv-reset");
  if (resetBtn && mv) {
    resetBtn.addEventListener("click", function () {
      mv.cameraOrbit = "32deg 74deg auto";
      mv.cameraTarget = "auto auto auto";
      mv.fieldOfView = "auto";
    });
  }

  /* ------------------------------------------------------------------ ports */
  var PORTS = [
    {
      name: "Greyhaven", sub: "Capital port, customs quay",
      note: "The home harbour: breakwater, customs quay, hillside town with the Anchor & Brass tavern and the foundry. Steam packets clear on the half-tide.",
      views: [["Noon, 400 m", "port_greyhaven_noon"], ["Night", "port_greyhaven_night"], ["North quay", "port_greyhaven_quay"], ["Tavern street", "port_greyhaven_tavern"], ["From the air", "harbour_aerial"]]
    },
    {
      name: "Ironcrag", sub: "Basalt coaling and foundry haven",
      note: "Three blast furnaces under a basalt bluff. Cheap bunker coal and pig iron; it wants salt fish, pit timber, brasswork and the Greyhaven mail.",
      views: [["Noon, 400 m", "port_ironcrag_noon"], ["Night", "port_ironcrag_night"], ["Quay", "port_ironcrag_quay"], ["Market", "ui_market_ironcrag"]]
    },
    {
      name: "Stigsby", sub: "Wooded fishing and timber outpost",
      note: "Smacks off the eastern shoals and a stacked sawmill. Salt fish, naval timber and whale oil; short of coal, sailcloth and iron.",
      views: [["Noon, 400 m", "port_stigsby_noon"], ["The cove", "port_stigsby_cove"], ["Logbook", "ui_logbook_stigsby"], ["Sea chart", "ui_chart_3ports"]]
    }
  ];
  var portTabs = $("port-tabs");
  var portImg = $("port-img");
  var portViews = $("port-views");
  function showView(p, vi) {
    var v = p.views[vi];
    portImg.src = "assets/" + v[1] + ".jpg";
    portImg.alt = p.name + ": " + v[0];
    portViews.querySelectorAll(".chip").forEach(function (c, j) { c.setAttribute("aria-pressed", String(j === vi)); });
  }
  function showPort(i) {
    var p = PORTS[i];
    portTabs.querySelectorAll(".port-tab").forEach(function (t, j) { t.setAttribute("aria-pressed", String(j === i)); });
    $("port-cap").textContent = p.note;
    portViews.innerHTML = "";
    p.views.forEach(function (v, vi) {
      var c = document.createElement("button");
      c.type = "button";
      c.className = "chip";
      c.textContent = v[0];
      c.addEventListener("click", function () { showView(p, vi); });
      portViews.appendChild(c);
    });
    showView(p, 0);
  }
  if (portTabs) {
    PORTS.forEach(function (p, i) {
      var t = document.createElement("button");
      t.type = "button";
      t.className = "port-tab";
      t.innerHTML = "<strong></strong><small></small>";
      t.querySelector("strong").textContent = p.name;
      t.querySelector("small").textContent = p.sub;
      t.addEventListener("click", function () { showPort(i); });
      portTabs.appendChild(t);
    });
    showPort(0);
  }

  /* ------------------------------------------------------------------ before / after */
  document.querySelectorAll("[data-ba]").forEach(function (el) {
    var input = el.querySelector("input");
    var set = function () { el.style.setProperty("--pos", input.value + "%"); };
    input.addEventListener("input", set);
    set();
  });

  /* ------------------------------------------------------------------ wake lab */
  var canvas = $("wake-canvas");
  if (canvas) {
    var ctx = canvas.getContext("2d");
    var W = canvas.width, H = canvas.height;
    var kn = $("kn"), len = $("len");
    var layers = { wave: true, wash: true, lace: true, contact: true };
    var kel = null, pw = null, frame = 0, running = false, visible = true;
    var img = ctx.createImageData(W, H);

    function loadTable(src) {
      return new Promise(function (res, rej) {
        var im = new Image();
        im.onload = function () {
          var c = document.createElement("canvas");
          c.width = W; c.height = H;
          var x = c.getContext("2d");
          /* LUT rows run along the ship from 6.5 L astern (row 0) to 1.5 L ahead (last row),
             columns span 3 L either side. Flip vertically so the bow points up and the wake trails down. */
          x.translate(0, H);
          x.scale(1, -1);
          x.drawImage(im, 0, 0, W, H);
          res(x.getImageData(0, 0, W, H).data);
        };
        im.onerror = rej;
        im.src = src;
      });
    }

    function clamp01(v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

    function draw() {
      if (!kel || !pw) return;
      var v = parseFloat(kn.value), L = parseFloat(len.value);
      var fn = (v * 0.514444) / Math.sqrt(9.81 * L);
      var w12 = clamp01((fn - 0.12) / 0.10), w23 = clamp01((fn - 0.22) / 0.12);
      var gate = Math.pow(clamp01((v - 2) / 8), 1.25);
      var d = img.data;
      var t = frame * 0.06;
      for (var py = 0; py < H; py++) {
        var scroll = Math.sin((py + frame * 2.2) * 0.11);
        for (var px = 0; px < W; px++) {
          var i = (py * W + px) * 4;
          var hr = kel[i] / 127.5 - 1, hg = kel[i + 1] / 127.5 - 1, hb = kel[i + 2] / 127.5 - 1;
          var h = layers.wave ? ((hr * (1 - w12) + hg * w12) * (1 - w23) + hb * w23) : 0;
          var shade = 0.5 + 0.5 * Math.tanh(h * 2.6 * (0.25 + gate));
          var turb = 0.65 + 0.35 * scroll * Math.cos(px * 0.13 - t);
          var wash = layers.wash ? (pw[i] / 255) * gate * turb : 0;
          var lace = layers.lace ? (pw[i + 1] / 255) * Math.pow(gate, 1.15) : 0;
          var cont = layers.contact ? (pw[i + 2] / 255) * gate : 0;
          var r = 0.03 + 0.09 * shade + 0.05 * wash;
          var g = 0.09 + 0.18 * shade + 0.34 * wash;
          var b = 0.15 + 0.25 * shade + 0.39 * wash;
          var foam = clamp01(wash * 1.15 + lace * 0.95 + cont * 0.85);
          d[i] = (r * (1 - foam) + 0.91 * foam) * 255;
          d[i + 1] = (g * (1 - foam) + 0.96 * foam) * 255;
          d[i + 2] = (b * (1 - foam) + 0.98 * foam) * 255;
          d[i + 3] = 255;
        }
      }
      ctx.putImageData(img, 0, 0);
      /* hull silhouette: midships sits 6.5 L from the stern edge, so 1.5/8 from the top after the flip */
      var cx = W / 2, cy = H * (1.5 / 8), hullLen = H / 8, beam = W * (0.085 / 6);
      ctx.fillStyle = "#2a221b";
      ctx.beginPath();
      ctx.moveTo(cx, cy - hullLen * 0.5);
      ctx.quadraticCurveTo(cx + beam * 1.6, cy - hullLen * 0.1, cx + beam * 0.7, cy + hullLen * 0.5);
      ctx.lineTo(cx - beam * 0.7, cy + hullLen * 0.5);
      ctx.quadraticCurveTo(cx - beam * 1.6, cy - hullLen * 0.1, cx, cy - hullLen * 0.5);
      ctx.fill();
      var regime = fn < 0.2 ? "transverse waves" : fn < 0.3 ? "cusp at 19.47 deg" : "long diverging arms";
      $("lab-readout").textContent = v.toFixed(1) + " kn · " + L + " m hull · Fn " + fn.toFixed(2) + " · " + regime;
    }

    function loop() {
      if (!running) return;
      if (visible && !reduceMotion) { frame++; draw(); }
      requestAnimationFrame(loop);
    }

    function updateOut() {
      $("kn-out").textContent = parseFloat(kn.value).toFixed(1) + " kn";
      $("len-out").textContent = len.value + " m";
      if (reduceMotion || !running) draw();
    }
    kn.addEventListener("input", updateOut);
    len.addEventListener("input", updateOut);
    $("layers").addEventListener("click", function (e) {
      var c = e.target.closest(".chip");
      if (!c) return;
      var k = c.dataset.layer;
      layers[k] = !layers[k];
      c.setAttribute("aria-pressed", String(layers[k]));
      draw();
    });

    if ("IntersectionObserver" in window) {
      new IntersectionObserver(function (ents) { visible = ents[0].isIntersecting; }).observe(canvas);
    }

    Promise.all([loadTable("assets/T_IW_KelvinWaveLut.png"), loadTable("assets/T_IW_PropWashFoamLut.png")])
      .then(function (r) { kel = r[0]; pw = r[1]; running = true; updateOut(); draw(); loop(); })
      .catch(function () { $("lab-readout").textContent = "Could not load the wake tables."; });
  }

  /* ------------------------------------------------------------------ sound deck */
  var mixer = $("mixer"), pads = $("pads"), recipe = $("recipe");
  var stems = {}, clips = [];
  var PRESETS = {
    harbour: { harbour_lap_proc: 0.8, slow_lap_loop: 0.5, creak_wooden_hull: 0.35, wind_sea_proc: 0.25 },
    passage: { wind_sea_proc: 0.55, bow_rush_proc: 0.8, hull_wake_loop: 0.6, hull_hiss_proc: 0.45, creak_wooden_hull: 0.3 },
    squall: { gale_proc: 0.75, rain_proc: 0.7, storm_howling_wind: 0.45, bow_wash_loop: 0.6, creak_wooden_hull: 0.5 },
    off: {}
  };

  function setStem(id, vol) {
    var s = stems[id];
    if (!s) return;
    s.slider.value = Math.round(vol * 100);
    s.audio.volume = vol;
    var on = vol > 0;
    if (on && s.audio.paused) s.audio.play().catch(function () {});
    if (!on && !s.audio.paused) s.audio.pause();
    s.row.classList.toggle("on", on);
    s.btn.textContent = on ? "ON" : "OFF";
    s.btn.setAttribute("aria-pressed", String(on));
  }

  function showRecipe(c) {
    recipe.innerHTML = "";
    var b = document.createElement("b");
    b.textContent = c.title;
    recipe.appendChild(b);
    recipe.appendChild(document.createTextNode(" · " + c.bus + " bus · " + c.duration_s.toFixed(1) + " s. " + c.recipe));
  }

  function buildDeck(list) {
    clips = list;
    list.filter(function (c) { return c.category === "stem"; }).forEach(function (c) {
      var row = document.createElement("div");
      row.className = "strip";
      var btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = "OFF";
      btn.setAttribute("aria-pressed", "false");
      btn.setAttribute("aria-label", "Toggle " + c.title);
      var label = document.createElement("div");
      label.innerHTML = "<strong></strong><small></small>";
      label.querySelector("strong").textContent = c.title;
      label.querySelector("small").textContent = c.bus + " · " + c.duration_s.toFixed(0) + " s loop";
      var slider = document.createElement("input");
      slider.type = "range"; slider.min = "0"; slider.max = "100"; slider.value = "0";
      slider.setAttribute("aria-label", c.title + " volume");
      var audio = new Audio();
      audio.loop = true;
      audio.preload = "none";
      audio.src = c.file;
      row.appendChild(btn); row.appendChild(label); row.appendChild(slider);
      mixer.appendChild(row);
      stems[c.id] = { audio: audio, slider: slider, row: row, btn: btn };
      btn.addEventListener("click", function () { setStem(c.id, audio.paused ? 0.6 : 0); showRecipe(c); });
      slider.addEventListener("input", function () { setStem(c.id, slider.value / 100); });
      label.addEventListener("click", function () { showRecipe(c); });
    });
    list.filter(function (c) { return c.category === "sfx"; }).forEach(function (c) {
      var p = document.createElement("button");
      p.type = "button";
      p.className = "pad";
      p.innerHTML = "<strong></strong><small></small>";
      p.querySelector("strong").textContent = c.title;
      p.querySelector("small").textContent = c.bus + " · " + c.duration_s.toFixed(1) + " s";
      p.addEventListener("click", function () {
        var a = new Audio(c.file);
        a.volume = 0.9;
        p.classList.add("playing");
        a.addEventListener("ended", function () { p.classList.remove("playing"); });
        a.play().catch(function () { p.classList.remove("playing"); });
        showRecipe(c);
      });
      pads.appendChild(p);
    });
  }

  if (mixer && pads) {
    fetch("audio/manifest.json")
      .then(function (r) { return r.json(); })
      .then(function (d) {
        d.clips.forEach(function (c) { c.file = c.file.replace(/^audio\//, "audio/"); });
        buildDeck(d.clips);
      })
      .catch(function () { mixer.textContent = "Could not load the sound list."; });
    $("presets").addEventListener("click", function (e) {
      var b = e.target.closest("[data-preset]");
      if (!b) return;
      var p = PRESETS[b.dataset.preset];
      Object.keys(stems).forEach(function (id) { setStem(id, p[id] || 0); });
    });
  }

  /* ------------------------------------------------------------------ gallery */
  var SHOTS = [
    ["brig_broad_bow", "g-wide", "Tall Brig Curlew on a broad reach"],
    ["brig_reefed_harbour", "g-tall", "An NPC brig, reefed, in harbour"],
    ["wake_npc6_aerial", "g-third", "Six ships, six wakes, from the air"],
    ["paddle_steamer", "g-third", "Duchess of Kestrel, wheels turning"],
    ["salvage_tender", "g-third", "Dredger's Hope slews her crane"],
    ["ui_shipyard", "g-half", "The shipyard: four ships to command"],
    ["rowing_gig", "g-half", "A gig crew, mid-stroke"],
    ["brig_broad_stern", "g-third", "Curlew from the quarter"],
    ["port_ironcrag_night", "g-third", "Ironcrag's furnaces at night"],
    ["port_greyhaven_tavern", "g-third", "The Anchor & Brass, street level"],
    ["sheet_waterlines_npcs", "g-half", "Verification sheet: waterlines on every hull"],
    ["sheet_wake_scaling", "g-half", "Verification sheet: wake scales with speed"]
  ];
  var gal = $("gallery-grid");
  var lb = $("lightbox"), lbImg = $("lb-img"), lbCap = $("lb-cap");
  function closeLb() { lb.classList.remove("open"); lbImg.src = ""; }
  if (gal) {
    SHOTS.forEach(function (s) {
      var f = document.createElement("figure");
      f.className = s[1];
      var im = document.createElement("img");
      im.src = "assets/" + s[0] + ".jpg";
      im.alt = s[2];
      im.loading = "lazy";
      var cap = document.createElement("figcaption");
      cap.textContent = s[2];
      im.addEventListener("click", function () { lbImg.src = im.src; lbImg.alt = s[2]; lbCap.textContent = s[2]; lb.classList.add("open"); $("lb-close").focus(); });
      f.appendChild(im); f.appendChild(cap);
      gal.appendChild(f);
    });
    $("lb-close").addEventListener("click", closeLb);
    lb.addEventListener("click", function (e) { if (e.target === lb) closeLb(); });
    document.addEventListener("keydown", function (e) { if (e.key === "Escape" && lb.classList.contains("open")) closeLb(); });
  }
})();
