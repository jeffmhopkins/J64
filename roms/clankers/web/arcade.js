// ---- the arcade room's pinball table and Bug Blaster cabinet, and fishing in the yard's pond.
// Mirrors src/core/ck_arcade.c (the same tables, numbers and rules) and the drawing in draw.c.
const AHX = h => '#' + h.toString(16).padStart(6, '0');
const aDot = (x, y, r, c, a = 1) => { g.globalAlpha = a; K.circ(x, y, r, typeof c === 'number' ? AHX(c) : c); g.globalAlpha = 1; };
const aBox = (x, y, w, h, c, a = 1) => { g.globalAlpha = a; g.fillStyle = typeof c === 'number' ? AHX(c) : c; g.fillRect(x, y, w, h); g.globalAlpha = 1; };
const aLine = (x0, y0, x1, y1, w, c, a = 1) => { g.globalAlpha = a; g.strokeStyle = typeof c === 'number' ? AHX(c) : c; g.lineWidth = w; g.lineCap = 'butt'; g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke(); g.globalAlpha = 1; };
const aRing = (x, y, r, c) => { g.strokeStyle = AHX(c); g.lineWidth = 1.5; g.beginPath(); g.arc(x, y, r - 0.75, 0, P * 2); g.stroke(); };
const anyHeld = b => held(0, b) || held(1, b);
function scoreBox(x, y, label, v, col) { panel(x, y, 58, 30); text(label, x + 29, y + 11, 7, '#d9d2e6', 'center', null); text(String(v), x + 29, y + 26, 10, col, 'center', null); }
function gameOver(score, best) { rimBanner('GAME OVER', 14, 70, 34); hint('Score ' + score + '   Best ' + best, 124); hint('A play again', 142); }

