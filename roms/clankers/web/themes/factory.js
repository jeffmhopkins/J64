// World map theme: Gear Factory (World 2). The scenery is drawn here rather than listed in world.txt, so the
// world file only holds the nodes and roads. Level looks are in LTHEMES.factory below.
THEMES.factory = (W) => {
  const K = CK, P = Math.PI;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  const T = 16, C = c => c * T + 8;
  function rivetTile(g, c, r, a, b) {
    g.fillStyle = (c + r) % 2 ? a : b; g.fillRect(c * T, r * T, T, T);
    g.fillStyle = 'rgba(0,0,0,0.22)'; g.fillRect(c * T, r * T + T - 1, T, 1); g.fillRect(c * T + T - 1, r * T, 1, T);
    g.fillStyle = 'rgba(255,255,255,0.12)'; g.fillRect(c * T, r * T, T, 1);
    g.fillStyle = 'rgba(20,24,34,0.45)'; for (const [dx, dy] of [[2, 2], [13, 2], [2, 13], [13, 13]]) g.fillRect(c * T + dx, r * T + dy, 1.5, 1.5);
  }
  function hazard(g, x, y, w, h) {
    g.save(); g.beginPath(); g.rect(x, y, w, h); g.clip();
    g.fillStyle = '#ffcf3a'; g.fillRect(x, y, w, h); g.fillStyle = '#2b2230';
    for (let i = -h; i < w + h; i += 8) { g.beginPath(); g.moveTo(x + i, y + h); g.lineTo(x + i + 4, y + h); g.lineTo(x + i + 4 + h, y); g.lineTo(x + i + h, y); g.fill(); }
    g.restore();
  }
  function pipe(g, pts, w, c) {
    g.strokeStyle = 'rgba(0,0,0,0.35)'; g.lineWidth = w + 2; g.lineJoin = 'round'; g.lineCap = 'butt';
    g.beginPath(); pts.forEach(([x, y], i) => i ? g.lineTo(x, y) : g.moveTo(x, y)); g.stroke();
    g.strokeStyle = c; g.lineWidth = w; g.stroke();
    g.strokeStyle = 'rgba(255,255,255,0.3)'; g.lineWidth = 1; g.stroke();
  }
  function crateAt(g, x, y) { K.rr(x - 5, y - 5, 10, 10, 1, '#b07a44'); g.strokeStyle = '#7d5128'; g.lineWidth = 1; g.strokeRect(x - 4, y - 4, 8, 8); g.beginPath(); g.moveTo(x - 4, y - 4); g.lineTo(x + 4, y + 4); g.stroke(); }
  // conveyor belts: [col0, col1, row, dir]
  const BELTS = [[7, 14, 1.6, 1], [0, 4, 11, -1], [16, 19, 7, -1]];

  return {
    propKinds: [], featureKinds: [],
    road: '#cfd4de', roadEdge: '#2a2f3c', roadDot: '#9aa1b0',
    roadLocked: 'rgba(220,228,240,0.35)', roadLockedEdge: 'rgba(10,12,20,0.35)',
    drawGround(g) {
      for (let c = 0; c < 20; c++) for (let r = 0; r < 15; r++) rivetTile(g, c, r, '#5b6275', '#626a7e');
      // painted floor zones
      g.fillStyle = 'rgba(255,207,58,0.12)'; g.fillRect(C(9) - 8, C(6) - 8, 5 * T, 4 * T);
      hazard(g, C(13) - 8, C(13) + 6, 3 * T, 3);
      hazard(g, 0, 5 * T - 3, 4 * T, 3);
      // pipes along the walls
      pipe(g, [[0, 22], [20, 22], [20, 6], [130, 6]], 5, '#c75b4a');
      pipe(g, [[320, 150], [292, 150], [292, 200], [240, 200], [240, 240]], 5, '#4f86b0');
      pipe(g, [[0, 140], [24, 140], [24, 120]], 4, '#49a26a');
      for (const [x, y] of [[20, 22], [20, 6], [292, 150], [292, 200], [240, 200]]) K.circ(x, y, 3.5, '#3a3346');
      // paint shop splats by 2-3
      const q = rng(11), paints = ['#ff7a59', '#49f08a', '#5ad1ff', '#c28bff', '#ffd23f', '#ff8fc7'];
      for (let i = 0; i < 14; i++) { g.fillStyle = paints[i % 6]; g.globalAlpha = 0.75; g.beginPath(); g.ellipse(C(9) + q() * 70 - 6, C(9) + 4 + q() * 26, 2 + q() * 5, 1.5 + q() * 3, 0, 0, P * 2); g.fill(); }
      g.globalAlpha = 1;
      // paint cans
      for (let i = 0; i < 3; i++) { K.rr(C(12) + 4 + i * 7, C(10) - 2, 6, 8, 1, paints[i + 1]); g.fillStyle = 'rgba(0,0,0,0.3)'; g.fillRect(C(12) + 4 + i * 7, C(10) - 2, 6, 1.5); }
      // machines: a lathe and a control panel
      K.rr(C(1) - 6, C(5) + 4, 26, 14, 2, '#7d8397'); K.rr(C(1) - 3, C(5) + 7, 8, 5, 1, '#2b2230'); K.circ(C(2) + 6, C(5) + 11, 3, '#e04f4f'); K.circ(C(2) - 2, C(5) + 11, 2, '#49d17a');
      K.rr(C(8) - 6, C(11) - 6, 20, 16, 2, '#7d8397'); for (let i = 0; i < 3; i++) K.circ(C(8) - 1 + i * 5, C(11) - 1, 1.6, ['#ffd23f', '#49d17a', '#5ad1ff'][i]);
      // shelves of parts at the bottom right
      for (let i = 0; i < 2; i++) { K.rr(C(16) - 6, C(13) - 10 + i * 9, 50, 3, 1, '#3a3346'); for (let j = 0; j < 5; j++) K.gear(C(16) - 1 + j * 10, C(13) - 13 + i * 9, 3, 7, j, '#b8a6b6', '#3a3346'); }
      // belt beds
      for (const [c0, c1, r] of BELTS) { K.rr(c0 * T, r * T + 3, (c1 - c0 + 1) * T, 10, 3, '#2b2833'); K.circ(c0 * T + 5, r * T + 8, 4, '#4a4652'); K.circ((c1 + 1) * T - 5, r * T + 8, 4, '#4a4652'); }
    },
    drawHome(g, n) {
      // the big factory gate, back to World 1
      K.rr(n.x - 12, n.y - 14, 24, 18, 2, '#3a3346');
      g.fillStyle = '#7d8397'; for (let i = 0; i < 5; i++) g.fillRect(n.x - 10, n.y - 12 + i * 3.4, 20, 2);
      hazard(g, n.x - 12, n.y + 3, 24, 3);
      g.font = '7px "Titan One", sans-serif'; g.fillStyle = '#ffd23f'; g.textAlign = 'center'; g.fillText('W1', n.x, n.y - 15);
    },
    drawExit(g, n, t, ok) {
      // a mine lift cage, rocks waiting above for World 3
      g.save(); g.globalAlpha = ok ? 1 : 0.6;
      g.fillStyle = '#3a3346'; g.fillRect(n.x - 9, n.y - 26, 2, 34); g.fillRect(n.x + 7, n.y - 26, 2, 34); g.fillRect(n.x - 9, n.y - 26, 18, 2);
      const lift = ok ? Math.sin(t * 2) * 2 : 0;
      K.rr(n.x - 7, n.y - 8 + lift, 14, 14, 2, ok ? '#49d17a' : '#8c8496');
      g.strokeStyle = 'rgba(0,0,0,0.35)'; g.lineWidth = 1; for (let i = -4; i <= 4; i += 4) { g.beginPath(); g.moveTo(n.x + i, n.y - 8 + lift); g.lineTo(n.x + i, n.y + 6 + lift); g.stroke(); }
      g.font = '7px "Titan One", sans-serif'; g.fillStyle = '#2b2230'; g.textAlign = 'center'; g.fillText('W3', n.x, n.y + 2 + lift);
      g.strokeStyle = '#3a3346'; g.beginPath(); g.moveTo(n.x, n.y - 24); g.lineTo(n.x, n.y - 8 + lift); g.stroke();
      g.restore();
    },
    drawBoss(g, n, t, fixed) {
      // The Stamper: a press frame with a block that thumps down
      const x = n.x, y = n.y - 10;
      g.fillStyle = '#3a3346'; g.fillRect(x - 16, y - 30, 4, 32); g.fillRect(x + 12, y - 30, 4, 32); K.rr(x - 18, y - 34, 36, 7, 2, '#7d8397');
      const ph = (t * (fixed ? 0.5 : 1.1)) % 1, drop = ph < 0.15 ? ph / 0.15 : ph < 0.3 ? 1 : 1 - (ph - 0.3) / 0.7;
      const by = y - 26 + drop * 14;
      g.fillStyle = '#5d6378'; g.fillRect(x - 2, y - 28, 4, by - (y - 28));
      K.rr(x - 11, by, 22, 9, 2, fixed ? '#49d17a' : '#e04f4f');
      K.circ(x - 4, by + 4.5, 1.6, '#fff'); K.circ(x + 4, by + 4.5, 1.6, '#fff');
      if (!fixed) { g.fillStyle = '#fff'; g.fillRect(x - 6, by + 3.5, 4, 1); g.fillRect(x + 2, by + 3.5, 4, 1); }
      if (drop > 0.95 && !fixed) for (let i = 0; i < 3; i++) K.circ(x - 14 + i * 14, y + 2, 2, 'rgba(255,255,255,0.6)');
    },
    drawLive(g, t) {
      // belts: moving treads and crates riding along
      for (const [c0, c1, r, dir] of BELTS) {
        const x0 = c0 * T, x1 = (c1 + 1) * T, y = r * T + 8;
        g.save(); g.beginPath(); g.rect(x0 + 2, y - 4, x1 - x0 - 4, 8); g.clip();
        g.fillStyle = '#4a4652'; const off = ((t * 20 * dir) % 6 + 6) % 6;
        for (let x = x0 - 6 + off; x < x1 + 6; x += 6) g.fillRect(x, y - 4, 2, 8);
        g.restore();
        const span = x1 - x0;
        for (let i = 0; i < Math.max(1, (c1 - c0) / 4 | 0); i++) { const u = ((t * 20 * dir + i * 64) % span + span) % span; crateAt(g, x0 + u, y - 5); }
      }
      // big turning gears
      K.gear(C(17), C(3) + 4, 18, 14, t * 0.6, '#9aa1b0', '#5b6275'); K.gear(C(19) + 2, C(4) + 4, 10, 10, -t * 1.08, '#b8a6b6', '#5b6275');
      K.gear(C(4) - 4, C(13) + 2, 9, 10, t * 0.9, '#9aa1b0', '#5b6275');
      // steam puffs from the pipes
      for (const [px, py, ph] of [[130, 6, 0], [24, 120, 0.5]]) for (let i = 0; i < 3; i++) { const k = (t * 0.7 + i / 3 + ph) % 1; g.globalAlpha = 0.55 * (1 - k); K.circ(px + 2 + k * 4, py - 2 - k * 12, 2 + k * 4, '#e9edf4'); }
      g.globalAlpha = 1;
      // paint drip from a nozzle over 2-3
      g.fillStyle = '#3a3346'; g.fillRect(C(12) - 2, C(6) + 2, 4, 6);
      const dk = (t * 0.9) % 1; K.circ(C(12), C(6) + 9 + dk * 14, 1.6, ['#ff7a59', '#49f08a', '#5ad1ff'][Math.floor(t * 0.9) % 3]);
      // a Sweepy cleaning by the shop
      const sx = C(3) + Math.sin(t * 0.7) * 10, sy = C(10) - 2, c = K.shade(0x8FD3FF, 1), d = K.shade(0x8FD3FF, 0.22);
      K.circ(sx - 2.5, sy + 6, 1.5, '#2b2833'); K.circ(sx + 2.5, sy + 6, 1.5, '#2b2833');
      const sw = Math.sin(t * 5) * 0.5; g.strokeStyle = '#a0703c'; g.lineWidth = 1; g.beginPath(); g.moveTo(sx + 3, sy); g.lineTo(sx + 6 + sw * 3, sy + 7); g.stroke(); K.rr(sx + 4 + sw * 3, sy + 6, 5, 2, 0.5, '#e0b04a');
      K.rr(sx - 4, sy - 8, 8, 13, 3, c); K.circ(sx - 1.5, sy - 4, 1.1, '#fff'); K.circ(sx + 1.5, sy - 4, 1.1, '#fff'); K.circ(sx - 1.5, sy - 3.8, 0.55, d); K.circ(sx + 1.5, sy - 3.8, 0.55, d);
    },
  };
};

