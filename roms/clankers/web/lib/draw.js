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
    if (p.jet) { K.flame(lf[0], lf[1] + 16, 60, 26, t); K.flame(rf[0], rf[1] + 16, 60, 26, t + 0.3); }
    g.translate(0, bob);
    const armL = p.armL ?? 22, armR = p.armR ?? -22;
    if (Math.abs(armL) <= 95) limb(-66, 6, armL, 74, 28, lim);
    let clawEnd = null;
    if (Math.abs(armR) <= 95 && !p.grab) limb(66, 6, armR, 74, 28, lim);
    rr(-72, -26, 144, 132, 40, torso);
    rr(-26, 10, 52, 40, 12, plate);
    g.fillStyle = col; g.fillRect(tilt - 4, HY - HH - 30, 8, 30); circ(tilt, HY - HH - 36, 11, col);
    rr(tilt - HW, HY - HH, HW * 2, HH * 2, 40, col);
    const ex = HW * 0.43, ey = HY - HH * 0.17, eo = Object.assign({ open: K.blink(t, p.phase ?? 0.3), happy: 0.45 }, p.eyes || {});
    eye(tilt - ex, ey, 52, 60, dark, col, Object.assign({}, eo, { angry: eo.angry ? 1 : 0 }));
    eye(tilt + ex, ey, 52, 60, dark, col, Object.assign({}, eo, { angry: eo.angry ? -1 : 0 }));
    K.mouth(p.mouth || 'smile', tilt, HY + HH * 0.52 - 16, 30, 9, dark);
    if (Math.abs(armL) > 95) limb(-66, 6, armL, 74, 28, lim);
    if (Math.abs(armR) > 95 && !p.grab) limb(66, 6, armR, 74, 28, lim);
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
    }
    g.translate(0, bob);
    rr(ox - 72, 222, 144, 112, 54, body);
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
    if (p.jet) K.flame(bf[0], bf[1] + 16, 60, 26, t + 0.3);
    g.translate(0, bob);
    limb(-8, 6, p.armB ?? 10, 74, 28, far);
    rr(-54, -26, 108, 132, 36, torso);
    rr(30, 10, 20, 40, 8, plate);
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
    if (p.jet) K.flame(ff[0], ff[1] + 16, 60, 26, t);
    g.translate(0, bob);
    if (!p.grab) limb(8, 6, p.armF ?? -10, 74, 28, lim);
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
    const legDraw = (hx, deg, ln, c, knee, toe) => {
      const [[ex, ey], fd] = legK(hx, 318 + bob, deg, ln - bob, 20, c, knee);
      const td0 = toe ?? fd;
      for (const [td, tl] of [[-100, 24], [-70, 20], [55, 14]]) limb(ex, ey, td + td0, tl, 11, c);
    };
    legDraw(-6, p.legB ?? 0, p.legLenB ?? 92, far, p.kneeB, p.toeB);
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
  // acc = { head, face, neck }, each an id from K.ACC or empty. Every hero view passes where its head,
  // eyes and neck are, in its own units, and these draw on top: w = head width, top = head top.
  K.ACC = [
    { id: 'party', slot: 'head', name: 'Party hat', cost: 15 },
    { id: 'flower', slot: 'head', name: 'Flower', cost: 15 },
    { id: 'propeller', slot: 'head', name: 'Propeller cap', cost: 30 },
    { id: 'tophat', slot: 'head', name: 'Top hat', cost: 40 },
    { id: 'crown', slot: 'head', name: 'Crown', cost: 60 },
    { id: 'shades', slot: 'face', name: 'Cool shades', cost: 25 },
    { id: 'hearts', slot: 'face', name: 'Heart glasses', cost: 25 },
    { id: 'bowtie', slot: 'neck', name: 'Bow tie', cost: 15 },
    { id: 'scarf', slot: 'neck', name: 'Scarf', cost: 20 },
  ];
  function heart(x, y, r, c) {
    const g = K.g; g.fillStyle = c; g.beginPath();
    g.moveTo(x, y + r * 0.95);
    g.bezierCurveTo(x - r * 1.5, y - r * 0.1, x - r * 0.8, y - r * 1.2, x, y - r * 0.45);
    g.bezierCurveTo(x + r * 0.8, y - r * 1.2, x + r * 1.5, y - r * 0.1, x, y + r * 0.95);
    g.fill();
  }
  K.wearHead = (id, cx, top, w, t = 0, side = false) => {
    const g = K.g, k = w / 100;
    g.save(); g.translate(cx, top); g.scale(k, k);
    if (id === 'party') {
      g.fillStyle = '#ff8fc7'; g.beginPath(); g.moveTo(-24, 6); g.lineTo(0, -62); g.lineTo(24, 6); g.closePath(); g.fill();
      g.save(); g.clip(); g.fillStyle = '#ffd23f'; for (const y of [-40, -16]) { g.beginPath(); g.moveTo(-40, y); g.lineTo(40, y - 14); g.lineTo(40, y - 4); g.lineTo(-40, y + 10); g.fill(); } g.restore();
      circ(0, -64, 10, '#5ad1ff'); circ(-3, -67, 3.5, '#ffffff');
    } else if (id === 'flower') {
      const fx = side ? -26 : 30, fy = -2;
      limb(fx, fy, 160, 14, 5, '#3f9e57');
      for (let i = 0; i < 6; i++) { const a = i / 6 * P * 2 + t * 0.4; circ(fx + Math.cos(a) * 12, fy - 16 + Math.sin(a) * 12, 9, '#ff7a59'); }
      circ(fx, fy - 16, 8, '#ffd23f');
    } else if (id === 'propeller') {
      rr(-38, -6, 76, 14, 7, '#e0564f');                 // brim band
      g.fillStyle = '#5ad1ff'; g.beginPath(); g.ellipse(0, -4, 34, 30, 0, P, 0); g.fill();
      g.fillStyle = '#ffd23f'; g.beginPath(); g.ellipse(0, -4, 34, 30, 0, P * 1.38, P * 1.62); g.lineTo(0, -4); g.fill();
      g.fillStyle = '#3a3346'; g.fillRect(-2.5, -50, 5, 18);
      const sp = Math.cos(t * 18); rr(-34 * Math.abs(sp), -54, 68 * Math.abs(sp) + 1, 8, 4, '#e0564f'); circ(0, -50, 5, '#ffd23f');
    } else if (id === 'tophat') {
      rr(-46, -4, 92, 10, 5, '#1f1b26');
      rr(-28, -56, 56, 56, 5, '#2b2833');
      g.fillStyle = '#e0564f'; g.fillRect(-28, -18, 56, 10);
      g.fillStyle = 'rgba(255,255,255,0.18)'; g.fillRect(-22, -52, 6, 32);
    } else if (id === 'crown') {
      g.fillStyle = '#ffcf3a'; g.beginPath(); g.moveTo(-36, 6); g.lineTo(-36, -30); g.lineTo(-18, -12); g.lineTo(0, -38); g.lineTo(18, -12); g.lineTo(36, -30); g.lineTo(36, 6); g.closePath(); g.fill();
      g.fillStyle = '#e8a91f'; g.fillRect(-36, -4, 72, 10);
      circ(-36, -32, 5, '#fff6a0'); circ(0, -40, 6, '#fff6a0'); circ(36, -32, 5, '#fff6a0');
      circ(-18, 1, 4.5, '#e0564f'); circ(0, 1, 4.5, '#5ad1ff'); circ(18, 1, 4.5, '#49f08a');
    }
    g.restore();
  };
  // eyes: [[x, y, rx, ry], ...] near eye first; side views pass the squeezed far eye second
  K.wearFace = (id, eyes, side = false) => {
    const g = K.g;
    if (id === 'shades') {
      const [a, b] = eyes, lw = Math.max(3, a[2] * 0.22);
      g.strokeStyle = '#1b1426'; g.lineWidth = lw; g.lineCap = 'round';
      g.beginPath(); g.moveTo(a[0] + (b[0] > a[0] ? a[2] : -a[2]), a[1] - a[3] * 0.3); g.lineTo(b[0] - (b[0] > a[0] ? b[2] : -b[2]), b[1] - b[3] * 0.3); g.stroke();
      if (side) { g.beginPath(); g.moveTo(a[0] - a[2], a[1] - a[3] * 0.3); g.lineTo(a[0] - a[2] * 2.6, a[1] - a[3] * 0.5); g.stroke(); }
      for (const [x, y, rx, ry] of eyes) {
        rr(x - rx - 4, y - ry * 0.62, rx * 2 + 8, ry * 1.2, ry * 0.45, '#1b1426');
        g.fillStyle = 'rgba(255,255,255,0.35)'; g.beginPath(); g.moveTo(x - rx * 0.6, y - ry * 0.4); g.lineTo(x - rx * 0.1, y - ry * 0.4); g.lineTo(x - rx * 0.5, y + ry * 0.3); g.closePath(); g.fill();
      }
    } else if (id === 'hearts') {
      const [a, b] = eyes;
      g.strokeStyle = '#c23b78'; g.lineWidth = Math.max(3, a[2] * 0.18); g.lineCap = 'round';
      g.beginPath(); g.moveTo(a[0], a[1] - a[3] * 0.5); g.lineTo(b[0], b[1] - b[3] * 0.5); g.stroke();
      for (const [x, y, rx, ry] of eyes) { g.save(); g.translate(x, y); g.scale(rx / ry, 1); heart(0, 0, ry * 0.95, '#c23b78'); heart(0, -ry * 0.04, ry * 0.75, '#ff8fc7'); g.restore(); }
    }
  };
  K.wearNeck = (id, x, y, w, dir = 0) => {
    const g = K.g, k = w / 100;
    g.save(); g.translate(x, y); g.scale(k * (dir ? 0.7 : 1), k);
    if (id === 'bowtie') {
      g.fillStyle = '#e0564f';
      g.beginPath(); g.moveTo(0, 0); g.lineTo(-30, -16); g.lineTo(-30, 16); g.closePath(); g.fill();
      g.beginPath(); g.moveTo(0, 0); g.lineTo(30, -16); g.lineTo(30, 16); g.closePath(); g.fill();
      circ(0, 0, 8, '#a8323b');
    } else if (id === 'scarf') {
      rr(-46, -10, 92, 22, 11, '#49d17a');
      g.fillStyle = '#ffffff'; for (const sx of [-26, 0, 26]) g.fillRect(sx - 4, -10, 8, 22);
      g.save(); g.translate(dir ? -30 : 24, 6); g.rotate(dir ? 0.5 : 0.2); rr(-10, 0, 20, 40, 6, '#49d17a'); g.fillStyle = '#ffffff'; g.fillRect(-10, 14, 20, 7); g.restore();
    }
    g.restore();
  };
  // draws whatever the hero wears; a: head/face/neck anchors in the view's own units
  function wear(p, a) {
    const acc = p.acc; if (!acc) return;
    if (acc.neck) K.wearNeck(acc.neck, a.neck[0], a.neck[1], a.neck[2], a.side ? 1 : 0);
    if (acc.face) K.wearFace(acc.face, a.eyes, a.side);
    if (acc.head) K.wearHead(acc.head, a.cx, a.top, a.w, p.t ?? 0, a.side);
  }

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
