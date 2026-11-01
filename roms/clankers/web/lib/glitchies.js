// The Glitchies (from the approved characters sheet). GLITCH[i] = { name, hex, draw(g, x, feetY, s, t, glitchy) }
const GLITCH = (() => {
  const K = CK, P = Math.PI, FONT = '"Titan One", "Arial Black", Impact, sans-serif';
  function faceEyes(cx, cy, w, h, dark, face, t, glitchy, phase) {
    const o = glitchy ? { angry: 1, happy: 0, open: K.blink(t, phase) } : { happy: 0.5, open: K.blink(t, phase) };
    K.eye(cx - w * 0.62, cy, w, h, dark, face, Object.assign({}, o, { angry: glitchy ? 1 : 0 }));
    K.eye(cx + w * 0.62, cy, w, h, dark, face, Object.assign({}, o, { angry: glitchy ? -1 : 0 }));
  }
  function fizz(g, x, y, r, t) {
    g.strokeStyle = '#bff6ff'; g.lineWidth = 1;
    for (let i = 0; i < 3; i++) {
      if (((t * 7 + i * 1.7) % 2) > 0.6) continue;
      const a = i * 2.1 + Math.floor(t * 7) * 0.9, px = x + Math.cos(a) * r, py = y + Math.sin(a) * r;
      g.beginPath(); g.moveTo(px - 3, py - 2); g.lineTo(px, py + 1); g.lineTo(px - 1, py - 1); g.lineTo(px + 3, py + 2); g.stroke();
    }
  }
  function begin(g, x, feetY, s, t, glitchy) {
    g.save(); g.translate(x + (glitchy && Math.floor(t * 12) % 9 === 0 ? 1.5 : 0), feetY); g.scale(s, s);
  }
  const glitchies = [
    { name: 'Bumblebot', hex: 0xFF7A59, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      const roll = t * 5;
      K.circ(0, -28, 28, '#2b2833'); K.circ(0, -28, 12, '#5b5666');
      g.strokeStyle = '#5b5666'; g.lineWidth = 4; g.beginPath(); g.moveTo(Math.cos(roll) * 22, -28 + Math.sin(roll) * 22); g.lineTo(-Math.cos(roll) * 22, -28 - Math.sin(roll) * 22); g.stroke();
      K.circ(0, -100, 62, c); K.circ(0, -100, 62 * 0.8, K.shade(h, 0.92));
      g.fillStyle = c; g.fillRect(-4, -190, 8, 30); K.circ(0, -194, 11, gl ? '#ff3b3b' : c);
      faceEyes(0, -112, 36, 42, d, K.shade(h, 0.92), t, gl, 0.2);
      K.mouth(gl ? 'zig' : 'smile', 0, -92, 20, 7, d);
      g.restore(); if (gl) fizz(g, x, y - 100 * s, 64 * s, t);
    } },
    { name: 'Boing Sprocket', hex: 0xC28BFF, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      const comp = 0.6 + 0.4 * Math.abs(Math.sin(t * 4)), top = -70 * comp;
      g.strokeStyle = '#8e8a99'; g.lineWidth = 8; g.lineJoin = 'round'; g.beginPath();
      for (let i = 0; i <= 6; i++) g.lineTo(i % 2 ? 22 : -22, top * i / 6);
      g.stroke();
      K.gear(0, top - 58, 64, 10, t * (gl ? 3 : 0.5), c, null); K.circ(0, top - 58, 44, K.shade(h, 0.92));
      faceEyes(0, top - 66, 28, 34, d, K.shade(h, 0.92), t, gl, 0.9);
      K.mouth(gl ? 'zig' : 'smile', 0, top - 50, 16, 6, d);
      g.restore(); if (gl) fizz(g, x, y + (top - 58) * s, 66 * s, t);
    } },
    { name: 'Grumble Crate', hex: 0xC08850, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.3); begin(g, x, y, s, t, gl);
      const step = Math.sin(t * 6);
      K.limb(-30, -24, 10 * step, 16, 14, d); K.limb(30, -24, -10 * step, 16, 14, d);
      K.rr(-62, -140, 124, 118, 10, c);
      g.strokeStyle = K.shade(h, 0.7); g.lineWidth = 6; g.strokeRect(-54, -132, 108, 102);
      g.fillStyle = K.shade(h, 0.7); g.fillRect(-54, -70, 108, 5);
      faceEyes(0, -104, 30, 34, '#2a1d12', c, t, gl, 1.4);
      K.mouth(gl ? 'frown' : 'smile', 0, -84, 18, 6, '#2a1d12');
      g.restore(); if (gl) fizz(g, x, y - 80 * s, 74 * s, t);
    } },
    { name: 'Drip Drone', hex: 0x49F08A, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      const hov = -60 - Math.sin(t * 2) * 22;
      g.fillStyle = '#9a95a8'; g.fillRect(-4, hov - 110, 8, 20);
      const bl = Math.cos(t * 30) * 70; K.rr(-Math.abs(bl), hov - 116, Math.abs(bl) * 2, 8, 4, '#cfcbd8');
      K.rr(-60, hov - 92, 120, 84, 36, c);
      faceEyes(0, hov - 56, 28, 32, d, c, t, gl, 2.1);
      K.mouth(gl ? 'zig' : 'smile', 0, hov - 40, 14, 6, d);
      if (gl) { const dp = (t * 1.2) % 1; g.fillStyle = '#4a3d6b'; g.beginPath(); g.ellipse(0, hov + dp * 70, 6, 9, 0, 0, P * 2); g.fill(); }
      g.restore(); if (gl) fizz(g, x, y + (hov - 50) * s, 64 * s, t);
    } },
    { name: 'Sweepy', hex: 0x8FD3FF, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      K.circ(-24, -14, 14, '#2b2833'); K.circ(24, -14, 14, '#2b2833');
      const swp = Math.sin(t * 5) * 30;
      const [bx, by] = K.limb(40, -90, -30 + swp, 70, 16, K.shade(h, 0.78));
      g.strokeStyle = '#a0703c'; g.lineWidth = 8; g.beginPath(); g.moveTo(bx, by); g.lineTo(bx + 12, 0 - 10); g.stroke();
      K.rr(bx - 10, -14, 44, 14, 4, '#e0b04a');
      K.rr(-42, -170, 84, 150, 34, c);
      faceEyes(0, -130, 24, 30, d, c, t, gl, 2.8);
      K.mouth(gl ? 'frown' : 'smile', 0, -114, 14, 6, d);
      g.restore(); if (gl) fizz(g, x, y - 100 * s, 60 * s, t);
    } },
    { name: 'Spark Plug', hex: 0xFFE14D, draw(g, x, y, s, t, gl) {
      const h = this.hex, d = '#3a3346'; begin(g, x, y, s, t, gl);
      const run = Math.sin(t * 8) * 4;
      g.fillStyle = '#9a95a8'; g.fillRect(-6, -30, 12, 30);
      K.gear(0, -44, 30, 6, 0, '#b9b4c4', null);
      K.rr(-30, -150, 60, 96, 26, '#f5f1e8');
      faceEyes(0, -116, 20, 24, d, '#f5f1e8', t, gl, 3.3);
      K.mouth(gl ? 'zig' : 'smile', 0, -100, 12, 5, d);
      g.strokeStyle = K.shade(h, 1); g.lineWidth = 6; g.lineJoin = 'round';
      for (let i = 0; i < 4; i++) {
        if (((t * 9 + i) % 2) > 1.2) continue;
        const a = i * P / 2 + t * 2; g.beginPath();
        g.moveTo(Math.cos(a) * 46, -100 + Math.sin(a) * 60); g.lineTo(Math.cos(a) * 62 + run, -100 + Math.sin(a) * 74 - 8); g.lineTo(Math.cos(a) * 74, -100 + Math.sin(a) * 86); g.stroke();
      }
      g.restore();
    } },
    { name: 'Magnet Mite', hex: 0x7FA7C9, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      const step = Math.sin(t * 10);
      for (const i of [-1, 0, 1]) { K.limb(-30 + i * 24, -24, 30 + step * 10 * (i % 2 ? 1 : -1), 22, 8, d); K.limb(30 + i * 24, -24, -30 - step * 10 * (i % 2 ? 1 : -1), 22, 8, d); }
      g.fillStyle = c; g.beginPath(); g.ellipse(0, -40, 70, 38, 0, P, 0); g.fill(); g.fillRect(-70, -40, 140, 14);
      K.arc(0, -98, 28, P, 0, 18, '#ff5252'); g.fillStyle = '#dcd8e6'; g.fillRect(-37, -100, 18, 16); g.fillRect(19, -100, 18, 16);
      faceEyes(0, -48, 22, 26, d, c, t, gl, 0.6);
      if (gl) { g.strokeStyle = 'rgba(255,82,82,0.6)'; g.lineWidth = 3; for (let i = 0; i < 2; i++) K.arc(0, -98, 46 + i * 14 + (t * 30) % 14, P * 1.2, P * 1.8, 3, 'rgba(255,82,82,0.5)'); }
      g.restore();
    } },
    { name: 'Rolly Rock', hex: 0x9c8f86, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s, t, gl);
      g.save(); g.translate(0, -62); g.rotate(gl ? t * 3 : 0);
      g.fillStyle = c; g.beginPath();
      for (let i = 0; i < 9; i++) { const a = i / 9 * P * 2, r = 62 + (i % 3 ? 6 : -6); g.lineTo(Math.cos(a) * r, Math.sin(a) * r); }
      g.closePath(); g.fill();
      K.circ(-22, 24, 9, K.shade(h, 0.8)); K.circ(26, -28, 7, K.shade(h, 0.8));
      g.restore();
      faceEyes(0, -70, 26, 30, d, c, t, gl, 1.1);
      K.mouth(gl ? 'frown' : 'smile', 0, -54, 14, 6, d);
      g.restore();
    } },
    { name: 'Swoop Bat', hex: 0x9b6bff, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22), w = K.shade(h, 0.7); begin(g, x, y, s, t, gl);
      const sy = -110 + Math.sin(t * 2) * 26, fl = Math.sin(t * 14) * 0.5;
      for (const side of [-1, 1]) {
        g.save(); g.translate(side * 36, sy); g.rotate(side * fl); g.fillStyle = w; g.beginPath();
        g.moveTo(0, -14); g.lineTo(side * 90, -30); g.lineTo(side * 74, 4); g.lineTo(side * 52, -4); g.lineTo(side * 34, 14); g.lineTo(0, 14); g.closePath(); g.fill(); g.restore();
      }
      g.fillStyle = c; g.beginPath(); g.moveTo(-34, sy - 30); g.lineTo(-24, sy - 64); g.lineTo(-8, sy - 36); g.moveTo(34, sy - 30); g.lineTo(24, sy - 64); g.lineTo(8, sy - 36); g.fill();
      K.circ(0, sy, 46, c);
      faceEyes(0, sy - 8, 24, 28, d, c, t, gl, 2.5);
      K.mouth(gl ? 'zig' : 'smile', 0, sy + 8, 14, 5, d);
      g.restore(); if (gl) fizz(g, x, y + sy * s, 50 * s, t);
    } },
    { name: 'Moon Blob', hex: 0x3BF0FF, draw(g, x, y, s, t, gl) {
      const h = this.hex, d = K.shade(h, 0.2); begin(g, x, y, s, t, gl);
      const b = Math.abs(Math.sin(t * 2.2)), sq = 1 + (1 - b) * 0.25;
      g.translate(0, -b * 40); g.scale(sq, 1 / sq);
      g.globalAlpha = 0.85; g.fillStyle = K.shade(h, 0.9); g.beginPath(); g.ellipse(0, -50, 66, 56, 0, P, 0); g.lineTo(66, -6); g.quadraticCurveTo(0, 6, -66, -6); g.closePath(); g.fill(); g.globalAlpha = 1;
      K.circ(-30, -82, 10, 'rgba(255,255,255,0.7)');
      faceEyes(0, -52, 24, 28, d, K.shade(h, 0.9), t, gl, 1.9);
      K.mouth(gl ? 'zig' : 'smile', 0, -36, 14, 5, d);
      g.restore();
    } },
    { name: 'Snooze Bot', hex: 0xF5A3C7, draw(g, x, y, s, t, gl) {
      const h = this.hex, c = K.shade(h, 1), d = K.shade(h, 0.22); begin(g, x, y, s * 0.85, t, gl);
      const br = Math.sin(t * 1.6) * 4;
      K.circ(-110, -16, 16, '#2b2833'); K.circ(110, -16, 16, '#2b2833');
      K.rr(-150, -96 - br, 300, 84 + br, 36, c);
      K.rr(-60, -82 - br, 120, 18, 8, K.shade(h, 0.8));
      K.rr(-150 + 6, -150 - br, 110, 64, 26, c);
      K.arc(-112, -120 - br, 12, 0.1 * P, 0.9 * P, 5, d); K.arc(-76, -120 - br, 12, 0.1 * P, 0.9 * P, 5, d);
      g.font = `${36 + br * 2}px ${FONT}`; g.fillStyle = '#d9d2e6'; g.textAlign = 'left';
      const z = (t * 0.8) % 1; g.globalAlpha = 1 - z; g.fillText('z', -40 + z * 30, -160 - z * 50); g.globalAlpha = 1;
      g.restore();
    } },
  ];
  return glitchies;
})();