// ---- pinball
const PIN = {
  R: 4, FLIP_LEN: 27, BALLS: 3, REST: 0.5, UP: -0.45, LANE_X: 241, LANE_Y: 227,
  walls: [[70, 168, 70, 44], [70, 44, 98, 16], [98, 16, 222, 16], [222, 16, 250, 44], [250, 44, 250, 232], [232, 232, 250, 232], [232, 232, 232, 76], [70, 168, 128, 206], [232, 168, 192, 206]],
  gate: [250, 58, 232, 76],   // over the launch lane: the ball only goes out through it
  bumps: [[124, 72, 11, 100], [196, 72, 11, 100], [160, 104, 11, 100], [96, 136, 6, 50], [206, 136, 6, 50]],
  lights: [[122, 32], [160, 32], [198, 32]],
  pivot: [[128, 206], [192, 206]],
};
const pinFlipper = (side, a) => { const [x0, y0] = PIN.pivot[side]; return [x0, y0, x0 + (side ? -1 : 1) * Math.cos(a) * PIN.FLIP_LEN, y0 + Math.sin(a) * PIN.FLIP_LEN]; };
function pinballScene(house) {
  const S = { song: 'select', t: 0, best: 0 };
  const newBall = () => { S.x = PIN.LANE_X; S.y = PIN.LANE_Y; S.vx = S.vy = 0; };
  const reset = () => { S.balls = PIN.BALLS; S.score = 0; S.flip = [PIN.REST, PIN.REST]; S.flipW = [0, 0]; S.flash = PIN.bumps.map(() => 0); S.lit = 0; S.litT = 0; S.over = false; S.overT = 0; newBall(); };
  reset();
  // push the ball out of a fat line and bounce it; a flipper turning w radians a frame flicks it
  function seg(ax, ay, bx, by, rr, e, side, w) {
    const dx = bx - ax, dy = by - ay, len2 = dx * dx + dy * dy;
    const t = len2 > 0 ? clamp(((S.x - ax) * dx + (S.y - ay) * dy) / len2, 0, 1) : 0;
    const cx = ax + dx * t, cy = ay + dy * t; let nx = S.x - cx, ny = S.y - cy; const d = Math.hypot(nx, ny);
    if (d >= PIN.R + rr || d < 0.0001) return;
    nx /= d; ny /= d; S.x = cx + nx * (PIN.R + rr); S.y = cy + ny * (PIN.R + rr);
    let svx = 0, svy = 0;
    if (w) { const rx = cx - ax, ry = cy - ay; if (side === 0) { svx = -w * ry; svy = w * rx; } else { svx = w * ry; svy = -w * rx; } }
    const vn = (S.vx - svx) * nx + (S.vy - svy) * ny;
    if (vn < 0) { S.vx -= (1 + e) * vn * nx; S.vy -= (1 + e) * vn * ny; }
  }
  S.update = function () {
    S.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    if (S.over) { if (++S.overT > 60 && anyHit('a')) { reset(); SND.fx.confirm(); } return; }
    const hold = [anyHeld('left') || anyHeld('b') || anyHeld('l'), anyHeld('right') || anyHeld('a') || anyHeld('r')], was = [...S.flip];
    for (let s = 0; s < 2; s++) {
      S.flip[s] = hold[s] ? Math.max(PIN.UP, S.flip[s] - 0.32) : Math.min(PIN.REST, S.flip[s] + 0.18);
      if (hold[s] && was[s] === PIN.REST) SND.fx.clank();
      S.flipW[s] = S.flip[s] - was[s];
    }
    if (S.x > 232 && S.y > 218 && Math.abs(S.vy) < 1 && anyHit('a')) { S.vy = -8.6 - Math.random() * 0.4; SND.fx.spring(); }
    for (let sub = 0; sub < 4; sub++) {
      S.vy += 0.14 / 4; S.x += S.vx / 4; S.y += S.vy / 4;
      for (const w of PIN.walls) seg(w[0], w[1], w[2], w[3], 1, 0.5, 0, 0);
      if (S.vy > 0) seg(...PIN.gate, 1, 0.4, 0, 0);
      for (let s = 0; s < 2; s++) { const [x0, y0, x1, y1] = pinFlipper(s, was[s] + S.flipW[s] * (sub + 1) / 4); seg(x0, y0, x1, y1, 3, 0.3, s, S.flipW[s]); }
      PIN.bumps.forEach(([bx, by, r, score], k) => {
        let nx = S.x - bx, ny = S.y - by; const d = Math.hypot(nx, ny);
        if (d >= r + PIN.R || d < 0.0001) return;
        nx /= d; ny /= d; S.x = bx + nx * (r + PIN.R); S.y = by + ny * (r + PIN.R);
        const vn = S.vx * nx + S.vy * ny; if (vn < 0) { S.vx -= 2 * vn * nx; S.vy -= 2 * vn * ny; }
        const kick = score >= 100 ? 2.6 : 1.8; S.vx += nx * kick; S.vy += ny * kick;
        if (S.flash[k] < 8) { S.score += score; (score >= 100 ? SND.fx.boing : SND.fx.hop)(); sparks(bx + nx * r, by + ny * r, 4, '#ffd23f'); }
        S.flash[k] = 12;
      });
      const sp = Math.hypot(S.vx, S.vy); if (sp > 9) { S.vx *= 9 / sp; S.vy *= 9 / sp; }
    }
    S.flash = S.flash.map(f => Math.max(0, f - 1));
    PIN.lights.forEach(([lx, ly], k) => {
      if (S.lit & (1 << k) || Math.hypot(S.x - lx, S.y - ly) > 8) return;
      S.lit |= 1 << k; S.score += 250; SND.fx.star();
      if (S.lit === 7) { S.score += 1000; S.lit = 0; S.litT = 90; SND.fx.flag(); confetti(160, 30, 30); }
    });
    if (S.litT > 0) S.litT--;
    if (S.y > 250) { SND.fx.ouch(); if (--S.balls <= 0) { S.over = true; S.overT = 0; S.best = Math.max(S.best, S.score); } else newBall(); }
  };
  S.draw = function () {
    K.g = g;
    aBox(0, 0, W, H, 0x2B2240);
    for (let k = 0; k < 24; k++) aDot((k * 53) % 320, (k * 37) % 240, 1, 0x5A4F78);
    aBox(70, 16, 180, H - 16, 0x1B2A4A);
    for (let y = 30; y < H; y += 20) aBox(72, y, 158, 1, 0x22355C);
    for (const w of PIN.walls) aLine(w[0], w[1], w[2], w[3], 3, 0xC9C4E0);
    aLine(...PIN.gate, 2, 0xFFD23F);
    PIN.lights.forEach(([x, y], k) => { const on = (S.lit & (1 << k)) || (S.litT > 0 && Math.floor(S.litT / 6) % 2); aDot(x, y, 5, on ? 0xFFD23F : 0x3A3650); aRing(x, y, 6, 0x8F8AA8); });
    const bcol = [0xFF5A5A, 0x5AD1FF, 0xC28BFF, 0x49D17A, 0x49D17A];
    PIN.bumps.forEach(([x, y, r0], k) => { const r = r0 + (S.flash[k] > 0 ? 2 : 0); aDot(x, y, r, S.flash[k] > 0 ? 0xFFFFFF : bcol[k]); aDot(x, y, r * 0.55, 0x1B1426); aDot(x, y, r * 0.3, bcol[k]); });
    for (let s = 0; s < 2; s++) { const [x0, y0, x1, y1] = pinFlipper(s, S.flip[s]); aLine(x0, y0, x1, y1, 6, 0xFFD23F); aDot(x0, y0, 4, 0xFF8A3D); }
    if (!S.over) { aDot(S.x, S.y, PIN.R, 0xE6E6F0); aDot(S.x - 1.2, S.y - 1.2, 1.4, 0xFFFFFF); }
    drawParticles(0);
    scoreBox(6, 20, 'SCORE', S.score, '#ffd23f'); scoreBox(6, 56, 'BALLS', S.balls, '#fff6d8'); scoreBox(256, 20, 'BEST', S.best, '#49d17a');
    if (S.litT > 0) rimBanner('BONUS!', 10, 120, 22);
    if (S.over) { gameOver(S.score, S.best); return; }
    hint(S.x > 232 && S.y > 218 ? 'A launch!   START leave' : 'B left flipper   A right flipper   START leave', H - 12);
  };
  return S;
}

