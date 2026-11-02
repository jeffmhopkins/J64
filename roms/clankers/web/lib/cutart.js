// The little films between worlds: their backdrops and the things that move in them. The browser
// game plays them (cutScene in game.js) and tools/bake_sprites.mjs bakes these same pictures for
// the N64. Set CK.g first.
//   bg(g, k)   a 320x240 backdrop: 1 Junkyard to the Gear Factory, 2 up to Magnet Mountain,
//              3 blast off to the Moon, 4 the parade home
//   gate, cage, rocket, flame, float: the moving pieces, each drawn around (0, 0)
const CUTART = (() => {
  const K = CK, P = Math.PI;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  const GROUND = 196;
  function sky(g, a, b, c) { const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, a); gr.addColorStop(0.7, b); gr.addColorStop(1, c); g.fillStyle = gr; g.fillRect(0, 0, 320, 240); }
  function stars(g, n, h, seed) { const q = rng(seed); for (let i = 0; i < n; i++) K.circ(q() * 320, q() * h, q() > 0.85 ? 1.1 : 0.6, '#ffffff'); }
  function hazard(g, x, y, w, h) {
    g.save(); g.beginPath(); g.rect(x, y, w, h); g.clip(); g.fillStyle = '#ffcf3a'; g.fillRect(x, y, w, h); g.fillStyle = '#2b2230';
    for (let i = -h; i < w + h; i += 8) { g.beginPath(); g.moveTo(x + i, y + h); g.lineTo(x + i + 4, y + h); g.lineTo(x + i + 4 + h, y); g.lineTo(x + i + h, y); g.fill(); }
    g.restore();
  }
  // The factory front with its gate: the opening is x 236..292, from y 136 down to the ground.
  const GATE = { x: 236, y: 136, w: 56, h: 60 };
  const SHAFT = { x: 160, top: 64, bottom: GROUND };   // the mine lift's rails
  const PAD = { x: 214, y: GROUND };                   // the rocket's launch pad
  function bg(g, k) {
    if (k === 1) {
      sky(g, '#5b6fc0', '#f2a37a', '#ffcf8a');
      K.circ(54, 70, 26, 'rgba(255,240,200,0.6)'); K.gear(54, 70, 22, 12, 0, 'rgba(255,231,163,0.9)', null); K.circ(54, 70, 17, '#fff1c4');
      g.fillStyle = '#8a5f96'; g.beginPath(); g.moveTo(0, 240); for (let x = 0; x <= 320; x += 20) g.lineTo(x, 160 + Math.sin(x * 0.03) * 10); g.lineTo(320, 240); g.fill();
      // junk piles on the left
      for (const [x, w, h] of [[20, 50, 26], [90, 40, 18]]) { g.fillStyle = '#857585'; g.beginPath(); g.ellipse(x, GROUND, w / 2, h, 0, P, 0); g.fill(); }
      K.circ(26, GROUND - 16, 5, '#2b2833'); K.gear(92, GROUND - 13, 5, 8, 0.4, '#c9b8c7', '#857585');
      // the Gear Factory: wall, sawtooth roof, chimneys, windows, the gate opening
      g.fillStyle = '#5d6378'; g.fillRect(206, 96, 114, 100);
      g.beginPath(); g.moveTo(206, 98); for (let i = 0; i < 5; i++) { g.lineTo(206 + i * 24 + 12, 82); g.lineTo(206 + (i + 1) * 24, 98); } g.fill();
      g.fillRect(222, 40, 12, 46); g.fillRect(290, 30, 12, 56); g.fillStyle = '#4c5264'; g.fillRect(220, 38, 16, 5); g.fillRect(288, 28, 16, 5);
      for (let i = 0; i < 4; i++) K.rr(214 + i * 26, 106, 16, 18, 1.5, '#ffd37a');
      K.gear(263, 120, 9, 9, 0.2, '#ffd23f', '#5d6378');
      g.fillStyle = '#1b1426'; g.fillRect(GATE.x, GATE.y, GATE.w, GATE.h);
      hazard(g, GATE.x - 4, GATE.y - 6, GATE.w + 8, 5);
      // the road in
      g.fillStyle = '#8a5233'; g.fillRect(0, GROUND, 320, 44); g.fillStyle = '#c98b55'; g.fillRect(0, GROUND, 320, 4);
      g.fillStyle = '#ecc791'; g.fillRect(0, GROUND + 10, 320, 6);
    } else if (k === 2) {
      sky(g, '#7fb6e8', '#c9e2f5', '#eef6ff');
      const peak = (x, w, h, c) => { g.fillStyle = c; g.beginPath(); g.moveTo(x - w / 2, 240); g.lineTo(x, 240 - h); g.lineTo(x + w / 2, 240); g.fill(); };
      peak(250, 300, 230, '#a49ac0'); peak(60, 220, 150, '#a49ac0');
      g.fillStyle = '#ffffff'; g.beginPath(); g.moveTo(250, 10); g.lineTo(226, 42); g.lineTo(240, 36); g.lineTo(250, 44); g.lineTo(262, 36); g.lineTo(274, 42); g.fill();
      // the cliff with the ledge at the top
      g.fillStyle = '#7a6e86'; g.beginPath(); g.moveTo(176, 240); g.lineTo(176, SHAFT.top); g.lineTo(320, SHAFT.top - 6); g.lineTo(320, 240); g.fill();
      g.fillStyle = '#a89cb6'; g.fillRect(176, SHAFT.top - 2, 144, 4); g.fillStyle = '#ffffff'; g.fillRect(180, SHAFT.top - 4, 40, 3); g.fillRect(250, SHAFT.top - 6, 30, 3);
      for (const [x, y, r] of [[210, 110, 6], [270, 140, 8], [230, 180, 7], [296, 96, 5]]) K.circ(x, y, r, '#695d75');
      g.fillStyle = '#3bf0ff'; for (const [x, y] of [[250, 120], [290, 170]]) { g.beginPath(); g.moveTo(x - 3, y); g.lineTo(x, y - 9); g.lineTo(x + 3, y); g.fill(); }
      // the factory roof on the left
      g.fillStyle = '#5d6378'; g.fillRect(0, 150, 120, 46); g.beginPath(); g.moveTo(0, 152); for (let i = 0; i < 5; i++) { g.lineTo(i * 24 + 12, 138); g.lineTo((i + 1) * 24, 152); } g.fill();
      for (let i = 0; i < 4; i++) K.rr(8 + i * 28, 160, 16, 14, 1.5, '#ffd37a');
      // the lift shaft: two rails and a wheel on top
      g.fillStyle = '#3a3346'; g.fillRect(SHAFT.x - 24, SHAFT.top - 30, 4, SHAFT.bottom - SHAFT.top + 30); g.fillRect(SHAFT.x + 20, SHAFT.top - 30, 4, SHAFT.bottom - SHAFT.top + 30);
      g.fillRect(SHAFT.x - 26, SHAFT.top - 32, 52, 5); K.gear(SHAFT.x, SHAFT.top - 34, 8, 8, 0, '#9aa1b0', '#3a3346');
      g.fillStyle = '#8a5233'; g.fillRect(0, GROUND, 176, 44); g.fillStyle = '#9aa3b8'; g.fillRect(0, GROUND, 176, 4);
    } else if (k === 3) {
      sky(g, '#0d1030', '#2a2a6a', '#5b4f8a'); stars(g, 70, 150, 23);
      K.circ(266, 52, 26, '#f4f1e6'); for (const [x, y, r] of [[256, 44, 5], [276, 60, 4], [262, 64, 3]]) K.circ(x, y, r, '#d9d4c6');
      g.fillStyle = '#4a3f5c'; g.beginPath(); g.moveTo(0, 240); g.lineTo(0, 150); g.lineTo(60, 120); g.lineTo(110, 160); g.lineTo(150, 140); g.lineTo(190, 170); g.lineTo(320, 160); g.lineTo(320, 240); g.fill();
      g.fillStyle = '#e9eef8'; g.beginPath(); g.moveTo(60, 120); g.lineTo(48, 130); g.lineTo(60, 128); g.lineTo(72, 130); g.fill();
      g.fillStyle = '#5d5070'; g.fillRect(0, GROUND, 320, 44); g.fillStyle = '#a89cb6'; g.fillRect(0, GROUND, 320, 3);
      // the launch tower and pad
      g.fillStyle = '#5d566a'; g.fillRect(PAD.x + 30, 90, 6, GROUND - 90); for (let y = 96; y < GROUND; y += 14) g.fillRect(PAD.x + 18, y, 18, 2);
      K.rr(PAD.x - 30, GROUND - 6, 60, 8, 2, '#6c7388'); hazard(g, PAD.x - 30, GROUND - 2, 60, 3);
    } else {
      sky(g, '#6a6fc0', '#f2a37a', '#ffcf8a');
      K.circ(274, 96, 30, 'rgba(255,240,200,0.6)'); K.gear(274, 96, 26, 13, 0, 'rgba(255,231,163,0.9)', null); K.circ(274, 96, 20, '#fff1c4');
      g.fillStyle = '#8a5f96'; g.beginPath(); g.moveTo(0, 240); for (let x = 0; x <= 320; x += 20) g.lineTo(x, 168 + Math.sin(x * 0.03) * 10 + Math.sin(x * 0.07) * 5); g.lineTo(320, 240); g.fill();
      // the home garage with bunting
      K.rr(30, 128, 108, 68, 3, '#b07a9a'); g.fillStyle = '#7d4a6a'; g.beginPath(); g.moveTo(22, 132); g.lineTo(84, 98); g.lineTo(146, 132); g.fill();
      K.gear(84, 118, 7, 8, 0, '#ffd23f', '#7d4a6a'); K.rr(52, 146, 64, 50, 2, '#ffcf7a');
      const flags = ['#ff8fc7', '#5ad1ff', '#ffd23f', '#49f08a', '#c28bff'];
      g.strokeStyle = '#3a2630'; g.lineWidth = 1; g.beginPath(); g.moveTo(84, 100); g.quadraticCurveTo(200, 140, 316, 92); g.stroke();
      for (let i = 0; i < 12; i++) { const u = (i + 0.5) / 12, x = (1 - u) * (1 - u) * 84 + 2 * (1 - u) * u * 200 + u * u * 316, y = (1 - u) * (1 - u) * 100 + 2 * (1 - u) * u * 140 + u * u * 92; g.fillStyle = flags[i % 5]; g.beginPath(); g.moveTo(x - 4, y); g.lineTo(x + 4, y); g.lineTo(x, y + 8); g.fill(); }
      g.fillStyle = '#8a5233'; g.fillRect(0, GROUND, 320, 44); g.fillStyle = '#c98b55'; g.fillRect(0, GROUND, 320, 4);
      g.fillStyle = '#ecc791'; g.fillRect(0, GROUND + 12, 320, 6);
    }
  }
  // the factory gate's roller door (56 x 60, top-left at (0, 0))
  function gate(g) {
    g.fillStyle = '#7d8397'; g.fillRect(0, 0, GATE.w, GATE.h);
    g.fillStyle = '#5d6378'; for (let y = 3; y < GATE.h; y += 6) g.fillRect(0, y, GATE.w, 2);
    hazard(g, 0, GATE.h - 6, GATE.w, 6);
  }
  // the lift cage, 44 wide and 46 tall, bottom middle at (0, 0): 0 the back, 1 the front bars
  function cage(g, front) {
    if (!front) { K.rr(-22, -46, 44, 46, 2, 'rgba(58,51,70,0.55)'); g.fillStyle = '#3a3346'; g.fillRect(-1, -70, 2, 26); return; }
    g.fillStyle = '#3a3346'; g.fillRect(-22, -46, 44, 4); g.fillRect(-22, -3, 44, 3); g.fillRect(-22, -46, 3, 46); g.fillRect(19, -46, 3, 46);
    g.fillStyle = '#5d566a'; for (let x = -12; x <= 12; x += 8) g.fillRect(x - 0.75, -42, 1.5, 22);
    K.rr(-14, -48, 28, 4, 1.5, '#ffcf3a');
  }
  // the rocket, 44 wide and 92 tall, bottom middle at (0, 0)
  function rocket(g) {
    g.fillStyle = '#c75b4a';
    g.beginPath(); g.moveTo(-20, 0); g.lineTo(-11, -26); g.lineTo(-11, 0); g.fill(); g.beginPath(); g.moveTo(20, 0); g.lineTo(11, -26); g.lineTo(11, 0); g.fill();
    K.rr(-12, -74, 24, 74, 11, '#eef1f6'); g.fillStyle = '#c75b4a'; g.beginPath(); g.moveTo(-12, -60); g.quadraticCurveTo(0, -96, 12, -60); g.fill();
    K.circ(0, -48, 6, '#5ad1ff'); K.circ(0, -48, 4, '#bff6ff'); K.circ(-1.5, -50, 1.5, '#ffffff');
    g.fillStyle = '#c9cfdb'; g.fillRect(-12, -22, 24, 3); K.gear(0, -30, 4, 7, 0, '#ffd23f', '#eef1f6');
    K.rr(-7, -4, 14, 5, 2, '#5d566a');
  }
  // its flame, top middle at (0, 0); two flickers
  function flame(g, i) { K.flame(0, 2, 34 + i * 6, 16 - i * 2, i * 0.3); }
  // the parade float: 140 wide, wheels on the ground at (0, 0)
  function float(g) {
    for (const x of [-48, -16, 16, 48]) { K.circ(x, -7, 7, '#2b2833'); K.circ(x, -7, 3, '#9aa1b0'); }
    K.rr(-66, -30, 132, 20, 5, '#ff8fc7'); K.rr(-66, -30, 132, 6, 3, '#ffd23f');
    for (let i = 0; i < 6; i++) K.circ(-55 + i * 22, -18, 3.5, ['#5ad1ff', '#49f08a', '#c28bff'][i % 3]);
    // balloons tied at the corners
    for (const [x, c] of [[-60, '#5ad1ff'], [60, '#49f08a']]) {
      g.strokeStyle = '#e8e4f0'; g.lineWidth = 0.8; g.beginPath(); g.moveTo(x, -30); g.lineTo(x, -62); g.stroke();
      g.fillStyle = c; g.beginPath(); g.ellipse(x, -68, 7, 9, 0, 0, P * 2); g.fill(); K.circ(x - 2, -71, 2, 'rgba(255,255,255,0.6)');
    }
  }
  return { bg, gate, cage, rocket, flame, float, GATE, SHAFT, PAD, GROUND };
})();
