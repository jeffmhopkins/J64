// ---- the arcade room's pinball table and its Bug Blaster, Bolt Ball and Jet Run cabinets, and fishing in the yard's pond.
// Mirrors src/core/ck_arcade.c (the same tables, numbers and rules) and the drawing in draw.c.
const AHX = h => '#' + h.toString(16).padStart(6, '0');
const aDot = (x, y, r, c, a = 1) => { g.globalAlpha = a; K.circ(x, y, r, typeof c === 'number' ? AHX(c) : c); g.globalAlpha = 1; };
const aBox = (x, y, w, h, c, a = 1) => { g.globalAlpha = a; g.fillStyle = typeof c === 'number' ? AHX(c) : c; g.fillRect(x, y, w, h); g.globalAlpha = 1; };
const aLine = (x0, y0, x1, y1, w, c, a = 1) => { g.globalAlpha = a; g.strokeStyle = typeof c === 'number' ? AHX(c) : c; g.lineWidth = w; g.lineCap = 'butt'; g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke(); g.globalAlpha = 1; };
const aRing = (x, y, r, c) => { g.strokeStyle = AHX(c); g.lineWidth = 1.5; g.beginPath(); g.arc(x, y, r - 0.75, 0, P * 2); g.stroke(); };
const anyHeld = b => held(0, b) || held(1, b);
function scoreBox(x, y, label, v, col) { panel(x, y, 58, 30); text(label, x + 29, y + 11, 7, '#d9d2e6', 'center', null); text(String(v), x + 29, y + 26, 10, col, 'center', null); }
function gameOver(score, best) { rimBanner('GAME OVER', 14, 70, 34); hint('Score ' + score + '   Best ' + best, 124); hint('A play again', 142); }

