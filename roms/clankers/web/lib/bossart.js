// The Big Glitch bosses of Worlds 2 to 4, drawn by the browser game and baked for the N64 by
// tools/bake_sprites.mjs. Each part is drawn around (0, 0); set CK.g first.
//   face: 0 glitchy and cross, 1 ouch (just got its button pressed), 2 fixed and happy, 3 dizzy
const BOSSART = (() => {
  const K = CK, P = Math.PI;
  const RED = '#e04f4f', GREEN = '#49d17a', DARK = '#2b2230';
  function xEyes(g, x, y, s) {
    g.strokeStyle = '#ffffff'; g.lineWidth = 1.6 * s; g.lineCap = 'round';
    for (const ex of [-1, 1]) { const cx = x + ex * 6 * s; g.beginPath(); g.moveTo(cx - 2.5 * s, y - 2.5 * s); g.lineTo(cx + 2.5 * s, y + 2.5 * s); g.moveTo(cx + 2.5 * s, y - 2.5 * s); g.lineTo(cx - 2.5 * s, y + 2.5 * s); g.stroke(); }
  }
  function eyes(g, x, y, s, col, face) {
    if (face === 1) { xEyes(g, x, y, s); return; }
    if (face === 3) { g.strokeStyle = '#ffffff'; g.lineWidth = 1.2 * s; for (const ex of [-1, 1]) { g.beginPath(); g.arc(x + ex * 6 * s, y, 2.8 * s, 0, P * 1.6); g.stroke(); } return; }
    K.eye(x - 6 * s, y, 8 * s, 10 * s, DARK, col, { angry: face === 2 ? 0 : 1, happy: face === 2 ? 0.5 : 0 });
    K.eye(x + 6 * s, y, 8 * s, 10 * s, DARK, col, { angry: face === 2 ? 0 : -1, happy: face === 2 ? 0.5 : 0 });
  }
  // The Stamper (World 2): a press head, 48 wide and 30 tall, hanging from (0, 0) at its top middle.
  function stamper(g, face) {
    const body = face === 2 ? GREEN : RED;
    K.rr(-10, 0, 20, 6, 2, '#5d6378');
    K.rr(-24, 4, 48, 26, 4, body); K.rr(-22, 6, 44, 4, 2, 'rgba(255,255,255,0.3)');
    eyes(g, 0, 15, 1, body, face);
    K.mouth(face === 2 ? 'smile' : face === 1 ? 'o' : 'zig', 0, 20, 5, 1.5, DARK);
    // the hammer face along the bottom
    g.save(); g.beginPath(); g.rect(-24, 25, 48, 5); g.clip();
    for (let k = -4; k < 8; k++) { g.fillStyle = k % 2 ? DARK : '#ffcf3a'; g.beginPath(); g.moveTo(-24 + k * 8, 30); g.lineTo(-24 + k * 8 + 5, 25); g.lineTo(-24 + k * 8 + 13, 25); g.lineTo(-24 + k * 8 + 8, 30); g.fill(); }
    g.restore();
  }
  // Rumbleboulder (World 3): a round rock bot. The rock turns as it rolls; its face stays upright.
  function boulderRock(g) {
    g.fillStyle = '#7a6e86'; g.beginPath();
    for (let i = 0; i < 14; i++) { const a = i / 14 * P * 2, r = 18 + (i % 3 ? 1 : -1.2); g.lineTo(Math.cos(a) * r, Math.sin(a) * r); }
    g.closePath(); g.fill();
    for (const [x, y, r] of [[-9, 9, 3.2], [8, -10, 2.6], [11, 7, 2], [-12, -6, 2.2], [2, 13, 1.8]]) K.circ(x, y, r, '#695d75');
    g.fillStyle = '#3bf0ff'; g.beginPath(); g.moveTo(-3, -17); g.lineTo(-1, -22); g.lineTo(1, -17); g.fill();
    K.circ(-6, -7, 6, 'rgba(255,255,255,0.12)');
  }
  function boulderFace(g, face) {
    const col = '#7a6e86';
    if (face === 1) xEyes(g, 0, -2, 1.1);
    else if (face === 3) { g.strokeStyle = '#ffffff'; g.lineWidth = 1.4; for (const ex of [-1, 1]) { g.beginPath(); g.arc(ex * 6.5, -2, 3, 0, P * 1.6); g.stroke(); } }
    else { K.circ(-6.5, -2, 4, '#ffffff'); K.circ(6.5, -2, 4, '#ffffff'); K.circ(-6, -1.5, 1.8, DARK); K.circ(6, -1.5, 1.8, DARK);
      if (face === 0) { g.fillStyle = col; g.beginPath(); g.moveTo(-11, -8); g.lineTo(-2, -8); g.lineTo(-2, -4); g.fill(); g.beginPath(); g.moveTo(11, -8); g.lineTo(2, -8); g.lineTo(2, -4); g.fill(); } }
    K.arc(0, 4, 4.5, face === 2 ? P * 0.15 : P * 1.15, face === 2 ? P * 0.85 : P * 1.85, 1.6, DARK);
    if (face === 2) { K.circ(-11, 4, 2, 'rgba(255,143,199,0.6)'); K.circ(11, 4, 2, 'rgba(255,143,199,0.6)'); }
  }
  // The Sky Hauler (World 4): a flying freighter, 72 wide, centred on (0, 0).
  function hauler(g, face) {
    const cab = face === 2 ? GREEN : RED;
    K.rr(-36, -6, 72, 16, 8, '#7d8397'); K.rr(-34, -5, 68, 4, 2, 'rgba(255,255,255,0.25)');
    K.rr(-22, -16, 44, 12, 6, '#9aa1b0');
    for (let i = 0; i < 5; i++) K.circ(-24 + i * 12, 3, 2.2, face === 2 ? '#ffd37a' : (i % 2 ? '#ffd37a' : '#4a4652'));
    K.rr(-12, -15, 24, 11, 4, cab);
    if (face === 1) xEyes(g, 0, -9.5, 0.7);
    else if (face === 3) { g.strokeStyle = '#fff'; g.lineWidth = 1; for (const ex of [-1, 1]) { g.beginPath(); g.arc(ex * 4.5, -9.5, 2, 0, P * 1.6); g.stroke(); } }
    else { K.circ(-4.5, -9.5, 2.4, '#fff'); K.circ(4.5, -9.5, 2.4, '#fff'); K.circ(-4.2, -9.2, 1.1, DARK); K.circ(4.8, -9.2, 1.1, DARK);
      if (face === 0) { g.fillStyle = cab; g.fillRect(-7.5, -13, 5, 1.6); g.fillRect(2.5, -13, 5, 1.6); } }
    // engines and the beam nozzle
    for (const ex of [-30, 30]) { K.rr(ex - 6, 6, 12, 6, 2, '#5d6378'); K.circ(ex, 12, 2.5, face === 2 ? '#8fd3ff' : '#ff9a7a'); }
    K.rr(-7, 9, 14, 6, 2, '#4a4652'); K.circ(0, 14, 3, face === 2 ? '#8fd3ff' : '#bff6ff');
  }
  return { stamper, boulderRock, boulderFace, hauler };
})();
