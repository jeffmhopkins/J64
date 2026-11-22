// Clankers shared drawing kit. Primitives mirror JBrain2 firmware/main/face.c.
// Set CK.g to a 2D context before drawing.
const CK = (() => {
  const K = { g: null };
  const P = Math.PI;
  K.shade = (hex, f) => {
    const r = Math.min(255, ((hex >> 16) & 255) * f) | 0;
    const g = Math.min(255, ((hex >> 8) & 255) * f) | 0;
    const b = Math.min(255, (hex & 255) * f) | 0;
    return `rgb(${r},${g},${b})`;
  };
  const shade = K.shade;
  K.rr = (x, y, w, h, r, c) => {
    const g = K.g; r = Math.max(0, Math.min(r, w / 2, h / 2));
    g.fillStyle = c; g.beginPath();
    g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r);
    g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); g.fill();
  };
  K.circ = (x, y, r, c) => { const g = K.g; g.fillStyle = c; g.beginPath(); g.arc(x, y, Math.max(0, r), 0, P * 2); g.fill(); };
  K.limb = (x, y, deg, ln, w, c) => {
    const g = K.g, a = deg * P / 180, ex = -Math.sin(a), ey = Math.cos(a);
    g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round';
    g.beginPath(); g.moveTo(x, y); g.lineTo(x + ex * ln, y + ey * ln); g.stroke();
    K.circ(x + ex * ln, y + ey * ln, w * 0.62, c);
    return [x + ex * ln, y + ey * ln];
  };
  K.arc = (cx, cy, r, a0, a1, w, c) => {
    const g = K.g; g.strokeStyle = c; g.lineWidth = w; g.lineCap = 'round';
    g.beginPath(); g.arc(cx, cy, r, a0, a1); g.stroke();
  };
  const { rr, circ, limb, arc } = K;

  // The eye. angry: +1 = inner corner on the right (a left eye), -1 = on the left.
  K.eye = (cx, cy, w, h, dark, face, o = {}) => {
    const g = K.g, open = o.open ?? 1, happy = o.happy ?? 0, look = o.look ?? 0, lookY = o.lookY ?? 0;
    const hh = h * open;
    if (hh < 6) { rr(cx - w / 2, cy - 3, w, 6, 3, dark); return; }
    rr(cx - w / 2, cy - hh / 2, w, hh, Math.min(w, hh) * 0.38, '#f6f9fc');
    g.save(); g.beginPath(); g.rect(cx - w / 2, cy - hh / 2, w, hh); g.clip();
    const pr = w * (o.pupil ?? 0.33);
    circ(cx + look * w * 0.14, cy + lookY * hh * 0.14, pr, dark);
    circ(cx + look * w * 0.14 - pr * 0.33, cy + lookY * hh * 0.14 - pr * 0.4, pr * 0.3, '#fff');
    if (happy > 0) {
      g.fillStyle = face; g.beginPath();
      g.ellipse(cx, cy + hh / 2 + hh * (0.55 - happy * 0.5), w * 0.8, hh * 0.5, 0, 0, P * 2); g.fill();
    }
    if (o.angry) {
      const a = o.angry, L = cx - w / 2 - 2, R = cx + w / 2 + 2, T = cy - hh / 2 - 2;
      g.fillStyle = face; g.beginPath(); g.moveTo(L, T); g.lineTo(R, T);
      if (a > 0) { g.lineTo(R, cy - hh / 2 + hh * 0.5); g.lineTo(L, cy - hh / 2 + hh * 0.08); }
      else { g.lineTo(R, cy - hh / 2 + hh * 0.08); g.lineTo(L, cy - hh / 2 + hh * 0.5); }
      g.closePath(); g.fill();
    }
    g.restore();
  };
  const eye = K.eye;

  K.mouth = (kind, cx, cy, r, w, dark) => {
    const g = K.g;
    if (kind === 'o') { circ(cx, cy + r * 0.6, r * 0.42, dark); return; }
    if (kind === 'frown') { arc(cx, cy + r * 1.35, r * 0.8, P * 1.2, P * 1.8, w, dark); return; }
    if (kind === 'zig' || kind === 'wavy') {
      g.strokeStyle = dark; g.lineWidth = w; g.lineCap = 'round'; g.lineJoin = 'round'; g.beginPath();
      const n = 5;
      for (let i = 0; i <= n; i++) {
        const x = cx - r + (2 * r) * i / n, y = cy + r * 0.75 + (i % 2 ? -1 : 1) * r * (kind === 'zig' ? 0.22 : 0.12);
        i ? g.lineTo(x, y) : g.moveTo(x, y);
      }
      g.stroke(); return;
    }
    arc(cx, cy, r, P * 0.15, P * 0.85, w, dark);
  };

  K.blink = (t, phase) => ((t + phase) % 3.6) < 0.12 ? 0.08 : 1;

  K.stars = (cx, cy, rad, t, c = '#ffe66b') => {
    for (let i = 0; i < 3; i++) {
      const a = t * 4 + i * P * 2 / 3;
      K.star(cx + Math.cos(a) * rad, cy + Math.sin(a) * rad * 0.35, rad * 0.22, c);
    }
  };
  K.star = (x, y, r, c) => {
    const g = K.g; g.fillStyle = c; g.beginPath();
    for (let i = 0; i < 10; i++) { const a = -P / 2 + i * P / 5, rad = i % 2 ? r * 0.45 : r; g.lineTo(x + Math.cos(a) * rad, y + Math.sin(a) * rad); }
    g.closePath(); g.fill();
  };
  K.flame = (x, y, len, w, t) => {
    const g = K.g, f = 0.75 + 0.25 * Math.sin(t * 40);
    g.fillStyle = '#ff8a3d'; g.beginPath(); g.moveTo(x - w / 2, y); g.quadraticCurveTo(x, y + len * 1.2 * f, x + w / 2, y); g.fill();
    g.fillStyle = '#ffe66b'; g.beginPath(); g.moveTo(x - w / 4, y); g.quadraticCurveTo(x, y + len * 0.7 * f, x + w / 4, y); g.fill();
  };
  K.nut = (x, y, r, rot, c = '#ffd23f') => {
    const g = K.g; g.save(); g.translate(x, y); g.rotate(rot); g.fillStyle = c; g.beginPath();
    for (let i = 0; i < 6; i++) { const a = i / 6 * P * 2; g.lineTo(Math.cos(a) * r, Math.sin(a) * r); }
    g.closePath(); g.fill(); circ(0, 0, r * 0.42, 'rgba(60,40,10,0.7)'); g.restore();
  };
  K.gear = (x, y, r, teeth, rot, c, hole) => {
    const g = K.g; g.save(); g.translate(x, y); g.rotate(rot); g.fillStyle = c; g.beginPath();
    for (let i = 0; i < teeth * 2; i++) {
      const a = i / (teeth * 2) * P * 2, a2 = (i + 1) / (teeth * 2) * P * 2, rad = i % 2 ? r * 0.78 : r;
      g.lineTo(Math.cos(a) * rad, Math.sin(a) * rad); g.lineTo(Math.cos(a2) * rad, Math.sin(a2) * rad);
    }
    g.closePath(); g.fill(); if (hole) circ(0, 0, r * 0.32, hole); g.restore();
  };

  // ROBOT. Panel units, feet at local y = 171. p = pose.
  K.robot = (x, feetY, s, hex, p = {}) => {
    const g = K.g, t = p.t ?? 0;
    const col = shade(hex, 1), dark = shade(hex, 0.22), torso = shade(hex, 0.88), plate = shade(hex, 0.55), lim = shade(hex, 0.78);
    g.save(); g.translate(x, feetY); g.rotate(p.rot ?? 0); g.translate(0, -171 * s); g.scale(s * (p.sx ?? 1), s * (p.sy ?? 1));
    const HY = -96, HW = 108, HH = 88, bob = p.bob ?? 0, tilt = p.tilt ?? 0;
    const legLen = p.legLen ?? 54;
    const lf = limb(-34, 98 + bob, p.legL ?? 6, legLen - bob, 30, lim);
    const rf = limb(34, 98 + bob, p.legR ?? -6, legLen - bob, 30, lim);
    wearFoot(p, lf[0], lf[1] + 2, 34, false, false); wearFoot(p, rf[0], rf[1] + 2, 34, false, false);
    if (p.jet) { K.flame(lf[0], lf[1] + 16, 60, 26, t); K.flame(rf[0], rf[1] + 16, 60, 26, t + 0.3); }
    g.translate(0, bob);
    const armL = p.armL ?? 22, armR = p.armR ?? -22;
    const arm = (ax, deg) => { limb(ax, 6, deg, 74, 28, lim); wearArm(p, ax, 6, deg, 74, 28, false); };
    if (Math.abs(armL) <= 95) arm(-66, armL);
    let clawEnd = null;
    if (Math.abs(armR) <= 95 && !p.grab) arm(66, armR);
    rr(-72, -26, 144, 132, 40, torso);
    rr(-26, 10, 52, 40, 12, plate);
    wearBody(p, { x: -72, y: -26, w: 144, h: 132, r: 40 });
    g.fillStyle = col; g.fillRect(tilt - 4, HY - HH - 30, 8, 30); circ(tilt, HY - HH - 36, 11, col);
    rr(tilt - HW, HY - HH, HW * 2, HH * 2, 40, col);
    const ex = HW * 0.43, ey = HY - HH * 0.17, eo = Object.assign({ open: K.blink(t, p.phase ?? 0.3), happy: 0.45 }, p.eyes || {});
    eye(tilt - ex, ey, 52, 60, dark, col, Object.assign({}, eo, { angry: eo.angry ? 1 : 0 }));
    eye(tilt + ex, ey, 52, 60, dark, col, Object.assign({}, eo, { angry: eo.angry ? -1 : 0 }));
    K.mouth(p.mouth || 'smile', tilt, HY + HH * 0.52 - 16, 30, 9, dark);
    if (Math.abs(armL) > 95) arm(-66, armL);
    if (Math.abs(armR) > 95 && !p.grab) arm(66, armR);
    if (p.grab) {
      // grabber arm: a telescoping arm with a two-finger claw
      const len = 74 + (p.reach ?? 60);
      const a = (p.grabDeg ?? -90) * P / 180, ux = -Math.sin(a), uy = Math.cos(a);
      g.strokeStyle = lim; g.lineCap = 'round'; g.lineWidth = 28; g.beginPath(); g.moveTo(66, 6); g.lineTo(66 + ux * 74, 6 + uy * 74); g.stroke();
      g.lineWidth = 16; g.strokeStyle = plate; g.beginPath(); g.moveTo(66 + ux * 60, 6 + uy * 60); g.lineTo(66 + ux * len, 6 + uy * len); g.stroke();
      const cx = 66 + ux * len, cy = 6 + uy * len, open = p.clawOpen ?? 0.5;
      const base = p.grabDeg ?? -90;
      limb(cx, cy, base + 40 + open * 30, 30, 12, lim); limb(cx, cy, base - 40 - open * 30, 30, 12, lim);
      circ(cx, cy, 14, lim);
      clawEnd = [cx, cy];
    }
    wear(p, { cx: tilt, top: HY - HH, w: HW * 2 * 0.95, eyes: [[tilt - ex, ey, 26, 30], [tilt + ex, ey, 26, 30]], neck: [tilt, HY + HH + 4, 150] });
    if (p.stars) K.stars(tilt, HY - HH - 30, 90, t);
    g.restore();
    return clawEnd;
  };

  // ROBOT OSTRICH. ostrich-mock.py at true geometry, ox = 0, feet at local y = 429.
  K.ostrich = (x, feetY, s, hex, p = {}) => {
    const g = K.g, t = p.t ?? 0;
    const col = shade(hex, 1), head = shade(hex, 0.94), body = shade(hex, 0.86), neck = shade(hex, 0.76),
      leg = shade(hex, 0.70), beak = shade(hex, 0.52), wing = shade(hex, 0.64), dark = shade(hex, 0.22);
    g.save(); g.translate(x, feetY); g.rotate(p.rot ?? 0); g.translate(0, -429 * s); g.scale(s, s);
    const ox = 0, bob = p.bob ?? 0;
    const flick = p.flick ?? 0;
    for (const [deg, ln] of [[108, 76], [126, 88], [144, 72]]) limb(ox - 56, 250 + bob, deg + flick, ln, 16, col);
    const legs = [[-1, p.legL ?? -3, p.legLenL ?? 92], [1, p.legR ?? 3, p.legLenR ?? 92]];
    for (const [side, deg, ln] of legs) {
      const hx = ox + side * 30;
      const [ex, ey] = limb(hx, 318 + bob, deg, ln - bob, 20, leg);
      for (const td of [-62, 0, 62]) limb(ex, ey, td + deg, 19, 11, leg);
      wearFoot(p, ex, ey + 4, 30, false, false);
    }
    g.translate(0, bob);
    rr(ox - 72, 222, 144, 112, 54, body);
    wearBody(p, { x: ox - 72, y: 222, w: 144, h: 112, r: 54 });
    if (p.flap) {
      g.save(); g.translate(ox + 10, 250); g.rotate(-p.flap * 1.1);
      rr(-8, -6, 74, 66, 30, wing);
      for (const r of [20, 34, 48]) arc(-6, 2, r, P * 0.06, P * 0.44, 3, col);
      g.restore();
    } else {
      rr(ox + 2, 244, 74, 66, 30, wing);
      for (const r of [20, 34, 48]) arc(ox + 4, 252, r, P * 0.06, P * 0.44, 3, col);
    }
    const lean = p.lean ?? 0;
    for (let i = 0; i < 7; i++) {
      const tt = i / 6, sw = 50 - 16 * tt, y = 218 - i * 14;
      rr(ox - sw / 2 + 7 * tt + lean * tt, y, sw, 11, 5, neck);
    }
    const hx = ox + 7 + lean;
    for (const [deg, ln] of [[-18, 42], [0, 50], [18, 42]]) limb(hx, 62, deg + 180 + (p.crest ?? 0), ln, 9, col);
    rr(hx - 64, 58, 128, 98, 44, head);
    for (const [w, h, y] of [[42, 13, 132], [33, 12, 143], [23, 11, 153], [13, 10, 162]]) rr(hx - w / 2, y, w, h, 5, beak);
    if (p.beakOpen) rr(hx - 14, 150, 28, 10 * p.beakOpen, 4, dark);
    g.fillStyle = shade(hex, 0.28); g.fillRect(hx - 16, 147, 32, 2);
    const eo = Object.assign({ open: K.blink(t, p.phase ?? 1.7), happy: 0.4 }, p.eyes || {});
    eye(hx - 31, 100, 45, 53, dark, head, Object.assign({}, eo, { angry: eo.angry ? 1 : 0 }));
    eye(hx + 31, 100, 45, 53, dark, head, Object.assign({}, eo, { angry: eo.angry ? -1 : 0 }));
    wear(p, { cx: hx, top: 58, w: 128, eyes: [[hx - 31, 100, 22, 26], [hx + 31, 100, 22, 26]], neck: [hx - 2, 160, 90] });
    if (p.stars) K.stars(hx, 30, 80, t);
    g.restore();
  };

  // BABY BOT: the robot's head on a small body. feet at local y = 120.
  K.baby = (x, feetY, s, hex, p = {}) => {
    const g = K.g, t = p.t ?? 0;
    const col = shade(hex, 1), dark = shade(hex, 0.22), torso = shade(hex, 0.88), lim = shade(hex, 0.78);
    g.save(); g.translate(x, feetY - 120 * s); g.scale(s, s);
    const bob = p.bob ?? 0;
    limb(-22, 60, 4, 34 - bob, 24, lim); limb(22, 60, -4, 34 - bob, 24, lim);
    g.translate(0, bob);
    limb(-46, 10, p.armL ?? 30, 40, 20, lim); limb(46, 10, p.armR ?? -30, 40, 20, lim);
    rr(-48, -10, 96, 80, 30, torso);
    g.fillStyle = col; g.fillRect(-4, -190, 8, 30); circ(0, -196, 12, col);
    rr(-100, -164, 200, 160, 48, col);
    const eo = Object.assign({ open: K.blink(t, p.phase ?? 0.9), happy: 0.3, pupil: 0.36 }, p.eyes || {});
    eye(-42, -92, 58, 68, dark, col, eo); eye(42, -92, 58, 68, dark, col, eo);
    K.mouth(p.mouth || 'smile', 0, -50, 22, 8, dark);
    g.restore();
  };

  // ---------- SIDE VIEWS (facing right; dir = -1 mirrors to face left) ----------
  // A three-quarter profile: the near eye big, the far eye squeezed at the head's front edge,
  // so the face still reads at 30 px tall. Same one-hue shading; far limbs a step darker.
  function clipRR(x, y, w, h, r) {
    const g = K.g; r = Math.min(r, w / 2, h / 2); g.beginPath();
    g.moveTo(x + r, y); g.arcTo(x + w, y, x + w, y + h, r); g.arcTo(x + w, y + h, x, y + h, r);
    g.arcTo(x, y + h, x, y, r); g.arcTo(x, y, x + w, y, r); g.closePath(); g.clip();
  }
  // a leg with an optional knee: the hip half at deg, the foot half at deg + knee
  const legK = (x, y, deg, ln, w, c, knee) => {
    if (!knee) return [limb(x, y, deg, ln, w, c), deg];
    const [kx, ky] = limb(x, y, deg, ln * 0.55, w, c);
    return [limb(kx, ky, deg + knee, ln * 0.55, w, c), deg + knee];
  };
  K.robotSide = (x, feetY, s, hex, p = {}) => {
    const g = K.g, t = p.t ?? 0, dir = p.dir ?? 1;
    const col = shade(hex, 1), dark = shade(hex, 0.22), torso = shade(hex, 0.88), plate = shade(hex, 0.55),
      lim = shade(hex, 0.78), far = shade(hex, 0.6);
    g.save(); g.translate(x, feetY); g.rotate((p.rot ?? 0) * dir); g.scale(p.sx ?? 1, p.sy ?? 1); g.translate(0, -171 * s); g.scale(s * dir, s);
    const HY = -96, HH = 88, bob = p.bob ?? 0, legLen = p.legLen ?? 54;
    const [bf] = legK(-16, 98 + bob, p.legB ?? 0, legLen - bob, 30, far, p.kneeB);
    wearFoot(p, bf[0], bf[1] + 2, 34, true, true);
    if (p.jet) K.flame(bf[0], bf[1] + 16, 60, 26, t + 0.3);
    g.translate(0, bob);
    limb(-8, 6, p.armB ?? 10, 74, 28, far); wearArm(p, -8, 6, p.armB ?? 10, 74, 28, true);
    rr(-54, -26, 108, 132, 36, torso);
    rr(30, 10, 20, 40, 8, plate);
    wearBody(p, { x: -54, y: -26, w: 108, h: 132, r: 36, side: true });
    // head
    g.fillStyle = col; g.fillRect(-14, HY - HH - 30, 8, 30); circ(-10, HY - HH - 36, 11, col);
    rr(-84, HY - HH, 168, HH * 2, 40, col);
    circ(-40, HY + 4, 16, plate); circ(-40, HY + 4, 7, torso);   // the side bolt
    g.save(); clipRR(-84, HY - HH, 168, HH * 2, 40);
    const eo = Object.assign({ open: K.blink(t, p.phase ?? 0.3), happy: 0.45, look: 0.9 }, p.eyes || {});
    const ey = HY - HH * 0.17;
    eye(74, ey, 30, 58, dark, col, Object.assign({}, eo, { angry: 0, pupil: 0.3 }));
    eye(22, ey, 52, 60, dark, col, Object.assign({}, eo, { angry: 0 }));
    K.mouth(p.mouth || 'smile', 50, HY + HH * 0.52 - 16, 24, 9, dark);
    g.restore();
    let end = null;
    if (p.grab) {
      const len = 74 + (p.reach ?? 60);
      g.strokeStyle = lim; g.lineCap = 'round'; g.lineWidth = 28; g.beginPath(); g.moveTo(6, 6); g.lineTo(6 + 74, 6); g.stroke();
      g.lineWidth = 16; g.strokeStyle = plate; g.beginPath(); g.moveTo(66, 6); g.lineTo(6 + len, 6); g.stroke();
      const cx = 6 + len, open = p.clawOpen ?? 0.5;
      limb(cx, 6, -90 + 40 + open * 30, 30, 12, lim); limb(cx, 6, -90 - 40 - open * 30, 30, 12, lim);
      circ(cx, 6, 14, lim); end = [cx, 6];
    }
    wear(p, { side: true, cx: 0, top: HY - HH, w: 168 * 0.95, eyes: [[22, ey, 26, 30], [74, ey, 15, 29]], neck: [20, HY + HH + 4, 130] });
    g.translate(0, -bob);
    const [ff] = legK(16, 98 + bob, p.legF ?? 0, legLen - bob, 30, lim, p.kneeF);
    wearFoot(p, ff[0], ff[1] + 2, 34, true, false);
    if (p.jet) K.flame(ff[0], ff[1] + 16, 60, 26, t);
    g.translate(0, bob);
    if (!p.grab) { limb(8, 6, p.armF ?? -10, 74, 28, lim); wearArm(p, 8, 6, p.armF ?? -10, 74, 28, false); }
    if (p.stars) K.stars(0, HY - HH - 30, 90, t);
    g.restore();
    return end;
  };

  K.ostrichSide = (x, feetY, s, hex, p = {}) => {
    const g = K.g, t = p.t ?? 0, dir = p.dir ?? 1;
    const col = shade(hex, 1), head = shade(hex, 0.94), body = shade(hex, 0.86), neck = shade(hex, 0.76),
      leg = shade(hex, 0.70), beak = shade(hex, 0.52), wing = shade(hex, 0.64), dark = shade(hex, 0.22), far = shade(hex, 0.55);
    g.save(); g.translate(x, feetY); g.rotate((p.rot ?? 0) * dir); g.scale(p.sx ?? 1, p.sy ?? 1); g.translate(0, -429 * s); g.scale(s * dir, s);
    const bob = p.bob ?? 0, flick = p.flick ?? 0;
    // tail plumes out the back
    for (const [deg, ln] of [[96, 70], [114, 84], [132, 68]]) limb(-70, 252 + bob, deg + flick, ln, 16, col);
    const legDraw = (hx, deg, ln, c, knee, toe, isFar) => {
      const [[ex, ey], fd] = legK(hx, 318 + bob, deg, ln - bob, 20, c, knee);
      const td0 = toe ?? fd;
      for (const [td, tl] of [[-100, 24], [-70, 20], [55, 14]]) limb(ex, ey, td + td0, tl, 11, c);
      wearFoot(p, ex + 6, ey + 4, 30, true, isFar);
    };
    legDraw(-6, p.legB ?? 0, p.legLenB ?? 92, far, p.kneeB, p.toeB, true);
    g.translate(0, bob);
    // FLAP: wings become fans of plumes (the tail's own shape), far one first and darker
    // A WING: a rounded panel from the shoulder, pointing back, with flight plumes hanging off
    // its trailing edge. up = radians the wing is raised (0 = straight back).
    const fan = (wx, wy, up, c, plume, k) => {
      g.save(); g.translate(wx, wy); g.rotate(up);
      for (let i = 0; i < 4; i++) limb((-36 - i * 28) * k, 12 * k, 28 + i * 6, (30 + i * 8) * k, 15 * k, plume);
      rr(-134 * k, -24 * k, 150 * k, 48 * k, 24 * k, c);
      for (const r of [26, 44]) arc(6 * k, 0, r * k, P * 0.9, P * 1.1, 3, plume);
      g.restore();
    };
    if (p.wings != null) fan(4, 236, p.wings + 0.18, far, far, 1.15);
    rr(-82, 222, 164, 108, 52, body);
    wearBody(p, { x: -82, y: 222, w: 164, h: 108, r: 52, side: true });
    // neck: an S from the front of the body up to the head
    const lean = p.lean ?? 0;
    for (let i = 0; i < 8; i++) {
      const tt = i / 7, sw = 46 - 14 * tt;
      const nx = 44 + Math.sin(tt * 2.4) * 18 + lean * tt, ny = 228 - tt * 150;
      rr(nx - sw / 2, ny, sw, 12, 5, neck);
    }
    const hx = 64 + lean, hy = 50;
    for (const [deg, ln] of [[128, 40], [146, 48], [164, 40]]) limb(hx - 10, hy + 8, deg + (p.crest ?? 0), ln, 9, col);
    rr(hx - 52, hy, 112, 94, 42, head);
    // beak pointing forward, darkest shade, stepped like the front view's
    for (const [bx, by, w, h] of [[hx + 46, hy + 48, 30, 26], [hx + 70, hy + 52, 22, 19], [hx + 86, hy + 56, 14, 12]]) rr(bx, by, w, h, 7, beak);
    g.fillStyle = shade(hex, 0.28); g.fillRect(hx + 48, hy + 62, 46, 2);
    if (p.beakOpen) rr(hx + 50, hy + 64, 36, 8 * p.beakOpen, 4, dark);
    g.save(); clipRR(hx - 52, hy, 112, 94, 42);
    const eo = Object.assign({ open: K.blink(t, p.phase ?? 1.7), happy: 0.4, look: 0.9 }, p.eyes || {});
    eye(hx + 46, hy + 40, 26, 50, dark, head, Object.assign({}, eo, { angry: 0, pupil: 0.3 }));
    eye(hx + 4, hy + 40, 44, 52, dark, head, Object.assign({}, eo, { angry: 0 }));
    g.restore();
    // wing on the near side
    if (p.wings != null) {
      fan(0, 242, p.wings, wing, col, 1.25);
    } else if (p.flap) {
      g.save(); g.translate(-40, 248); g.rotate(-p.flap * 1.2);
      rr(-10, -8, 96, 60, 28, wing); for (const r of [20, 34, 48]) arc(-6, 0, r, P * 0.06, P * 0.44, 3, col);
      g.restore();
    } else {
      rr(-50, 244, 96, 60, 28, wing); for (const r of [20, 34, 48]) arc(-46, 252, r, P * 0.06, P * 0.44, 3, col);
    }
    wear(p, { side: true, cx: hx + 4, top: hy, w: 112, eyes: [[hx + 4, hy + 40, 22, 26], [hx + 46, hy + 40, 13, 25]], neck: [hx - 4, hy + 100, 80] });
    g.translate(0, -bob);
    legDraw(8, p.legF ?? 0, p.legLenF ?? 92, leg, p.kneeF, p.toeF);
    if (p.stars) { g.translate(0, bob); K.stars(hx, 30, 80, t); }
    g.restore();
  };


  // ---------- ACCESSORIES (bought with bolts at the parts shop) ----------
  // acc = { head, face, neck, body, feet }: one item per slot, all worn at once. Each hero view tells
  // these where its head, eyes, neck, body and feet are, in its own units, and they draw on top.
  // Items marked rainbow cycle through colours (the N64 bakes them white and tints them).
  K.ACC = [
    { id: 'party', slot: 'head', name: 'Party hat', cost: 15 },
    { id: 'flower', slot: 'head', name: 'Flower', cost: 15 },
    { id: 'beanie', slot: 'head', name: 'Beanie', cost: 20 },
    { id: 'baseball', slot: 'head', name: 'Ball cap', cost: 20 },
    { id: 'chef', slot: 'head', name: 'Chef hat', cost: 25 },
    { id: 'propeller', slot: 'head', name: 'Propeller cap', cost: 30 },
    { id: 'cowboy', slot: 'head', name: 'Cowboy hat', cost: 30 },
    { id: 'pirate', slot: 'head', name: 'Pirate hat', cost: 35 },
    { id: 'tophat', slot: 'head', name: 'Top hat', cost: 40 },
    { id: 'wizard', slot: 'head', name: 'Wizard hat', cost: 45 },
    { id: 'crown', slot: 'head', name: 'Crown', cost: 60 },
    { id: 'rainbowcap', slot: 'head', name: 'Rainbow cap', cost: 80, rainbow: true },
    { id: 'glasses', slot: 'face', name: 'Eyeglasses', cost: 15 },
    { id: 'shades', slot: 'face', name: 'Cool shades', cost: 25 },
    { id: 'hearts', slot: 'face', name: 'Heart glasses', cost: 25 },
    { id: 'stars', slot: 'face', name: 'Star glasses', cost: 25 },
    { id: 'goggles', slot: 'face', name: 'Goggles', cost: 30 },
    { id: 'mustache', slot: 'face', name: 'Mustache', cost: 20 },
    { id: 'monocle', slot: 'face', name: 'Monocle', cost: 35 },
    { id: 'bowtie', slot: 'neck', name: 'Bow tie', cost: 15 },
    { id: 'scarf', slot: 'neck', name: 'Scarf', cost: 20 },
    { id: 'necktie', slot: 'neck', name: 'Necktie', cost: 20 },
    { id: 'bandana', slot: 'neck', name: 'Bandana', cost: 20 },
    { id: 'medal', slot: 'neck', name: 'Gold medal', cost: 50 },
    { id: 'sweater', slot: 'body', name: 'Stripy sweater', cost: 30 },
    { id: 'overalls', slot: 'body', name: 'Overalls', cost: 30 },
    { id: 'jersey', slot: 'body', name: 'Team jersey', cost: 35 },
    { id: 'tux', slot: 'body', name: 'Fancy suit', cost: 50 },
    { id: 'super', slot: 'body', name: 'Super suit', cost: 60 },
    { id: 'disco', slot: 'body', name: 'Disco suit', cost: 90, rainbow: true },
    { id: 'sneakers', slot: 'feet', name: 'Sneakers', cost: 20 },
    { id: 'boots', slot: 'feet', name: 'Boots', cost: 25 },
    { id: 'rainboots', slot: 'feet', name: 'Rain boots', cost: 25 },
    { id: 'flippers', slot: 'feet', name: 'Flippers', cost: 30 },
    { id: 'skates', slot: 'feet', name: 'Roller skates', cost: 40 },
    { id: 'lightup', slot: 'feet', name: 'Light-up shoes', cost: 70, rainbow: true },
    // added later (appended, so older saves keep their bits): costume pieces and the fashion line
    { id: 'mirror', slot: 'head', name: 'Head mirror', cost: 25 },
    { id: 'policecap', slot: 'head', name: 'Police cap', cost: 30 },
    { id: 'mohawk', slot: 'head', name: 'Rock star hair', cost: 40 },
    { id: 'firehat', slot: 'head', name: 'Fire helmet', cost: 35 },
    { id: 'spacehelm', slot: 'head', name: 'Space helmet', cost: 50 },
    { id: 'hardhat', slot: 'head', name: 'Hard hat', cost: 25 },
    { id: 'tiara', slot: 'head', name: 'Tiara', cost: 45 },
    { id: 'ninjaband', slot: 'head', name: 'Ninja headband', cost: 25 },
    { id: 'clownwig', slot: 'head', name: 'Clown wig', cost: 30 },
    { id: 'knighthelm', slot: 'head', name: 'Knight helmet', cost: 45 },
    { id: 'eyepatch', slot: 'face', name: 'Eye patch', cost: 15 },
    { id: 'rednose', slot: 'face', name: 'Clown nose', cost: 15 },
    { id: 'mask', slot: 'face', name: 'Doctor mask', cost: 15 },
    { id: 'stetho', slot: 'neck', name: 'Stethoscope', cost: 25 },
    { id: 'pearls', slot: 'neck', name: 'Pearls', cost: 40 },
    { id: 'chain', slot: 'neck', name: 'Star chain', cost: 35 },
    { id: 'chefcoat', slot: 'body', name: 'Chef coat', cost: 35 },
    { id: 'labcoat', slot: 'body', name: 'Lab coat', cost: 40 },
    { id: 'police', slot: 'body', name: 'Police uniform', cost: 45 },
    { id: 'leather', slot: 'body', name: 'Rock jacket', cost: 50 },
    { id: 'firecoat', slot: 'body', name: 'Fire coat', cost: 45 },
    { id: 'spacesuit', slot: 'body', name: 'Space suit', cost: 60 },
    { id: 'vest', slot: 'body', name: 'Safety vest', cost: 30 },
    { id: 'pirateshirt', slot: 'body', name: 'Sailor shirt', cost: 35 },
    { id: 'ninjasuit', slot: 'body', name: 'Ninja suit', cost: 45 },
    { id: 'clownsuit', slot: 'body', name: 'Clown suit', cost: 40 },
    { id: 'armor', slot: 'body', name: 'Armor', cost: 60 },
    { id: 'robe', slot: 'body', name: 'Wizard robe', cost: 50 },
    { id: 'cowvest', slot: 'body', name: 'Ranch vest', cost: 35 },
    { id: 'sundress', slot: 'body', name: 'Sundress', cost: 40 },
    { id: 'partydress', slot: 'body', name: 'Party dress', cost: 45 },
    { id: 'polkadress', slot: 'body', name: 'Polka dress', cost: 45 },
    { id: 'tutu', slot: 'body', name: 'Tutu', cost: 40 },
    { id: 'ballgown', slot: 'body', name: 'Ball gown', cost: 70 },
    { id: 'sparkledress', slot: 'body', name: 'Sparkle dress', cost: 90, rainbow: true },
    { id: 'bluesuit', slot: 'body', name: 'Blue suit', cost: 50 },
    { id: 'pinstripe', slot: 'body', name: 'Pinstripe suit', cost: 55 },
    { id: 'whitesuit', slot: 'body', name: 'White suit', cost: 55 },
    { id: 'plaidsuit', slot: 'body', name: 'Plaid suit', cost: 55 },
    { id: 'velvetsuit', slot: 'body', name: 'Velvet suit', cost: 65 },
    { id: 'goldsuit', slot: 'body', name: 'Gold suit', cost: 80 },
    { id: 'dressshoes', slot: 'feet', name: 'Dress shoes', cost: 30 },
    { id: 'rockboots', slot: 'feet', name: 'Rocker boots', cost: 40 },
    { id: 'moonboots', slot: 'feet', name: 'Moon boots', cost: 40 },
    { id: 'clownshoes', slot: 'feet', name: 'Clown shoes', cost: 30 },
    { id: 'slippers', slot: 'feet', name: 'Glass slippers', cost: 50 },
    { id: 'heels', slot: 'feet', name: 'High heels', cost: 35 },
    { id: 'flats', slot: 'feet', name: 'Ballet flats', cost: 25 },
    { id: 'sandals', slot: 'feet', name: 'Sandals', cost: 20 },
    { id: 'hightops', slot: 'feet', name: 'High-tops', cost: 35 },
    { id: 'cowboots', slot: 'feet', name: 'Cowboy boots', cost: 40 },
  ];
  // whole looks: one buy gets every piece, one press puts them all on (the same list as ck_costumes)
  K.COSTUMES = [
    { id: 'c_chef', name: 'Chef', pieces: ['chef', 'chefcoat', 'mustache'] },
    { id: 'c_doctor', name: 'Doctor', pieces: ['mirror', 'mask', 'stetho', 'labcoat'] },
    { id: 'c_police', name: 'Police', pieces: ['policecap', 'shades', 'police', 'dressshoes'] },
    { id: 'c_rock', name: 'Rock star', pieces: ['mohawk', 'stars', 'chain', 'leather', 'rockboots'] },
    { id: 'c_fire', name: 'Firefighter', pieces: ['firehat', 'firecoat', 'rainboots'] },
    { id: 'c_space', name: 'Astronaut', pieces: ['spacehelm', 'spacesuit', 'moonboots'] },
    { id: 'c_builder', name: 'Builder', pieces: ['hardhat', 'vest', 'boots'] },
    { id: 'c_pirate', name: 'Pirate', pieces: ['pirate', 'eyepatch', 'pirateshirt', 'boots'] },
    { id: 'c_princess', name: 'Princess', pieces: ['tiara', 'pearls', 'ballgown', 'slippers'] },
    { id: 'c_ballerina', name: 'Ballerina', pieces: ['flower', 'tutu', 'flats'] },
    { id: 'c_ninja', name: 'Ninja', pieces: ['ninjaband', 'ninjasuit'] },
    { id: 'c_clown', name: 'Clown', pieces: ['clownwig', 'rednose', 'clownsuit', 'clownshoes'] },
    { id: 'c_knight', name: 'Knight', pieces: ['knighthelm', 'armor', 'boots'] },
    { id: 'c_wizard', name: 'Wizard', pieces: ['wizard', 'robe', 'medal'] },
    { id: 'c_cowboy', name: 'Cowboy', pieces: ['cowboy', 'bandana', 'cowvest', 'cowboots'] },
    { id: 'c_hero', name: 'Super hero', pieces: ['goggles', 'super', 'boots'] },
  ];
  // the parts shop's sections, in order (the same as ck_shop_secs); body pieces split three ways
  K.SHOP_SECS = [
    ['COSTUMES', K.COSTUMES.map(c => c.id)],
    ['HATS', ['party', 'flower', 'beanie', 'baseball', 'chef', 'propeller', 'cowboy', 'pirate', 'tophat', 'wizard', 'crown', 'rainbowcap', 'mirror', 'policecap', 'mohawk', 'firehat', 'spacehelm', 'hardhat', 'tiara', 'ninjaband', 'clownwig', 'knighthelm']],
    ['FACES', ['glasses', 'shades', 'hearts', 'stars', 'goggles', 'mustache', 'monocle', 'eyepatch', 'rednose', 'mask']],
    ['NECKWEAR', ['bowtie', 'scarf', 'necktie', 'bandana', 'medal', 'stetho', 'pearls', 'chain']],
    ['OUTFITS', ['sweater', 'overalls', 'jersey', 'super', 'disco', 'chefcoat', 'labcoat', 'police', 'leather', 'firecoat', 'spacesuit', 'vest', 'pirateshirt', 'ninjasuit', 'clownsuit', 'armor', 'robe', 'cowvest']],
    ['DRESSES', ['sundress', 'partydress', 'polkadress', 'tutu', 'ballgown', 'sparkledress']],
    ['SUITS', ['tux', 'bluesuit', 'pinstripe', 'whitesuit', 'plaidsuit', 'velvetsuit', 'goldsuit']],
    ['SHOES', ['sneakers', 'boots', 'rainboots', 'flippers', 'skates', 'lightup', 'dressshoes', 'rockboots', 'moonboots', 'clownshoes', 'slippers', 'heels', 'flats', 'sandals', 'hightops', 'cowboots']],
    ['HEROES & GEAR', null],
  ];
  K.ACC_SLOTS = ['head', 'face', 'neck', 'body', 'feet'];
  // a colour that cycles; the N64 baker sets K.plain so it draws white for the console to tint
  const rb = (t, k = 0, l = 60) => K.plain ? `hsl(0,0%,${Math.min(100, l + 32)}%)` : `hsl(${((t * 140 + k * 70) % 360 + 360) % 360},90%,${l}%)`;
  function heart(x, y, r, c) {
    const g = K.g; g.fillStyle = c; g.beginPath();
    g.moveTo(x, y + r * 0.95);
    g.bezierCurveTo(x - r * 1.5, y - r * 0.1, x - r * 0.8, y - r * 1.2, x, y - r * 0.45);
    g.bezierCurveTo(x + r * 0.8, y - r * 1.2, x + r * 1.5, y - r * 0.1, x, y + r * 0.95);
    g.fill();
  }
  // a line at least ~1.3 screen pixels wide, so thin frames survive tiny sizes (and the N64 bake)
  const pxw = lw => { const m = K.g.getTransform(); return Math.max(lw, 1.3 / (Math.hypot(m.a, m.b) || 1)); };
  const poly = (pts, c) => { const g = K.g; g.fillStyle = c; g.beginPath(); pts.forEach(([x, y], i) => i ? g.lineTo(x, y) : g.moveTo(x, y)); g.closePath(); g.fill(); };
  // HATS: drawn at the head's top middle, 100 units = the head's width; side = facing right
  K.wearHead = (id, cx, top, w, t = 0, side = false) => {
    const g = K.g, k = w / 100, f = side ? 1 : 0;
    g.save(); g.translate(cx, top); g.scale(k, k);
    if (id === 'party') {
      poly([[-24, 6], [0, -62], [24, 6]], '#ff8fc7');
      g.save(); g.beginPath(); g.moveTo(-24, 6); g.lineTo(0, -62); g.lineTo(24, 6); g.closePath(); g.clip(); g.fillStyle = '#ffd23f'; for (const y of [-40, -16]) { g.beginPath(); g.moveTo(-40, y); g.lineTo(40, y - 14); g.lineTo(40, y - 4); g.lineTo(-40, y + 10); g.fill(); } g.restore();
      circ(0, -64, 10, '#5ad1ff'); circ(-3, -67, 3.5, '#ffffff');
    } else if (id === 'flower') {
      const fx = side ? -26 : 30, fy = -2;
      limb(fx, fy, 160, 14, 5, '#3f9e57');
      for (let i = 0; i < 6; i++) { const a = i / 6 * P * 2 + t * 0.4; circ(fx + Math.cos(a) * 12, fy - 16 + Math.sin(a) * 12, 9, '#ff7a59'); }
      circ(fx, fy - 16, 8, '#ffd23f');
    } else if (id === 'beanie') {
      g.fillStyle = '#3a8de0'; g.beginPath(); g.ellipse(0, 4, 44, 38, 0, P, 0); g.fill();
      g.save(); g.beginPath(); g.ellipse(0, 4, 44, 38, 0, P, 0); g.clip(); g.fillStyle = '#8fd3ff'; for (const y of [-26, -12]) g.fillRect(-50, y, 100, 6); g.restore();
      rr(-46, -4, 92, 14, 6, '#2d6fb5'); circ(0, -38, 11, '#ffd23f');
    } else if (id === 'baseball') {
      g.fillStyle = '#e0564f'; g.beginPath(); g.ellipse(0, 4, 42, 34, 0, P, 0); g.fill();
      if (side) rr(20, -4, 50, 10, 5, '#a8323b'); else { g.fillStyle = '#a8323b'; g.beginPath(); g.ellipse(0, 4, 40, 9, 0, 0, P); g.fill(); }
      circ(0, -30, 5, '#a8323b'); g.fillStyle = '#ffffff'; g.font = 'bold 26px sans-serif'; g.textAlign = 'center'; g.fillText('C', side ? -6 : 0, -4);
    } else if (id === 'chef') {
      rr(-30, -16, 60, 22, 4, '#f5f1e8');
      for (const [x, y, r] of [[-22, -34, 18], [0, -44, 22], [22, -34, 18]]) circ(x, y, r, '#ffffff');
      g.fillStyle = '#d9d2e6'; g.fillRect(-30, -2, 60, 3);
    } else if (id === 'propeller') {
      rr(-38, -6, 76, 14, 7, '#e0564f');
      g.fillStyle = '#5ad1ff'; g.beginPath(); g.ellipse(0, -4, 34, 30, 0, P, 0); g.fill();
      g.fillStyle = '#ffd23f'; g.beginPath(); g.ellipse(0, -4, 34, 30, 0, P * 1.38, P * 1.62); g.lineTo(0, -4); g.fill();
      g.fillStyle = '#3a3346'; g.fillRect(-2.5, -50, 5, 18);
      const sp = Math.cos(t * 18); rr(-34 * Math.abs(sp), -54, 68 * Math.abs(sp) + 1, 8, 4, '#e0564f'); circ(0, -50, 5, '#ffd23f');
    } else if (id === 'cowboy') {
      g.fillStyle = '#8a5a30'; g.beginPath(); g.ellipse(0, 2, 66, 13, 0, 0, P * 2); g.fill();
      g.fillStyle = '#b07a44'; g.beginPath(); g.moveTo(-32, 0); g.lineTo(-28, -36); g.quadraticCurveTo(0, -24, 28, -36); g.lineTo(32, 0); g.closePath(); g.fill();
      g.fillStyle = '#5d3a1c'; g.fillRect(-31, -10, 62, 7);
    } else if (id === 'pirate') {
      g.fillStyle = '#1f1b26'; g.beginPath(); g.moveTo(-54, 4); g.quadraticCurveTo(0, -70, 54, 4); g.quadraticCurveTo(0, -12, -54, 4); g.fill();
      circ(0, -22, 9, '#f5f1e8'); circ(-3, -24, 2.2, '#1f1b26'); circ(3, -24, 2.2, '#1f1b26');
      g.strokeStyle = '#ffd23f'; g.lineWidth = 3; g.beginPath(); g.moveTo(-50, 1); g.quadraticCurveTo(0, -14, 50, 1); g.stroke();
    } else if (id === 'tophat') {
      rr(-46, -4, 92, 10, 5, '#1f1b26');
      rr(-28, -56, 56, 56, 5, '#2b2833');
      g.fillStyle = '#e0564f'; g.fillRect(-28, -18, 56, 10);
      g.fillStyle = 'rgba(255,255,255,0.18)'; g.fillRect(-22, -52, 6, 32);
    } else if (id === 'wizard') {
      g.fillStyle = '#5b3f86'; g.beginPath(); g.ellipse(0, 2, 56, 11, 0, 0, P * 2); g.fill();
      poly([[-30, 0], [side ? 22 : 8, -78], [30, 0]], '#7a58a8');
      K.star(-6, -24, 7, '#ffd23f'); K.star(12, -46, 5, '#ffd23f'); K.star(8, -10, 4, '#fff6a0');
    } else if (id === 'crown') {
      g.fillStyle = '#ffcf3a'; g.beginPath(); g.moveTo(-36, 6); g.lineTo(-36, -30); g.lineTo(-18, -12); g.lineTo(0, -38); g.lineTo(18, -12); g.lineTo(36, -30); g.lineTo(36, 6); g.closePath(); g.fill();
      g.fillStyle = '#e8a91f'; g.fillRect(-36, -4, 72, 10);
      circ(-36, -32, 5, '#fff6a0'); circ(0, -40, 6, '#fff6a0'); circ(36, -32, 5, '#fff6a0');
      circ(-18, 1, 4.5, '#e0564f'); circ(0, 1, 4.5, '#5ad1ff'); circ(18, 1, 4.5, '#49f08a');
    } else if (id === 'rainbowcap') {
      for (let i = 0; i < 5; i++) { g.fillStyle = rb(t, i); g.beginPath(); g.ellipse(0, 4, 46 - i * 8, 40 - i * 7, 0, P, 0); g.fill(); }
      rr(-48, -4, 96, 12, 6, rb(t, 5, 45)); K.star(0, -42, 9, K.plain ? '#ffffff' : '#fff6a0');
    } else if (id === 'mirror') {
      rr(-50, 6, 100, 10, 5, '#e8e8f0');
      const mx = side ? 22 : -20;
      circ(mx, 4, 17, '#8a93a3'); circ(mx, 4, 13, '#dfe6f0'); circ(mx - 4, 0, 4, '#ffffff');
    } else if (id === 'policecap') {
      g.fillStyle = '#2b3f7a'; g.beginPath(); g.ellipse(0, -8, 50, 26, 0, P, 0); g.fill(); g.fillRect(-36, -10, 72, 8);
      rr(-40, -6, 80, 12, 4, '#1b2550');
      if (side) rr(22, 2, 40, 8, 4, '#14141e'); else { g.fillStyle = '#14141e'; g.beginPath(); g.ellipse(0, 6, 40, 9, 0, 0, P); g.fill(); }
      K.star(side ? 4 : 0, -18, 9, '#ffd23f');
    } else if (id === 'mohawk') {
      for (let i = -2; i <= 2; i++) poly([[i * 12 - 9, 8], [i * 12 + (side ? -10 : i * 3), -46 + Math.abs(i) * 9], [i * 12 + 9, 8]], i % 2 ? '#c23bd0' : '#ff4fa3');
      for (let i = -2; i <= 2; i++) circ(i * 12 + (side ? -10 : i * 3), -44 + Math.abs(i) * 9, 4, '#ffd23f');
    } else if (id === 'firehat') {
      g.fillStyle = '#b02020'; g.beginPath(); g.ellipse(side ? -6 : 0, 4, side ? 70 : 60, 11, 0, 0, P * 2); g.fill();
      g.fillStyle = '#d8322c'; g.beginPath(); g.ellipse(0, 2, 42, 40, 0, P, 0); g.fill();
      g.fillStyle = '#ff6a5a'; g.fillRect(-4, -36, 8, 36);
      if (!side) { rr(-13, -30, 26, 22, 5, '#ffd23f'); g.fillStyle = '#b02020'; g.font = 'bold 18px sans-serif'; g.textAlign = 'center'; g.fillText('1', 0, -13); }
      else rr(20, -26, 12, 20, 4, '#ffd23f');
    } else if (id === 'spacehelm') {
      // a glass bowl: just the rim and a shine, so the face still shows (the N64 has no half-clear)
      g.strokeStyle = '#d8dde8'; g.lineWidth = 9; g.beginPath(); g.arc(0, 40, 60, 0, P * 2); g.stroke();
      g.strokeStyle = '#ffffff'; g.lineWidth = 6; g.lineCap = 'round'; g.beginPath(); g.arc(0, 40, 48, P * 1.15, P * 1.45); g.stroke();
      rr(-44, 92, 88, 12, 6, '#9aa1b0');
      g.fillStyle = '#9aa1b0'; g.fillRect(-2.5, -36, 5, 18); circ(0, -38, 6, '#e0564f');
    } else if (id === 'hardhat') {
      rr(-58, -4, 116, 11, 5, '#e6a800');
      g.fillStyle = '#ffc21a'; g.beginPath(); g.ellipse(0, 0, 44, 38, 0, P, 0); g.fill();
      rr(-7, -38, 14, 38, 5, '#ffd84d');
    } else if (id === 'tiara') {
      g.fillStyle = '#ffcf3a'; g.beginPath(); g.moveTo(-30, 4); g.lineTo(-30, -6); g.lineTo(-18, -14); g.lineTo(-10, -6); g.lineTo(0, -26); g.lineTo(10, -6); g.lineTo(18, -14); g.lineTo(30, -6); g.lineTo(30, 4); g.closePath(); g.fill();
      circ(0, -12, 6, '#ff5aa8'); circ(-18, -6, 3.5, '#5ad1ff'); circ(18, -6, 3.5, '#5ad1ff'); circ(-1.5, -14, 2, '#ffffff');
    } else if (id === 'ninjaband') {
      rr(-52, 8, 104, 15, 5, '#d8322c');
      if (!side) rr(-14, 9, 28, 13, 3, '#9aa1b0');
      else rr(14, 9, 22, 13, 3, '#9aa1b0');
      const bx = side ? -50 : 44;
      poly([[bx, 10], [bx + (side ? -28 : 26), 26], [bx + (side ? -22 : 20), 34], [bx, 20]], '#b02020');
      poly([[bx, 14], [bx + (side ? -32 : 30), 40], [bx + (side ? -24 : 22), 46], [bx, 22]], '#d8322c');
    } else if (id === 'clownwig') {
      const cols = ['#ff5a5a', '#5ad1ff', '#ffd23f', '#49d17a'];
      [[-48, 16, 18], [-58, 36, 15], [-34, -4, 16], [48, 16, 18], [58, 36, 15], [34, -4, 16], [-12, -14, 15], [12, -14, 15]].forEach(([x, y, r], i) => {
        if (side && x > 30) return;
        circ(side ? x * 0.8 - 10 : x, y, r, cols[i % 4]);
      });
    } else if (id === 'knighthelm') {
      g.fillStyle = '#b8c0cc'; g.beginPath(); g.ellipse(0, 8, 50, 46, 0, P, 0); g.fill();
      rr(-52, 0, 104, 12, 4, '#8a93a3');
      for (const rx of [-36, 0, 36]) circ(side ? rx * 0.8 : rx, 6, 3, '#dfe6f0');
      g.fillStyle = '#dfe6f0'; g.fillRect(-4, -36, 8, 36);
      for (let i = 0; i < 4; i++) circ((side ? -14 : 0) - i * (side ? 7 : 0), -42 - i * 8 + (side ? i * 6 : 0), 10 - i, i % 2 ? '#ff7a6a' : '#e0564f');
    }
    g.restore();
  };
  // FACE: eyes = [[x, y, rx, ry], ...], the near eye first (side views pass the squeezed far eye second)
  K.wearFace = (id, eyes, side = false) => {
    const g = K.g, [a, b] = eyes, dir = b[0] > a[0] ? 1 : -1;
    const bridge = (c, lw, k = 0.3) => { g.strokeStyle = c; g.lineWidth = pxw(lw); g.lineCap = 'round'; g.beginPath(); g.moveTo(a[0] + dir * a[2], a[1] - a[3] * k); g.lineTo(b[0] - dir * b[2], b[1] - b[3] * k); g.stroke();
      if (side) { g.beginPath(); g.moveTo(a[0] - a[2], a[1] - a[3] * k); g.lineTo(a[0] - a[2] * 2.6, a[1] - a[3] * (k + 0.2)); g.stroke(); } };
    if (id === 'shades') {
      bridge('#1b1426', Math.max(1.2, a[2] * 0.22));
      for (const [x, y, rx, ry] of eyes) {
        rr(x - rx * 1.15, y - ry * 0.62, rx * 2.3, ry * 1.2, ry * 0.45, '#1b1426');
        g.fillStyle = 'rgba(255,255,255,0.35)'; g.beginPath(); g.moveTo(x - rx * 0.6, y - ry * 0.4); g.lineTo(x - rx * 0.1, y - ry * 0.4); g.lineTo(x - rx * 0.5, y + ry * 0.3); g.closePath(); g.fill();
      }
    } else if (id === 'glasses') {
      bridge('#3a3346', Math.max(1.2, a[2] * 0.16), 0);
      for (const [x, y, rx, ry] of eyes) { g.strokeStyle = '#3a3346'; g.lineWidth = pxw(Math.max(1.2, rx * 0.18)); g.beginPath(); g.ellipse(x, y, rx * 1.2, ry * 1.1, 0, 0, P * 2); g.stroke(); }
    } else if (id === 'hearts' || id === 'stars') {
      bridge(id === 'hearts' ? '#c23b78' : '#d49a10', Math.max(1.2, a[2] * 0.18), 0.5);
      for (const [x, y, rx, ry] of eyes) {
        g.save(); g.translate(x, y); g.scale(rx / ry, 1);
        if (id === 'hearts') { heart(0, 0, ry * 0.95, '#c23b78'); heart(0, -ry * 0.04, ry * 0.75, '#ff8fc7'); }
        else { K.star(0, 0, ry * 1.15, '#d49a10'); K.star(0, 0, ry * 0.85, '#ffd23f'); }
        g.restore();
      }
    } else if (id === 'goggles') {
      const y0 = a[1] - a[3] * 0.1;
      g.strokeStyle = '#5d3a1c'; g.lineWidth = a[3] * 0.5; g.beginPath(); g.moveTo(a[0] - dir * a[2] * (side ? 3 : 2), y0); g.lineTo(b[0] + dir * b[2] * (side ? 0.5 : 2), y0); g.stroke();
      for (const [x, y, rx, ry] of eyes) { const r = Math.max(rx, ry * 0.8); circ(x, y, r * 1.28, '#9aa1b0'); g.globalAlpha = 0.55; circ(x, y, r * 1.08, '#8fd3ff'); g.globalAlpha = 1; circ(x - rx * 0.4, y - ry * 0.4, ry * 0.18, '#ffffff'); }
    } else if (id === 'mustache') {
      const mx = side ? a[0] + (b[0] - a[0]) * 0.7 : (a[0] + b[0]) / 2, my = a[1] + a[3] * 1.15, s = a[3] * 1.2;
      g.fillStyle = '#5d3a1c';
      for (const d of side ? [1] : [-1, 1]) { g.beginPath(); g.moveTo(mx, my - s * 0.15); g.quadraticCurveTo(mx + d * s * 0.9, my - s * 0.55, mx + d * s * 1.3, my - s * 0.1); g.quadraticCurveTo(mx + d * s * 0.8, my + s * 0.25, mx, my + s * 0.15); g.fill(); }
      if (side) { g.beginPath(); g.moveTo(mx, my - s * 0.15); g.quadraticCurveTo(mx - s * 0.6, my - s * 0.4, mx - s * 0.8, my); g.quadraticCurveTo(mx - s * 0.4, my + s * 0.2, mx, my + s * 0.15); g.fill(); }
    } else if (id === 'monocle') {
      const [x, y, rx, ry] = side ? a : b, pad = rx * 0.2;
      g.strokeStyle = '#d49a10'; g.lineWidth = pxw(Math.max(1.5, rx * 0.2)); g.beginPath(); g.ellipse(x, y, rx + pad, ry + pad, 0, 0, P * 2); g.stroke();
      g.lineWidth = pxw(Math.max(1, rx * 0.08)); g.beginPath(); g.moveTo(x, y + ry + pad); g.quadraticCurveTo(x - rx * 0.6, y + ry * 2.2, x - rx * 1.4, y + ry * 1.9); g.stroke();
    } else if (id === 'eyepatch') {
      const [x, y, rx, ry] = a;
      g.strokeStyle = '#1b1426'; g.lineWidth = pxw(Math.max(1.2, ry * 0.22)); g.beginPath(); g.moveTo(x - rx * 2.4, y - ry * 1.6); g.lineTo(x + rx * 2.6, y + ry * 0.2); g.stroke();
      g.fillStyle = '#1b1426'; g.beginPath(); g.ellipse(x, y, rx * 1.25, ry * 1.05, 0, 0, P * 2); g.fill();
    } else if (id === 'rednose') {
      const nx = side ? a[0] + (b[0] - a[0]) * 0.95 : (a[0] + b[0]) / 2, ny = a[1] + a[3] * (side ? 0.6 : 0.95), r = a[3] * 0.75;
      circ(nx, ny, r, '#ff3030'); circ(nx - r * 0.35, ny - r * 0.35, r * 0.3, '#ffb0b0');
    } else if (id === 'mask') {
      const x0 = Math.min(a[0], b[0]) - (side ? a[2] * 1.6 : a[2] * 1.3), x1 = Math.max(a[0], b[0]) + (side ? b[2] * 1.2 : b[2] * 1.3), y0 = a[1] + a[3] * 0.85, h = a[3] * 1.7;
      rr(x0, y0, x1 - x0, h, h * 0.35, '#9fd8f0');
      g.fillStyle = '#7cc0dc'; for (const k of [0.33, 0.66]) g.fillRect(x0 + (x1 - x0) * 0.08, y0 + h * k, (x1 - x0) * 0.84, Math.max(0.8, h * 0.08));
      g.strokeStyle = '#e8f4fa'; g.lineWidth = pxw(Math.max(1, a[3] * 0.12)); g.beginPath(); g.moveTo(x0, y0 + h * 0.3); g.lineTo(x0 - a[2] * 1.2, y0 - a[3] * 0.4); g.stroke();
      if (!side) { g.beginPath(); g.moveTo(x1, y0 + h * 0.3); g.lineTo(x1 + a[2] * 1.2, y0 - a[3] * 0.4); g.stroke(); }
    }
  };
  // NECK: at the bottom middle of the head, 100 units = the neck piece's width
  K.wearNeck = (id, x, y, w, dir = 0) => {
    const g = K.g, k = w / 100;
    g.save(); g.translate(x, y); g.scale(k * (dir ? 0.7 : 1), k);
    if (id === 'bowtie') {
      poly([[0, 0], [-30, -16], [-30, 16]], '#e0564f'); poly([[0, 0], [30, -16], [30, 16]], '#e0564f');
      circ(0, 0, 8, '#a8323b');
    } else if (id === 'scarf') {
      rr(-46, -10, 92, 22, 11, '#49d17a');
      g.fillStyle = '#ffffff'; for (const sx of [-26, 0, 26]) g.fillRect(sx - 4, -10, 8, 22);
      g.save(); g.translate(dir ? -30 : 24, 6); g.rotate(dir ? 0.5 : 0.2); rr(-10, 0, 20, 40, 6, '#49d17a'); g.fillStyle = '#ffffff'; g.fillRect(-10, 14, 20, 7); g.restore();
    } else if (id === 'necktie') {
      poly([[-9, -8], [9, -8], [6, 6], [-6, 6]], '#3a8de0');
      poly([[-6, 6], [6, 6], [12, 56], [0, 68], [-12, 56]], '#3a8de0');
      g.fillStyle = '#8fd3ff'; for (const yy of [20, 38]) { g.beginPath(); g.moveTo(-9, yy); g.lineTo(10, yy - 8); g.lineTo(11, yy - 2); g.lineTo(-10, yy + 6); g.fill(); }
    } else if (id === 'bandana') {
      poly([[-44, -8], [44, -8], [0, 38]], '#e0564f');
      for (const [px, py] of [[-20, 2], [0, 4], [20, 2], [-8, 18], [8, 18]]) circ(px, py, 3.5, '#ffffff');
    } else if (id === 'medal') {
      poly([[-26, -8], [-10, -8], [4, 34], [-6, 38]], '#3a8de0'); poly([[26, -8], [10, -8], [-4, 34], [6, 38]], '#e0564f');
      circ(0, 46, 17, '#d49a10'); circ(0, 46, 13, '#ffd23f'); K.star(0, 46, 8, '#fff6a0');
    } else if (id === 'stetho') {
      g.strokeStyle = '#3a3346'; g.lineWidth = 7; g.lineCap = 'round';
      g.beginPath(); g.moveTo(-36, -8); g.quadraticCurveTo(-36, 34, -4, 40); g.stroke();
      g.beginPath(); g.moveTo(36, -8); g.quadraticCurveTo(34, 30, 10, 42); g.lineTo(14, 52); g.stroke();
      circ(16, 58, 11, '#8a93a3'); circ(16, 58, 7, '#dfe6f0');
    } else if (id === 'pearls') {
      for (let i = 0; i <= 8; i++) { const an = P * (0.08 + i * 0.105); const px = Math.cos(an) * 40, py = -4 + Math.sin(an) * 30; circ(px, py, 7, '#d9d2e6'); circ(px, py, 5.5, '#f8f5ff'); circ(px - 2, py - 2, 1.8, '#ffffff'); }
    } else if (id === 'chain') {
      for (let i = 0; i <= 10; i++) { const an = P * (0.1 + i * 0.08); circ(Math.cos(an) * 36, -4 + Math.sin(an) * 34, 4.5, i % 2 ? '#d49a10' : '#ffd23f'); }
      K.star(0, 44, 16, '#d49a10'); K.star(0, 44, 12, '#ffd23f');
    }
    g.restore();
  };
  // BODY: the torso's rounded box { x, y, w, h, r }; drawn just over the body, under the head and arms
  K.wearBody = (id, b, t = 0, side = false) => {
    const g = K.g;
    g.save(); g.beginPath(); g.moveTo(b.x + b.r, b.y); g.arcTo(b.x + b.w, b.y, b.x + b.w, b.y + b.h, b.r); g.arcTo(b.x + b.w, b.y + b.h, b.x, b.y + b.h, b.r); g.arcTo(b.x, b.y + b.h, b.x, b.y, b.r); g.arcTo(b.x, b.y, b.x + b.w, b.y, b.r); g.closePath(); g.clip();
    const { x, y, w, h } = b, cx = x + w / 2;
    if (id === 'sweater') {
      g.fillStyle = '#e0564f'; g.fillRect(x, y, w, h);
      g.fillStyle = '#ffffff'; for (let k = 1; k < 5; k++) g.fillRect(x, y + h * k / 5 - h * 0.05, w, h * 0.09);
    } else if (id === 'overalls') {
      g.fillStyle = '#3a6fc0'; g.fillRect(x, y + h * 0.42, w, h);
      g.fillRect(cx - w * 0.22, y + h * 0.22, w * 0.44, h * 0.3);
      for (const d of side ? [1] : [-1, 1]) { g.save(); g.translate(cx + d * w * 0.2, y + h * 0.24); g.rotate(d * 0.25); g.fillRect(-w * 0.05, -h * 0.3, w * 0.1, h * 0.32); g.restore(); circ(cx + d * w * 0.17, y + h * 0.27, w * 0.04, '#ffd23f'); }
      g.fillStyle = '#2b55a0'; g.fillRect(cx - w * 0.12, y + h * 0.5, w * 0.24, h * 0.16);
    } else if (id === 'jersey') {
      g.fillStyle = '#49d17a'; g.fillRect(x, y, w, h);
      g.fillStyle = '#ffffff'; g.fillRect(x, y + h * 0.82, w, h * 0.06);
      g.font = `bold ${Math.round(h * 0.5)}px sans-serif`; g.textAlign = 'center'; g.fillText('7', side ? cx + w * 0.1 : cx, y + h * 0.66);
    } else if (id === 'tux') {
      g.fillStyle = '#2b2833'; g.fillRect(x, y, w, h);
      if (side) { poly([[x + w * 0.62, y], [x + w, y], [x + w, y + h], [x + w * 0.86, y + h]], '#f5f1e8'); circ(x + w * 0.82, y + h * 0.45, w * 0.04, '#2b2833'); }
      else {
        poly([[cx - w * 0.2, y], [cx + w * 0.2, y], [cx, y + h * 0.62]], '#f5f1e8');
        poly([[cx - w * 0.12, y], [cx - w * 0.02, y + h * 0.18], [cx - w * 0.2, y + h * 0.14]], '#1b1426'); poly([[cx + w * 0.12, y], [cx + w * 0.02, y + h * 0.18], [cx + w * 0.2, y + h * 0.14]], '#1b1426');
        for (const k of [0.68, 0.82]) circ(cx, y + h * k, w * 0.03, '#f5f1e8');
      }
    } else if (id === 'super') {
      g.fillStyle = '#3a6fc0'; g.fillRect(x, y, w, h);
      g.fillStyle = '#e0564f'; g.fillRect(x, y + h * 0.78, w, h * 0.12);
      const ex = side ? cx + w * 0.15 : cx, ey = y + h * 0.38, er = Math.min(w, h) * 0.2;
      circ(ex, ey, er, '#ffd23f');
      poly([[ex + er * 0.2, ey - er * 0.8], [ex - er * 0.5, ey + er * 0.1], [ex, ey + er * 0.1], [ex - er * 0.3, ey + er * 0.8], [ex + er * 0.5, ey - er * 0.15], [ex, ey - er * 0.15]], '#e0564f');
    } else if (id === 'disco') {
      const n = 6;
      for (let i = 0; i < n; i++) for (let j = 0; j < n; j++) { g.fillStyle = rb(t * 2, i + j * 2, 55 + ((i + j) % 2) * 12); g.fillRect(x + i * w / n, y + j * h / n, w / n + 0.5, h / n + 0.5); }
      if (!K.plain) { g.globalAlpha = 0.5 + 0.5 * Math.sin(t * 9); K.star(x + w * 0.3, y + h * 0.3, Math.min(w, h) * 0.08, '#ffffff'); g.globalAlpha = 1; }
    } else if (SUITS[id]) {
      // a jacket over a shirt and tie (the side view shows the shirt down the front edge)
      const [jacket, shirt, tie, stripe] = SUITS[id];
      g.fillStyle = jacket; g.fillRect(x, y, w, h);
      if (stripe) { g.fillStyle = stripe; for (let k = 1; k < 8; k++) g.fillRect(x + w * k / 8, y, Math.max(1, w * 0.02), h); }
      if (id === 'plaidsuit') { g.fillStyle = '#2a5a36'; for (let k = 0; k < 4; k++) g.fillRect(x, y + h * (0.12 + k * 0.25), w, h * 0.06); g.fillStyle = '#ffd23f'; for (let k = 0; k < 4; k++) g.fillRect(x + w * (0.1 + k * 0.25), y, Math.max(1, w * 0.02), h); }
      if (id === 'goldsuit' || id === 'velvetsuit') { g.fillStyle = id === 'goldsuit' ? '#fff6a0' : '#a050b0'; g.fillRect(x + w * 0.08, y, w * 0.08, h); }
      if (side) { poly([[x + w * 0.62, y], [x + w, y], [x + w, y + h], [x + w * 0.86, y + h]], shirt); if (tie) rr(x + w * 0.86, y, w * 0.08, h * 0.55, 2, tie); circ(x + w * 0.74, y + h * 0.5, w * 0.04, '#1b1426'); }
      else {
        poly([[cx - w * 0.22, y], [cx + w * 0.22, y], [cx, y + h * 0.62]], shirt);
        if (tie) { poly([[cx - w * 0.05, y], [cx + w * 0.05, y], [cx + w * 0.08, y + h * 0.4], [cx, y + h * 0.5], [cx - w * 0.08, y + h * 0.4]], tie); }
        else { poly([[cx, y + h * 0.08], [cx - w * 0.12, y], [cx - w * 0.12, y + h * 0.16]], '#1b1426'); poly([[cx, y + h * 0.08], [cx + w * 0.12, y], [cx + w * 0.12, y + h * 0.16]], '#1b1426'); }
        for (const k of [0.7, 0.84]) circ(cx, y + h * k, w * 0.03, '#1b1426');
      }
    } else if (id === 'chefcoat') {
      g.fillStyle = '#f5f5f0'; g.fillRect(x, y, w, h);
      g.fillStyle = '#d9d2e6'; g.fillRect(side ? x + w * 0.6 : cx - w * 0.01, y, w * 0.02, h);
      for (const k of [0.32, 0.52, 0.72]) for (const d of side ? [0.72] : [-0.16, 0.16]) circ(side ? x + w * d : cx + w * d, y + h * k, w * 0.035, '#5d566a');
      if (!side) poly([[cx - w * 0.18, y], [cx + w * 0.18, y], [cx, y + h * 0.2]], '#e0564f');
    } else if (id === 'labcoat') {
      g.fillStyle = '#f2f4f8'; g.fillRect(x, y, w, h);
      if (side) { rr(x + w * 0.3, y + h * 0.55, w * 0.3, h * 0.2, 2, '#dfe3ea'); g.fillStyle = '#3a8de0'; g.fillRect(x + w * 0.36, y + h * 0.48, w * 0.04, h * 0.12); }
      else {
        g.fillStyle = '#7cb4e8'; g.fillRect(cx - w * 0.12, y, w * 0.24, h);
        poly([[cx - w * 0.12, y], [cx - w * 0.26, y], [cx - w * 0.12, y + h * 0.35]], '#dfe3ea'); poly([[cx + w * 0.12, y], [cx + w * 0.26, y], [cx + w * 0.12, y + h * 0.35]], '#dfe3ea');
        rr(x + w * 0.12, y + h * 0.5, w * 0.2, h * 0.18, 2, '#dfe3ea'); g.fillStyle = '#e0564f'; g.fillRect(x + w * 0.16, y + h * 0.42, w * 0.04, h * 0.12);
      }
    } else if (id === 'police') {
      g.fillStyle = '#2b4a9a'; g.fillRect(x, y, w, h);
      g.fillStyle = '#1b1426'; g.fillRect(x, y + h * 0.78, w, h * 0.1); rr(side ? x + w * 0.6 : cx - w * 0.08, y + h * 0.77, w * 0.16, h * 0.12, 2, '#ffd23f');
      K.star(side ? x + w * 0.62 : cx - w * 0.22, y + h * 0.3, Math.min(w, h) * 0.1, '#ffd23f');
      if (!side) { g.fillStyle = '#1f3a7a'; g.fillRect(cx + w * 0.1, y + h * 0.2, w * 0.24, h * 0.12); for (const k of [0.25, 0.45, 0.65]) circ(cx, y + h * k, w * 0.025, '#ffd23f'); }
    } else if (id === 'leather') {
      g.fillStyle = '#1f1b26'; g.fillRect(x, y, w, h);
      const sx = side ? x + w * 0.66 : cx - w * 0.13, sw = side ? w * 0.34 : w * 0.26;
      g.fillStyle = '#e0564f'; g.fillRect(sx, y, sw, h);
      poly([[sx + sw * 0.6, y + h * 0.15], [sx + sw * 0.2, y + h * 0.45], [sx + sw * 0.5, y + h * 0.45], [sx + sw * 0.3, y + h * 0.8], [sx + sw * 0.85, y + h * 0.38], [sx + sw * 0.55, y + h * 0.38]], '#ffd23f');
      for (let k = 0; k < 4; k++) { circ(sx - w * 0.04, y + h * (0.12 + k * 0.2), w * 0.025, '#c9d2e0'); if (!side) circ(sx + sw + w * 0.04, y + h * (0.12 + k * 0.2), w * 0.025, '#c9d2e0'); }
    } else if (id === 'firecoat') {
      g.fillStyle = '#d4a64a'; g.fillRect(x, y, w, h);
      for (const k of [0.5, 0.78]) { g.fillStyle = '#ffd23f'; g.fillRect(x, y + h * k, w, h * 0.12); g.fillStyle = '#e8eef4'; g.fillRect(x, y + h * (k + 0.03), w, h * 0.06); }
      if (!side) { g.fillStyle = '#3a3346'; for (const k of [0.15, 0.32]) g.fillRect(cx - w * 0.08, y + h * k, w * 0.16, h * 0.05); }
    } else if (id === 'spacesuit') {
      g.fillStyle = '#eef1f6'; g.fillRect(x, y, w, h);
      g.fillStyle = '#9aa1b0'; g.fillRect(x, y + h * 0.8, w, h * 0.08);
      const px = side ? x + w * 0.5 : cx - w * 0.2;
      rr(px, y + h * 0.25, w * 0.4, h * 0.3, 3, '#5d566a');
      circ(px + w * 0.1, y + h * 0.35, w * 0.045, '#e0564f'); circ(px + w * 0.22, y + h * 0.35, w * 0.045, '#5ad1ff'); rr(px + w * 0.06, y + h * 0.44, w * 0.28, h * 0.06, 1, '#ffd23f');
      if (!side) { rr(x + w * 0.1, y + h * 0.12, w * 0.16, h * 0.1, 1, '#3a6fc0'); g.fillStyle = '#ffffff'; g.fillRect(x + w * 0.1, y + h * 0.16, w * 0.16, h * 0.02); }
    } else if (id === 'vest') {
      g.fillStyle = '#6a7a8c'; g.fillRect(x, y, w, h);
      if (side) { g.fillStyle = '#ff8a1a'; g.fillRect(x, y, w * 0.8, h); }
      else { g.fillStyle = '#ff8a1a'; g.fillRect(x, y, w * 0.38, h); g.fillRect(x + w * 0.62, y, w * 0.38, h); }
      g.fillStyle = '#e8eef4'; for (const k of [0.42, 0.68]) { g.fillRect(x, y + h * k, side ? w * 0.8 : w * 0.38, h * 0.07); if (!side) g.fillRect(x + w * 0.62, y + h * k, w * 0.38, h * 0.07); }
    } else if (id === 'pirateshirt') {
      g.fillStyle = '#f5f1e8'; g.fillRect(x, y, w, h);
      g.fillStyle = '#2b4a9a'; for (let k = 0; k < 5; k++) g.fillRect(x, y + h * (0.06 + k * 0.2), w, h * 0.09);
      g.fillStyle = '#5d3a1c'; g.fillRect(x, y + h * 0.74, w, h * 0.12); rr(side ? x + w * 0.6 : cx - w * 0.1, y + h * 0.72, w * 0.2, h * 0.16, 2, '#ffd23f'); rr(side ? x + w * 0.64 : cx - w * 0.06, y + h * 0.75, w * 0.12, h * 0.1, 1, '#5d3a1c');
    } else if (id === 'ninjasuit') {
      g.fillStyle = '#24202e'; g.fillRect(x, y, w, h);
      poly([[x + w * (side ? 0.5 : 0.25), y], [x + w * (side ? 0.68 : 0.42), y], [x + w * (side ? 1 : 0.78), y + h * 0.7], [x + w * (side ? 0.82 : 0.6), y + h * 0.7]], '#14121a');
      g.fillStyle = '#d8322c'; g.fillRect(x, y + h * 0.66, w, h * 0.12);
      if (!side) poly([[cx + w * 0.1, y + h * 0.72], [cx + w * 0.2, y + h], [cx + w * 0.28, y + h]], '#b02020');
    } else if (id === 'clownsuit') {
      g.fillStyle = '#ffd23f'; g.fillRect(x, y, w, h);
      if (!side) { g.fillStyle = '#ff8fc7'; g.fillRect(cx, y, w / 2, h); }
      [[0.25, 0.35, '#e0564f'], [0.7, 0.3, '#3a8de0'], [0.4, 0.7, '#49d17a'], [0.78, 0.72, '#c28bff'], [0.15, 0.82, '#3a8de0']].forEach(([u, v, c]) => circ(x + w * u, y + h * v, Math.min(w, h) * 0.08, c));
      for (let k = 0; k < 6; k++) circ(x + w * (k + 0.5) / 6, y + h * 0.02, w * 0.1, '#ffffff');
    } else if (id === 'armor') {
      g.fillStyle = '#b8c0cc'; g.fillRect(x, y, w, h);
      g.fillStyle = '#8a93a3'; for (let k = 1; k < 5; k++) g.fillRect(x, y + h * k / 5, w, Math.max(1, h * 0.025));
      g.fillStyle = '#dfe6f0'; g.fillRect(x + w * 0.1, y, w * 0.08, h);
      const ex = side ? x + w * 0.6 : cx;
      g.fillStyle = '#e0564f'; g.fillRect(ex - w * 0.05, y + h * 0.15, w * 0.1, h * 0.42); g.fillRect(ex - w * 0.16, y + h * 0.26, w * 0.32, h * 0.09);
    } else if (id === 'robe') {
      g.fillStyle = '#5b3f86'; g.fillRect(x, y, w, h);
      g.fillStyle = '#ffd23f'; g.fillRect(side ? x + w * 0.82 : cx - w * 0.05, y, w * 0.1, h);
      [[0.2, 0.25, 0.09], [0.32, 0.65, 0.07], [0.75, 0.4, 0.08], [0.68, 0.82, 0.06]].forEach(([u, v, r]) => K.star(x + w * u, y + h * v, Math.min(w, h) * r, '#ffd23f'));
    } else if (id === 'cowvest') {
      g.fillStyle = '#c0392b'; g.fillRect(x, y, w, h);
      g.fillStyle = '#8a2a20'; for (let k = 1; k < 5; k++) { g.fillRect(x, y + h * k / 5, w, Math.max(1, h * 0.03)); g.fillRect(x + w * k / 5, y, Math.max(1, w * 0.03), h); }
      g.fillStyle = '#8a5a30'; if (side) g.fillRect(x, y, w * 0.7, h); else { g.fillRect(x, y, w * 0.34, h); g.fillRect(x + w * 0.66, y, w * 0.34, h); }
      K.star(side ? x + w * 0.4 : x + w * 0.18, y + h * 0.32, Math.min(w, h) * 0.1, '#ffd23f');
    } else if (DRESSES[id]) {
      // a fitted top and a skirt from the waist down, with the dress's trim
      const [top, skirt, trim] = DRESSES[id];
      const pc = id === 'sparkledress' ? rb(t * 2, 1, 60) : top;
      g.fillStyle = pc; g.fillRect(x, y, w, h);
      g.fillStyle = id === 'sparkledress' ? rb(t * 2, 3, 55) : skirt; g.fillRect(x, y + h * 0.5, w, h * 0.5);
      if (id === 'sundress') [[0.2, 0.7], [0.5, 0.85], [0.8, 0.68], [0.35, 0.3], [0.7, 0.25]].forEach(([u, v]) => { for (let i = 0; i < 5; i++) circ(x + w * u + Math.cos(i * 1.26) * w * 0.04, y + h * v + Math.sin(i * 1.26) * w * 0.04, w * 0.03, '#ffffff'); circ(x + w * u, y + h * v, w * 0.025, '#ff8a3d'); });
      if (id === 'polkadress') for (let i = 0; i < 4; i++) for (let j = 0; j < 4; j++) if ((i + j) % 2) circ(x + w * (i + 0.5) / 4, y + h * (j + 0.5) / 4, Math.min(w, h) * 0.05, '#ffffff');
      if (id === 'partydress') { g.fillStyle = trim; for (const k of [0.62, 0.8]) for (let i = 0; i < 6; i++) { g.beginPath(); g.arc(x + w * (i + 0.5) / 6, y + h * k, w / 12, 0, P); g.fill(); } }
      if (id === 'tutu') { g.fillStyle = trim; g.beginPath(); g.moveTo(x, y + h * 0.48); for (let i = 0; i <= 10; i++) g.lineTo(x + w * i / 10, y + h * (i % 2 ? 0.8 : 0.66)); g.lineTo(x + w, y + h * 0.48); g.closePath(); g.fill(); }
      if (id === 'ballgown') { g.fillStyle = trim; for (let i = 1; i < 5; i++) g.fillRect(x + w * i / 5, y + h * 0.55, Math.max(1, w * 0.025), h * 0.45); for (const [u, v] of [[0.25, 0.3], [0.7, 0.2], [0.4, 0.75], [0.8, 0.7]]) K.star(x + w * u, y + h * v, Math.min(w, h) * 0.05, '#ffffff'); }
      if (id === 'sparkledress' && !K.plain) { g.globalAlpha = 0.5 + 0.5 * Math.sin(t * 9); K.star(x + w * 0.7, y + h * 0.3, Math.min(w, h) * 0.08, '#ffffff'); g.globalAlpha = 1; }
      g.fillStyle = id === 'sparkledress' ? (K.plain ? '#ffffff' : '#fff6a0') : trim; g.fillRect(x, y + h * 0.46, w, h * 0.07);
      if (id === 'partydress' || id === 'polkadress') rr(side ? x + w * 0.6 : cx - w * 0.12, y + h * 0.42, w * 0.24, h * 0.14, 3, id === 'partydress' ? '#e0408f' : '#1b1426');
    }
    g.restore();
  };
  // [jacket, shirt, tie (null: a black bow tie), stripes]
  const SUITS = { bluesuit: ['#2b4a9a', '#f5f1e8', '#e0564f'], pinstripe: ['#4a4a58', '#f5f1e8', '#7a58a8', '#8a8a9a'], whitesuit: ['#f5f1e8', '#ff8fc7', null],
    plaidsuit: ['#3a7a4a', '#f5f1e8', '#ffd23f'], velvetsuit: ['#7a2a8a', '#f5f1e8', null], goldsuit: ['#e8b83a', '#1b1426', '#e0564f'] };
  // [top, skirt, trim]
  const DRESSES = { sundress: ['#ffd23f', '#ffc21a', '#ffffff'], partydress: ['#ff8fc7', '#ff8fc7', '#ffc8e4'], polkadress: ['#e0564f', '#e0564f', '#1b1426'],
    tutu: ['#ffb3d9', '#ffb3d9', '#ff8fc7'], ballgown: ['#3a6fc0', '#5a8de0', '#8fb8f0'], sparkledress: ['#ffffff', '#ffffff', '#ffffff'] };
  // FEET: one shoe at a foot's end; s = how big the foot is (units); side views point the toe forward
  K.wearFoot = (id, x, y, s, t = 0, side = false, far = false) => {
    const g = K.g, k = s / 30;
    g.save(); g.translate(x, y); g.scale(k, k);
    if (far) g.globalAlpha = 0.85;
    const toe = side ? 12 : 0;   // side views: the shoe sticks out forward
    const shoe = (c, top = -12, sole = '#f5f1e8') => { rr(-22 + toe * 0.3, top, 44 + toe, 26 - top - 12, 10, c); rr(-24 + toe * 0.3, 10, 48 + toe, 6, 3, sole); };
    if (id === 'sneakers') { shoe('#e0564f'); g.fillStyle = '#ffffff'; for (const dx of [-6, 2]) g.fillRect(dx + toe * 0.3, -8, 3, 10); }
    else if (id === 'boots') { rr(-18 + toe * 0.2, -30, 32, 30, 6, '#8a5a30'); shoe('#8a5a30', -12, '#5d3a1c'); }
    else if (id === 'rainboots') { rr(-18 + toe * 0.2, -34, 32, 34, 6, '#ffd23f'); shoe('#ffd23f', -12, '#d49a10'); g.fillStyle = '#d49a10'; g.fillRect(-18 + toe * 0.2, -34, 32, 5); }
    else if (id === 'flippers') { poly([[-14, -6], [14, -6], [34 + toe * 1.5, 14], [-34 + (side ? 14 : 0), 14]], '#49d17a'); rr(-14, -12, 28, 14, 6, '#3f9e57'); }
    else if (id === 'skates') { shoe('#c28bff', -14, '#7a58a8'); for (const dx of [-14, 14]) { circ(dx + toe * 0.5, 20, 6, '#2b2833'); circ(dx + toe * 0.5, 20, 2.5, '#ffd23f'); } }
    else if (id === 'lightup') { shoe('#f5f1e8', -12, rb(t, 0, 55)); for (let i = 0; i < 4; i++) circ(-15 + i * 10 + toe * 0.4, 13, 3, rb(t, i + 1, 65)); }
    else if (id === 'dressshoes') { shoe('#1f1b26', -10, '#3a3346'); rr(-10 + toe * 0.6, -6, 10, 3, 1.5, '#8a8496'); }
    else if (id === 'rockboots') { rr(-18 + toe * 0.2, -34, 32, 34, 6, '#24202e'); shoe('#24202e', -12, '#5d566a'); for (const y of [-26, -14]) rr(-19 + toe * 0.2, y, 34, 4, 2, '#c9d2e0'); }
    else if (id === 'moonboots') { rr(-22 + toe * 0.2, -30, 42, 32, 10, '#eef1f6'); shoe('#eef1f6', -12, '#9aa1b0'); g.fillStyle = '#9aa1b0'; for (const y of [-22, -12]) g.fillRect(-20 + toe * 0.2, y, 38, 3); }
    else if (id === 'clownshoes') { g.fillStyle = '#e0303a'; g.beginPath(); g.ellipse(toe * 0.9, 4, 34 + toe, 14, 0, 0, P * 2); g.fill(); circ(toe * 0.9 + 10, -2, 4, '#ff9090'); rr(-12, -10, 24, 8, 4, '#ffd23f'); }
    else if (id === 'slippers') { shoe('#bfe8ff', -8, '#8fd3ff'); K.star(-4 + toe * 0.5, -2, 6, '#ffffff'); }
    else if (id === 'heels') { rr(-20 + toe * 0.3, -6, 40 + toe, 14, 7, '#e0303a'); rr(side ? -22 : -20, 4, 7, 12, 2, '#a01e28'); if (!side) rr(13, 4, 7, 12, 2, '#a01e28'); rr(-4 + toe * 0.6, 8, 24, 4, 2, '#a01e28'); }
    else if (id === 'flats') { shoe('#ffb3d9', -6, '#ff8fc7'); circ(6 + toe * 0.6, -3, 4, '#ff5aa8'); circ(-0 + toe * 0.6, -3, 4, '#ff5aa8'); }
    else if (id === 'sandals') { rr(-24 + toe * 0.3, 10, 48 + toe, 6, 3, '#b07a44'); rr(-18 + toe * 0.3, -4, 36 + toe * 0.5, 5, 2.5, '#8a5a30'); rr(-14 + toe * 0.4, 3, 30 + toe * 0.5, 5, 2.5, '#8a5a30'); }
    else if (id === 'hightops') { rr(-18 + toe * 0.2, -30, 32, 30, 6, '#7a58a8'); shoe('#7a58a8'); circ(-2 + toe * 0.2, -18, 6, '#ffffff'); circ(-2 + toe * 0.2, -18, 3.5, '#e0564f'); g.fillStyle = '#ffffff'; for (const y of [-26, -8]) g.fillRect(4 + toe * 0.3, y, 8, 2); }
    else if (id === 'cowboots') { rr(-18 + toe * 0.2, -34, 32, 34, 6, '#a0622c'); shoe('#a0622c', -12, '#5d3a1c'); g.fillStyle = '#e8c890'; g.fillRect(-12 + toe * 0.2, -28, 2, 18); g.fillRect(6 + toe * 0.2, -28, 2, 18); K.star(-22, 2, 5, '#ffd23f'); }
    g.restore();
  };
  // what the hero wears on the head, face and neck; a: the view's head anchors
  function wear(p, a) {
    if (K.onWear) K.onWear('head', a);   // the N64 sprite baker asks where each frame's parts are
    const acc = p.acc; if (!acc) return;
    if (acc.neck) K.wearNeck(acc.neck, a.neck[0], a.neck[1], a.neck[2], a.side ? 1 : 0);
    if (acc.face) K.wearFace(acc.face, a.eyes, a.side);
    if (acc.head) K.wearHead(acc.head, a.cx, a.top, a.w, p.t ?? 0, a.side);
  }
  function wearBody(p, b) { if (K.onWear) K.onWear('body', b); if (p.acc && p.acc.body) K.wearBody(p.acc.body, b, p.t ?? 0, b.side); }
  // sleeves: outfits with arms cover the robot's upper arms too (the ostrich has wings, not arms)
  K.SLEEVE = { sweater: '#e0564f', jersey: '#49d17a', tux: '#2b2833', super: '#3a6fc0', disco: 'rainbow',
    chefcoat: '#f5f5f0', labcoat: '#f2f4f8', police: '#2b4a9a', leather: '#1f1b26', firecoat: '#d4a64a', spacesuit: '#eef1f6', vest: '#6a7a8c',
    pirateshirt: '#f5f1e8', ninjasuit: '#24202e', clownsuit: '#ffd23f', armor: '#9aa3b3', robe: '#5b3f86', cowvest: '#c0392b',
    bluesuit: '#2b4a9a', pinstripe: '#4a4a58', whitesuit: '#f5f1e8', plaidsuit: '#3a7a4a', velvetsuit: '#7a2a8a', goldsuit: '#e8b83a' };
  // sleeves with a white cuff at the wrist
  K.CUFF = { sweater: '#ffffff', tux: '#f5f1e8', bluesuit: '#f5f1e8', pinstripe: '#f5f1e8', plaidsuit: '#f5f1e8', velvetsuit: '#f5f1e8', goldsuit: '#1b1426', whitesuit: '#ff8fc7', firecoat: '#ffd23f' };
  K.wearSleeve = (id, x, y, deg, ln, w, t = 0, far = false) => {
    const c0 = K.SLEEVE[id]; if (!c0) return;
    const g = K.g, a = deg * P / 180, ex = -Math.sin(a), ey = Math.cos(a), l = ln * 0.72;
    let c = c0 === 'rainbow' ? rb(t * 2, 3, 58) : c0;
    if (far && c0 !== 'rainbow') c = K.shade(c, 0.75);
    g.strokeStyle = c; g.lineWidth = w * 1.3; g.lineCap = 'round';
    g.beginPath(); g.moveTo(x, y); g.lineTo(x + ex * l, y + ey * l); g.stroke();
    if (K.CUFF[id]) { g.strokeStyle = K.CUFF[id]; g.lineWidth = w * 1.3; g.lineCap = 'butt'; g.beginPath(); g.moveTo(x + ex * l * 0.86, y + ey * l * 0.86); g.lineTo(x + ex * l * 1.02, y + ey * l * 1.02); g.stroke(); }
  };
  function wearArm(p, x, y, deg, ln, w, far) {
    if (K.onWear) K.onWear('arm', { x, y, deg, ln, w, far });
    if (p.acc && p.acc.body) K.wearSleeve(p.acc.body, x, y, deg, ln, w, p.t ?? 0, far);
  }
  function wearFoot(p, x, y, s, side, far) { if (K.onWear) K.onWear('foot', { x, y, s, side, far }); if (p.acc && p.acc.feet) K.wearFoot(p.acc.feet, x, y, s, p.t ?? 0, side, far); }

  // Poses in one place so the mocks and the game agree. kind: idle|run|jump|special|wobble
  K.pose = (hero, kind, t) => {
    const sw = Math.sin(t * 9);
    if (hero === 'robot') {
      if (kind === 'run') {
        // a real cycle: the leg swinging forward lifts its foot (knee bends back), the body
        // dips when the legs are spread and rides high as they pass
        const f = t * 11, a = Math.sin(f), c = Math.cos(f);
        return { t, legF: -42 * a, kneeF: 55 * Math.max(0, c), legB: 42 * a, kneeB: 55 * Math.max(0, -c),
          legLen: 62, armF: 45 * a, armB: -45 * a, bob: 7 * Math.abs(a) - 3, rot: 0.1, tilt: 0 };
      }
      if (kind === 'jump') return { t, legF: -70, kneeF: 95, legB: -20, kneeB: 70, legLen: 58, armF: -155, armB: 70, mouth: 'o', eyes: { lookY: -0.5, happy: 0.2 } };
      if (kind === 'jet') return { t, legF: -10, legB: 10, legLen: 44, armF: 40, armB: 60, jet: true, eyes: { lookY: -0.3 } };
      if (kind === 'wobble') return { t, rot: Math.sin(t * 12) * 0.12, stars: true, mouth: 'wavy', eyes: { open: 0.35, happy: 0, look: 0 } };
      return { t };
    }
    if (kind === 'run') {
      const f = t * 10, a = Math.sin(f), c = Math.cos(f);
      return { t, legF: -46 * a, kneeF: 110 * Math.max(0, c), toeF: -46 * a, legB: 46 * a, kneeB: 110 * Math.max(0, -c), toeB: 46 * a,
        legLenF: 100, legLenB: 100, bob: 12 * Math.abs(a) - 4, rot: 0.1, lean: 16 + 6 * Math.abs(a), flick: -14 + 10 * Math.abs(a), crest: -14 };
    }
    if (kind === 'jump') return { t, legF: -75, kneeF: 140, toeF: 20, legB: -45, kneeB: 130, toeB: 40, legLenF: 96, legLenB: 96, lean: 22, flap: 0.35, flick: 24, crest: -20, beakOpen: 0.8, eyes: { lookY: -0.3, happy: 0.3 } };
    if (kind === 'flap') { const w = Math.sin(t * 13); return { t, wings: 0.38 + 0.66 * w, bob: -w * 6, legF: 8, kneeF: 30, legB: 22, kneeB: 30, toeF: 20, toeB: 30, rot: -0.04, flick: 20 + w * 8, crest: -10 - w * 8, eyes: { happy: 0.6 } }; }
    if (kind === 'wobble') return { t, rot: Math.sin(t * 12) * 0.12, stars: true, eyes: { open: 0.35, happy: 0, look: 0 } };
    return { t };
  };

  return K;
})();