// ---- Bug Blaster
const BLAST = { COLS: 8, ROWS: 4, DX: 24, DY: 18, Y: 216, BOMBS: 6, SHIELDS: [52, 142, 232], SW: 6, SH: 3, CELL: 6, SY: 180 };
function blastScene(house) {
  const B = BLAST, S = { song: 'select', t: 0, best: 0 };
  const bugAt = k => [S.bx + (k % B.COLS) * B.DX, S.by + Math.floor(k / B.COLS) * B.DY];
  const wave = () => { S.bugs = new Array(B.COLS * B.ROWS).fill(true); S.bx = 76; S.by = 40 + Math.min(S.wave, 5) * 6; S.dir = 1; S.stepT = 0; S.bombs = []; S.shots = [null, null]; S.waveT = 90; };
  const reset = () => { S.lives = 3; S.score = 0; S.wave = 0; S.over = false; S.overT = 0; S.frame = 0; S.gun = [140, 180]; S.inv = [0, 0]; S.shield = B.SHIELDS.map(() => new Array(B.SW * B.SH).fill(1)); wave(); };
  reset();
  const shieldHit = (x, y) => {
    for (let s = 0; s < B.SHIELDS.length; s++) {
      const cx = Math.floor((x - B.SHIELDS[s]) / B.CELL), cy = Math.floor((y - B.SY) / B.CELL);
      if (cx < 0 || cy < 0 || cx >= B.SW || cy >= B.SH || !S.shield[s][cy * B.SW + cx]) continue;
      S.shield[s][cy * B.SW + cx] = 0; return true;
    }
    return false;
  };
  const end = () => { S.over = true; S.overT = 0; S.best = Math.max(S.best, S.score); SND.fx.ouch(); };
  S.update = function () {
    S.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    if (S.over) { if (++S.overT > 60 && anyHit('a')) { reset(); SND.fx.confirm(); } return; }
    if (S.waveT > 0) S.waveT--;
    house.ps.forEach((h, i) => {
      const sl = h.m.slot;
      S.gun[i] = clamp(S.gun[i] + ((held(sl, 'right') ? 1 : 0) - (held(sl, 'left') ? 1 : 0)) * 2.4, 14, W - 14);
      if (S.inv[i] > 0) S.inv[i]--;
      if ((hit(sl, 'a') || hit(sl, 'b')) && !S.shots[i] && S.waveT <= 0) { S.shots[i] = { x: S.gun[i], y: B.Y - 10 }; SND.fx.blow(); }
      const sh = S.shots[i]; if (!sh) return;
      sh.y -= 6;
      if (sh.y < 10 || shieldHit(sh.x, sh.y)) { S.shots[i] = null; return; }
      for (let k = 0; k < S.bugs.length; k++) {
        if (!S.bugs[k]) continue;
        const [bx, by] = bugAt(k);
        if (Math.abs(sh.x - bx) > 8 || Math.abs(sh.y - by) > 7) continue;
        S.bugs[k] = false; S.shots[i] = null; S.score += (B.ROWS - Math.floor(k / B.COLS)) * 10; SND.fx.pop();
        sparks(bx, by, 6, ['#ff5a5a', '#c28bff', '#49d17a', '#49d17a'][Math.floor(k / B.COLS)]);
        break;
      }
    });
    if (!S.bugs.some(Boolean)) { S.wave++; SND.fx.flag(); confetti(160, 40, 30); wave(); return; }
    const alive = S.bugs.filter(Boolean).length;
    if (S.waveT <= 0 && ++S.stepT >= 3 + Math.floor(alive / 3) - Math.min(S.wave, 3)) {
      S.stepT = 0; S.frame ^= 1;
      let lo = 999, hi = -999, low = 0;
      S.bugs.forEach((a, k) => { if (!a) return; const [x, y] = bugAt(k); lo = Math.min(lo, x); hi = Math.max(hi, x); low = Math.max(low, y); });
      if ((S.dir > 0 && hi + 4 > W - 12) || (S.dir < 0 && lo - 4 < 12)) { S.dir = -S.dir; S.by += 8; } else S.bx += S.dir * 4;
      (S.frame ? SND.fx.step : SND.fx.tick)();
      if (low + 8 >= B.SY) { end(); return; }
    }
    if (S.waveT <= 0 && Math.random() < 0.02 + S.wave * 0.006 && S.bombs.length < B.BOMBS) {
      const col = Math.floor(Math.random() * B.COLS);
      for (let r = B.ROWS - 1; r >= 0; r--) { const k = r * B.COLS + col; if (!S.bugs[k]) continue; const [x, y] = bugAt(k); S.bombs.push({ x, y: y + 6 }); break; }
    }
    S.bombs = S.bombs.filter(bm => {
      bm.y += 1.6 + S.wave * 0.15;
      if (bm.y > H || shieldHit(bm.x, bm.y)) return false;
      for (let i = 0; i < house.ps.length; i++) {
        if (S.inv[i] > 0 || Math.abs(bm.x - S.gun[i]) > 10 || Math.abs(bm.y - B.Y) > 7) continue;
        S.inv[i] = 120; SND.fx.clank(); puff(S.gun[i], B.Y, 8, '#ffd23f');
        if (--S.lives <= 0) end();
        return false;
      }
      return true;
    });
  };
  function bug(row, x, y, f) {
    const col = row === 0 ? 0xFF5A5A : row === 1 ? 0xC28BFF : 0x49D17A, lg = f ? 2 : -2;
    aBox(x - 8, y + 1 + lg * 0.5, 3, 4, col); aBox(x + 5, y + 1 - lg * 0.5, 3, 4, col);
    aBox(x - 5 + lg, y + 4, 2, 4, col); aBox(x + 3 - lg, y + 4, 2, 4, col);
    aDot(x, y, 6, col); aBox(x - 3, y - 9, 1, 4, col); aBox(x + 2, y - 9, 1, 4, col);
    aDot(x - 2.5, y - 1, 2, 0xFFFFFF); aDot(x + 2.5, y - 1, 2, 0xFFFFFF);
    aDot(x - 2.5 + (f ? 0.6 : -0.6), y - 1, 1, 0x1B1426); aDot(x + 2.5 + (f ? 0.6 : -0.6), y - 1, 1, 0x1B1426);
  }
  S.draw = function () {
    K.g = g;
    aBox(0, 0, W, H, 0x14102A);
    for (let k = 0; k < 40; k++) aDot((k * 97) % 320, ((k * 53) % 240 + S.t * (5 + (k % 3) * 4)) % 240, ((k % 3) + 1) * 0.5, 0x8F8AA8);
    S.bugs.forEach((a, k) => { if (a) { const [x, y] = bugAt(k); bug(Math.floor(k / B.COLS), x, y, S.frame); } });
    S.shield.forEach((cells, s) => cells.forEach((c, k) => { if (c) aBox(B.SHIELDS[s] + (k % B.SW) * B.CELL, B.SY + Math.floor(k / B.SW) * B.CELL, B.CELL, B.CELL, (Math.floor(k / B.SW) + k) % 2 ? 0x49D17A : 0x3FB86A); }));
    for (const bm of S.bombs) aDot(bm.x + Math.sin(bm.y * 0.4) * 1.5, bm.y, 2.5, 0xFFD23F);
    house.ps.forEach((h, i) => {
      const sh = S.shots[i]; if (sh) aBox(sh.x - 1, sh.y - 4, 2, 8, 0xFFFFFF);
      if (S.inv[i] > 0 && Math.floor(S.inv[i] / 4) % 2) return;
      const x = S.gun[i], hex = h.m.hex;
      aBox(x - 9, B.Y - 2, 18, 7, hex); aBox(x - 6, B.Y - 5, 12, 4, hex); aBox(x - 1.5, B.Y - 10, 3, 6, 0xE6E6F0); aDot(x, B.Y - 2, 2, 0xBFF0FF);
    });
    drawParticles(0);
    text('SCORE ' + S.score, 8, 12, 8, '#ffd23f', 'left'); text('BEST ' + S.best, W / 2, 12, 8, '#49d17a');
    for (let k = 0; k < S.lives; k++) { aBox(W - 16 - k * 16, 5, 12, 5, 0xFFD23F); aBox(W - 11 - k * 16, 2, 2, 4, 0xE6E6F0); }
    if (S.over) { gameOver(S.score, S.best); return; }
    if (S.waveT > 0) rimBanner('WAVE ' + (S.wave + 1), 14, 100, 34);
    hint('Stick move   A or B fire   START leave', H - 6);
  };
  return S;
}

