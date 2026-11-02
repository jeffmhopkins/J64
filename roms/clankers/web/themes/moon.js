// World map theme: Moon Base (World 4). The scenery is drawn here rather than listed in world.txt, so the
// world file only holds the nodes and roads. Level looks are in LTHEMES.moon below.
THEMES.moon = (W) => {
  const K = CK, P = Math.PI;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  const T = 16, C = c => c * T + 8, SKY = 66;
  const STARS = (() => { const q = rng(77), a = []; for (let i = 0; i < 40; i++) a.push([q() * 320, q() * (SKY - 6), q()]); return a; })();
  function crater(g, x, y, r) {
    g.fillStyle = 'rgba(0,0,0,0.18)'; g.beginPath(); g.ellipse(x, y, r, r * 0.55, 0, 0, P * 2); g.fill();
    g.fillStyle = '#9da3b4'; g.beginPath(); g.ellipse(x, y + r * 0.12, r * 0.8, r * 0.4, 0, 0, P * 2); g.fill();
    g.strokeStyle = 'rgba(255,255,255,0.35)'; g.lineWidth = 1; g.beginPath(); g.ellipse(x, y, r, r * 0.55, 0, P * 0.1, P * 0.9); g.stroke();
  }
  function dome(g, x, y, r, light) {
    K.rr(x - r - 2, y - 2, 2 * r + 4, 5, 2, '#6c7388');
    g.fillStyle = 'rgba(160,220,255,0.55)'; g.beginPath(); g.arc(x, y - 1, r, P, 0); g.fill();
    g.strokeStyle = '#dff3ff'; g.lineWidth = 1; g.beginPath(); g.arc(x, y - 1, r, P, 0); g.stroke();
    g.beginPath(); g.moveTo(x, y - 1 - r); g.lineTo(x, y - 1); g.moveTo(x - r * 0.7, y - 1 - r * 0.7); g.quadraticCurveTo(x, y - r * 0.4, x + r * 0.7, y - 1 - r * 0.7); g.stroke();
    K.circ(x - r * 0.4, y - r * 0.55, r * 0.18, 'rgba(255,255,255,0.6)');
    if (light) K.rr(x - 3, y - 6, 6, 5, 1, '#ffd37a');
  }
  function solar(g, x, y) { g.fillStyle = '#6c7388'; g.fillRect(x - 0.5, y - 2, 1, 6); g.save(); g.translate(x, y - 3); g.rotate(-0.3); K.rr(-9, -3, 18, 6, 1, '#2e4f8c'); g.strokeStyle = '#8fb7ff'; g.lineWidth = 0.6; for (let i = -6; i < 9; i += 4) { g.beginPath(); g.moveTo(i, -3); g.lineTo(i, 3); g.stroke(); } g.restore(); }

  return {
    propKinds: [], featureKinds: [],
    road: '#cdeeff', roadEdge: '#3d4a66', roadDot: '#93c3e6',
    roadLocked: 'rgba(205,238,255,0.3)', roadLockedEdge: 'rgba(20,24,40,0.35)',
    drawGround(g) {
      g.fillStyle = '#0d1030'; g.fillRect(0, 0, 320, SKY + 6);
      const neb = g.createRadialGradient(240, 20, 4, 240, 20, 120); neb.addColorStop(0, 'rgba(120,80,200,0.35)'); neb.addColorStop(1, 'rgba(120,80,200,0)'); g.fillStyle = neb; g.fillRect(0, 0, 320, SKY);
      // Earth
      K.circ(284, 46, 15, '#3f7fd6'); g.fillStyle = '#49c27a';
      g.beginPath(); g.ellipse(278, 40, 6, 4, 0.4, 0, P * 2); g.fill(); g.beginPath(); g.ellipse(290, 52, 5, 3.5, -0.3, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(255,255,255,0.7)'; g.beginPath(); g.ellipse(284, 34, 7, 2, 0, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(13,16,48,0.45)'; g.beginPath(); g.arc(284, 46, 15, -P / 2, P / 2); g.fill();
      // moon surface tiles with a curved horizon
      for (let c = 0; c < 20; c++) for (let r = 0; r < 15; r++) {
        if (r * T + T < SKY) continue;
        g.fillStyle = (c + r) % 2 ? '#b4b9c7' : '#adb2c1'; g.fillRect(c * T, r * T, T, T);
      }
      g.fillStyle = '#0d1030'; g.beginPath(); g.moveTo(0, SKY + 6); g.quadraticCurveTo(160, SKY - 10, 320, SKY + 6); g.lineTo(320, 0); g.lineTo(0, 0); g.fill();
      for (const [x, y, k] of STARS) if (y < SKY - 4) K.circ(x, y, k > 0.8 ? 1 : 0.6, '#ffffff');
      K.circ(284, 46, 15, '#3f7fd6'); g.fillStyle = '#49c27a';
      g.beginPath(); g.ellipse(278, 40, 6, 4, 0.4, 0, P * 2); g.fill(); g.beginPath(); g.ellipse(290, 52, 5, 3.5, -0.3, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(255,255,255,0.7)'; g.beginPath(); g.ellipse(284, 34, 7, 2, 0, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(13,16,48,0.45)'; g.beginPath(); g.arc(284, 46, 15, -P / 2, P / 2); g.fill();
      const q = rng(9);
      for (let i = 0; i < 160; i++) { g.fillStyle = q() > 0.5 ? 'rgba(80,86,104,0.35)' : 'rgba(255,255,255,0.35)'; g.fillRect(q() * 320, SKY + 6 + q() * 170, 1, 1); }
      [[C(4), C(9), 9], [C(9), C(13), 11], [C(13), C(10), 7], [C(17), C(13), 9], [C(1), C(13) + 4, 6], [C(9), C(5), 6], [C(18), C(6), 8], [C(13), C(13), 5]].forEach(([x, y, r]) => crater(g, x, y, r));
      dome(g, C(8), C(9) + 4, 14, true); dome(g, C(13), C(5) + 4, 11, true); dome(g, C(4), C(5) + 2, 9, false);
      solar(g, C(16), C(12)); solar(g, C(18), C(12) + 4); solar(g, C(1), C(9));
      // flag
      g.fillStyle = '#e8e4f0'; g.fillRect(C(9) - 3, C(11) - 14, 1, 14); g.fillStyle = '#ffd23f'; g.fillRect(C(9) - 2, C(11) - 14, 8, 5); K.gear(C(9) + 2, C(11) - 11.5, 1.8, 6, 0, '#c75b4a', null);
    },
    drawHome(g, n, t) {
      // the rocket they flew in on, standing on its pad
      K.rr(n.x - 12, n.y + 2, 24, 4, 1.5, '#6c7388');
      const x = n.x, y = n.y - 8;
      g.fillStyle = '#c75b4a'; g.beginPath(); g.moveTo(x - 8, y + 9); g.lineTo(x - 4, y + 2); g.lineTo(x - 4, y + 9); g.fill(); g.beginPath(); g.moveTo(x + 8, y + 9); g.lineTo(x + 4, y + 2); g.lineTo(x + 4, y + 9); g.fill();
      K.rr(x - 4, y - 12, 8, 21, 4, '#eef1f6'); g.fillStyle = '#c75b4a'; g.beginPath(); g.moveTo(x - 4, y - 8); g.quadraticCurveTo(x, y - 19, x + 4, y - 8); g.fill();
      K.circ(x, y - 3, 2, '#5ad1ff');
      g.font = '7px "Titan One", sans-serif'; g.fillStyle = '#ffd23f'; g.textAlign = 'center'; g.fillText('W3', n.x, n.y + 14);
    },
    drawExit(g, n, t, ok) {
      // the parade dock: a stage with balloons, confetti when it opens
      g.save(); g.globalAlpha = ok ? 1 : 0.6;
      K.rr(n.x - 12, n.y - 2, 24, 8, 2, ok ? '#49d17a' : '#8c8496'); K.rr(n.x - 10, n.y - 1, 20, 3, 1, 'rgba(255,255,255,0.3)');
      const cols = ['#ff8fc7', '#5ad1ff', '#ffd23f'];
      for (let i = 0; i < 3; i++) { const bx = n.x - 8 + i * 8, by = n.y - 16 - (i % 2) * 3 + (ok ? Math.sin(t * 2 + i) * 1.5 : 0); g.strokeStyle = '#e8e4f0'; g.lineWidth = 0.6; g.beginPath(); g.moveTo(bx, by + 4); g.lineTo(n.x - 6 + i * 6, n.y - 2); g.stroke(); g.fillStyle = cols[i]; g.beginPath(); g.ellipse(bx, by, 3.2, 4, 0, 0, P * 2); g.fill(); }
      if (ok) for (let i = 0; i < 8; i++) { const k = (t * 0.6 + i / 8) % 1; g.fillStyle = cols[i % 3]; g.fillRect(n.x - 14 + ((i * 37) % 28), n.y - 24 + k * 22, 1.5, 1.5); }
      g.restore();
    },
    drawBoss(g, n, t, fixed) {
      // The Sky Hauler hovering above its panel
      const x = n.x, y = n.y - 30 + Math.sin(t * 1.3) * 2, tilt = fixed ? 0 : Math.sin(t * 2.2) * 0.12;
      g.save(); g.translate(x, y); g.rotate(tilt);
      g.fillStyle = 'rgba(160,220,255,0.15)'; g.beginPath(); g.moveTo(-8, 8); g.lineTo(8, 8); g.lineTo(12, 28); g.lineTo(-12, 28); g.fill();
      K.rr(-22, -4, 44, 12, 6, '#7d8397'); K.rr(-14, -10, 28, 8, 4, '#9aa1b0');
      for (let i = 0; i < 4; i++) K.circ(-15 + i * 10, 2, 2, fixed ? '#ffd37a' : (Math.floor(t * 4 + i) % 2 ? '#ffd37a' : '#4a4652'));
      K.rr(-6, -9, 12, 6, 2, fixed ? '#49d17a' : '#e04f4f'); K.circ(-2.5, -6, 1.1, '#fff'); K.circ(2.5, -6, 1.1, '#fff');
      g.restore();
      if (!fixed && Math.floor(t * 6) % 3 === 0) { g.strokeStyle = '#bff6ff'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 20, y - 6); g.lineTo(x + 23, y - 3); g.lineTo(x + 21, y - 2); g.lineTo(x + 24, y + 1); g.stroke(); }
    },
    drawLive(g, t) {
      for (let i = 0; i < 10; i++) { const [x, y] = STARS[i]; if (y > SKY - 6) continue; const k = Math.max(0, Math.sin(t * 1.5 + i * 2)); if (k > 0.9) K.star(x, y, 2.2, '#fff8d8'); }
      const sk = (t * 0.25) % 1; if (sk < 0.25) { const u = sk / 0.25, x = 20 + u * 160, y = 8 + u * 30; g.strokeStyle = 'rgba(255,255,255,' + (1 - u) + ')'; g.lineWidth = 1; g.beginPath(); g.moveTo(x, y); g.lineTo(x - 12, y - 2.4); g.stroke(); }
      // radar dish turning
      const dx = C(16), dy = C(9); g.fillStyle = '#6c7388'; g.fillRect(dx - 1, dy - 4, 2, 10); const a = Math.sin(t * 0.7) * 0.6;
      g.save(); g.translate(dx, dy - 5); g.rotate(a); g.fillStyle = '#e8ecf4'; g.beginPath(); g.ellipse(0, 0, 7, 3, 0, P, 0); g.fill(); g.fillRect(-0.5, -6, 1, 5); K.circ(0, -6.5, 1.2, '#e04f4f'); g.restore();
      // Moon Blobs bouncing
      for (const [bx, ph] of [[C(3), 0], [C(13) + 6, 1.7]]) {
        const by = C(13) - 4, b = Math.abs(Math.sin(t * 2 + ph)), sq = 1 + (1 - b) * 0.25, y = by - b * 10;
        g.save(); g.translate(bx, y); g.scale(sq, 1 / sq); g.globalAlpha = 0.85; g.fillStyle = '#35d8e6'; g.beginPath(); g.ellipse(0, -3, 6, 5, 0, P, 0); g.lineTo(6, 0); g.lineTo(-6, 0); g.fill(); g.globalAlpha = 1;
        K.circ(-2, -3, 1, '#fff'); K.circ(2, -3, 1, '#fff'); g.restore();
      }
      // Snooze Bot asleep by the dome
      const sx = C(17) - 4, sy = C(7) + 6, br = Math.sin(t * 1.6) * 0.6, pc = K.shade(0xF5A3C7, 1);
      K.circ(sx - 8, sy + 1, 1.6, '#2b2833'); K.circ(sx + 8, sy + 1, 1.6, '#2b2833');
      K.rr(sx - 11, sy - 6 - br, 22, 7 + br, 3, pc); K.rr(sx - 11, sy - 10 - br, 9, 6, 2.5, pc);
      g.strokeStyle = '#3a2333'; g.lineWidth = 0.7; g.beginPath(); g.arc(sx - 8.5, sy - 7.5 - br, 1, 0.1, P - 0.1); g.stroke(); g.beginPath(); g.arc(sx - 5, sy - 7.5 - br, 1, 0.1, P - 0.1); g.stroke();
      const z = (t * 0.6) % 1; g.globalAlpha = 1 - z; g.font = '6px "Titan One", sans-serif'; g.fillStyle = '#e8e4f0'; g.textAlign = 'left'; g.fillText('z', sx - 4 + z * 4, sy - 12 - z * 8); g.globalAlpha = 1;
    },
  };
};

// Level looks (see lib/levelart.js): deep space with Earth, grey moon rock, white station panels.
LTHEMES.moon = (() => {
  const K = CK, P = Math.PI;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  return {
    debris: ['#b07a44', '#7d5128'],
    far(g, boss) {
      const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#070920'); gr.addColorStop(1, boss ? '#3a1a5a' : '#1c2350');
      g.fillStyle = gr; g.fillRect(0, 0, 640, 240);
      const neb = g.createRadialGradient(boss ? 420 : 200, 80, 6, boss ? 420 : 200, 80, 180);
      neb.addColorStop(0, boss ? 'rgba(220,80,160,0.35)' : 'rgba(120,80,200,0.35)'); neb.addColorStop(1, 'rgba(120,80,200,0)');
      g.fillStyle = neb; g.fillRect(0, 0, 640, 240);
      const q = rng(41);
      for (let i = 0; i < 140; i++) { const x = q() * 640, y = q() * 200, r = q() > 0.85 ? 1.2 : 0.6; K.circ(x, y, r, '#ffffff'); }
      // Earth
      K.circ(470, 64, 24, '#3f7fd6'); g.fillStyle = '#49c27a';
      g.beginPath(); g.ellipse(462, 56, 9, 6, 0.4, 0, P * 2); g.fill(); g.beginPath(); g.ellipse(480, 74, 8, 5, -0.3, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(255,255,255,0.7)'; g.beginPath(); g.ellipse(470, 46, 11, 3, 0, 0, P * 2); g.fill();
      g.fillStyle = 'rgba(7,9,32,0.45)'; g.beginPath(); g.arc(470, 64, 24, -P / 2, P / 2); g.fill();
      // far grey hills
      g.fillStyle = '#4a5068'; g.beginPath(); g.moveTo(0, 240);
      for (let x = 0; x <= 640; x += 32) g.lineTo(x, 168 + Math.sin(x * 0.017) * 12 + Math.sin(x * 0.043) * 6);
      g.lineTo(640, 240); g.fill();
    },
    mid(g) {
      const c = '#7d8399', d = '#6a7086';
      g.fillStyle = c; g.beginPath(); g.moveTo(0, 240);
      for (let x = 0; x <= 640; x += 16) g.lineTo(x, 194 + Math.sin(x * 0.024) * 8);
      g.lineTo(640, 240); g.fill();
      for (let i = 0; i < 10; i++) { const x = i * 66 + 20; g.fillStyle = d; g.beginPath(); g.ellipse(x, 214 + (i % 2) * 6, 12, 3.5, 0, 0, P * 2); g.fill(); }
      // a dome and an antenna
      for (const x of [120, 440]) {
        g.fillStyle = 'rgba(160,220,255,0.55)'; g.beginPath(); g.arc(x, 200, 22, P, 0); g.fill();
        g.strokeStyle = '#dff3ff'; g.lineWidth = 1; g.beginPath(); g.arc(x, 200, 22, P, 0); g.stroke(); K.rr(x - 24, 198, 48, 5, 2, '#6c7388');
        K.rr(x - 4, 190, 8, 6, 1, '#ffd37a');
      }
      g.fillStyle = '#6c7388'; g.fillRect(300, 170, 2, 30); K.circ(301, 168, 2.5, '#e04f4f');
      g.fillStyle = c; g.fillRect(0, 218, 640, 22);
    },
    tile: LEVELART.tiles({
      ground: (g, i) => LEVELART.ground(g, i, { body: '#9a9fb0', speck: '#868b9c', top: '#c3c8d6', rim: '#eef1f8', under: '#7f8496' }, (g, k) => {
        if (k === 1) { g.fillStyle = 'rgba(0,0,0,0.18)'; g.beginPath(); g.ellipse(9, 2.6, 4, 1.4, 0, 0, P * 2); g.fill(); }
        if (k === 3) { g.fillStyle = 'rgba(0,0,0,0.18)'; g.beginPath(); g.ellipse(4, 2.6, 2.5, 1, 0, 0, P * 2); g.fill(); }
      }, (g, k) => { if (k === 2) { g.fillStyle = 'rgba(0,0,0,0.12)'; g.beginPath(); g.ellipse(8, 11, 4, 2, 0, 0, P * 2); g.fill(); } }),
      girder(g) {
        // a white station panel with a blinking light
        K.rr(0, 0, 16, 16, 2, '#dfe5f1'); g.fillStyle = '#a9b3c8'; g.fillRect(0, 13, 16, 3); g.fillStyle = '#ffffff'; g.fillRect(0, 0, 16, 2);
        g.fillStyle = '#a9b3c8'; g.fillRect(2, 4, 12, 1); g.fillRect(2, 9, 12, 1); K.circ(12, 7, 1.4, '#5ad1ff');
      },
      plank(g) {
        // a glass walkway
        g.fillStyle = 'rgba(140,210,255,0.75)'; g.fillRect(0, 0, 16, 5); g.fillStyle = '#e8f6ff'; g.fillRect(0, 0, 16, 1.2);
        g.fillStyle = '#6c7388'; g.fillRect(0, 4, 16, 1); g.fillRect(7, 5, 2, 4);
      },
    }),
  };
})();