// ---- pinball: a tall table drawn at ZOOM and scrolled to follow the ball
const PIN = {
  W: 240, H: 452, ZOOM: 0.8, VIEW: 300, OX: 64, DRAIN: 452,
  R: 4, BALLS: 3, MULTI: 3, REST: 0.5, UP: -0.45, LANE_X: 221, LANE_Y: 434, GRAV: 0.12, MAXV: 10,
  walls: [[10, 352, 10, 60], [10, 60, 24, 28], [24, 28, 52, 10], [52, 10, 182, 10], [182, 10, 212, 24],
    [212, 24, 230, 50], [230, 50, 230, 440], [212, 440, 230, 440], [212, 440, 212, 96],
    [34, 250, 34, 84],   // the left orbit's inner wall
    [95, 18, 95, 40], [127, 18, 127, 40],   // between the rollover lanes
    [30, 316, 30, 346], [30, 346, 58, 362], [192, 316, 192, 346], [192, 346, 164, 362],   // the slingshots' backs
    [194, 262, 212, 252],   // the upper flipper's bracket
    [10, 352, 76, 398], [212, 352, 146, 398]],
  gate: [230, 74, 212, 96],   // over the launch lane: the ball only goes out through it
  spinner: [10, 150, 34, 150],
  slings: [[30, 316, 58, 362], [192, 316, 164, 362]],
  drops: [[205, 150, 205, 162], [205, 166, 205, 178], [205, 182, 205, 194], [205, 198, 205, 210]],
  bumps: [[82, 84, 11, 100], [140, 84, 11, 100], [111, 118, 11, 100], [60, 160, 6, 50], [168, 132, 6, 50]],
  lights: [[80, 30], [111, 30], [142, 30]],
  saucer: [150, 176],
  flips: [[78, 400, 27, 0], [144, 400, 27, 1], [194, 262, 22, 1]],   // pivot, length, side (1 points left)
  MSG: ['', 'BONUS!', 'BALL LOCKED', 'MULTIBALL!', 'DOUBLE SCORE!'],
};
const pinFlipper = (k, a) => { const [x0, y0, len, side] = PIN.flips[k]; return [x0, y0, x0 + (side ? -1 : 1) * Math.cos(a) * len, y0 + Math.sin(a) * len]; };
const pinInLane = b => b.on && b.x > 212 && b.y > PIN.LANE_Y - 9 && Math.abs(b.vy) < 1;
function pinballScene(house) {
  const S = { song: 'pinball', t: 0, best: 0 };
  const sx = x => PIN.OX + x * PIN.ZOOM, sy = y => (y - S.cam) * PIN.ZOOM;   // a table spot on the screen
  const newBall = () => { S.ball = [0, 1, 2].map(i => ({ x: PIN.LANE_X, y: PIN.LANE_Y, vx: 0, vy: 0, on: i === 0, held: 0 })); };
  const reset = () => {
    Object.assign(S, { balls: PIN.BALLS, score: 0, flip: PIN.flips.map(() => PIN.REST), flipW: PIN.flips.map(() => 0), flash: PIN.bumps.map(() => 0), sling: [0, 0],
      lit: 0, litT: 0, drops: 0, dropT: 0, spin: 0, spinA: 0, locks: 0, x2T: 0, msg: 0, msgT: 0, cam: PIN.H - PIN.VIEW, over: false, overT: 0 });
    newBall();
  };
  reset();
  const add = n => { S.score += S.x2T > 0 ? n * 2 : n; };
  const say = m => { S.msg = m; S.msgT = 90; };
  const live = () => S.ball.filter(b => b.on).length;
  // push the ball out of a fat line and bounce it; a flipper turning w radians a frame flicks it
  function seg(b, ax, ay, bx, by, rr, e, side, w) {
    const dx = bx - ax, dy = by - ay, len2 = dx * dx + dy * dy;
    const t = len2 > 0 ? clamp(((b.x - ax) * dx + (b.y - ay) * dy) / len2, 0, 1) : 0;
    const cx = ax + dx * t, cy = ay + dy * t; let nx = b.x - cx, ny = b.y - cy; const d = Math.hypot(nx, ny);
    if (d >= PIN.R + rr || d < 0.0001) return false;
    nx /= d; ny /= d; b.x = cx + nx * (PIN.R + rr); b.y = cy + ny * (PIN.R + rr);
    let svx = 0, svy = 0;
    if (w) { const rx = cx - ax, ry = cy - ay; if (side === 0) { svx = -w * ry; svy = w * rx; } else { svx = w * ry; svy = -w * rx; } }
    const vn = (b.vx - svx) * nx + (b.vy - svy) * ny;
    if (vn < 0) { b.vx -= (1 + e) * vn * nx; b.vy -= (1 + e) * vn * ny; }
    return true;
  }
  function step(b, was, sub) {
    const py = b.y;
    b.vy += PIN.GRAV / 4; b.x += b.vx / 4; b.y += b.vy / 4;
    for (const w of PIN.walls) seg(b, w[0], w[1], w[2], w[3], 1, 0.5, 0, 0);
    if (b.vy > 0) seg(b, ...PIN.gate, 1, 0.4, 0, 0);
    for (let s = 0; s < PIN.flips.length; s++) { const [x0, y0, x1, y1] = pinFlipper(s, was[s] + S.flipW[s] * (sub + 1) / 4); seg(b, x0, y0, x1, y1, 3, 0.3, PIN.flips[s][3], S.flipW[s]); }
    PIN.slings.forEach((w, k) => {   // slingshots kick the ball back out
      const vx = b.vx, vy = b.vy;
      if (!seg(b, w[0], w[1], w[2], w[3], 1, 0.5, 0, 0)) return;
      let nx = w[3] - w[1], ny = w[0] - w[2]; const l = Math.hypot(nx, ny); nx /= l; ny /= l;
      if (k === 1) { nx = -nx; ny = -ny; }
      if (S.sling[k] < 6 && vx * nx + vy * ny < -0.5) { b.vx += nx * 3.2; b.vy += ny * 3.2; add(10); S.sling[k] = 10; SND.fx.kick(); }
    });
    PIN.drops.forEach((w, k) => {   // drop targets go down when hit; the whole bank gives double scoring
      if (S.drops & (1 << k) || !seg(b, w[0], w[1], w[2], w[3], 2, 0.5, 0, 0)) return;
      S.drops |= 1 << k; add(200); SND.fx.crate(); sparks(sx(w[0]), sy((w[1] + w[3]) / 2), 4, '#ff8a3d');
      if (S.drops === 15) { add(1500); S.x2T = 60 * 20; S.dropT = 60; say(4); SND.fx.power(); }
    });
    PIN.bumps.forEach(([bx, by, r, score], k) => {
      let nx = b.x - bx, ny = b.y - by; const d = Math.hypot(nx, ny);
      if (d >= r + PIN.R || d < 0.0001) return;
      nx /= d; ny /= d; b.x = bx + nx * (r + PIN.R); b.y = by + ny * (r + PIN.R);
      const vn = b.vx * nx + b.vy * ny; if (vn < 0) { b.vx -= 2 * vn * nx; b.vy -= 2 * vn * ny; }
      const kick = score >= 100 ? 2.6 : 1.8; b.vx += nx * kick; b.vy += ny * kick;
      if (S.flash[k] < 8) { add(score); (score >= 100 ? SND.fx.boing : SND.fx.hop)(); sparks(sx(bx + nx * r), sy(by + ny * r), 4, '#ffd23f'); }
      S.flash[k] = 12;
    });
    const sp = PIN.spinner;   // going through the spinner spins it, faster the faster the ball goes
    if (b.x > sp[0] && b.x < sp[2] && (py - sp[1]) * (b.y - sp[1]) <= 0 && py !== b.y) S.spin = Math.min(S.spin + Math.abs(b.vy) * 0.08, 1.2);
    const v = Math.hypot(b.vx, b.vy); if (v > PIN.MAXV) { b.vx *= PIN.MAXV / v; b.vy *= PIN.MAXV / v; }
  }
  S.update = function () {
    S.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    if (S.over) { if (++S.overT > 60 && anyHit('a')) { reset(); SND.fx.confirm(); } return; }
    const hold = [anyHeld('left') || anyHeld('b') || anyHeld('l'), anyHeld('right') || anyHeld('a') || anyHeld('r')], was = [...S.flip];
    for (let s = 0; s < PIN.flips.length; s++) {
      const h = hold[PIN.flips[s][3]];
      S.flip[s] = h ? Math.max(PIN.UP, S.flip[s] - 0.32) : Math.min(PIN.REST, S.flip[s] + 0.18);
      if (h && was[s] === PIN.REST && s < 2) SND.fx.clank();
      S.flipW[s] = S.flip[s] - was[s];
    }
    for (const b of S.ball) if (pinInLane(b) && anyHit('a')) { b.vy = -10 - Math.random() * 0.4; SND.fx.spring(); }
    for (const b of S.ball) {
      if (!b.on) continue;
      if (b.held > 0) { if (--b.held === 0) { b.vx = -2.4 + Math.random(); b.vy = -4.2; b.y -= 8; SND.fx.blow(); } continue; }
      for (let sub = 0; sub < 4; sub++) step(b, was, sub);
      // the saucer catches a slow ball; every second catch starts multiball
      const dx = b.x - PIN.saucer[0], dy = b.y - PIN.saucer[1];
      if (dx * dx + dy * dy < 36 && b.vx * b.vx + b.vy * b.vy < 30) {
        Object.assign(b, { x: PIN.saucer[0], y: PIN.saucer[1], vx: 0, vy: 0, held: 50 }); add(500); SND.fx.grab();
        if (live() === 1 && ++S.locks % 2 === 0) {
          S.ball.forEach((o, j) => { if (!o.on) Object.assign(o, { x: PIN.saucer[0], y: PIN.saucer[1] - 8, vx: j === 1 ? 2.2 : -1, vy: -5, on: true, held: 0 }); });
          say(3); SND.fx.flag(); confetti(sx(PIN.saucer[0]), sy(PIN.saucer[1]), 30);
        } else if (live() === 1) say(2);
      }
      if (b.y > PIN.DRAIN) {   // down the middle: that ball is lost
        b.on = false;
        if (live() > 0) continue;
        SND.fx.ouch();
        if (--S.balls <= 0) { S.over = true; S.overT = 0; S.best = Math.max(S.best, S.score); } else newBall();
      }
    }
    S.flash = S.flash.map(f => Math.max(0, f - 1)); S.sling = S.sling.map(f => Math.max(0, f - 1));
    if (S.spin > 0.01) {   // the spinner scores each turn it makes
      const turns = Math.floor(S.spinA + S.spin) - Math.floor(S.spinA);
      S.spinA += S.spin; S.spin *= 0.97;
      if (turns > 0) { add(25 * turns); SND.fx.tick(); }
    } else S.spin = 0;
    if (S.dropT > 0 && --S.dropT === 0) S.drops = 0;   // the bank pops back up
    if (S.x2T > 0) S.x2T--;
    if (S.msgT > 0) S.msgT--;
    for (const b of S.ball) if (b.on) PIN.lights.forEach(([lx, ly], k) => {
      if (S.lit & (1 << k) || Math.hypot(b.x - lx, b.y - ly) > 8) return;
      S.lit |= 1 << k; add(250); SND.fx.star();
      if (S.lit === 7) { add(1000); S.lit = 0; S.litT = 90; say(1); SND.fx.flag(); confetti(160, 30, 30); }
    });
    if (S.litT > 0) S.litT--;
    const low = Math.max(-1, ...S.ball.filter(b => b.on).map(b => b.y));   // the view follows the lowest ball
    if (low >= 0) S.cam += (clamp(low - 170, 0, PIN.H - PIN.VIEW) - S.cam) * 0.12;
  };
  S.draw = function () {
    K.g = g;
    const lit = (S.lit & 1) + (S.lit >> 1 & 1) + (S.lit >> 2 & 1), party = S.litT > 0 && Math.floor(S.litT / 6) % 2;
    const hazard = (x0, y0, x1, y1, w, n) => { for (let k = 0; k < n; k++) { const a = k / n, b = (k + 1) / n; aLine(x0 + (x1 - x0) * a, y0 + (y1 - y0) * a, x0 + (x1 - x0) * b, y0 + (y1 - y0) * b, w, k % 2 ? 0x1B1426 : 0xFFD23F); } };
    aBox(0, 0, W, H, 0x2B2240);
    for (let k = 0; k < 24; k++) aDot((k * 53) % 320, (k * 37) % 240, 1, 0x5A4F78);
    g.save(); g.translate(PIN.OX, -S.cam * PIN.ZOOM); g.scale(PIN.ZOOM, PIN.ZOOM);   // table units from here
    aBox(10, 10, 220, PIN.H - 10, 0x34404F);   // the steel plate, its seams and rivets
    for (let y = 50; y < PIN.H; y += 50) aBox(10, y, 220, 1, 0x283240);
    for (let x = 66; x < 212; x += 56) aBox(x, 10, 1, PIN.H - 10, 0x283240);
    for (let y = 50; y < PIN.H; y += 50) for (let x = 66; x < 212; x += 56) aDot(x, y, 1.5, 0x6A7888);
    aBox(10, 10, 24, 240, 0x2C3644);   // the left orbit's lane
    aBox(87, 262, 48, 36, 0x4A5666); aBox(90, 265, 42, 30, 0x3E4856); aBox(110, 252, 2, 10, 0x8F8AA8);   // the robot face on the table
    aDot(111, 251, 3, party ? 0xFFD23F : 0xE04F4F);
    for (let e = 0; e < 2; e++) { aDot(101 + e * 20, 275, 5.5, 0x1B2A3A); if (party || lit > e) aDot(101 + e * 20, 275, 4, 0x5AD1FF); }
    aBox(98, 284, 26, 6, 0x1B1426); for (let k = 0; k < 5; k++) aBox(100 + k * 5, 285, 2, 4, party || lit >= 3 ? 0xFFD23F : 0x6A7888);
    aDot(190, 224, 4, S.x2T > 0 && Math.floor(S.x2T / 8) % 2 ? 0xFF8A3D : 0x5A3A2A);   // the DOUBLE lamp and the LOCK lamps
    for (let k = 0; k < 2; k++) aDot(140 + k * 20, 196, 3, S.locks % 2 > k ? 0x5AD1FF : 0x2A3A4A);
    hazard(10, PIN.DRAIN - 4, 212, PIN.DRAIN - 4, 6, 18);   // the drain
    aBox(215, 436, 12, 6, 0xE04F4F); aBox(219, 442, 4, 10, 0x8F8AA8);   // the plunger
    PIN.walls.forEach((w, k) => { if (k < PIN.walls.length - 2) aLine(w[0], w[1], w[2], w[3], 3, 0xC9C4E0); else hazard(w[0], w[1], w[2], w[3], 4, 7); });
    aLine(...PIN.gate, 2, 0xFFD23F);
    PIN.slings.forEach((w, k) => aLine(w[0], w[1], w[2], w[3], 4, S.sling[k] > 0 ? 0xFFFFFF : 0xE04F4F));
    PIN.drops.forEach((w, k) => {   // drop targets: orange plates, flat when down
      if (S.drops & (1 << k)) aLine(w[0], w[1] + 2, w[2], w[3] - 2, 1.5, 0x5A3A2A);
      else { aLine(w[0], w[1], w[2], w[3], 5, S.dropT > 0 ? 0xFFFFFF : 0xFF8A3D); aLine(w[0] + 1.5, w[1] + 3, w[2] + 1.5, w[3] - 3, 1.5, 0xFFD23F); }
    });
    { const w = PIN.spinner, c = Math.cos(S.spinA * P * 2), h = 4 * Math.abs(c) + 1;   // the spinner's flap turns over
      aBox(w[0], w[1] - 0.5, w[2] - w[0], 1, 0x8F8AA8); aBox(w[0] + 3, w[1] - h / 2, w[2] - w[0] - 6, h, c > 0 ? 0x5AD1FF : 0x2E8FB8); }
    aDot(PIN.saucer[0], PIN.saucer[1], 8, 0x1B1426); aRing(PIN.saucer[0], PIN.saucer[1], 8, 0x8F8AA8);   // the saucer
    PIN.lights.forEach(([x, y], k) => {   // bolt targets in the rollover lanes
      const on = (S.lit & (1 << k)) || party;
      aDot(x, y, 7, 0x1B1426); aRing(x, y, 7, on ? 0xFFD23F : 0x6A7888); K.nut(x, y, 4.6, on ? S.t * 3 : 0, on ? '#ffd23f' : '#5a5a6e');
    });
    const bcol = [0xFF5A5A, 0x5AD1FF, 0xC28BFF, 0x49D17A, 0x49D17A];
    PIN.bumps.forEach(([x, y, r0], k) => {
      const hitB = S.flash[k] > 0, r = r0 + (hitB ? 2 : 0);
      if (r0 > 8) {   // big bumpers are gears that spin when hit
        S.spinG = S.spinG || [0, 0, 0, 0, 0]; S.spinG[k] += (k % 2 ? -1 : 1) * (hitB ? 0.1 : 0.013);
        if (hitB) aDot(x, y, r + 3, 0xFFFFFF, 0.6);
        K.gear(x, y, r * 1.05, 10, S.spinG[k], '#ffd854', '#a37d13');
        aDot(x, y, r * 0.45, 0x1B1426); aDot(x, y, r * 0.3, hitB ? 0xFFFFFF : bcol[k]);
      } else { aDot(x, y, r, hitB ? 0xFFFFFF : 0x25222C); aDot(x, y, r * 0.55, 0x3C3846); aDot(x, y, r * 0.28, bcol[k]); }   // small ones are tyres
    });
    for (let s = 0; s < PIN.flips.length; s++) {   // the flippers are robot arms
      const [x0, y0, x1, y1] = pinFlipper(s, S.flip[s]);
      aLine(x0, y0, x1, y1, 7, 0x8F8AA8); aLine(x0, y0, x1, y1, 3, 0xC9C4E0);
      aDot(x1, y1, 3.5, 0xFF8A3D); aDot(x0, y0, 4.5, 0x5A5E6A); aDot(x0, y0, 2, 0xFFD23F);
    }
    for (const b of S.ball) if (b.on) { aDot(b.x, b.y, PIN.R, 0xE6E6F0); aDot(b.x - 1.2, b.y - 1.2, 1.4, 0xFFFFFF); }
    g.restore();
    drawParticles(0);
    text('CLANK-O', 32, 12, 8, '#ffd23f', 'center', null); text('MATIC', 32, 22, 8, '#ffd23f', 'center', null);
    scoreBox(3, 30, 'SCORE', S.score, '#ffd23f'); scoreBox(3, 66, 'BALLS', S.balls, '#fff6d8'); scoreBox(259, 30, 'BEST', S.best, '#49d17a');
    if (S.x2T > 0) text('x2 ' + (Math.floor(S.x2T / 60) + 1), 288, 80, 8, '#ffd23f', 'center', null);
    if (S.msgT > 0) rimBanner(PIN.MSG[S.msg], 10, 100, 22);
    if (S.over) { gameOver(S.score, S.best); return; }
    hint(S.ball.some(pinInLane) ? 'A launch!   START leave' : 'B left flippers   A right flippers   START leave', H - 12);
  };
  return S;
}

