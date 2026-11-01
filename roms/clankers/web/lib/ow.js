// Clankers overworld engine for the mocks. A world = { name, num, nodes, edges, drawGround(g,t), drawLive(g,t), start }.
// Nodes: { id, x, y, kind: 'home'|'level'|'boss'|'shop'|'bonus'|'exit', label, num }
// Edges: [from, to, {ctrl:[x,y], gate: gearsNeeded}] — a quadratic curve from node to node.
const OW = (() => {
  const K = CK, P = Math.PI, FONT = '"Titan One", "Arial Black", Impact, sans-serif';
  function core(world, g, opts = {}) {
    const W = 320, H = 240;
    const byId = Object.fromEntries(world.nodes.map(n => [n.id, n]));
    const ground = document.createElement('canvas'); ground.width = W; ground.height = H;
    let st;
    function reset() {
      st = {
        beaten: new Set(world.start.beaten), at: world.start.at, gears: world.start.gears, bolts: world.start.bolts,
        open: new Set(), walk: null, reveal: [], babies: world.start.beaten.filter(id => byId[id].kind === 'level').length,
        msg: null,
      };
      for (const id of st.beaten) openFrom(id, true);
      openFrom(world.nodes[0].id, true);
    }
    const ekey = e => e[0] + '>' + e[1];
    function openFrom(id, instant) {
      for (const e of world.edges) {
        if (e[0] !== id || st.open.has(ekey(e))) continue;
        const gate = e[2] && e[2].gate;
        if (gate && st.gears < gate) continue;
        if (byId[id].kind !== 'home' && byId[id].kind !== 'shop' && !st.beaten.has(id)) continue;
        st.open.add(ekey(e));
        if (!instant) st.reveal.push({ e, t0: now, dur: world.grid ? Math.max(0.5, poly(e).total / 70) : 1.1 });
      }
    }
    const isOpen = e => st.open.has(ekey(e));
    // points along an edge
    // an edge is a quadratic curve, or (with pts) a straight-segment road through grid points
    const polyCache = new Map();
    function poly(e) {
      if (polyCache.has(e)) return polyCache.get(e);
      const a = byId[e[0]], b = byId[e[1]], pts = [[a.x, a.y], ...((e[2] && e[2].pts) || []), [b.x, b.y]];
      const lens = [0]; for (let i = 1; i < pts.length; i++) lens.push(lens[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
      const r = { pts, lens, total: lens[lens.length - 1] }; polyCache.set(e, r); return r;
    }
    function pt(e, u) {
      if (world.grid) {
        const { pts, lens, total } = poly(e), d = Math.max(0, Math.min(1, u)) * total;
        let i = 1; while (i < lens.length - 1 && lens[i] < d) i++;
        const k = (d - lens[i - 1]) / ((lens[i] - lens[i - 1]) || 1);
        return [pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * k, pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * k];
      }
      const a = byId[e[0]], b = byId[e[1]], c = (e[2] && e[2].ctrl) || [(a.x + b.x) / 2, (a.y + b.y) / 2];
      const v = 1 - u; return [v * v * a.x + 2 * v * u * c[0] + u * u * b.x, v * v * a.y + 2 * v * u * c[1] + u * u * b.y];
    }
    function neighbours(id) {
      const out = [];
      for (const e of world.edges) if (isOpen(e)) {
        if (e[0] === id) out.push([e[1], e, false]);
        if (e[1] === id) out.push([e[0], e, true]);
      }
      return out;
    }
    function route(from, to) {
      const prev = { [from]: null }, q = [from];
      while (q.length) { const c = q.shift(); if (c === to) break; for (const [n, e, rev] of neighbours(c)) if (!(n in prev)) { prev[n] = [c, e, rev]; q.push(n); } }
      if (!(to in prev)) return null;
      const legs = []; let c = to; while (prev[c]) { legs.unshift(prev[c]); c = prev[c][0]; }
      return legs.map(([, e, rev]) => ({ e, rev }));
    }
    function walkTo(id) {
      if (st.walk || id === st.at) return;
      const r = route(st.at, id); if (!r) { st.msg = { text: 'Not open yet', t0: now }; return; }
      st.walk = { legs: r, i: 0, t0: now, dest: id };
    }
    function beat(res) {
      const n = byId[st.at];
      if (st.walk || !['level', 'boss', 'bonus'].includes(n.kind)) { st.msg = { text: 'Walk to a level first', t0: now }; return; }
      if (res) { st.bolts += res.bolts || 0; st.gears = Math.min(world.gearsMax, st.gears + (res.gears || 0)); }
      if (!st.beaten.has(n.id)) {
        st.beaten.add(n.id);
        const got = res ? (res.gears || 0) : n.kind === 'bonus' ? 0 : n.kind === 'boss' ? 0 : 2;
        if (!res) { st.gears = Math.min(world.gearsMax, st.gears + got); st.bolts += 20 + (n.kind === 'boss' ? 30 : 0); }
        if (n.kind === 'level' || n.kind === 'boss') st.babies++;
        st.msg = { text: n.kind === 'boss' ? (world.bossMsg || 'Fixed! Path to World ' + (world.num + 1) + ' open') : (got ? '+' + got + (got > 1 ? ' golden gears' : ' golden gear') : 'Level clear!'), t0: now };
      } else if (res && res.gears) st.msg = { text: '+' + res.gears + (res.gears > 1 ? ' golden gears' : ' golden gear'), t0: now };
      openFrom(n.id, false);
      for (const id of st.beaten) openFrom(id, false);   // gear gates may now open
    }
    // --- drawing ---
    function road(e, upto) {
      if (world.grid) return gridRoad(e, upto, true);
      const steps = 28, cut = Math.max(0, Math.min(1, upto));
      g.strokeStyle = world.roadEdge || 'rgba(60,30,20,0.35)'; g.lineWidth = 9; g.lineCap = 'round'; g.beginPath();
      for (let i = 0; i <= steps * cut; i++) { const [x, y] = pt(e, i / steps); i ? g.lineTo(x, y) : g.moveTo(x, y); } g.stroke();
      g.strokeStyle = world.road || '#ecc791'; g.lineWidth = 6; g.stroke();
      const n = Math.floor(steps * cut);
      for (let i = 1; i < n; i += 2) { const [x, y] = pt(e, i / steps); K.circ(x, y, 1.3, world.roadDot || '#fff3d6'); }
    }
    function gridRoad(e, upto, open) {
      const { pts, lens, total } = poly(e), cut = Math.max(0, Math.min(1, upto)) * total;
      const seg = (w, c, dash) => {
        g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'square'; g.lineJoin = 'miter'; g.setLineDash(dash || []);
        g.beginPath(); g.moveTo(pts[0][0], pts[0][1]);
        for (let i = 1; i < pts.length; i++) {
          if (lens[i] <= cut) g.lineTo(pts[i][0], pts[i][1]);
          else { const k = (cut - lens[i - 1]) / (lens[i] - lens[i - 1]); g.lineTo(pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * k, pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * k); break; }
        }
        g.stroke(); g.setLineDash([]);
      };
      if (!open) { seg(8, world.roadLockedEdge || 'rgba(60,30,20,0.25)'); seg(4, world.roadLocked || 'rgba(255,240,215,0.35)', [3, 3]); return; }
      seg(10, world.roadEdge || '#7a4a2c'); seg(7, world.road || '#f2d7a6');
      // stitches across the road every half tile, like paving
      g.fillStyle = world.roadDot || '#d9b77f';
      for (let d = 4; d < cut; d += 8) {
        const [x, y] = pt(e, d / total), [x2, y2] = pt(e, Math.min(1, (d + 1) / total));
        if (Math.abs(x2 - x) > Math.abs(y2 - y)) g.fillRect(x - 0.5, y - 3, 1, 6); else g.fillRect(x - 3, y - 0.5, 6, 1);
      }
      if (upto < 1 && upto > 0) { const [x, y] = pt(e, upto); for (let i = 0; i < 3; i++) K.circ(x + (i - 1) * 3, y - 2 - i, 2.2, 'rgba(255,245,225,0.8)'); }
    }
    function pad(n, t) {
      const beaten = st.beaten.has(n.id), reachable = isReachable(n.id), here = st.at === n.id && !st.walk;
      if (n.kind === 'home') { world.drawHome(g, n, t); return; }
      if (n.kind === 'shop') { drawShop(n, t, reachable); return; }
      if (n.kind === 'exit') { world.drawExit(g, n, t, reachable); return; }
      if (n.kind === 'boss') { world.drawBoss(g, n, t, beaten, reachable); }
      const r = n.kind === 'boss' ? 10 : 8;
      if (world.grid) { gridPad(n, t, beaten, reachable, r); return; }
      g.save(); g.globalAlpha = reachable ? 1 : 0.45;
      K.circ(n.x, n.y + 2, r + 1, 'rgba(30,15,20,0.35)');
      K.circ(n.x, n.y, r + 1, '#2b2230');
      const fill = n.kind === 'bonus' ? '#ffd23f' : beaten ? '#49d17a' : reachable ? (Math.sin(t * 5) > 0 ? '#ffb63f' : '#ff9a3f') : '#8c8496';
      K.circ(n.x, n.y, r, fill); K.circ(n.x, n.y - 1.5, r * 0.62, 'rgba(255,255,255,0.25)');
      if (n.kind === 'bonus') K.gear(n.x, n.y, r * 0.7, 8, t, '#b8860b', fill);
      else {
        g.font = `${n.kind === 'boss' ? 9 : 8}px ${FONT}`; g.textAlign = 'center'; g.textBaseline = 'middle';
        g.fillStyle = '#2b2230'; g.fillText(n.kind === 'boss' ? '!' : String(n.num), n.x, n.y + 0.5); g.textBaseline = 'alphabetic';
      }
      g.restore();
      if (beaten && n.kind !== 'bonus') {
        // flag and the rescued baby bot waving beside it
        g.fillStyle = '#efe7da'; g.fillRect(n.x + r - 1, n.y - 18, 1.5, 16);
        g.fillStyle = '#49d17a'; g.beginPath(); g.moveTo(n.x + r + 0.5, n.y - 18); g.lineTo(n.x + r + 8 + Math.sin(t * 6) * 1, n.y - 15); g.lineTo(n.x + r + 0.5, n.y - 12); g.fill();
        K.baby(n.x - r - 4, n.y + 3, 0.045, n.babyHex || 0xFF8FC7, { t: t + n.x, armR: -150 + Math.sin(t * 7 + n.x) * 20 });
      }
      if (n.gate && !reachable) {
        g.font = `7px ${FONT}`; g.fillStyle = '#ffd23f'; g.textAlign = 'center';
        g.fillText(n.gate + ' gears', n.x, n.y + r + 9);
      }
    }
    function gridPad(n, t, beaten, reachable, r) {
      const sz = n.kind === 'boss' ? 16 : 14, x = n.x - sz / 2, y = n.y - sz / 2;
      K.rr(x - 1, y + 1, sz + 2, sz + 2, 3, 'rgba(30,15,20,0.4)');
      K.rr(x - 1, y - 1, sz + 2, sz + 2, 3, '#2b2230');
      const fill = n.kind === 'bonus' ? (reachable ? '#ffd23f' : '#8c8496') : beaten ? '#49d17a' : reachable ? '#ff9a3f' : '#8c8496';
      K.rr(x, y, sz, sz, 2, fill); K.rr(x + 1.5, y + 1.5, sz - 3, sz / 2 - 2, 1.5, 'rgba(255,255,255,0.28)');
      if (reachable && !beaten && n.kind !== 'bonus' && Math.sin(t * 5) > 0) { g.strokeStyle = '#fff6d8'; g.lineWidth = 1; g.strokeRect(x - 2.5, y - 2.5, sz + 5, sz + 5); }
      g.textAlign = 'center'; g.textBaseline = 'middle';
      if (n.kind === 'bonus') K.gear(n.x, n.y, 5, 8, t, '#8a5d00', fill);
      else if (!reachable && n.kind === 'level') { K.rr(n.x - 3.5, n.y - 1, 7, 5.5, 1, '#5d566a'); K.arc(n.x, n.y - 1.5, 2.4, P, 0, 1.5, '#5d566a'); }
      else { g.font = `${n.kind === 'boss' ? 10 : 9}px ${FONT}`; g.fillStyle = '#2b2230'; g.fillText(n.kind === 'boss' ? '!' : String(n.num), n.x, n.y + 1); }
      g.textBaseline = 'alphabetic';
      if (beaten && n.kind !== 'bonus') {
        g.fillStyle = '#efe7da'; g.fillRect(x + sz - 1, y - 9, 1.5, 10);
        g.fillStyle = '#49d17a'; g.beginPath(); g.moveTo(x + sz + 0.5, y - 9); g.lineTo(x + sz + 7 + Math.sin(t * 6), y - 6.5); g.lineTo(x + sz + 0.5, y - 4); g.fill();
      }
      if (n.gate && !reachable) { g.font = `7px ${FONT}`; g.fillStyle = '#ffd23f'; g.textAlign = 'center'; g.fillText(n.gate + ' gears', n.x, y + sz + 8); }
    }
    function drawShop(n, t, ok) {
      g.save(); g.globalAlpha = ok ? 1 : 0.5;
      K.rr(n.x - 11, n.y - 9, 22, 14, 2, '#7a5aa6'); g.fillStyle = '#5b3f86';
      g.beginPath(); g.moveTo(n.x - 14, n.y - 8); g.lineTo(n.x, n.y - 19); g.lineTo(n.x + 14, n.y - 8); g.fill();
      K.rr(n.x - 4, n.y - 3, 8, 8, 1, '#2b2230');
      K.gear(n.x, n.y - 13, 4, 8, t, '#ffd23f', '#5b3f86');
      g.restore();
    }
    function isReachable(id) { return id === st.at || !!route(st.at, id); }
    function heroes(t) {
      let x, y, moving = false, dir = 1;
      const here = byId[st.at];
      if (st.walk) {
        const w = st.walk, leg = w.legs[w.i], dur = world.grid ? Math.max(0.3, poly(leg.e).total / 75) : 0.75, u = Math.min(1, (now - w.t0) / dur);
        const uu = leg.rev ? 1 - u : u;
        [x, y] = pt(leg.e, uu); const [x2] = pt(leg.e, leg.rev ? Math.max(0, uu - 0.05) : Math.min(1, uu + 0.05));
        dir = x2 >= x ? 1 : -1; moving = true;
        if (u >= 1) { st.at = leg.rev ? leg.e[0] : leg.e[1]; w.i++; w.t0 = now; if (w.i >= w.legs.length) st.walk = null; }
      } else { x = here.x; y = here.y - 6; }
      // rescued babies trail behind (on the map they bob in a little line)
      const nb = Math.min(st.babies, 4);
      for (let i = 0; i < nb; i++) {
        const hop = Math.abs(Math.sin(t * 6 + i)) * (moving ? 2 : 0.6);
        K.baby(x - dir * (16 + i * 6), y - 2 - hop, 0.032, world.babyHexes[i % world.babyHexes.length], { t: t + i * 0.7 });
      }
      const s = 0.062, party = opts.party || [{ hero: 'robot', hex: 0x5AD1FF }, { hero: 'ostrich', hex: 0xFFD23F }];
      party.forEach((m, i) => {
        const off = party.length === 1 ? 0 : (i === 0 ? -6 : 6), tt = t + i * 0.2;
        if (moving) {
          const ox = party.length === 1 ? 0 : (i === 0 ? -5 : 4) * dir;
          if (m.hero === 'robot') K.robotSide(x + ox, y, s, m.hex, Object.assign(K.pose('robot', 'run', tt), { dir }));
          else K.ostrichSide(x + ox, y - 1, s * 0.95, m.hex, Object.assign(K.pose('ostrich', 'run', tt), { dir }));
        } else {
          if (m.hero === 'robot') K.robot(x + off, y, s, m.hex, { t: tt, bob: Math.sin(tt * 3) * 3 });
          else K.ostrich(x + off, y - 1, s * 0.95, m.hex, { t: tt, bob: Math.abs(Math.sin(tt * 2.6)) * 8 });
        }
      });
    }
    function banner(text, x, y, w, align) {
      K.rr(x, y, w, 15, 4, 'rgba(27,20,38,0.82)');
      g.font = `9px ${FONT}`; g.textAlign = align || 'left'; g.fillStyle = '#fff6d8';
      g.fillText(text, align === 'center' ? x + w / 2 : x + 6, y + 11);
    }
    function hud(t) {
      banner(`WORLD ${world.num} · ${world.name.toUpperCase()}`, 6, 6, 150);
      // bolts and golden gears
      K.rr(232, 6, 82, 15, 4, 'rgba(27,20,38,0.82)');
      K.nut(242, 13.5, 4, t, '#ffd23f'); g.font = `9px ${FONT}`; g.textAlign = 'left'; g.fillStyle = '#fff6d8';
      g.fillText('×' + st.bolts, 248, 17);
      K.gear(280, 13.5, 4.5, 8, t * 0.7, '#ffd23f', '#3a2a10'); g.fillText(st.gears + '/' + world.gearsMax, 286, 17);
      // the sign for where you are
      const n = st.walk ? null : byId[st.at];
      let text = '...';
      if (n) {
        if (n.kind === 'level') text = `${world.num}-${n.num}  ${n.label.toUpperCase()}`;
        else text = n.label.toUpperCase();
      }
      const w = 172; K.rr((W - w) / 2, 214, w, 20, 5, 'rgba(27,20,38,0.86)');
      g.font = `9px ${FONT}`; g.textAlign = 'center'; g.fillStyle = '#fff6d8'; g.fillText(text, W / 2, 227);
      if (n && n.kind !== 'home') {
        const done = st.beaten.has(n.id);
        K.circ((W - w) / 2 + w - 12, 224, 6, '#3a8de0'); g.font = `7px ${FONT}`; g.fillStyle = '#fff'; g.fillText('A', (W - w) / 2 + w - 12, 226.5);
        if (done && n.kind === 'level') { g.font = `7px ${FONT}`; g.fillStyle = '#49d17a'; g.fillText('✓', (W - w) / 2 + 12, 227); }
      }
      if (st.msg && now - st.msg.t0 < 1.8) {
        const k = Math.min(1, (now - st.msg.t0) * 6), wd = 150;
        g.save(); g.globalAlpha = Math.min(1, (1.8 - (now - st.msg.t0)) * 3);
        K.rr((W - wd) / 2, 30 - (1 - k) * 8, wd, 18, 5, '#ffd23f');
        g.font = `9px ${FONT}`; g.fillStyle = '#2b2230'; g.textAlign = 'center'; g.fillText(st.msg.text, W / 2, 42 - (1 - k) * 8);
        g.restore();
      }
    }
    let now = 0;
    function frame(t) {
      now = t;
      g.drawImage(ground, 0, 0);
      world.drawLive(g, t, st);
      // locked edges as faint ghosts so you can see where the path will go
      if (world.grid) { for (const e of world.edges) if (!isOpen(e)) gridRoad(e, 1, false); }
      else for (const e of world.edges) if (!isOpen(e)) {
        for (let i = 2; i < 27; i += 3) { const [x, y] = pt(e, i / 28); K.circ(x, y, 0.9, 'rgba(255,255,255,0.18)'); }
      }
      // roads
      for (const e of world.edges) {
        if (!isOpen(e)) continue;
        const rv = st.reveal.find(r => r.e === e);
        road(e, rv ? (t - rv.t0) / rv.dur : 1);
      }
      st.reveal = st.reveal.filter(r => t - r.t0 < r.dur);
      const order = world.nodes.slice().sort((a, b) => a.y - b.y);
      for (const n of order) pad(n, t);
      heroes(t);
      world.drawFront && world.drawFront(g, t, st);
      hud(t);
    }
    function step(dirKey) {
      // the stick: go to the open neighbour that best matches the pushed direction
      const v = { ArrowLeft: [-1, 0], ArrowRight: [1, 0], ArrowUp: [0, -1], ArrowDown: [0, 1] }[dirKey];
      if (!v || st.walk) return false;
      const here = byId[st.at]; let best = null, bs = 0.3;
      for (const [id] of neighbours(st.at)) {
        const n = byId[id], dx = n.x - here.x, dy = n.y - here.y, l = Math.hypot(dx, dy) || 1, sc = (dx * v[0] + dy * v[1]) / l;
        if (sc > bs) { bs = sc; best = id; }
      }
      if (best) { walkTo(best); return true; }
      return false;
    }
    function init() { const gg = ground.getContext('2d'); const old = K.g; K.g = gg; world.drawGround(gg); K.g = old; reset(); }
    return {
      init, frame, step, beat, walkTo, reset,
      get st() { return st; }, set st(v) { st = v; },
      here: () => (st.walk ? null : byId[st.at]), byId,
      say: text => { st.msg = { text, t0: now }; },
      walking: () => !!st.walk,
    };
  }
  function run(world, cv, ui) {
    const g = cv.getContext('2d'), W = 320, H = 240;
    const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches;
    const m = core(world, g);
    const { frame, step, beat, walkTo, reset } = m;
    // input
    cv.addEventListener('click', ev => {
      const r = cv.getBoundingClientRect(), x = (ev.clientX - r.left) / r.width * W, y = (ev.clientY - r.top) / r.height * H;
      let best = null, bd = 18 * 18;
      for (const n of world.nodes) { const d = (n.x - x) ** 2 + (n.y - y) ** 2; if (d < bd) { bd = d; best = n; } }
      if (best) walkTo(best.id);
    });
    window.addEventListener('keydown', e => {
      if (e.key.startsWith('Arrow')) { step(e.key); e.preventDefault(); }
      if (e.key === 'a' || e.key === 'A' || e.key === 'Enter') beat();
    });
    ui.beat.addEventListener('click', () => beat());
    ui.reset.addEventListener('click', () => { reset(); });
    for (const b of ui.dirs) b.addEventListener('click', () => step(b.dataset.dir));
    // go
    function boot() {
      m.init(); K.g = g;
      const t0 = performance.now();
      let failed = null;
      const safe = tt => {
        if (failed) return;
        try { K.g = g; frame(tt); }
        catch (err) {
          failed = err; console.error(err);
          g.fillStyle = 'rgba(20,10,20,0.85)'; g.fillRect(10, 90, 300, 60);
          g.fillStyle = '#ffd23f'; g.font = '10px sans-serif'; g.textAlign = 'left';
          g.fillText('Map error (please screenshot this):', 18, 108);
          g.fillStyle = '#fff'; g.fillText(String(err && err.message || err).slice(0, 60), 18, 124);
          g.fillText(String(err && err.stack || '').split('\n')[1] || '', 18, 140);
        }
      };
      const loop = n => { safe((n - t0) / 1000); requestAnimationFrame(loop); };
      if (reduce) { const tick = () => safe((performance.now() - t0) / 1000); tick(); setInterval(tick, 250); }
      else requestAnimationFrame(loop);
    }
    (document.fonts && document.fonts.load ? document.fonts.load('9px "Titan One"').catch(() => {}) : Promise.resolve()).then(boot, boot);
  }
  return { run, core };
})();
