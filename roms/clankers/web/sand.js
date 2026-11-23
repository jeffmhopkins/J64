// ---- the back garden's sandbox: a falling-sand toy. Mirrors src/core/ck_sand.c (the same tank,
// rules and numbers) and draw_sand in src/n64/draw.c. Sand falls and piles up, water falls and
// lies flat, acid eats sand, clay and water, and clay stays just where it is put. Gravity leans a
// little left and right every ten seconds, so a pond sloshes; the screen itself never turns.
const SAND = {
  W: 156, H: 100, CELL: 2, X: 4, Y: 20, SWAY: 600, LEAN: 0.11, SPREAD: 4,
  EMPTY: 0, SAND: 1, WATER: 2, ACID: 3, CLAY: 4, MAT: 7, MOVED: 8,
  TOOLS: ['Sand', 'Water', 'Acid', 'Clay', 'Rubber', 'Sway', 'Wipe'], ERASE: 4, TILT: 5, CLEAR: 6, SIZES: 3,
};
// the tank, kept while the page is open (as the N64 keeps it until it is switched off)
const sandBox = { cell: new Uint8Array(SAND.W * SAND.H), cur: [{}, {}], lean: 0, swayT: 0, still: false, ready: false, rng: 0x2545f491, tick: 0, fizz: 0, fx: 0, fy: 0 };
function sandHex(c) {
  const base = [0x232a3d, 0xe8c27a, 0x3f8fe8, 0x8cf04a, 0xb8653f], spread = [0, 2, 1, 1, 2];
  let m = c & SAND.MAT; if (m > 4) m = 0;
  const h = base[m]; if (!m) return h;
  const k = 100 + ((c >> 4) - 8) * spread[m] * 2, f = v => Math.min(255, Math.trunc(v * k / 100));
  return f(h >> 16 & 255) << 16 | f(h >> 8 & 255) << 8 | f(h & 255);
}
const sandRadius = size => size <= 0 ? 1 : size === 1 ? 3 : 6;
function sandRules(S) {
  const W = SAND.W, H = SAND.H, C = S.cell;
  const rng = () => { let x = S.rng; x ^= x << 13; x >>>= 0; x ^= x >>> 17; x ^= x << 5; x >>>= 0; S.rng = x; return x; };
  const chance = p => (rng() & 255) < p;
  const mat = (x, y) => x < 0 || x >= W || y < 0 || y >= H ? SAND.CLAY : C[y * W + x] & SAND.MAT;
  const liquid = m => m === SAND.WATER || m === SAND.ACID;
  const move = (x, y, nx, ny, ahead) => { const c = C[y * W + x]; C[ny * W + nx] = ahead ? c | SAND.MOVED : c; C[y * W + x] = 0; };
  const swap = (x, y, nx, ny) => { const c = C[y * W + x]; C[y * W + x] = C[ny * W + nx]; C[ny * W + nx] = c; };
  function sandCell(x, y, side, lean) {
    if (mat(x, y + 1) === 0) {
      if (lean && chance(lean) && mat(x + side, y + 1) === 0) move(x, y, x + side, y + 1, false); else move(x, y, x, y + 1, false);
      return;
    }
    const below = mat(x, y + 1), a = chance(128 + lean) ? side : -side, ma = mat(x + a, y + 1), mb = mat(x - a, y + 1);
    if (ma === 0) { move(x, y, x + a, y + 1, false); return; }
    if (mb === 0) { move(x, y, x - a, y + 1, false); return; }
    if (liquid(below)) { if (chance(120)) swap(x, y, x, y + 1); return; }
    if (liquid(ma)) { if (chance(60)) swap(x, y, x + a, y + 1); return; }
    if (liquid(mb)) { if (chance(60)) swap(x, y, x - a, y + 1); }
  }
  function acidBite(x, y) {
    const k = rng() >>> 8 & 3, nx = x + [0, -1, 1, 0][k], ny = y + [1, 0, 0, -1][k], n = mat(nx, ny);
    if (nx < 0 || ny < 0 || nx >= W || ny >= H) return false;
    if (n !== SAND.SAND && n !== SAND.CLAY && n !== SAND.WATER) return false;
    if (!chance(60)) return false;   // sand, clay and water all go at the same rate
    C[ny * W + nx] = 0; S.fizz++; S.fx = nx; S.fy = ny;
    if (chance(128)) { C[y * W + x] = 0; return true; }
    return false;
  }
  function liquidCell(x, y, m, side, lean, ltr) {
    if (m === SAND.ACID && acidBite(x, y)) return;
    if (mat(x, y + 1) === 0) {
      if (lean && chance(lean) && mat(x + side, y + 1) === 0) move(x, y, x + side, y + 1, false); else move(x, y, x, y + 1, false);
      return;
    }
    if (mat(x - 1, y + 1) && mat(x + 1, y + 1) && mat(x - 1, y) && mat(x + 1, y)) {   // boxed in: the quick way out
      if (lean && mat(x, y - 1) === 0 && mat(x + side, y - 1) === 0 && chance(Math.trunc(lean * 3 / 2))) move(x, y, x + side, y - 1, true);
      return;
    }
    let a = chance(128 + lean) ? side : -side;
    if (mat(x + a, y + 1) === 0) { move(x, y, x + a, y + 1, false); return; }
    if (mat(x - a, y + 1) === 0) { move(x, y, x - a, y + 1, false); return; }
    for (let pass = 0; pass < 2; pass++, a = -a) {   // run sideways; uphill is harder the more gravity leans
      if (a !== side && lean && !chance(256 - lean * 2)) continue;
      let nx = x;
      for (let k = 1; k <= SAND.SPREAD; k++) {
        const tx = x + a * k;
        if (mat(tx, y) !== 0) break;
        nx = tx;
        if (mat(tx, y + 1) === 0) break;
      }
      if (nx !== x) { move(x, y, nx, y, ltr ? nx > x : nx < x); return; }
    }
    if (lean && mat(x, y - 1) === 0 && mat(x + side, y - 1) === 0 && mat(x + side, y) !== 0 && chance(Math.trunc(lean * 3 / 2))) move(x, y, x + side, y - 1, true);
  }
  return {
    rng, chance,
    step() {
      S.tick++; S.fizz = 0;
      if (!S.still) S.swayT = (S.swayT + 1) % SAND.SWAY;
      const want = S.still ? 0 : SAND.LEAN * Math.sin(S.swayT * (2 * Math.PI / SAND.SWAY));
      S.lean += (want - S.lean) * 0.05;
      const side = S.lean < 0 ? -1 : 1, lean = Math.trunc(Math.abs(S.lean) * 256), ltr = (S.tick & 1) === 1;
      for (let y = H - 1; y >= 0; y--) {
        const r = y * W;
        for (let i = 0; i < W; i++) {
          const x = ltr ? i : W - 1 - i, c = C[r + x], m = c & SAND.MAT;
          if (m === 0 || m === SAND.CLAY) continue;
          if (c & SAND.MOVED) { C[r + x] = c & ~SAND.MOVED; continue; }
          if (m === SAND.SAND) sandCell(x, y, side, lean); else liquidCell(x, y, m, side, lean, ltr);
        }
      }
    },
    pour(cx, cy, tool, size) {
      const r = sandRadius(size);
      for (let dy = -r; dy <= r; dy++) for (let dx = -r; dx <= r; dx++) {
        const x = cx + dx, y = cy + dy;
        if (dx * dx + dy * dy > r * r + r || x < 0 || y < 0 || x >= W || y >= H) continue;
        const i = y * W + x;
        if (tool === SAND.ERASE) { C[i] = 0; continue; }
        const m = C[i] & SAND.MAT, solid = tool === 0 || tool === 3;
        if (tool > 3 || (solid && m === tool + 1)) continue;
        if (!solid && liquid(m)) {   // poured into a pond: push the water above up a cell, so the level rises
          if (!chance(64)) continue;
          let top = y - 1;
          while (top >= 0 && liquid(C[top * W + x] & SAND.MAT)) top--;
          if (top < 0 || (C[top * W + x] & SAND.MAT)) continue;
          for (let k = top; k < y; k++) C[k * W + x] = C[(k + 1) * W + x];
          C[i] = (tool + 1) | (rng() & 0xf0);
          continue;
        }
        if (m && !solid) continue;   // sand and clay take over what is under the brush; water and acid never do
        if (!m && tool !== 3 && !chance(64)) continue;   // into thin air, sand, water and acid spray
        C[i] = (tool + 1) | (rng() & 0xf0);
      }
    },
  };
}