// ---- Bug Blaster
const BLAST = { COLS: 8, ROWS: 4, DX: 24, DY: 18, Y: 216, BOMBS: 6, SHIELDS: [52, 142, 232], SW: 6, SH: 3, CELL: 6, SY: 180 };
function blastScene(house) {
  const B = BLAST, S = { song: 'blaster', t: 0, best: 0 };
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
    { name: 'Minnow', col: 0xA9C4D6, col2: 0xA9C4D6, len: 7, speed: 1.2, pull: 0.6, bolts: 1 },
    { name: 'Goldfish', col: 0xFF8A3D, col2: 0xFFC27A, len: 9, speed: 0.9, pull: 0.9, bolts: 2 },
    { name: 'Big bass', col: 0x5E8C4A, col2: 0xC9D98A, len: 15, speed: 0.7, pull: 1.5, bolts: 3 },
    { name: 'Golden fish', col: 0xFFD23F, col2: 0xFFF6A0, len: 11, speed: 1.6, pull: 2.0, bolts: 10 },
    { name: 'Old boot', col: 0x6F4630, col2: 0x3A2A20, len: 10, speed: 0, pull: 0.2, bolts: 0 },
    { name: 'Sardine', col: 0xBFC8D4, col2: 0xBFC8D4, len: 6, speed: 1.0, pull: 0.5, bolts: 1 },
    { name: 'Perch', col: 0xD9C24A, col2: 0x3F5A2E, len: 10, speed: 1.0, pull: 1.1, bolts: 2 },
    { name: 'Rainbow trout', col: 0x8FB8A8, col2: 0xFF7AA8, len: 13, speed: 1.3, pull: 1.6, bolts: 4 },
    { name: 'Catfish', col: 0x6A5E50, col2: 0x3A3028, len: 16, speed: 0.6, pull: 2.0, bolts: 5 },
    { name: 'Pike', col: 0x7A9A4A, col2: 0xE6E0A0, len: 18, speed: 1.5, pull: 2.4, bolts: 7 },
    { name: 'Koi', col: 0xFFF6E6, col2: 0xFF5A2A, len: 14, speed: 0.8, pull: 1.7, bolts: 6 },
    { name: 'Robo-fish', col: 0x9AA4B4, col2: 0x5AD1FF, len: 14, speed: 1.8, pull: 2.6, bolts: 9 },
    { name: 'King carp', col: 0xC98A3A, col2: 0xFFD23F, len: 22, speed: 0.6, pull: 3.0, bolts: 12 },
  ],
  // the pond starts with plain little fish; the more the family catches, the bigger and harder they get
  LEVEL_AT: [0, 5, 10, 15, 20],   // fish caught this visit
  POPS: [   // [kind, how many in a hundred]
    [[0, 45], [5, 45], [4, 10]],
    [[0, 20], [5, 10], [1, 30], [6, 30], [4, 10]],
    [[1, 15], [6, 20], [2, 20], [7, 20], [8, 12], [3, 5], [4, 8]],
    [[2, 15], [7, 15], [8, 15], [9, 20], [10, 20], [3, 7], [4, 8]],
    [[7, 10], [9, 15], [10, 15], [8, 10], [11, 22], [12, 15], [3, 8], [4, 5]],
  ],
};
const pondLevel = n => { let l = 0; while (l + 1 < POND.LEVEL_AT.length && n >= POND.LEVEL_AT[l + 1]) l++; return l; };
const rodTip = i => [46 + i * 14, 62 + i * 6];
function fishScene(house) {
  const PD = POND, S = { song: 'select', t: 0, caught: 0, last: -1, level: 0, levelT: 0, fished: 0 };   // every visit starts with the little fish
  const newFish = f => {
    const pop = PD.POPS[S.level]; let r = Math.floor(Math.random() * 100), k = 0;
    while (k < pop.length - 1 && r >= pop[k][1]) r -= pop[k++][1];
    f.kind = pop[k][0];
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
            else if (--R.t <= 0) { if (--R.nibble > 0) { R.t = 30 + Math.floor(Math.random() * 40); SND.fx.tick(); } else { R.state = 'bite'; R.t = 50 - Math.floor(PD.catches[f.kind].pull * 6); SND.fx.button(); } }
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
            if (f.kind !== 4) S.fished++;
            const lv = pondLevel(S.fished);
            if (lv > S.level) {   // the pond levels up: the fish not on a line make way for bigger ones
              S.level = lv; S.levelT = 180;
              S.fish.forEach((o, k) => { if (o !== f && !taken(k)) { newFish(o); o.x = W - 10 - k * 6; } });
              confetti(160, 40, 40);
            }
            newFish(f); f.x = W - 20; f.scare = 60; R.fish = -1;
          }
          break;
        }
        case 'got': case 'snap': if (--R.t <= 0 || (R.t < 110 && hit(sl, 'a'))) R.state = 'idle'; break;
      }
      // the bobber trails behind a hooked fish, so the line under it runs on a slant
      R.bob = R.state === 'reel' ? R.bob + (R.x + (tx - R.x) * 0.3 - R.bob) * 0.05 : R.x;
    });
    if (S.levelT > 0) S.levelT--;
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
    const L = c.len, w = Math.sin(t * 8) * 1.2, d = dir;
    if (kind === 9) aDot(x - d * L * 0.45, y, L * 0.4, c.col);   // a long body
    if (kind !== 0 && kind !== 5) { aDot(x - d * L * 0.1, y - L * 0.45, L * 0.22, c.col2); aDot(x - d * L * 0.1, y + L * 0.42, L * 0.16, c.col2); }   // fins on the bigger fish
    aDot(x - d * L * 0.75, y + w, L * 0.4, kind === 10 || kind === 7 ? c.col2 : c.col); aDot(x, y, L * 0.55, c.col); aDot(x + d * L * 0.25, y, L * 0.4, c.col);
    if (kind === 6) for (let k = -1; k <= 1; k++) aBox(x + d * k * L * 0.25 - 1, y - L * 0.4, 2, L * 0.7, c.col2);   // stripes
    if (kind === 7) aBox(x - L * 0.5, y - 1, L, 2.5, c.col2);   // the rainbow along its side
    if (kind === 10) { aDot(x - d * L * 0.15, y - L * 0.15, L * 0.25, c.col2); aDot(x + d * L * 0.3, y - L * 0.2, L * 0.15, c.col2); }
    if (kind === 8) { aLine(x + d * L * 0.55, y + 1, x + d * L * 0.95, y + 4, 2, c.col2); aLine(x + d * L * 0.55, y + 2, x + d * L * 0.85, y + 7, 2, c.col2); }   // whiskers
    if (kind === 9) for (let k = 0; k < 3; k++) aDot(x - d * k * L * 0.3, y - L * 0.1, 1.8, c.col2);
    if (kind === 11) {   // a clockwork fish: rivets and a blinking antenna
      for (let k = -1; k <= 1; k++) aDot(x + d * k * L * 0.25, y + L * 0.15, 1.2, 0x5A5E6A);
      aBox(x - 0.5, y - L * 0.75, 1.5, L * 0.3, 0x5A5E6A); aDot(x, y - L * 0.78, 2, Math.sin(t * 6) > 0 ? c.col2 : 0x2A3A4A);
    }
    if (kind === 12) K.star(x + d * L * 0.15, y - L * 0.55, 5, AHX(c.col2));   // its crown
    aDot(x + d * L * 0.35, y - L * 0.12, kind === 11 ? 2 : 1.4, kind === 11 ? c.col2 : 0x1B1426);
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
      aLine(hx + 6, 78, tx, ty, 4, 0x4A2E1C); aLine(hx + 6, 78, tx, ty, 2, 0xB07A4C);   // the rod
      if (R.state === 'charge') { panel(hx - 14, 30 + i * 8, 30, 7); aBox(hx - 13, 31 + i * 8, 28 * R.power, 5, R.power > 0.8 ? 0xFF5A5A : 0xFFD23F); }
      if (R.state === 'fly') aLine(tx, ty, R.x, R.y, 1.5, 0xFFF6D8);
      if (R.state === 'wait' || R.state === 'bite' || R.state === 'reel') {
        const dip = R.state === 'bite' ? 4 + Math.sin(t * 30) * 2 : (R.fish >= 0 && R.t < 6) ? 2 : Math.sin(t * 3) * 0.8, by = PD.TOP - 2 + dip;
        const bx = R.state === 'reel' ? R.bob : R.x;
        aLine(tx, ty, bx, by, 1.5, 0xFFF6D8); aLine(bx, by, R.x, R.y, 1.5, 0xFFF6D8, 0.7);
        aDot(bx, by, 3, 0xFFFFFF); aDot(bx, by - 1.5, 2, 0xFF5A5A); aRing(R.x, R.y, 2.5, 0xC9C4E0);
        if (R.state === 'bite') { K.rr(R.x - 15, by - 40, 30, 26, 5, '#ff5a5a'); K.rr(R.x - 13, by - 38, 26, 22, 4, '#1b1426'); text('!', R.x, by - 21, 14, '#ffd23f', 'center', null); }
      }
      if (R.state === 'reel') {   // how tight the line is: player one's top left, two's top right
        const bx = i ? W - 108 : 4;
        panel(bx, 4, 104, 16); text('P' + (i + 1), bx + 4, 15, 7, h.m.slot ? '#8fd3ff' : '#ffd23f', 'left', null);
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
    panel(W / 2 - 62, 22, 124, 14); text('Caught ' + S.caught + '   Pond level ' + (S.level + 1), W / 2, 32, 8, '#fff6d8', 'center', null);
    if (S.levelT > 0) rimBanner('Bigger fish in the pond!', 10, 150, 22);
    const st = S.rods[lastIn(house.ps)].state;   // the help follows whoever pressed last
    hint(st === 'charge' ? 'Let go of A to cast!' : st === 'wait' ? '^ v hook depth   B reel in   START leave' : st === 'bite' ? 'A bite! Press A!' : st === 'reel' ? 'Hold A to reel   let go when it goes red' : 'Hold A, let go to cast   START leave', H - 6);
  };
  return S;
}

