// The animated title screen (from the approved title mock). makeTitle(g, W, H, reduce) -> { frame(t), nextPal() }
function makeTitle(g, W, H, reduce) {
  // ---------- colour helpers (the panel's one-hue shading) ----------
  const shade = (hex, f) => {
    const r = Math.min(255, ((hex >> 16) & 255) * f) | 0;
    const gg = Math.min(255, ((hex >> 8) & 255) * f) | 0;
    const b = Math.min(255, (hex & 255) * f) | 0;
    return `rgb(${r},${gg},${b})`;
  };
  const PALS = [[0x5AD1FF, 0xFFD23F], [0xFF7A59, 0x49F08A], [0xC28BFF, 0x3BF0FF], [0x7FA7C9, 0xFF8FC7]];
  let pal = 0;

  // ---------- primitives that mirror firmware/main/face.c ----------
  function rr(x, y, w, h, r, c) {
    r = Math.min(r, w / 2, h / 2);
    g.fillStyle = c; g.beginPath();
    g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r);
    g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); g.fill();
  }
  function circ(x, y, r, c) { g.fillStyle = c; g.beginPath(); g.arc(x, y, r, 0, Math.PI * 2); g.fill(); }
  // draw_limb: 0 = straight down, positive swings LEFT; a capsule with a ball on the end
  function limb(x, y, deg, ln, w, c) {
    const a = deg * Math.PI / 180, ex = -Math.sin(a), ey = Math.cos(a);
    g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round';
    g.beginPath(); g.moveTo(x, y); g.lineTo(x + ex * ln, y + ey * ln); g.stroke();
    circ(x + ex * ln, y + ey * ln, w * 0.62, c);
    return [x + ex * ln, y + ey * ln];
  }
  function arcStroke(cx, cy, r, a0, a1, w, c) {
    g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round';
    g.beginPath(); g.arc(cx, cy, r, a0, a1); g.stroke();
  }
  // The eye: white rounded box, dark pupil, highlight; a raised lower lid (in the face colour) makes it happy
  function eye(cx, cy, w, h, dark, face, open, happy, look) {
    const hh = h * open;
    if (hh < 6) { rr(cx - w / 2, cy - 3, w, 6, 3, dark); return; }
    rr(cx - w / 2, cy - hh / 2, w, hh, Math.min(w, hh) * 0.38, '#f6f9fc');
    g.save();
    g.beginPath(); g.rect(cx - w / 2, cy - hh / 2, w, hh); g.clip();
    const pr = w * 0.33;
    circ(cx + look * w * 0.12, cy, pr, dark);
    circ(cx + look * w * 0.12 - pr * 0.33, cy - pr * 0.4, pr * 0.3, '#ffffff');
    if (happy > 0) {
      g.fillStyle = face; g.beginPath();
      g.ellipse(cx, cy + hh / 2 + hh * (0.55 - happy * 0.5), w * 0.8, hh * 0.5, 0, 0, Math.PI * 2); g.fill();
    }
    g.restore();
  }

  // ---------- the robot (draw_robot, panel units; feet at local y≈171) ----------
  function robot(x, feetY, s, hex, t, pose) {
    const col = shade(hex, 1), dark = shade(hex, 0.22), torso = shade(hex, 0.88),
      plate = shade(hex, 0.55), lim = shade(hex, 0.78);
    g.save(); g.translate(x, feetY - 171 * s); g.scale(s, s);
    const bob = Math.sin(t * 3) * 3, tilt = Math.sin(t * 1.5) * 4;
    g.translate(0, bob);
    const HY = -96, HW = 108, HH = 88;
    limb(-34, 98, 6, 54 - bob, 30, lim); limb(34, 98, -6, 54 - bob, 30, lim);
    limb(-66, 6, 22, 74, 28, lim);
    const wave = pose.wave ? -150 + Math.sin(t * 8) * 22 : -22;
    if (!pose.wave) limb(66, 6, wave, 74, 28, lim);
    rr(-72, -26, 144, 132, 40, torso);
    rr(-26, 10, 52, 40, 12, plate);
    g.fillStyle = col; g.fillRect(tilt - 4, HY - HH - 30, 8, 30);
    circ(tilt, HY - HH - 36, 11, col);
    rr(tilt - HW, HY - HH, HW * 2, HH * 2, 40, col);
    const ex = HW * 0.43, ey = HY - HH * 0.17;
    const blink = blinkAt(t, 0.3);
    eye(tilt - ex, ey, 52, 60, dark, col, blink, 0.45, Math.sin(t * 0.7) * 0.5);
    eye(tilt + ex, ey, 52, 60, dark, col, blink, 0.45, Math.sin(t * 0.7) * 0.5);
    arcStroke(tilt, HY + HH * 0.52 - 16, 30, Math.PI * 0.15, Math.PI * 0.85, 9, dark);
    if (pose.wave) limb(66, 6, wave, 74, 28, lim);   // a raised arm is drawn over the head
    g.restore();
  }
  function blinkAt(t, phase) {
    const p = (t + phase) % 3.6;
    return p < 0.12 ? 0.08 : 1;
  }

  // ---------- the robot ostrich (draw_ostrich from ostrich-mock.py; feet at local y≈429) ----------
  function ostrich(x, feetY, s, hex, t) {
    const col = shade(hex, 1), head = shade(hex, 0.94), body = shade(hex, 0.86), neck = shade(hex, 0.76),
      leg = shade(hex, 0.70), beak = shade(hex, 0.52), wing = shade(hex, 0.64), dark = shade(hex, 0.22);
    g.save(); g.translate(x, feetY - 429 * s); g.scale(s, s);
    const ox = 0, bob = Math.abs(Math.sin(t * 2.6)) * 10;
    // tail plumes, flicking
    const flick = Math.sin(t * 2.6) * 6;
    for (const [deg, ln] of [[108, 76], [126, 88], [144, 72]]) limb(ox - 56, 250 + bob, deg + flick, ln, 16, col);
    // long legs, three toes
    for (const side of [-1, 1]) {
      const hx = ox + side * 30;
      const [ex, ey] = limb(hx, 318 + bob, side * 3, 92 - bob, 20, leg);
      for (const td of [-62, 0, 62]) limb(ex, ey, td, 19, 11, leg);
    }
    g.translate(0, bob);
    rr(ox - 72, 222, 144, 112, 54, body);
    rr(ox + 2, 244, 74, 66, 30, wing);
    for (const r of [20, 34, 48]) arcStroke(ox + 4, 252, r, Math.PI * 0.06, Math.PI * 0.44, 3, col);
    for (let i = 0; i < 7; i++) {
      const tt = i / 6, sw = 50 - 16 * tt, y = 218 - i * 14;
      rr(ox - sw / 2 + 7 * tt, y, sw, 11, 5, neck);
    }
    const hx = ox + 7;
    for (const [deg, ln] of [[-18, 42], [0, 50], [18, 42]]) limb(hx, 62, deg + 180 + Math.sin(t * 5) * 4, ln, 9, col);
    rr(hx - 64, 58, 128, 98, 44, head);
    for (const [w, h, y] of [[42, 13, 132], [33, 12, 143], [23, 11, 153], [13, 10, 162]]) rr(hx - w / 2, y, w, h, 5, beak);
    g.fillStyle = shade(hex, 0.28); g.fillRect(hx - 16, 147, 32, 2);
    const blink = blinkAt(t, 1.7);
    eye(hx - 31, 100, 45, 53, dark, head, blink, 0.4, Math.sin(t * 0.7 + 1) * 0.5);
    eye(hx + 31, 100, 45, 53, dark, head, blink, 0.4, Math.sin(t * 0.7 + 1) * 0.5);
    g.restore();
  }

  // ---------- scenery ----------
  function rng(seed) { return () => (seed = (seed * 16807) % 2147483647) / 2147483647; }
  function hillPath(baseY, amp, seed, step) {
    const r = rng(seed), pts = [];
    for (let x = -10; x <= W + 10; x += step) pts.push([x, baseY - r() * amp]);
    return pts;
  }
  const FAR = hillPath(150, 26, 7, 22), MID = hillPath(176, 20, 19, 16);
  const STARS = Array.from({ length: 26 }, (_, i) => { const r = rng(100 + i); return [r() * W, r() * 70, r()]; });
  const JUNK = (() => { const r = rng(42), out = []; for (let i = 0; i < 18; i++) out.push([r() * W, r(), r() * 3 | 0]); return out; })();

  function smoothHill(pts, fill) {
    g.fillStyle = fill; g.beginPath(); g.moveTo(-10, H);
    g.lineTo(pts[0][0], pts[0][1]);
    for (let i = 1; i < pts.length; i++) {
      const [x0, y0] = pts[i - 1], [x1, y1] = pts[i];
      g.quadraticCurveTo(x0, y0, (x0 + x1) / 2, (y0 + y1) / 2);
    }
    g.lineTo(W + 10, H); g.closePath(); g.fill();
  }
  function hillY(pts, x) {
    for (let i = 1; i < pts.length; i++) if (pts[i][0] >= x) {
      const [x0, y0] = pts[i - 1], [x1, y1] = pts[i], k = (x - x0) / (x1 - x0); return y0 + (y1 - y0) * k;
    }
    return pts[pts.length - 1][1];
  }
  function gear(x, y, r, teeth, rot, c, hole) {
    g.save(); g.translate(x, y); g.rotate(rot); g.fillStyle = c;
    g.beginPath();
    for (let i = 0; i < teeth * 2; i++) {
      const a = i / (teeth * 2) * Math.PI * 2, rad = i % 2 ? r * 0.78 : r;
      const a2 = (i + 1) / (teeth * 2) * Math.PI * 2;
      g.lineTo(Math.cos(a) * rad, Math.sin(a) * rad); g.lineTo(Math.cos(a2) * rad, Math.sin(a2) * rad);
    }
    g.closePath(); g.fill();
    circ(0, 0, r * 0.32, hole); g.restore();
  }
  function tyre(x, y, r) { circ(x, y, r, '#25222c'); circ(x, y, r * 0.55, '#3c3846'); circ(x, y, r * 0.28, '#25222c'); }
  function nut(x, y, r, rot, c) {
    g.save(); g.translate(x, y); g.rotate(rot); g.fillStyle = c; g.beginPath();
    for (let i = 0; i < 6; i++) { const a = i / 6 * Math.PI * 2; g.lineTo(Math.cos(a) * r, Math.sin(a) * r); }
    g.closePath(); g.fill(); circ(0, 0, r * 0.42, 'rgba(60,40,10,0.7)'); g.restore();
  }

  function sky(t) {
    const gr = g.createLinearGradient(0, 0, 0, 170);
    gr.addColorStop(0, '#26306e'); gr.addColorStop(0.45, '#6a4f9a'); gr.addColorStop(0.8, '#f08a6a'); gr.addColorStop(1, '#ffc27a');
    g.fillStyle = gr; g.fillRect(0, 0, W, H);
    for (const [x, y, k] of STARS) { g.globalAlpha = 0.25 + 0.5 * Math.abs(Math.sin(t * (0.8 + k) + k * 9)); circ(x, y, k > 0.7 ? 1.1 : 0.7, '#fff6d8'); }
    g.globalAlpha = 1;
    // the rising sun, with a gear rim
    const sy = 150;
    g.globalAlpha = 0.35; circ(160, sy, 54, '#ffd98a'); g.globalAlpha = 1;
    gear(160, sy, 40, 14, t * 0.05, '#ffe7a3', '#ffe7a3');
    circ(160, sy, 33, '#fff1c4');
    // drifting clouds
    for (let i = 0; i < 3; i++) {
      const cx = ((i * 130 + t * (6 + i * 2)) % (W + 120)) - 60, cy = 40 + i * 22;
      g.globalAlpha = 0.5; rr(cx, cy, 54, 12, 6, '#f7c2b0'); rr(cx + 12, cy - 7, 28, 12, 6, '#f7c2b0'); g.globalAlpha = 1;
    }
  }

  function scenery(t) {
    smoothHill(FAR, '#7a4f86');
    // far junk silhouettes: a crane and piles
    g.fillStyle = '#6a4378';
    g.fillRect(262, 92, 4, 48); g.fillRect(232, 92, 50, 4); g.fillRect(236, 96, 2, 18); rr(230, 113, 14, 6, 2, '#6a4378');
    smoothHill(MID, '#57365f');
    for (const [fx, k, kind] of JUNK) {
      const y = hillY(MID, fx) + 2;
      if (kind === 0) tyre(fx, y, 4 + k * 3);
      else if (kind === 1) gear(fx, y, 5 + k * 4, 8, k * 3, '#4a2d52', '#57365f');
      else rr(fx - 6, y - 7, 12 + k * 6, 8, 2, '#4a2d52');
    }
    // near ground
    const gr = g.createLinearGradient(0, 196, 0, H);
    gr.addColorStop(0, '#a46a43'); gr.addColorStop(0.18, '#7d4e33'); gr.addColorStop(1, '#4e3022');
    g.fillStyle = gr; g.beginPath(); g.moveTo(0, H);
    g.lineTo(0, 202); g.quadraticCurveTo(80, 194, 160, 200); g.quadraticCurveTo(250, 206, 320, 198); g.lineTo(320, H); g.fill();
    // little bits in the dirt
    const r = rng(9);
    for (let i = 0; i < 40; i++) { g.globalAlpha = 0.5; circ(r() * W, 206 + r() * 34, r() * 1.4 + 0.4, '#c48a5c'); }
    g.globalAlpha = 1;
  }

  // ---------- the logo: riveted metal letters that clank in turn ----------
  function logo(t) {
    const word = 'CLANKERS', size = 38;
    g.font = `${size}px "Titan One", "Arial Black", Impact, sans-serif`;
    g.textBaseline = 'alphabetic'; g.textAlign = 'left';
    const widths = [...word].map(ch => g.measureText(ch).width + 1);
    const total = widths.reduce((a, b) => a + b, 0);
    let x = (W - total) / 2;
    const baseY = 62;
    [...word].forEach((ch, i) => {
      const ph = (t * 1.6 - i * 0.16) % 2.2;
      const hop = ph > 0 && ph < 0.35 ? Math.sin(ph / 0.35 * Math.PI) * 5 : 0;
      const rot = (i % 2 ? 1 : -1) * 0.05 + (hop ? (i % 2 ? -0.04 : 0.04) : 0);
      const cx = x + widths[i] / 2;
      g.save(); g.translate(cx, baseY - hop); g.rotate(rot);
      // drop shadow + thick outline
      g.lineJoin = 'round';
      g.fillStyle = '#1b1426'; g.fillText(ch, -widths[i] / 2 + 2, 4);
      g.strokeStyle = '#1b1426'; g.lineWidth = 6; g.strokeText(ch, -widths[i] / 2, 0);
      const mg = g.createLinearGradient(0, -size * 0.75, 0, 2);
      mg.addColorStop(0, '#fbfdff'); mg.addColorStop(0.45, '#b8c6d6'); mg.addColorStop(0.55, '#8c9cb0'); mg.addColorStop(1, '#d4dde8');
      g.fillStyle = mg; g.fillText(ch, -widths[i] / 2, 0);
      // rivets
      circ(-widths[i] * 0.22, -size * 0.6, 1.6, '#5d6a7c'); circ(widths[i] * 0.2, -size * 0.12, 1.6, '#5d6a7c');
      g.restore();
      x += widths[i];
    });
    // a bolt as the underline: a strip of steel with nuts on the ends
    rr(W / 2 - 92, 72, 184, 6, 3, '#1b1426');
    rr(W / 2 - 90, 72, 180, 4, 2, '#ffd23f');
    nut(W / 2 - 94, 75, 6, t * 0.6, '#ffd23f'); nut(W / 2 + 94, 75, 6, -t * 0.6, '#ffd23f');
  }

  function pressStart(t) {
    if (Math.floor(t * 1.6) % 2 === 1 && !reduce) return;
    g.font = '11px "Titan One", "Arial Black", Impact, sans-serif'; g.textAlign = 'center';
    g.lineJoin = 'round'; g.strokeStyle = '#1b1426'; g.lineWidth = 4; g.strokeText('PRESS START', W / 2, 230);
    g.fillStyle = '#fff6d8'; g.fillText('PRESS START', W / 2, 230);
  }

  function crate(x, y, w, h) {
    rr(x, y, w, h, 2, '#8a5a32'); g.strokeStyle = '#5e3a1e'; g.lineWidth = 2;
    g.strokeRect(x + 2, y + 2, w - 4, h - 4);
    g.beginPath(); g.moveTo(x + 3, y + 3); g.lineTo(x + w - 3, y + h - 3); g.stroke();
    circ(x + 4, y + 4, 1, '#d9b48a'); circ(x + w - 4, y + 4, 1, '#d9b48a'); circ(x + 4, y + h - 4, 1, '#d9b48a'); circ(x + w - 4, y + h - 4, 1, '#d9b48a');
  }

  function babyBot(x, y, s, hex, t, phase) {
    // a tiny robot head peeking up and down out of the tyre
    const peek = reduce ? 1 : 0.55 + 0.45 * Math.max(0, Math.sin(t * 1.2 + phase));
    g.save(); g.beginPath(); g.rect(x - 30, 0, 60, y); g.clip();
    g.translate(x, y - 5 + (1 - peek) * 12); g.scale(s, s);
    const col = shade(hex, 1), dark = shade(hex, 0.22);
    g.fillStyle = col; g.fillRect(-4, -150, 8, 30); circ(0, -156, 11, col);
    rr(-108, -120, 216, 176, 40, col);
    const b = blinkAt(t, phase);
    eye(-46, -50, 60, 70, dark, col, b, 0, 0);
    eye(46, -50, 60, 70, dark, col, b, 0, 0);
    g.restore();
  }

  function bolts(t) {
    for (let i = 0; i < 6; i++) {
      const x = 30 + i * 52 + Math.sin(i * 7) * 10, y = 108 + Math.sin(t * 2 + i) * 5 + (i % 2) * 14;
      if (i === 2 || i === 3) continue;
      nut(x, y, 4.2, t * 1.5 + i, '#ffd23f');
      if (((t * 2 + i) % 3) < 0.25) { g.strokeStyle = '#fff'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 4, y - 7); g.lineTo(x + 4, y - 3); g.moveTo(x + 2, y - 5); g.lineTo(x + 6, y - 5); g.stroke(); }
    }
  }

  function frame(t) {
    g.clearRect(0, 0, W, H);
    sky(t); scenery(t);
    bolts(t);
    const [rc, oc] = PALS[pal];
    // baby bots in a tyre, centre
    babyBot(150, 196, 0.075, 0xFF8FC7, t, 0.5);
    babyBot(171, 196, 0.075, 0x49F08A, t, 2.2);
    tyre(160, 210, 16);
    crate(66, 186, 40, 22);
    robot(86, 188, 0.205, rc, t, { wave: true });
    ostrich(236, 207, 0.205, oc, t + 0.4);
    logo(t);
    pressStart(t);
  }

  return { frame, nextPal: () => { pal = (pal + 1) % PALS.length; } };
}