function sandScene(house) {
  const S = sandBox, R = sandRules(S), T = { song: 'select', t: 0 };
  T.ps = house.ps.map(h => ({ m: h.m }));
  S.cur.forEach((c, i) => {
    Object.assign(c, { x: SAND.X + SAND.W * SAND.CELL * (i ? 0.7 : 0.3), y: SAND.Y + 30, down: false, armed: false, rep: 0 });
    if (!S.ready || !(c.tool >= 0 && c.tool < SAND.CLEAR)) c.tool = i ? 1 : 0;
    if (!S.ready || !(c.size >= 0 && c.size < SAND.SIZES)) c.size = 1;
  });
  S.ready = true;
  const tw = SAND.W * SAND.CELL, th = SAND.H * SAND.CELL;
  const stroke = (x0, y0, x1, y1, tool, size) => {
    const n = Math.max(1, Math.ceil(Math.hypot(x1 - x0, y1 - y0) / SAND.CELL));
    for (let k = 1; k <= n; k++) R.pour(Math.floor((x0 + (x1 - x0) * k / n - SAND.X) / SAND.CELL), Math.floor((y0 + (y1 - y0) * k / n - SAND.Y) / SAND.CELL), tool, size);
  };
  T.update = function () {
    T.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    T.ps.forEach((p, i) => {
      const c = S.cur[i], sl = p.m.slot;
      const dx = (held(sl, 'right') ? 1 : 0) - (held(sl, 'left') ? 1 : 0), dy = (held(sl, 'down') ? 1 : 0) - (held(sl, 'up') ? 1 : 0);
      c.rep = dx || dy ? c.rep + 1 : 0;
      let sp = c.rep < 8 ? 1 : c.rep < 30 ? 2 : 3.2;   // a tap nudges, holding speeds up
      if (dx && dy) sp *= 0.75;
      c.x = clamp(c.x + dx * sp, SAND.X, SAND.X + tw - 1); c.y = clamp(c.y + dy * sp, SAND.Y, SAND.Y + th - 1);
      if (hit(sl, 'r')) { c.tool = (c.tool + 1) % SAND.TOOLS.length; SND.fx.select(); }
      if (hit(sl, 'l')) { c.tool = (c.tool + SAND.TOOLS.length - 1) % SAND.TOOLS.length; SND.fx.select(); }
      if (hit(sl, 'c')) { c.size = (c.size + 1) % SAND.SIZES; SND.fx.tick(); }
      if (!held(sl, 'a') && !held(sl, 'b')) c.armed = true;   // the A that opened the sandbox pours nothing
      if (!c.armed) return;
      const tool = held(sl, 'b') ? SAND.ERASE : c.tool;
      if (tool === SAND.TILT || tool === SAND.CLEAR) {
        c.down = false;
        if (!hit(sl, 'a')) return;
        if (tool === SAND.TILT) { S.still = !S.still; S.still ? SND.fx.shrink() : SND.fx.power(); }
        else { S.cell.fill(0); SND.fx.whoosh(); for (let k = 0; k < 12; k++) puff(20 + k * 25, SAND.Y + 40 + (k % 3) * 60, 2, 'rgba(255,255,255,0.86)'); }
        return;
      }
      if (!held(sl, 'a') && !held(sl, 'b')) { c.down = false; return; }
      if (!c.down) { c.lx = c.x; c.ly = c.y; c.down = true; }
      stroke(c.lx, c.ly, c.x, c.y, tool, c.size);
      c.lx = c.x; c.ly = c.y;
    });
    R.step();
    if (S.fizz && S.tick % 8 === 0) {   // acid fizzing: a little green puff where it last bit
      if (S.tick % 32 === 0) SND.fx.blow();
      puff(SAND.X + S.fx * SAND.CELL, SAND.Y + S.fy * SAND.CELL - 2, 1, 'rgba(184,255,122,0.8)');
    }
  };
  // the tank: one pixel a cell on a small canvas, drawn at twice the size
  const cv = document.createElement('canvas'); cv.width = SAND.W; cv.height = SAND.H;
  const cg = cv.getContext('2d'), img = cg.createImageData(SAND.W, SAND.H), px = new Uint32Array(img.data.buffer);
  const lut = new Uint32Array(256).map((_, c) => { const h = sandHex(c & ~SAND.MOVED); return 0xff000000 | (h & 255) << 16 | (h >> 8 & 255) << 8 | h >> 16; });
  const swatch = (k, x, y, r) => {
    if (k <= 3) K.circ(x, y, r, hexStr(sandHex((k + 1) | 8 << 4)));
    else if (k === SAND.ERASE) { K.circ(x, y, r, '#fff6e6'); text('x', x, y + 3, 8, '#e04f4f', 'center', null); }
    else if (k === SAND.TILT) {   // gravity's arrow, pointing down the way it leans
      K.circ(x, y, r, '#8fd3ff');
      const a = Math.asin(S.lean / SAND.LEAN * 0.5);
      for (let i = -2; i <= 3; i++) K.circ(x + Math.sin(a) * i * r * 0.22, y + Math.cos(a) * i * r * 0.22, i === 3 ? 2.2 : 1.3, '#1b1426');
    } else { K.circ(x, y, r, '#e04f4f'); text('ALL', x, y + 2.5, 5, '#ffffff', 'center', '#1b1426'); }
  };
  T.draw = function () {
    K.g = g; g.fillStyle = '#5dbb63'; g.fillRect(0, 0, W, H);
    g.fillStyle = '#8a5a3c'; g.fillRect(SAND.X - 4, SAND.Y - 4, tw + 8, th + 8);
    g.fillStyle = '#c98a4b'; g.fillRect(SAND.X - 3, SAND.Y - 3, tw + 6, 2);
    for (let i = 0; i < px.length; i++) px[i] = lut[S.cell[i]];
    cg.putImageData(img, 0, 0);
    const sm = g.imageSmoothingEnabled; g.imageSmoothingEnabled = false;
    g.drawImage(cv, SAND.X, SAND.Y, tw, th);
    g.imageSmoothingEnabled = sm;
    for (let k = 0; k < SAND.TOOLS.length; k++) {
      const x = 30 + k * 44, y = 231;
      swatch(k, x, y, 7);
      if (k === SAND.TILT && S.still) { g.fillStyle = '#e04f4f'; g.fillRect(x - 8, y - 1, 16, 2); }
      T.ps.forEach((p, j) => { if (S.cur[j].tool === k) { g.strokeStyle = p.m.slot ? '#8fd3ff' : '#ffd23f'; g.lineWidth = 2; g.beginPath(); g.arc(x, y, 9 + j * 2, 0, P * 2); g.stroke(); } });
    }
    T.ps.forEach((p, i) => {   // each cursor: a ring the brush's size, and what it pours beside it
      const c = S.cur[i], erase = held(p.m.slot, 'b'), r = (sandRadius(c.size) + 0.5) * SAND.CELL;
      if (c.tool <= SAND.ERASE || erase) {
        g.lineWidth = 1.5;
        g.strokeStyle = 'rgba(27,20,38,0.7)'; g.beginPath(); g.arc(c.x, c.y, r + 2, 0, P * 2); g.stroke();
        g.strokeStyle = erase ? '#fff6e6' : p.m.slot ? '#8fd3ff' : '#ffd23f'; g.beginPath(); g.arc(c.x, c.y, r + 1, 0, P * 2); g.stroke();
      }
      swatch(erase ? SAND.ERASE : c.tool, c.x + r + 6, c.y - r - 6, 4.5);
    });
    drawParticles(0);
    const t = S.cur[lastIn(T.ps)].tool;
    hint(t === SAND.TILT ? (S.still ? 'A sway on   B rub out   L R pick   START leave' : 'A sway off   B rub out   L R pick   START leave')
      : t === SAND.CLEAR ? 'A wipe it all   B rub out   L R pick   START leave' : 'A pour   B rub out   L R pick   C size   START leave', 11);
  };
  return T;
}