// ---- Bolt Ball: a paddle each side, first to 7 (ck_arcade.c ck_bolt_update, draw.c draw_bolt)
const BOLT = { TOP: 26, BOT: 226, PX: 22, PW: 6, PH: 34, BIG: 52, R: 4, WIN: 7, WAIT: 180 };
function boltScene(house) {
  const B = BOLT, S = { song: 'boltball', t: 0 }, MID = (B.TOP + B.BOT) / 2;
  const ready = () => Object.assign(S, { state: 'ready', stateT: 0, ready: [false, false], readyT: 0, who: [-1, -1], py: [MID, MID], ph: [B.PH, B.PH], big: [0, 0],
    bx: W / 2, by: MID, vx: 0, vy: 0, score: [0, 0], serveTo: 0, rally: 0, last: -1, win: -1, aim: 0, bolt: false, boltX: 0, boltY: 0, boltT: 0 });
  const serve = to => Object.assign(S, { state: 'serve', stateT: 0, serveTo: to, rally: 0, last: -1, bx: W / 2, by: MID, vx: 0, vy: 0 });
  const begin = (l, r) => { S.who = [l, r]; S.score = [0, 0]; SND.fx.confirm(); btune('boltball'); serve(Math.random() < 0.5 ? 0 : 1); };
  const face = s => s ? W - B.PX : B.PX;
  const btune = n => { S.song = n; if (SND.ready) SND.playSong(n); };
  ready();
  S.update = function () {
    S.t += 1 / 60; stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    S.stateT++;
    const ps = house.ps;
    if (S.state === 'ready') {
      ps.forEach((h, i) => { if (hit(h.m.slot, 'a') && !S.ready[i]) { S.ready[i] = true; SND.fx.select(); if (!S.readyT) S.readyT = B.WAIT; } });
      if (ps.length === 1) { if (S.ready[0]) begin(0, -1); }
      else if (S.ready[0] && S.ready[1]) begin(0, 1);
      else if (S.readyT > 0 && --S.readyT === 0) begin(S.ready[0] ? 0 : -1, S.ready[1] ? 1 : -1);   // nobody else came: play the computer
      return;
    }
    if (S.state === 'won') { if (S.stateT > 60 && anyHit('a')) { ready(); SND.fx.confirm(); btune('boltball'); } return; }
    for (let s = 0; s < 2; s++) {
      if (S.big[s] > 0 && --S.big[s] === 0) SND.fx.shrink();
      S.ph[s] += ((S.big[s] > 0 ? B.BIG : B.PH) - S.ph[s]) * 0.15;
      let v;
      if (S.who[s] >= 0) { const sl = ps[S.who[s]].m.slot; v = ((held(sl, 'down') ? 1 : 0) - (held(sl, 'up') ? 1 : 0)) * 3.4; }
      else {
        const coming = S.state === 'play' && (s ? S.vx > 0 : S.vx < 0), want = coming ? S.by + S.aim : MID, top = Math.min(2.1 + S.rally * 0.06, 3.0);
        v = clamp((want - S.py[s]) * 0.2, -top, top);
      }
      S.py[s] = clamp(S.py[s] + v, B.TOP + S.ph[s] / 2, B.BOT - S.ph[s] / 2);
    }
    if (S.state === 'serve') {
      if (S.stateT < 60) return;
      const a = (Math.random() - 0.5) * 1.0;
      S.vx = (S.serveTo ? 1 : -1) * 2.6 * Math.cos(a); S.vy = 2.6 * Math.sin(a); S.aim = 0;
      S.state = 'play'; S.stateT = 0; SND.fx.blow();
    }
    if (S.bolt) { if (--S.boltT <= 0) S.bolt = false; }
    else if (S.rally >= 3 && Math.random() < 0.004) Object.assign(S, { bolt: true, boltT: 600, boltX: 120 + Math.random() * 80, boltY: B.TOP + 30 + Math.random() * (B.BOT - B.TOP - 60) });
    for (let sub = 0; sub < 2; sub++) {   // two half steps so a fast ball can't skip a paddle
      S.bx += S.vx * 0.5; S.by += S.vy * 0.5;
      if (S.by < B.TOP + B.R) { S.by = B.TOP + B.R; S.vy = Math.abs(S.vy); SND.fx.tick(); }
      if (S.by > B.BOT - B.R) { S.by = B.BOT - B.R; S.vy = -Math.abs(S.vy); SND.fx.tick(); }
      for (let s = 0; s < 2; s++) {
        const f = face(s), d = s ? S.bx + B.R - f : f - (S.bx - B.R);
        if ((s ? S.vx <= 0 : S.vx >= 0) || d < 0 || d > B.PW + 4) continue;
        const off = (S.by - S.py[s]) / (S.ph[s] / 2 + B.R);
        if (Math.abs(off) > 1) continue;
        const sp = Math.min(Math.hypot(S.vx, S.vy) + 0.2, 6.5), a = off;
        S.vx = (s ? -1 : 1) * sp * Math.cos(a); S.vy = sp * Math.sin(a);
        S.bx = s ? f - B.R : f + B.R;
        S.rally++; S.last = s;
        S.aim = (Math.random() - 0.5) * S.ph[1 - s] * 0.9;   // the computer won't be perfect
        SND.fx.boing(); sparks(S.bx, S.by, 4, '#ffd23f');
      }
      if (S.bolt && S.last >= 0 && Math.abs(S.bx - S.boltX) < 10 && Math.abs(S.by - S.boltY) < 12) {
        S.bolt = false; S.big[S.last] = 600; SND.fx.power(); sparks(S.boltX, S.boltY, 10, '#5ad1ff');
      }
    }
    if (S.bx < -10 || S.bx > W + 10) {   // missed: the other side scores
      const s = S.bx < 0 ? 1 : 0;
      S.score[s]++;
      if (S.score[s] >= B.WIN) { S.state = 'won'; S.stateT = 0; S.win = s; SND.fx.flag(); confetti(160, 40, 40); btune('victory'); return; }
      SND.fx.pop(); serve(1 - s);
    }
  };
  const name = s => S.who[s] < 0 ? 'CPU' : house.ps.length === 1 ? 'YOU' : 'P' + (house.ps[S.who[s]].m.slot + 1);
  const hexOf = s => S.who[s] < 0 ? 0x9AA4B4 : house.ps[S.who[s]].m.hex;
  const nameCol = s => S.who[s] < 0 ? '#b8b4c8' : house.ps[S.who[s]].m.slot ? '#8fd3ff' : '#ffd23f';
  S.draw = function () {
    K.g = g;
    aBox(0, 0, W, H, 0x14102A);
    for (let k = 0; k < 30; k++) aDot((k * 97) % 320, B.TOP + (k * 53) % (B.BOT - B.TOP), ((k % 3) + 1) * 0.5, 0x3A3456);
    aBox(0, B.TOP - 4, W, 4, 0x46386E); aBox(0, B.BOT, W, 4, 0x46386E);
    for (let y = B.TOP + 4; y < B.BOT; y += 16) aBox(W / 2 - 1, y, 2, 8, 0x46386E);
    if (S.bolt) {
      const x = S.boltX, y = S.boltY;
      aDot(x, y, 11, 0x5AD1FF, 0.25 + 0.15 * Math.sin(S.t * 6));
      aLine(x + 3, y - 9, x - 3, y + 1, 2.5, 0xFFD23F); aLine(x - 3, y + 1, x + 3, y - 1, 2.5, 0xFFD23F); aLine(x + 3, y - 1, x - 3, y + 9, 2.5, 0xFFD23F);
    }
    if (S.state !== 'ready') for (let s = 0; s < 2; s++) {   // the paddles: little robots standing on end
      const x = s ? W - B.PX : B.PX - B.PW, y = S.py[s] - S.ph[s] / 2, hex = hexOf(s), eye = S.who[s] < 0 ? 0xFF5A5A : 0x1B1426;
      if (S.big[s] > 0) aBox(x - 2, y - 2, B.PW + 4, S.ph[s] + 4, 0x5AD1FF, S.big[s] < 120 && Math.floor(S.big[s] / 8) % 2 ? 0.2 : 0.5);
      aBox(x, y, B.PW, S.ph[s], hex); aBox(x + (s ? 3 : 1), y + 3, 2, S.ph[s] - 6, 0xFFFFFF, 0.35);
      aBox(s ? x - 3 : x + B.PW, S.py[s] - 5, 3, 10, hex);
      const ex = s ? x - 1 : x + B.PW + 1; aDot(ex, S.py[s] - 2, 1.2, eye); aDot(ex, S.py[s] + 2, 1.2, eye);
    }
    if (S.state === 'play' || (S.state === 'serve' && Math.floor(S.stateT / 8) % 2)) {
      const c = S.rally >= 8 ? 0xFF8A3D : 0xFFD23F;
      for (let k = 3; k >= 1; k--) aDot(S.bx - S.vx * k * 1.5, S.by - S.vy * k * 1.5, B.R - k * 0.6, c, 0.12 * (4 - k));
      aDot(S.bx, S.by, B.R, c); aDot(S.bx - 1.2, S.by - 1.2, 1.3, 0xFFF6D8);
    }
    drawParticles(0);
    if (S.state !== 'ready') {
      text(name(0), 10, 15, 7, nameCol(0), 'left'); text(name(1), W - 10, 15, 7, nameCol(1), 'right');
      text(String(S.score[0]), W / 2 - 14, 17, 10, '#fff6d8', 'right'); text(String(S.score[1]), W / 2 + 14, 17, 10, '#fff6d8', 'left');
    }
    if (S.state === 'ready') {
      rimBanner('BOLT BALL', 14, 54, 34); hint('First to 7 points wins', 104);
      house.ps.forEach((h, i) => {
        const cx = house.ps.length === 1 ? W / 2 : i ? 240 : 80;
        panel(cx - 56, 126, 112, 40);
        text('P' + (h.m.slot + 1), cx, 143, 10, h.m.slot ? '#8fd3ff' : '#ffd23f');
        text(S.ready[i] ? 'READY!' : 'A to play', cx, 159, 8, S.ready[i] ? '#49d17a' : '#fff6d8');
      });
      if (house.ps.length === 2 && S.readyT > 0) hint('P' + (house.ps[S.ready[0] ? 0 : 1].m.slot + 1) + ' vs the computer in ' + (Math.floor(S.readyT / 60) + 1) + '...', 186);
      else if (house.ps.length === 1) hint('You vs the computer', 186);
      hint('A ready   START leave', H - 6);
      return;
    }
    if (S.state === 'won') {
      rimBanner(S.who[S.win] >= 0 && house.ps.length === 1 ? 'YOU WIN!' : name(S.win) + ' WINS!', 14, 80, 34);
      hint(S.score[S.win] + ' to ' + S.score[1 - S.win], 134);
      if (S.stateT > 60) hint('A play again   START leave', 152);
      return;
    }
    hint('Stick up and down   START leave', H - 6);
  };
  return S;
}