// Level looks (see lib/levelart.js): a factory hall with big gears and warm windows, steel floors.
LTHEMES.factory = (() => {
  const K = CK, P = Math.PI;
  return {
    debris: ['#b07a44', '#7d5128'],
    far(g, boss) {
      const gr = g.createLinearGradient(0, 0, 0, 240);
      gr.addColorStop(0, boss ? '#3a1f33' : '#28324a'); gr.addColorStop(0.6, boss ? '#6e3448' : '#3f4d6c'); gr.addColorStop(1, boss ? '#a8524e' : '#6a7896');
      g.fillStyle = gr; g.fillRect(0, 0, 640, 240);
      // roof trusses
      g.strokeStyle = boss ? 'rgba(30,10,20,0.5)' : 'rgba(15,20,35,0.45)'; g.lineWidth = 3;
      g.beginPath(); g.moveTo(0, 30); g.lineTo(640, 30); g.stroke(); g.lineWidth = 2;
      for (let x = 0; x < 640; x += 40) { g.beginPath(); g.moveTo(x, 30); g.lineTo(x + 20, 6); g.lineTo(x + 40, 30); g.stroke(); }
      // tall windows with warm light
      for (let i = 0; i < 8; i++) {
        const x = 26 + i * 80; K.rr(x, 50, 34, 64, 4, boss ? '#5a2a3a' : '#22293d');
        for (let r = 0; r < 3; r++) for (let c = 0; c < 2; c++) K.rr(x + 4 + c * 14, 55 + r * 19, 12, 16, 1.5, boss ? (r === 0 ? '#ff9a7a' : '#d8705e') : (r === 0 ? '#ffe2a0' : '#f2b86a'));
      }
      // big slow gears on the back wall
      const gc = boss ? 'rgba(40,14,26,0.55)' : 'rgba(20,26,44,0.5)';
      for (const [x, y, r] of [[90, 168, 46], [168, 140, 26], [330, 176, 58], [470, 150, 34], [560, 180, 48]]) { K.gear(x, y, r, Math.round(r / 3.4), x * 0.01, gc, null); K.circ(x, y, r * 0.3, boss ? '#6e3448' : '#3f4d6c'); }
      // a hanging lamp row
      for (let x = 60; x < 640; x += 120) { g.fillStyle = 'rgba(10,12,20,0.5)'; g.fillRect(x, 30, 1.5, 14); K.circ(x + 0.75, 46, 4, boss ? '#ff6b6b' : '#ffe9a8'); }
    },
    mid(g) {
      // machine silhouettes: presses, tanks and chimneys
      const c = '#2c3347', d = '#232838';
      g.fillStyle = c; g.fillRect(0, 206, 640, 40);
      for (let i = 0; i < 10; i++) {
        const x = i * 66 + 6, k = i % 3;
        if (k === 0) { K.rr(x, 178, 44, 30, 3, c); K.rr(x + 8, 168, 28, 12, 2, c); g.fillStyle = d; g.fillRect(x + 6, 186, 32, 4); K.circ(x + 12, 198, 3, '#ffcf3a'); }
        else if (k === 1) { K.rr(x + 4, 182, 30, 26, 13, c); g.fillStyle = d; g.fillRect(x + 4, 192, 30, 3); g.fillRect(x + 16, 166, 6, 18); }
        else { g.fillStyle = c; g.fillRect(x + 10, 166, 10, 42); g.fillRect(x + 6, 162, 18, 6); K.gear(x + 36, 194, 12, 10, i, c, d); }
      }
    },
    tile: LEVELART.tiles({
      ground: (g, i) => LEVELART.ground(g, i, { body: '#5b6275', speck: '#4c5264', top: '#9aa3b8', rim: '#c6cddc', under: '#454b5c' }, (g, k) => {
        // hazard paint on two of the four tops
        if (k === 2) { g.save(); g.beginPath(); g.rect(0, 0, 16, 5); g.clip(); g.fillStyle = '#ffcf3a'; g.fillRect(0, 0, 16, 5); g.fillStyle = '#2b2230'; for (let x = -6; x < 16; x += 6) { g.beginPath(); g.moveTo(x, 5); g.lineTo(x + 3, 5); g.lineTo(x + 8, 0); g.lineTo(x + 5, 0); g.fill(); } g.restore(); }
      }, (g, k) => {
        // riveted steel plates
        g.fillStyle = 'rgba(20,24,34,0.5)'; for (const [x, y] of [[2, 8], [13, 8], [2, 13], [13, 13]]) g.fillRect(x, y, 1.5, 1.5);
        g.fillStyle = 'rgba(0,0,0,0.18)'; g.fillRect(0, 15, 16, 1); g.fillRect(15, 0, 1, 16);
      }),
      girder(g) {
        K.rr(0, 0, 16, 16, 1.5, '#4f86b0'); g.fillStyle = '#3a6688'; g.fillRect(0, 13, 16, 3); g.fillStyle = '#7fb3d8'; g.fillRect(0, 0, 16, 2);
        g.strokeStyle = '#3a6688'; g.lineWidth = 1.5; g.beginPath(); g.moveTo(2, 3); g.lineTo(8, 12); g.lineTo(14, 3); g.stroke();
      },
      plank(g) {
        g.fillStyle = '#8a93a8'; g.fillRect(0, 0, 16, 5); g.fillStyle = '#c6cddc'; g.fillRect(0, 0, 16, 1.5);
        g.fillStyle = '#4c5264'; for (let x = 1; x < 16; x += 3) g.fillRect(x, 2, 1.5, 2.5);
        g.fillStyle = '#5b6275'; g.fillRect(7, 5, 2, 4);
      },
    }),
  };
})();