// ---- fishing in the pond
const POND = {
  FISH: 6, TOP: 112, BED: 226, BANK: 70,
  catches: [
    { name: 'Minnow', col: 0xA9C4D6, len: 7, speed: 1.2, pull: 0.6, bolts: 1 },
    { name: 'Goldfish', col: 0xFF8A3D, len: 9, speed: 0.9, pull: 0.9, bolts: 2 },
    { name: 'Big bass', col: 0x5E8C4A, len: 15, speed: 0.7, pull: 1.5, bolts: 3 },
    { name: 'Golden fish', col: 0xFFD23F, len: 11, speed: 1.6, pull: 1.8, bolts: 10 },
    { name: 'Old boot', col: 0x6F4630, len: 10, speed: 0, pull: 0.2, bolts: 0 },
  ],
};
const rodTip = i => [46 + i * 14, 62 + i * 6];
function fishScene(house) {
  const PD = POND, S = { song: 'select', t: 0, caught: 0, last: -1 };
  const newFish = f => {
    const r = Math.random();
    f.kind = r < 0.35 ? 0 : r < 0.65 ? 1 : r < 0.85 ? 2 : r < 0.95 ? 4 : 3;
    f.x = 110 + Math.random() * 190; f.y = f.kind === 4 ? PD.BED - 6 : PD.TOP + 18 + Math.random() * (PD.BED - PD.TOP - 30);
    f.vx = f.kind === 4 ? 0 : (Math.random() < 0.5 ? -1 : 1) * PD.catches[f.kind].speed * 0.5; f.scare = 0;
    return f;
  };
  S.fish = Array.from({ length: PD.FISH }, () => newFish({}));
  S.rods = house.ps.map(() => ({ state: 'idle', fish: -1, t: 0, nibble: 0, power: 0, x: 0, y: 0, vx: 0, vy: 0, tension: 0, pulling: false }));
  const taken = k => S.rods.some(R => R.fish === k);
  S.update = function () {
    S.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    house.ps.forEach((h, i) => {
      const R = S.rods[i], sl = h.m.slot, [tx, ty] = rodTip(i);
      switch (R.state) {
        case 'idle': if (hit(sl, 'a')) { R.state = 'charge'; R.power = 0; R.t = 0; } break;
        case 'charge':
          R.t++; R.power = 0.5 - 0.5 * Math.cos(R.t * 0.06);
          if (!held(sl, 'a')) { R.state = 'fly'; R.x = tx; R.y = ty; R.vx = 1.6 + R.power * 3.4; R.vy = -2.6; SND.fx.whoosh(); }
          break;
        case 'fly':
          R.vy += 0.12; R.x += R.vx; R.y += R.vy;
          if (R.x > W - 8) { R.x = W - 8; R.vx = 0; }
          if (R.y >= PD.TOP) { R.y = PD.TOP + 2; R.state = 'wait'; R.fish = -1; R.nibble = 0; SND.fx.pop(); puff(R.x, PD.TOP, 5, 'rgba(220,250,255,0.9)'); }
          break;
        case 'wait': case 'bite': {
          R.y = clamp(R.y + ((held(sl, 'down') ? 1 : 0) - (held(sl, 'up') ? 1 : 0)) * 0.8 + 0.1, PD.TOP + 2, PD.BED - 6);
          if (hit(sl, 'b')) { if (R.fish >= 0) S.fish[R.fish].scare = 60; R.state = 'idle'; R.fish = -1; SND.fx.select(); break; }
          if (R.state === 'bite') {
            if (hit(sl, 'a')) { R.state = 'reel'; R.tension = 20; R.t = 0; SND.fx.grab(); }
            else if (--R.t <= 0) { S.fish[R.fish].scare = 90; R.fish = -1; R.state = 'wait'; }
            break;
          }
          if (R.fish < 0) {
            const k = S.fish.findIndex((f, k) => f.scare <= 0 && !taken(k) && Math.hypot(f.x - R.x, f.y - R.y) <= 50);
            if (k >= 0) { R.fish = k; R.nibble = 2 + Math.floor(Math.random() * 3); R.t = 40; }
          } else {
            const f = S.fish[R.fish], d = Math.hypot(f.x - R.x, f.y - R.y);
            if (d > 6) { f.x += (R.x - f.x) * 0.04; f.y += (R.y - f.y) * 0.04; f.vx = R.x > f.x ? Math.abs(f.vx) + 0.001 : -Math.abs(f.vx) - 0.001; }
            else if (--R.t <= 0) { if (--R.nibble > 0) { R.t = 30 + Math.floor(Math.random() * 40); SND.fx.tick(); } else { R.state = 'bite'; R.t = 45; SND.fx.button(); } }
          }
          break;
        }
        case 'reel': {
          const f = S.fish[R.fish], c = PD.catches[f.kind];
          R.t++;
          if (R.t % 50 === 0) R.pulling = Math.random() < 0.6;
          if (held(sl, 'a')) {
            const dx = tx - R.x, dy = ty - R.y, d = Math.hypot(dx, dy);
            R.x += dx / d * 1.1; R.y += dy / d * 0.7;
            R.tension += R.pulling ? 1.3 * c.pull : 0.25;
            if (R.t % 8 === 0) SND.fx.tick();
          } else { R.tension -= 1.4; if (R.pulling) R.x = Math.min(W - 8, R.x + 0.7 * c.pull); }
          if (R.pulling) R.y = clamp(R.y + Math.sin(R.t * 0.2) * 0.8, PD.TOP + 4, PD.BED - 6);
          R.tension = clamp(R.tension, 0, 100);
          f.x = R.x; f.y = Math.max(R.y, PD.TOP + 4);
          if (R.tension >= 100) { R.state = 'snap'; R.t = 90; f.scare = 120; R.fish = -1; SND.fx.nope(); }
          else if (R.x <= PD.BANK + 4) {
            R.state = 'got'; R.t = 150; S.last = f.kind; S.caught++;
            prog.bolts = (prog.bolts || 0) + c.bolts; saveProg();
            (f.kind === 4 ? SND.fx.clank : SND.fx.gear)();
            if (f.kind === 3) confetti(160, 40, 40);
            sparks(R.x, PD.TOP - 10, 8, AHX(c.col));
            newFish(f); f.x = W - 20; f.scare = 60; R.fish = -1;
          }
          break;
        }
        case 'got': case 'snap': if (--R.t <= 0 || (R.t < 110 && hit(sl, 'a'))) R.state = 'idle'; break;
      }
    });
    S.fish.forEach((f, k) => {
      if (f.scare > 0) f.scare--;
      if (taken(k) || f.kind === 4) return;
      const sp = PD.catches[f.kind].speed * (f.scare > 0 ? 1.6 : 0.5);
      f.vx = (f.vx < 0 ? -1 : 1) * sp; f.x += f.vx; f.y += Math.sin(S.t * 1.3 + k) * 0.2;
      if (f.x < PD.BANK + 20) { f.x = PD.BANK + 20; f.vx = sp; }
      if (f.x > W - 10) { f.x = W - 10; f.vx = -sp; }
      if (Math.random() < 0.004) f.vx = -f.vx;
      f.y = clamp(f.y, PD.TOP + 12, PD.BED - 8);
    });
  };
  function fish(kind, x, y, dir, t) {
    const c = PD.catches[kind];
    if (kind === 4) { aBox(x - 5, y - 8, 7, 9, c.col); aBox(x - 5, y - 1, 13, 5, c.col); aBox(x - 6, y + 3, 15, 2, 0x3A2A20); return; }
    const L = c.len, w = Math.sin(t * 8) * 1.2;
    aDot(x - dir * L * 0.75, y + w, L * 0.4, c.col); aDot(x, y, L * 0.55, c.col); aDot(x + dir * L * 0.25, y, L * 0.4, c.col);
    aDot(x + dir * L * 0.35, y - L * 0.12, 1.4, 0x1B1426);
    if (kind === 3) { g.globalAlpha = 0.5 + 0.5 * Math.sin(t * 6); K.circ(x, y - L * 0.6, 2.5, '#ffffff'); g.globalAlpha = 1; }
  }
  const cloud = (x, y) => { aDot(x, y, 12, 0xFFFFFF); aDot(x + 12, y - 4, 14, 0xFFFFFF); aDot(x + 26, y, 11, 0xFFFFFF); };
  S.draw = function () {
    K.g = g;
    const t = S.t;
    aBox(0, 0, W, PD.TOP, 0x8FD3FF); cloud(150, 30); cloud(250, 18);
    for (let k = 0; k < 8; k++) aDot(90 + k * 32, PD.TOP - 4, 14 + (k % 3) * 4, 0x3F9D4E);
    aBox(0, PD.TOP, W, H - PD.TOP, 0x3A8FC0); aBox(0, PD.TOP + 40, W, 40, 0x347FB0); aBox(0, PD.TOP + 80, W, H, 0x2E70A0); aBox(0, PD.BED, W, H - PD.BED, 0xC9B27A);
    for (let k = 0; k < 6; k++) aBox(100 + k * 40, PD.BED - 18 - (k % 3) * 6, 2, 18 + (k % 3) * 6, 0x2F7A3C);
    for (let x = 0; x < W; x += 16) aBox(x + (t * 8) % 16, PD.TOP, 8, 1, 0xBFE8FF);
    S.fish.forEach((f, k) => fish(f.kind, f.x, f.y, f.vx < 0 ? -1 : 1, t + k));
    aBox(0, 100, PD.BANK, H - 100, 0x8A5A3C); aBox(0, 96, PD.BANK + 4, 8, 0x5DBB63);
    house.ps.forEach((h, i) => {
      const R = S.rods[i], hx = 22 + i * 18; let [tx, ty] = rodTip(i);
      if (R.state === 'charge') { tx -= R.power * 30; ty -= R.power * 6; }
      drawHeroPose(h.m, hx, 98, R.state === 'got' ? 'cheer' : 'idle', t + i * 0.7);
      aLine(hx + 6, 78, tx, ty, 2, 0x8A5A3C);
      if (R.state === 'charge') { panel(hx - 14, 30 + i * 8, 30, 7); aBox(hx - 13, 31 + i * 8, 28 * R.power, 5, R.power > 0.8 ? 0xFF5A5A : 0xFFD23F); }
      if (R.state === 'fly') aLine(tx, ty, R.x, R.y, 1, 0xFFF6D8, 0.9);
      if (R.state === 'wait' || R.state === 'bite' || R.state === 'reel') {
        const dip = R.state === 'bite' ? 4 + Math.sin(t * 30) * 2 : (R.fish >= 0 && R.t < 6) ? 2 : Math.sin(t * 3) * 0.8, by = PD.TOP - 2 + dip;
        aLine(tx, ty, R.x, by, 1, 0xFFF6D8, 0.9); aLine(R.x, by, R.x, R.y, 1, 0xFFF6D8, 0.6);
        aDot(R.x, by, 3, 0xFFFFFF); aDot(R.x, by - 1.5, 2, 0xFF5A5A); aRing(R.x, R.y, 2.5, 0xC9C4E0);
        if (R.state === 'bite') { K.rr(R.x - 15, by - 40, 30, 26, 5, '#ff5a5a'); K.rr(R.x - 13, by - 38, 26, 22, 4, '#1b1426'); text('!', R.x, by - 21, 14, '#ffd23f', 'center', null); }
      }
      if (R.state === 'reel') {
        const bx = 96 + i * 120;
        panel(bx, 4, 104, 16); text('LINE', bx + 4, 15, 7, '#d9d2e6', 'left', null);
        aBox(bx + 30, 8, 70, 8, 0x3A3650); aBox(bx + 30, 8, 70 * R.tension / 100, 8, R.tension > 75 ? 0xFF5A5A : R.tension > 45 ? 0xFFD23F : 0x49D17A);
      }
      if (R.state === 'got' && S.last >= 0) {
        const c = PD.catches[S.last];
        rimBanner((S.last === 4 ? 'An ' : 'A ') + c.name + '!', 14, 40, 32); fish(S.last, W / 2, 92, 1, t);
        if (c.bolts) hint('+' + c.bolts + ' bolts', 116);
      }
      if (R.state === 'snap') rimBanner('Snap! It got away', 14, 50, 32, '#ff5a5a', '#ff8a8a');
    });
    drawParticles(0);
    panel(W - 74, 4, 70, 16); text('Caught ' + S.caught, W - 39, 15, 8, '#fff6d8', 'center', null);
    const st = S.rods[0].state;
    hint(st === 'charge' ? 'Let go of A to cast!' : st === 'wait' ? '^ v hook depth   B reel in   START leave' : st === 'bite' ? 'A bite! Press A!' : st === 'reel' ? 'Hold A to reel   let go when it goes red' : 'Hold A, let go to cast   START leave', H - 6);
  };
  return S;
}