// ---- Jet Run: the long version of the jetpack zones, five sceneries (ck_arcade.c ck_run_update, draw.c draw_jetrun)
const RUN = { COLS: 6, BOLTS: 18, FLOOR: 214, X: 64, HW: 12, HH: 20, W: 28, THEMES: 9, STAGE: 2000, LIVES: 3, BOLT_LIFE: 300, GEAR_R: 20, DRONE_R: 10,
  NAMES: ['JUNK PIPES', 'SUNSET FACTORY', 'CANDY CLOUDS', 'ICE PEAKS', 'NEON NIGHT', 'STORM CLOUDS', 'LAVA CAVES', 'STAR LANES', 'GLITCH ZONE'],
  PALS: [[0x7D9A8A, 0xA5C4B2, 0x5F7A6C, 0x8FB09C, 0xC0703A], [0xB0663A, 0xE0925A, 0x7A4026, 0xC9784A, 0xFFD23F], [0xFF8FC7, 0xFFFFFF, 0xD0609A, 0xFFB8DC, 0x5AD1FF],
    [0xA8D8F0, 0xE8F8FF, 0x78A8C8, 0xC8E8F8, 0xFFFFFF], [0x2A2A48, 0x5AD1FF, 0x18182E, 0x3A3A60, 0xFF5AC8], [0x6A7A8E, 0x9AAABE, 0x4A5868, 0x7A8AA0, 0xFFD23F],
    [0x5A4A48, 0x8A7A70, 0x3A2E2C, 0x6A5A56, 0xFF6A2A], [0xD8DCE6, 0xFFFFFF, 0x9AA0B0, 0xE6E9F0, 0xFF5A5A], [0x2A1A3A, 0x49F08A, 0x1A0E28, 0x3A2A50, 0xC28BFF]] };
const TH = { JUNK: 0, SUNSET: 1, CANDY: 2, ICE: 3, NEON: 4, STORM: 5, LAVA: 6, STARS: 7, GLITCH: 8 };
const runHash = k => { let x = Math.imul(k, 2654435761) >>> 0; x ^= x >>> 15; x = Math.imul(x, 2246822519) >>> 0; x ^= x >>> 13; return (x & 1023) / 1023; };
const runMix = (a, b, t) => { const c = s => ((a >> s) & 255) + Math.trunc((((b >> s) & 255) - ((a >> s) & 255)) * t); return (c(16) << 16) | (c(8) << 8) | c(0); };
// a mountain or stalactite out of stacked strips (run_tri)
const runTri = (xc, y, w, h, col, down) => { for (let k = 0; k < 8; k++) { const ww = w * (1 - k / 8), hh = h / 8; aBox(xc - ww / 2, down ? y + k * hh : y - (k + 1) * hh, ww, hh + 0.5, col); } };
const runLx = (k, step, sc, par, span) => (((k * step - sc * par) % span) + span) % span - 60;
function runScene(house) {
  const R = RUN, S = { song: 'jetrun', t: 0, best: 0 };
  const level = () => Math.min(S.stage, 9);
  const gapY = c => c.gy + c.amp * Math.sin(S.t * c.spd + c.ph);
  const gapH = c => c.gap + c.pulse * Math.sin(S.t * 2.2 + c.ph);   // crushers open and shut
  const mid = c => c.x + R.W / 2, radius = c => c.kind === 'gear' ? R.GEAR_R : R.DRONE_R;
  const safeY = c => {   // where a hero's middle should be to get by it (ck_run_safe_y)
    const gy = gapY(c);
    if (c.kind === 'pillar') return gy - 40;
    if (c.kind === 'hang') return gy + 40;
    if (c.kind === 'gear' || c.kind === 'drone') { const r = radius(c), up = gy - r - 14, dn = R.FLOOR - (gy + r); return up > dn ? (14 + gy - r) / 2 : (gy + r + R.FLOOR) / 2; }
    return gy;
  };
  const hitDir = (c, hx, top, bot) => {   // which way column c knocks a hero's box, or 0 (run_hit)
    const gy = gapY(c);
    if (c.kind === 'gear' || c.kind === 'drone') {
      const r = radius(c) - 2, mx = mid(c), nx = clamp(mx, hx, hx + R.HW), ny = clamp(gy, top, bot);
      if ((nx - mx) ** 2 + (ny - gy) ** 2 > r * r) return 0;
      return (top + bot) / 2 < gy ? -1 : 1;
    }
    if (hx + R.HW < c.x + 3 || hx > c.x + R.W - 3) return 0;
    const gap = gapH(c);
    if (c.kind === 'pillar') return bot > gy ? -1 : 0;
    if (c.kind === 'hang') return top < gy ? 1 : 0;
    return top < gy - gap / 2 ? 1 : bot > gy + gap / 2 ? -1 : 0;
  };
  const reset = () => Object.assign(S, { state: 'ready', stateT: 0, scroll: 0, speed: 0, t: 0, nextX: 360, lastGy: 110, lives: R.LIVES, score: 0, bolts: 0, gates: 0, stage: 0, stageT: 0, shake: 0, lifeT: 0,
    col: [...Array(R.COLS)].map(() => ({ x: -1000 })), bolt: [...Array(R.BOLTS)].map(() => ({ on: false, x: 0, y: 0 })), ps: [0, 1].map(i => ({ x: R.X, y: 80 + i * 50, vy: 0, inv: 0, on: false })), readyT: 0 });
  reset();
  const spawn = () => {
    const c = S.col.find(c => c.x < S.scroll - 60); if (!c) return;
    const lv = level(), r = Math.random(), st = S.stage;
    c.x = S.nextX; c.passed = false; c.gap = Math.max(86 - lv * 3, 60);
    c.kind = st >= 1 && r < 0.14 ? 'pillar' : st >= 2 && r < 0.26 ? 'hang' : st >= 2 && r < 0.40 ? 'gear' : st >= 3 && r < 0.52 ? 'crush' : st >= 4 && r < 0.64 ? 'drone' : 'pipes';
    const space = Math.max(155 - lv * 5, 112), reach = (space - R.W - R.HW) / Math.max(S.speed, 1.2) * 1.4;
    c.amp = 0; c.spd = 1.6; c.pulse = 0; c.ph = Math.random() * 6.28;
    if (c.kind === 'gear' || c.kind === 'drone') {
      c.gy = 70 + Math.random() * 80;
      if (c.kind === 'drone') { c.amp = 24 + lv * 2; c.spd = 2.4; } else if (st >= 5) { c.amp = 10; c.spd = 1.2; }
    } else {
      c.gy = clamp(50 + Math.random() * (R.FLOOR - 100), S.lastGy - reach, S.lastGy + reach);
      if (c.kind === 'crush') { c.gap = 80; c.pulse = 24; c.gy = clamp(c.gy, 70, R.FLOOR - 70); }
      else if (st >= 1 && Math.random() < 0.25 + lv * 0.04) c.amp = 12 + Math.random() * (8 + lv * 2);
      if (c.amp) c.gy = clamp(c.gy, 40 + c.amp, R.FLOOR - 40 - c.amp);
    }
    const was = S.t; S.t = 0; const safe = safeY(c); S.t = was;   // the resting spot
    S.lastGy = safe; S.nextX += space;
    const moving = c.amp || c.pulse || c.kind === 'drone';
    for (let k = 0, n = 0; k < R.BOLTS && n < 3; k++) {
      const b = S.bolt[k]; if (b.on && b.x > S.scroll - 20) continue;
      Object.assign(b, { x: mid(c) + (n - 1) * 16, y: safe, on: !moving || n === 1 });
      n++;
    }
  };
  const over = () => { S.state = 'over'; S.stateT = 0; S.best = Math.max(S.best, S.score); SND.fx.ouch(); };
  S.update = function () {
    stepParticles();
    if (anyHit('start')) { SND.fx.select(); house.resume(); return; }
    S.t += 1 / 60; S.stateT++; if (S.shake > 0) S.shake--;
    if (S.state === 'over') { if (S.stateT > 60 && anyHit('a')) { reset(); SND.fx.confirm(); } return; }
    if (S.state === 'ready') {
      let n = 0;   // each player joins with A; the last one has a countdown to make up their mind
      house.ps.forEach((h, i) => {
        const p = S.ps[i]; p.y = 80 + i * 50 + Math.sin(S.t * 3 + i) * 4;
        if (hit(h.m.slot, 'a') && !p.on) { p.on = true; SND.fx.select(); if (!S.readyT) S.readyT = 180; }
        n += p.on ? 1 : 0;
      });
      if (n && (n === house.ps.length || --S.readyT <= 0)) { S.state = 'play'; S.stateT = 0; S.stageT = 150; SND.fx.confirm(); }
      return;
    }
    const stage = Math.floor(S.scroll / R.STAGE);
    if (stage !== S.stage) { S.stage = stage; S.stageT = 150; if (stage % 2 === 0 && S.lives < R.LIVES) S.lives++; SND.fx.flag(); confetti(S.scroll + 160, 30, 24); }
    if (S.stageT > 0) S.stageT--;
    if (S.lifeT > 0) S.lifeT--;
    S.speed = Math.min(1.2 + level() * 0.2, 3.0) * (little ? 0.85 : 1);   // an easy start, quicker every stage
    S.scroll += S.speed;
    for (const c of S.col) if (c.kind === 'drone' && c.x > S.scroll - 60) c.x -= 0.5;   // drones fly at you
    while (S.nextX < S.scroll + W + 40) { const was = S.nextX; spawn(); if (S.nextX === was) break; }
    for (const [i, h] of house.ps.entries()) {
      const p = S.ps[i], sl = h.m.slot, up = held(sl, 'up') || held(sl, 'a'), dn = held(sl, 'down');
      if (!p.on) continue;
      p.vy += (((dn ? 1 : 0) - (up ? 1 : 0)) * 2.1 - p.vy) * 0.2;
      p.y = clamp(p.y + p.vy, 14, R.FLOOR - R.HH);
      p.x = clamp(p.x + ((held(sl, 'right') ? 1 : 0) - (held(sl, 'left') ? 1 : 0)) * 1.8, 16, 240);
      if (p.inv > 0) p.inv--;
      const hx = S.scroll + p.x - R.HW / 2;
      for (const c of S.col) {
        if (p.inv > 0) break;
        const dir = hitDir(c, hx, p.y + 2, p.y + R.HH - 2);
        if (!dir) continue;
        p.inv = 90; p.vy = dir * 2.5;
        S.shake = 12; SND.fx.clank(); puff(hx + 6, p.y + 10, 8);
        if (--S.lives <= 0) { over(); return; }
      }
      for (const b of S.bolt) {
        if (!b.on || Math.abs(b.x - (hx + R.HW / 2)) > 10 || Math.abs(b.y - (p.y + R.HH / 2)) > 13) continue;
        b.on = false; S.bolts++; S.score += 5; SND.fx.bolt(); sparks(b.x, b.y, 3, '#ffd23f');
        if (S.bolts % R.BOLT_LIFE === 0 && S.lives < R.LIVES) { S.lives++; S.lifeT = 120; SND.fx.power(); }   // every 300 bolts, a life back
      }
      if ((Math.round(S.t * 60) + i) % 3 === 0) particles.push({ x: hx - 2, y: p.y + R.HH - 2, vx: -0.6, vy: 0.8, life: 16, max: 16, r: 1.5, col: 'rgba(255,200,120,0.9)', kind: 'puff' });
    }
    const back = Math.min(...house.ps.map((h, i) => S.ps[i].on ? S.ps[i].x : 999));   // a gate counts once the hindmost hero is past it
    for (const c of S.col) if (!c.passed && c.x > -999 && c.x + R.W <= S.scroll + back - R.HW / 2) { c.passed = true; S.gates++; S.score += 10; SND.fx.tick(); }
  };
  const sky = (top, bot) => { for (let k = 0; k < 12; k++) aBox(0, k * 18, W, 18, runMix(top, bot, k / 11)); };
  function bg(th, sc, t) {
    const F = R.FLOOR;
    if (th === 0) {
      const gr = g.createLinearGradient(0, 0, 0, H); gr.addColorStop(0, '#7fc8ff'); gr.addColorStop(1, '#cdeaff'); g.fillStyle = gr; g.fillRect(0, 0, W, H);
      for (let k = 0; k < 5; k++) { const cx = runLx(k, 110, sc, 0.15, 550) + 60 - 60, cy = 26 + (k % 3) * 24; K.rr(cx, cy + 8, 50, 14, 7, 'rgba(255,255,255,0.85)'); K.rr(cx + 12, cy, 28, 14, 7, 'rgba(255,255,255,0.85)'); }
      g.fillStyle = '#9fb7a0'; g.beginPath(); g.moveTo(0, F); for (let x = 0; x <= W; x += 16) g.lineTo(x, F - 30 + Math.sin((x + sc * 0.4) * 0.03) * 10); g.lineTo(W, F); g.fill();
      for (let k = 0; k < 8; k++) { const x = runLx(k, 70, sc, 0.7, 560), h = runHash(k) * 8; aBox(x, F - 10 - h, 14, 12 + h, 0x6F5A48); aBox(x + 3, F - 6 - h, 6, 3, 0x9A8A70); }
      aBox(0, F, W, H - F, 0x8A5233); aBox(0, F, W, 3, 0xC98B55);
      for (let k = 0; k < 12; k++) aBox(runLx(k, 32, sc, 1, 384), F + 8 + (k % 3) * 5, 6, 2, 0x6F4026);
    } else if (th === 1) {
      sky(0x5A3A8A, 0xFFA060);
      aDot(232, 150, 44, 0xFFC27A, 0.35); aDot(232, 150, 30, 0xFFE08A);
      for (let k = 0; k < 7; k++) { const x = runLx(k, 90, sc, 0.18, 630), h = 40 + runHash(k) * 50, w = 50 + runHash(k + 9) * 30; aBox(x, F - h, w, h, 0x7A3A5A); aBox(x + w * 0.6, F - h - 26, 8, 26, 0x7A3A5A); }
      for (let k = 0; k < 6; k++) {
        const x = runLx(k, 120, sc, 0.45, 720), h = 30 + runHash(k + 30) * 40, w = 60 + runHash(k + 40) * 30;
        aBox(x, F - h, w, h, 0x4A2440);
        for (let j = 0; j < 3; j++) aBox(x + 8 + j * 16, F - h + 8, 8, 6, (j + k) % 3 ? 0xFFB050 : 0x6A3050);
        const cx = x + w - 14, cy = F - h - 34; aBox(cx, cy, 10, 34, 0x3A1A34); aBox(cx - 1, cy, 12, 3, 0xC0503A);
        for (let j = 0; j < 4; j++) { const u = (t * 0.5 + j * 0.25 + runHash(k)) % 1; aDot(cx + 5 - u * 18, cy - 4 - u * 40, 4 + u * 7, 0xC8A0B0, 0.55 * (1 - u)); }
      }
      aBox(0, F, W, H - F, 0x3A2030); aBox(0, F, W, 2, 0xFF9A5A);
      for (let k = 0; k < 14; k++) aBox(runLx(k, 28, sc, 1, 392), F + 6, 14, 4, 0x5A3048);
      aBox(0, F + 5, W, 2, 0x9A6A6A); aBox(0, F + 11, W, 2, 0x9A6A6A);
    } else if (th === TH.NEON) {
      sky(0x08081E, 0x3A1A5A);
      for (let k = 0; k < 40; k++) aDot(runLx(k, 37, sc, 0.03, 400), 6 + runHash(k) * 110, 0.6 + runHash(k + 3) * 0.8, 0xFFFFFF, 0.4 + 0.6 * Math.abs(Math.sin(t * 2 + k)));
      aDot(60, 44, 16, 0xFFF6D8, 0.2); aDot(60, 44, 12, 0xF0E8D0); aDot(56, 41, 2.5, 0xD8D0B8); aDot(64, 48, 1.8, 0xD8D0B8);
      for (let k = 0; k < 9; k++) {
        const x = runLx(k, 64, sc, 0.2, 576), h = 60 + runHash(k + 50) * 70;
        aBox(x, F - h, 46, h, 0x1C1C40);
        for (let wy = 0; wy < h - 10; wy += 10) for (let wx = 0; wx < 4; wx++) if (runHash(k * 97 + wy + wx * 13) > 0.55) aBox(x + 5 + wx * 10, F - h + 6 + wy, 4, 4, 0xFFE066, 0.7);
      }
      for (let k = 0; k < 7; k++) {
        const x = runLx(k, 100, sc, 0.5, 700), h = 40 + runHash(k + 70) * 50, neon = k % 2 ? 0xFF5AC8 : 0x5AD1FF, gl = 0.6 + 0.4 * Math.sin(t * 4 + k * 1.7);
        aBox(x, F - h, 70, h, 0x101028); aBox(x + 8, F - h + 10, 54, 3, neon, gl); aBox(x + 8, F - h + 18, 30, 3, neon, gl * 0.8); aBox(x, F - h, 70, 2, neon, 0.5);
      }
      aBox(0, F, W, H - F, 0x14142A); aBox(0, F, W, 2, 0x5AD1FF);
      for (let k = 0; k < 10; k++) aBox(runLx(k, 40, sc, 1, 400), F + 12, 20, 2, 0xFFE066, 0.8);
    } else if (th === TH.STORM) {
      const cyc = t % 4.3;
      sky(0x2E3440, 0x6A7488);
      if (cyc < 0.25) {
        aBox(0, 0, W, H, 0xFFFFFF, 0.5 * (1 - cyc / 0.25));
        let lx = 40 + runHash(Math.floor(t / 4.3)) * 240, ly = 0;
        for (let s = 0; s < 6; s++) { const nx = lx + (runHash(Math.floor(t / 4.3) * 7 + s) - 0.5) * 30, ny = ly + 22; aLine(lx, ly, nx, ny, 2.5, 0xFFF6A0); lx = nx; ly = ny; }
      }
      for (let k = 0; k < 8; k++) aDot(runLx(k, 80, sc, 0.12, 640), 40 + runHash(k) * 30, 30 + runHash(k + 5) * 12, 0x4A5262);
      for (let k = 0; k < 7; k++) { const x = runLx(k, 100, sc, 0.3, 700); aDot(x, 8 + runHash(k + 20) * 14, 34, 0x2A303C); aDot(x + 30, 2, 28, 0x2A303C); }
      for (let k = 0; k < 9; k++) aDot(runLx(k, 72, sc, 0.55, 648), F + 4, 26 + runHash(k + 40) * 10, 0x3A4250);
      for (let k = 0; k < 50; k++) { const rx = ((k * 53 - t * 90 - sc * 0.8 + 4000) % 340) - 10, ry = (k * 31 + t * 260) % 240; aLine(rx, ry, rx - 3, ry + 9, 1.5, 0xB8C8E0, 0.7); }
      aBox(0, F, W, H - F, 0x2A3038); aBox(0, F, W, 2, 0x8A9AB0);
      for (let k = 0; k < 6; k++) aBox(runLx(k, 70, sc, 1, 420), F + 8 + (k % 2) * 6, 26, 2, 0x8A9AB0, 0.5);
    } else if (th === TH.CANDY) {
      sky(0xFFB0D8, 0xFFE8C8);
      for (let k = 0; k < 6; k++) { const x = runLx(k, 100, sc, 0.12, 600), y = 30 + runHash(k) * 50; aDot(x, y, 16, 0xFFFFFF, 0.9); aDot(x + 16, y - 6, 18, 0xFFF0F8, 0.9); aDot(x + 32, y, 14, 0xFFFFFF, 0.9); }
      for (let k = 0; k < 7; k++) { const x = runLx(k, 90, sc, 0.3, 630); aDot(x, F + 10, 40 + runHash(k + 4) * 16, k % 2 ? 0xC28BFF : 0xFF8FC7); aDot(x - 10, F - 18, 8, 0xFFFFFF, 0.5); }
      for (let k = 0; k < 6; k++) {
        const x = runLx(k, 110, sc, 0.6, 660), h = 40 + runHash(k + 9) * 30, c = k % 3 === 0 ? 0xFF5A8A : k % 3 === 1 ? 0x5AD1FF : 0xFFD23F;
        aBox(x - 1.5, F - h, 3, h, 0xFFFFFF); aDot(x, F - h, 12, c); aDot(x - 2, F - h - 2, 6, 0xFFFFFF, 0.6); aDot(x - 2, F - h - 2, 3, c);
      }
      aBox(0, F, W, H - F, 0xFFF0F6);
      for (let k = 0; k < 16; k++) aBox(runLx(k, 24, sc, 1, 384), F, 12, H - F, 0xFF8FC7);
      aBox(0, F, W, 3, 0xFFFFFF);
    } else if (th === TH.ICE) {
      sky(0x5A98D8, 0xD8F0FF);
      for (let k = 0; k < 6; k++) { const x = runLx(k, 110, sc, 0.15, 660) + 50, w = 120 + runHash(k) * 60, h = 90 + runHash(k + 3) * 50; runTri(x, F, w, h, 0xA8C8E8, false); runTri(x, F - h * 0.7, w * 0.3, h * 0.3, 0xFFFFFF, false); }
      for (let k = 0; k < 6; k++) { const x = runLx(k, 130, sc, 0.4, 780) + 50, w = 100 + runHash(k + 20) * 50, h = 60 + runHash(k + 23) * 40; runTri(x, F, w, h, 0x7898C0, false); runTri(x, F - h * 0.72, w * 0.28, h * 0.28, 0xF0F8FF, false); }
      for (let k = 0; k < 40; k++) { const fx = ((k * 41 - t * 20 - sc * 0.5 + 4000) % 330) - 5, fy = (k * 29 + t * (25 + (k % 3) * 10)) % 240; aDot(fx + Math.sin(t * 2 + k) * 3, fy, 1 + (k % 3) * 0.5, 0xFFFFFF, 0.9); }
      aBox(0, F, W, H - F, 0xF0F8FF); aBox(0, F, W, 3, 0xFFFFFF);
      for (let k = 0; k < 8; k++) aBox(runLx(k, 50, sc, 1, 400), F + 8 + (k % 2) * 6, 18, 2, 0xC8E0F0);
    } else if (th === TH.LAVA) {
      sky(0x140606, 0x4A1A10);
      for (let k = 0; k < 6; k++) aDot(runLx(k, 110, sc, 0.1, 660), 120 + runHash(k) * 60, 40, 0xFF5A2A, 0.12);
      for (let k = 0; k < 9; k++) runTri(runLx(k, 50, sc, 0.25, 450), 0, 30 + runHash(k) * 20, 40 + runHash(k + 2) * 40, 0x2A1410, true);
      for (let k = 0; k < 7; k++) { const x = runLx(k, 80, sc, 0.5, 560); runTri(x, 0, 24 + runHash(k + 30) * 14, 24 + runHash(k + 31) * 30, 0x3A1E18, true); runTri(x + 20, F, 30, 30 + runHash(k + 33) * 30, 0x3A1E18, false); }
      for (let k = 0; k < 24; k++) { const u = (t * 0.3 + runHash(k)) % 1; aDot(runLx(k, 31, sc, 0.8, 380) + Math.sin(t * 3 + k) * 4, F - u * 200, 1.2, k % 2 ? 0xFFB040 : 0xFF6A2A, 1 - u); }
      aBox(0, F, W, H - F, 0xFF6A2A);
      for (let x = 0; x < W; x += 8) aBox(x, F - 2 + Math.sin(x * 0.15 + t * 3 + sc * 0.05) * 2, 8, 4, 0xFFB040);
      for (let k = 0; k < 6; k++) { const u = (t * 0.8 + runHash(k + 50)) % 1; aDot(runLx(k, 64, sc, 1, 384), F + 10, 2 + u * 3, 0xFFD23F, 1 - u); }
    } else if (th === TH.GLITCH) {
      aBox(0, 0, W, H, 0x0E0618);
      for (let y = 10; y < F; y += 20) aBox(0, y, W, 1, 0x2A1240);
      for (let k = 0; k < 12; k++) aBox(runLx(k, 32, sc, 0.5, 384) + 60, 0, 1, F, 0x2A1240);
      const tick = Math.floor(t * 8);
      for (let k = 0; k < 10; k++) { const h1 = runHash(tick * 13 + k), h2 = runHash(tick * 7 + k * 3); if (h1 < 0.5) continue; aBox(h2 * 300, runHash(k * 5 + tick) * 200, 10 + h1 * 30, 3 + h2 * 6, k % 2 ? 0x49F08A : 0xFF5AC8, 0.35); }
      for (let k = 0; k < 8; k++) { const x = runLx(k, 80, sc, 0.35, 640); aBox(x, F - 50 - runHash(k) * 40, 40, 90, 0x1A0E28); aBox(x + 4, F - 46 - runHash(k) * 40, 32, 2, 0x49F08A, 0.5 + 0.5 * Math.sin(t * 5 + k)); }
      aBox(0, F, W, H - F, 0x1A0E28); aBox(0, F, W, 2, 0x49F08A);
      for (let k = 0; k < 14; k++) aBox(runLx(k, 28, sc, 1, 392), F + 6 + (k % 3) * 5, 6, 4, k % 2 ? 0xC28BFF : 0x49F08A, 0.6);
      if (runHash(tick) > 0.9) aBox(0, runHash(tick + 1) * 200, W, 6, 0xFFFFFF, 0.15);
    } else {
      aBox(0, 0, W, H, 0x05030F);
      { const mx = 70 - (sc * 0.01) % 400; aDot(mx, 150, 9, 0x8FA0C0); aDot(mx - 2, 148, 3, 0xB8C8E0); }
      for (let L = 0; L < 3; L++) for (let k = 0; k < 24; k++) aDot(runLx(k + L * 40, 23 + L * 7, sc, 0.04 + L * 0.12, 400), runHash(k * 3 + L * 100) * 200, 0.5 + L * 0.4, L === 2 ? 0xFFFFFF : 0xB8C0E0, 0.5 + L * 0.25);
      const px = 250 - (sc * 0.02) % 400; aDot(px, 62, 26, 0xC98A4B); aDot(px - 7, 55, 9, 0xE0A86A); aBox(px - 42, 61, 84, 3, 0xF0D8B0, 0.8);
      aBox(0, F, W, H - F, 0x8F8AA8); aBox(0, F, W, 2, 0xC0BCD4);
      for (let k = 0; k < 8; k++) aDot(runLx(k, 52, sc, 1, 416), F + 10 + (k % 2) * 6, 5 + (k % 3) * 2, 0x6E6A80);
    }
  }
  function pipe(x, y0, y1, lipY, P, th, t) {
    if (y1 <= y0) return;
    const [body, light, dark, lip, accent] = P;
    aBox(x + 2, y0, R.W - 4, y1 - y0, body); aBox(x + 5, y0, 3, y1 - y0, light, th === TH.NEON || th === TH.GLITCH ? 0.8 : 1); aBox(x + R.W - 8, y0, 4, y1 - y0, dark);
    aBox(x - 2, lipY, R.W + 4, 10, lip); aBox(x - 2, lipY, R.W + 4, 2, light, 0.7);
    aBox(x + 6, lipY + 4, 8, 3, accent, th === TH.STARS || th === TH.GLITCH ? (Math.sin(t * 5 + x) > 0 ? 1 : 0.3) : 1);
    aDot(x + 2, lipY + 5, 1.3, dark); aDot(x + R.W - 2, lipY + 5, 1.3, dark);
  }
  S.draw = function () {
    K.g = g;
    const th = S.stage % R.THEMES, P = R.PALS[th], sc = S.scroll + (S.shake > 0 ? Math.sin(S.shake * 2.3) * 3 : 0), t = S.t;
    bg(th, sc, t);
    for (const c of S.col) {
      const x = c.x - sc; if (x < -40 || x > W + 10) continue;
      const gy = gapY(c);
      if (c.kind === 'pillar') {
        for (let y = gy; y < R.FLOOR; y += 14) { const j = Math.floor((y - gy) / 14); aBox(x + (j % 2), y, R.W - (j % 2) * 2, 13, j % 2 ? P[3] : P[2]); aBox(x + 6, y + 4, R.W - 12, 4, j % 2 ? P[1] : P[0]); }
        aBox(x - 2, gy - 3, R.W + 4, 4, P[4]);
      } else if (c.kind === 'gear') {   // a spinning gear
        const mx = mid(c) - sc, a = t * 2.5 + c.ph;
        aDot(mx, gy, R.GEAR_R - 3, P[2]);
        for (let k = 0; k < 8; k++) aDot(mx + Math.cos(a + k * 0.785) * (R.GEAR_R - 3), gy + Math.sin(a + k * 0.785) * (R.GEAR_R - 3), 4, P[0]);
        aDot(mx, gy, R.GEAR_R - 7, P[0]);
        for (let k = 0; k < 3; k++) aLine(mx, gy, mx + Math.cos(a + k * 2.09) * 10, gy + Math.sin(a + k * 2.09) * 10, 2, P[2]);
        aDot(mx, gy, 4, P[4]);
      } else if (c.kind === 'drone') {   // a glitch drone: a body, two eyes, a whirring rotor
        const mx = mid(c) - sc, rw = 6 + 6 * Math.abs(Math.sin(t * 30 + c.ph));
        aDot(mx, gy, R.DRONE_R, 0xC23A6A); aDot(mx - 1, gy + 1, R.DRONE_R - 3, 0xFF5A8A);
        aDot(mx - 4, gy - 1, 2.5, 0xFFFFFF); aDot(mx + 2, gy - 1, 2.5, 0xFFFFFF); aDot(mx - 5, gy - 1, 1.2, 0x1B1426); aDot(mx + 1, gy - 1, 1.2, 0x1B1426);
        aBox(mx - 1, gy - R.DRONE_R - 4, 2, 4, 0x9AA1B0); aBox(mx - rw, gy - R.DRONE_R - 5, rw * 2, 2, 0xE6E6F0);
      } else {
        const gap = gapH(c), top = c.kind === 'hang' ? gy : gy - gap / 2;
        pipe(x, 0, top, top - 10, P, th, t);
        if (c.kind !== 'hang') pipe(x, gy + gap / 2, R.FLOOR, gy + gap / 2, P, th, t);
        if (c.kind === 'crush') for (let k = 0; k < 4; k++) { aBox(x - 2 + k * 8, top - 10, 4, 10, 0xFFD23F); aBox(x - 2 + k * 8, gy + gap / 2, 4, 10, 0xFFD23F); }
        if (c.amp) aDot(x + R.W / 2, top - 5, 2, 0x5AD1FF, 0.6 + 0.4 * Math.sin(t * 8));
      }
    }
    S.bolt.forEach((b, k) => { if (!b.on) return; const x = b.x - sc; if (x < -10 || x > W + 10) return; g.save(); g.translate(x, b.y); g.scale(Math.abs(Math.cos(t * 5 + k)) * 0.8 + 0.2, 1); K.nut(0, 0, 4.2, 0, '#ffd23f'); g.restore(); });
    if (S.speed > 2.2 && S.state === 'play') for (let k = 0; k < 10; k++) { const lx = ((k * 97 - S.scroll * 3 + 4000) % 360) - 20; aBox(lx, 20 + runHash(k) * 180, 24 + (k % 3) * 10, 1.5, 0xFFFFFF, 0.25); }
    drawParticles(sc);
    if (S.state !== 'over') house.ps.forEach((h, i) => {
      const p = S.ps[i]; if ((S.state === 'play' && !p.on) || (p.inv > 0 && Math.floor(p.inv / 4) % 2 === 0)) return;
      const m = h.m, fx = p.x, fy = p.y + R.HH + 4, tt = t + i;
      if (m.hero === 'robot') { K.rr(fx - 9, fy - 22, 6, 11, 2, '#9aa1b0'); K.flame(fx - 6, fy - 10, 30, 14, tt); K.robotSide(fx, fy, 0.074, m.hex, Object.assign(K.pose('robot', 'jet', tt), { dir: 1, rot: 0.15, acc: m.acc })); }
      else { K.rr(fx - 10, fy - 22, 7, 11, 2, '#9aa1b0'); K.flame(fx - 6.5, fy - 10, 30, 14, tt); K.ostrichSide(fx, fy, 0.071, m.hex, Object.assign(K.pose('ostrich', 'jump', tt), { dir: 1, rot: 0.2, acc: m.acc })); }
    });
    if (S.stageT > 120 && S.state === 'play' && S.stage > 0) aBox(0, 0, W, H, 0xFFFFFF, (S.stageT - 120) / 30 * 0.7);
    if (S.stageT > 0 && S.state === 'play') {
      const k = S.stageT > 130 ? (150 - S.stageT) / 20 : S.stageT < 20 ? S.stageT / 20 : 1, dy = (1 - k) * 30;
      panel(W / 2 - 80, 78 - dy, 160, 42); text('STAGE ' + (S.stage + 1), W / 2, 94 - dy, 8, '#fff6d8', 'center', null); text(R.NAMES[th], W / 2, 112 - dy, 11, '#ffd23f');
    }
    if (S.lifeT > 0) rimBanner('EXTRA LIFE!', 10, 40, 20, '#49d17a', '#49d17a');
    panel(4, 4, 62, 15); K.nut(13, 11.5, 4, t, '#ffd23f'); text('×' + S.bolts, 20, 16, 9, '#fff6d8', 'left', null);
    text('SCORE ' + S.score, W / 2, 12, 8, '#ffd23f'); text('BEST ' + S.best, W / 2, 22, 7, '#49d17a');
    for (let k = 0; k < R.LIVES; k++) { const x = W - 18 - k * 16; aBox(x, 6, 12, 8, 0x1B1426); aBox(x + 12, 8, 2, 4, 0x1B1426); if (k < S.lives) aBox(x + 1.5, 7.5, 9, 5, 0x49D17A); }
    if (S.state === 'ready') {
      rimBanner('JET RUN', 14, 50, 34); hint('▲ or A fly up   ▼ go down   ◀ ▶ too', 102);
      house.ps.forEach((h, i) => {
        const cx = house.ps.length === 1 ? W / 2 : i ? 230 : 90;
        panel(cx - 56, 120, 112, 40);
        text('P' + (h.m.slot + 1), cx, 137, 10, h.m.slot ? '#8fd3ff' : '#ffd23f');
        text(S.ps[i].on ? 'READY!' : 'A to fly', cx, 153, 8, S.ps[i].on ? '#49d17a' : '#fff6d8');
      });
      if (house.ps.length === 2 && S.readyT > 0) hint('Taking off in ' + (Math.floor(S.readyT / 60) + 1) + '...', 180);
      hint('A join   START leave', H - 6);
    }
    else if (S.state === 'over') gameOver(S.score, S.best);
  };
  return S;
}
