// World map theme: Magnet Mountain (World 3). The scenery is drawn here rather than listed in world.txt, so the
// world file only holds the nodes and roads. Level looks are in LTHEMES.mountain below.
THEMES.mountain = (W) => {
  const K = CK, P = Math.PI;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  const T = 16, C = c => c * T + 8;
  function crystals(g, x, y, k, hue) {
    const cols = hue ? ['#ff8fc7', '#ffc2e0'] : ['#3bf0ff', '#b8fbff'];
    for (const [dx, h, w, a] of [[-4, 9, 3.5, -0.3], [0, 13, 4.5, 0], [4, 8, 3.5, 0.35]]) {
      g.save(); g.translate(x + dx * k, y); g.rotate(a); g.fillStyle = cols[0];
      g.beginPath(); g.moveTo(-w * k / 2, 0); g.lineTo(-w * k / 2, -h * k * 0.7); g.lineTo(0, -h * k); g.lineTo(w * k / 2, -h * k * 0.7); g.lineTo(w * k / 2, 0); g.fill();
      g.fillStyle = cols[1]; g.fillRect(-w * k / 2 + 0.6, -h * k * 0.7, 1, h * k * 0.6); g.restore();
    }
  }
  function boulder(g, x, y, r, c) { g.fillStyle = 'rgba(30,20,30,0.3)'; g.beginPath(); g.ellipse(x, y + r * 0.7, r, r * 0.3, 0, 0, P * 2); g.fill(); K.circ(x, y, r, c); K.circ(x - r * 0.3, y - r * 0.3, r * 0.4, 'rgba(255,255,255,0.18)'); }
  function magnet(g, x, y, k) {
    g.fillStyle = '#5d566a'; g.fillRect(x - 1, y, 2, 10 * k);
    K.arc(x, y - 4 * k, 6 * k, P, 0, 4 * k, '#e04f4f'); g.fillStyle = '#e8e4f0'; g.fillRect(x - 8 * k, y - 4 * k, 4 * k, 3 * k); g.fillRect(x + 4 * k, y - 4 * k, 4 * k, 3 * k);
  }
  // the mine cart loop: a rectangle of track
  const TRACK = { x0: C(2) - 4, y0: C(4) - 2, x1: C(8) + 4, y1: C(6) + 2 };
  function trackPoint(u) {
    const { x0, y0, x1, y1 } = TRACK, w = x1 - x0, h = y1 - y0, L = 2 * (w + h); let d = ((u % 1) + 1) % 1 * L;
    if (d < w) return [x0 + d, y0, 0]; d -= w; if (d < h) return [x1, y0 + d, 1]; d -= h; if (d < w) return [x1 - d, y1, 0]; d -= w; return [x0, y1 - d, 1];
  }

  function hazardStripe(g, x, y, w, h) { g.fillStyle = '#ffcf3a'; g.fillRect(x, y, w, h); g.fillStyle = '#2b2230'; for (let i = 0; i < w; i += 4) g.fillRect(x + i, y, 2, h); }
  return {
    propKinds: [], featureKinds: [],
    road: '#e6d6bd', roadEdge: '#4a3b45', roadDot: '#c4ae8e',
    drawGround(g) {
      // sky band behind the peaks, then rock tiles getting darker toward the cave floor
      const sky = g.createLinearGradient(0, 0, 0, 70); sky.addColorStop(0, '#7fb6e8'); sky.addColorStop(1, '#c9e2f5');
      g.fillStyle = sky; g.fillRect(0, 0, 320, 70);
      // far peaks with snow
      const peak = (x, w, h, c, snow) => {
        g.fillStyle = c; g.beginPath(); g.moveTo(x - w / 2, 80); g.lineTo(x, 80 - h); g.lineTo(x + w / 2, 80); g.fill();
        g.fillStyle = snow; g.beginPath(); g.moveTo(x - w * 0.14, 80 - h * 0.72); g.lineTo(x, 80 - h); g.lineTo(x + w * 0.14, 80 - h * 0.72);
        g.lineTo(x + w * 0.07, 80 - h * 0.66); g.lineTo(x, 80 - h * 0.74); g.lineTo(x - w * 0.07, 80 - h * 0.66); g.fill();
      };
      peak(60, 150, 60, '#9a8fb0', '#f4f7ff'); peak(300, 160, 66, '#9a8fb0', '#f4f7ff'); peak(190, 220, 84, '#8a7fa2', '#ffffff');
      for (let c = 0; c < 20; c++) for (let r = 0; r < 15; r++) {
        const y = r * T; if (y < 64) continue;
        const dark = r > 9 ? 0.86 : 1;
        g.fillStyle = (c + r) % 2 ? K.shade(0x8b7f93, dark) : K.shade(0x83778b, dark); g.fillRect(c * T, y, T, T);
        g.fillStyle = 'rgba(0,0,0,0.12)'; g.fillRect(c * T + 2, y + 11, 5, 1); g.fillRect(c * T + 9, y + 5, 4, 1);
      }
      // a ridge line where the rock meets the sky
      g.fillStyle = '#9a8fa6'; g.beginPath(); g.moveTo(0, 70); for (let x = 0; x <= 320; x += 16) g.lineTo(x, 64 + ((x / 16) % 3) * 3); g.lineTo(320, 80); g.lineTo(0, 80); g.fill();
      // the crystal cave mouth under the bonus
      g.fillStyle = '#3a2d44'; g.beginPath(); g.ellipse(C(10), C(13) + 4, 26, 14, 0, P, 0); g.fill();
      // cave mouth for 3-1
      g.fillStyle = '#3a2d44'; g.beginPath(); g.ellipse(C(6), C(12) - 4, 12, 10, 0, P, 0); g.fill();
      // boulders and crystal clusters
      const q = rng(3);
      [[1, 6], [4, 10], [8, 10], [12, 7], [13, 11], [16, 8], [18, 11], [17, 13], [3, 14], [12, 13], [19, 6], [8, 6]].forEach(([c, r]) => boulder(g, C(c) + (q() - 0.5) * 6, C(r), 4 + q() * 3, '#6f6480'));
      [[4, 6, 0], [8, 13, 1], [12, 12, 0], [15, 10, 1], [18, 9, 0], [1, 10, 1], [11, 14, 1], [9, 14, 0]].forEach(([c, r, h]) => crystals(g, C(c), C(r) + 6, 0.9, h));
      // the mine cart track
      const { x0, y0, x1, y1 } = TRACK;
      g.strokeStyle = '#6b4325'; g.lineWidth = 6; g.strokeRect(x0, y0, x1 - x0, y1 - y0);
      g.strokeStyle = '#3a3346'; g.lineWidth = 1; g.strokeRect(x0 - 2, y0 - 2, x1 - x0 + 4, y1 - y0 + 4); g.strokeRect(x0 + 2, y0 + 2, x1 - x0 - 4, y1 - y0 - 4);
      // the rocket launch pad
      K.rr(C(17) - 14, C(4) + 6, 28, 6, 2, '#5d566a'); hazardStripe(g, C(17) - 14, C(4) + 11, 28, 2);
    },
    drawHome(g, n) {
      g.fillStyle = '#3a3346'; g.fillRect(n.x - 9, n.y - 22, 2, 28); g.fillRect(n.x + 7, n.y - 22, 2, 28); g.fillRect(n.x - 9, n.y - 22, 18, 2);
      K.rr(n.x - 7, n.y - 10, 14, 14, 2, '#7d8397');
      g.font = '7px "Titan One", sans-serif'; g.fillStyle = '#ffd23f'; g.textAlign = 'center'; g.fillText('W2', n.x, n.y);
    },
    drawExit(g, n, t, ok) {
      // the rocket to the Moon
      g.save(); g.globalAlpha = ok ? 1 : 0.65;
      const x = n.x, y = n.y - 6, wob = ok ? Math.sin(t * 20) * 0.4 : 0;
      g.fillStyle = '#c75b4a'; g.beginPath(); g.moveTo(x - 9, y + 10); g.lineTo(x - 5, y + 2); g.lineTo(x - 5, y + 10); g.fill(); g.beginPath(); g.moveTo(x + 9, y + 10); g.lineTo(x + 5, y + 2); g.lineTo(x + 5, y + 10); g.fill();
      K.rr(x - 5 + wob, y - 14, 10, 24, 5, '#eef1f6'); g.fillStyle = '#c75b4a'; g.beginPath(); g.moveTo(x - 5 + wob, y - 9); g.quadraticCurveTo(x + wob, y - 22, x + 5 + wob, y - 9); g.fill();
      K.circ(x + wob, y - 4, 2.5, ok ? '#5ad1ff' : '#8c8496'); K.circ(x + wob, y - 4, 1.2, '#fff');
      if (ok) { for (let i = 0; i < 3; i++) { const k = (t * 1.2 + i / 3) % 1; g.globalAlpha = 0.6 * (1 - k); K.circ(x - 6 + i * 6 + (i - 1) * k * 8, y + 12 + k * 2, 2 + k * 3, '#f2f2f2'); } }
      g.restore();
      g.font = '7px "Titan One", sans-serif'; g.fillStyle = '#ffd23f'; g.textAlign = 'center'; g.fillText('W4', n.x, n.y + 18);
    },
    drawBoss(g, n, t, fixed) {
      // Rumbleboulder: a huge rock bot rocking beside the peak panel
      const roll = fixed ? Math.sin(t * 1) * 0.05 : Math.sin(t * 1.6) * 0.35, x = n.x - 18 + roll * 14, y = n.y - 2;
      g.save(); g.translate(x, y); g.rotate(roll * 1.5);
      g.fillStyle = '#7a6e86'; g.beginPath();
      for (let i = 0; i < 10; i++) { const a = i / 10 * P * 2, r = 11 + (i % 3 ? 1.5 : -1.5); g.lineTo(Math.cos(a) * r, Math.sin(a) * r); }
      g.closePath(); g.fill(); K.circ(-4, 5, 2, '#695d75'); K.circ(5, -6, 1.6, '#695d75');
      g.restore();
      K.circ(x - 3.5, y - 2, 2.4, '#fff'); K.circ(x + 3.5, y - 2, 2.4, '#fff'); K.circ(x - 3.5, y - 1.8, 1.1, '#2b2230'); K.circ(x + 3.5, y - 1.8, 1.1, '#2b2230');
      if (!fixed) { g.fillStyle = '#7a6e86'; g.fillRect(x - 6.5, y - 5, 5, 1.6); g.fillRect(x + 1.5, y - 5, 5, 1.6); }
      K.arc(x, y + 1, 3, fixed ? P * 0.15 : P * 1.15, fixed ? P * 0.85 : P * 1.85, 1.2, '#2b2230');
      if (!fixed && Math.floor(t * 6) % 3 === 0) { g.strokeStyle = '#bff6ff'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 10, y - 10); g.lineTo(x + 13, y - 7); g.lineTo(x + 11, y - 6); g.lineTo(x + 14, y - 3); g.stroke(); }
    },
    drawLive(g, t) {
      // mine cart going round
      const [cx, cy, vert] = trackPoint(t * 0.08);
      K.rr(cx - 5, cy - 6, 10, 7, 1.5, '#6b4325'); g.fillStyle = '#3a3346'; g.fillRect(cx - 5, cy - 6, 10, 1.5);
      K.circ(cx - 3, cy + 1.5, 1.6, '#2b2833'); K.circ(cx + 3, cy + 1.5, 1.6, '#2b2833'); K.circ(cx - 1, cy - 7, 2, '#3bf0ff'); K.circ(cx + 2, cy - 7.5, 1.6, '#ff8fc7');
      // magnets with pulsing field lines
      for (const [mx, my] of [[C(12), C(10)], [C(3), C(3) + 26], [C(16), C(12)]]) {
        magnet(g, mx, my, 0.9);
        for (let i = 0; i < 2; i++) { const k = (t * 0.9 + i / 2) % 1; g.globalAlpha = 0.6 * (1 - k); K.arc(mx, my - 4, 8 + k * 10, P * 1.2, P * 1.8, 1, '#ff6b6b'); }
        g.globalAlpha = 1;
      }
      // crystal twinkles
      for (let i = 0; i < 5; i++) { const ph = (t * 0.8 + i * 0.37) % 1; if (ph < 0.2) K.star([C(4), C(8), C(12), C(15), C(18)][i] + 2, [C(6), C(13), C(12), C(10), C(9)][i] - 4, 2.5 * Math.sin(ph / 0.2 * P), '#ffffff'); }
      // wind streaks past the peak
      g.strokeStyle = 'rgba(255,255,255,0.7)'; g.lineWidth = 1;
      for (let i = 0; i < 4; i++) { const x = ((t * 60 + i * 90) % 400) - 40, y = 30 + i * 12; g.beginPath(); g.moveTo(x, y); g.quadraticCurveTo(x + 10, y - 3, x + 20, y); g.stroke(); }
      // a Swoop Bat circling
      const bx = C(9) + Math.cos(t * 0.9) * 24, by = C(5) - 6 + Math.sin(t * 1.8) * 6, fl = Math.sin(t * 14) * 0.6, c = K.shade(0x9b6bff, 1), w = K.shade(0x9b6bff, 0.7);
      for (const sd of [-1, 1]) { g.save(); g.translate(bx + sd * 3, by); g.rotate(sd * fl); g.fillStyle = w; g.beginPath(); g.moveTo(0, -1); g.lineTo(sd * 8, -3); g.lineTo(sd * 6, 1); g.lineTo(0, 2); g.fill(); g.restore(); }
      K.circ(bx, by, 3.6, c); K.circ(bx - 1.3, by - 0.5, 0.9, '#fff'); K.circ(bx + 1.3, by - 0.5, 0.9, '#fff');
    },
  };
};

// Level looks (see lib/levelart.js): snowy peaks over purple rock, mine beams and crystals.
LTHEMES.mountain = (() => {
  const K = CK, P = Math.PI;
  function crystal(g, x, y, k, pink) {
    g.fillStyle = pink ? '#ff8fc7' : '#3bf0ff';
    g.beginPath(); g.moveTo(x - 2 * k, y); g.lineTo(x - 2 * k, y - 4 * k); g.lineTo(x, y - 7 * k); g.lineTo(x + 2 * k, y - 4 * k); g.lineTo(x + 2 * k, y); g.fill();
    g.fillStyle = pink ? '#ffc2e0' : '#b8fbff'; g.fillRect(x - 1.5 * k, y - 4 * k, 0.8 * k, 3.5 * k);
  }
  return {
    debris: ['#b07a44', '#7d5128'],
    far(g, boss) {
      const gr = g.createLinearGradient(0, 0, 0, 240);
      gr.addColorStop(0, boss ? '#3b2a5e' : '#7fb6e8'); gr.addColorStop(0.6, boss ? '#b5607a' : '#c9e2f5'); gr.addColorStop(1, boss ? '#f2a37a' : '#eef6ff');
      g.fillStyle = gr; g.fillRect(0, 0, 640, 240);
      K.circ(500, 56, 20, boss ? 'rgba(255,200,170,0.8)' : 'rgba(255,248,220,0.9)');
      const peak = (x, w, h, c, snow) => {
        g.fillStyle = c; g.beginPath(); g.moveTo(x - w / 2, 240); g.lineTo(x, 240 - h); g.lineTo(x + w / 2, 240); g.fill();
        g.fillStyle = snow; g.beginPath(); g.moveTo(x - w * 0.12, 240 - h * 0.78); g.lineTo(x, 240 - h); g.lineTo(x + w * 0.12, 240 - h * 0.78);
        g.lineTo(x + w * 0.06, 240 - h * 0.72); g.lineTo(x, 240 - h * 0.8); g.lineTo(x - w * 0.06, 240 - h * 0.72); g.fill();
      };
      const far = boss ? '#6e5a8a' : '#a49ac0', near = boss ? '#5a4876' : '#8a7fa2';
      for (const [x, w, h] of [[40, 220, 150], [210, 260, 190], [400, 240, 160], [590, 260, 200]]) peak(x, w, h, far, '#f4f7ff');
      for (const [x, w, h] of [[120, 200, 120], [320, 220, 140], [500, 200, 116], [660, 200, 130]]) peak(x, w, h, near, '#ffffff');
    },
    mid(g) {
      const c = '#5d5070', d = '#4a3f5c';
      g.fillStyle = c; g.beginPath(); g.moveTo(0, 240);
      for (let x = 0; x <= 640; x += 16) g.lineTo(x, 186 + Math.sin(x * 0.031) * 10 + ((x / 16) % 3) * 3);
      g.lineTo(640, 240); g.fill();
      for (let i = 0; i < 12; i++) { const x = i * 54 + 18; K.circ(x, 200 + (i % 3) * 4, 8 + (i % 2) * 3, d); }
      for (let i = 0; i < 7; i++) crystal(g, i * 92 + 40, 204, 1.6, i % 2);
      g.fillStyle = c; g.fillRect(0, 212, 640, 28);
    },
    tile: LEVELART.tiles({
      ground: (g, i) => LEVELART.ground(g, i, { body: '#7a6e86', speck: '#695d75', top: '#a89cb6', rim: '#e9eef8', under: '#5f546c' }, (g, k) => {
        g.fillStyle = '#ffffff'; if (k === 0 || k === 3) { g.fillRect(2, 0, 5, 2.5); g.fillRect(10, 0, 4, 2); }   // snow caps
        if (k === 1) crystal(g, 11, 4, 0.8, false);
        if (k === 2) crystal(g, 5, 4, 0.8, true);
      }),
      girder(g) {
        // a mine beam with iron bands
        K.rr(0, 0, 16, 16, 1.5, '#8a5a32'); g.fillStyle = '#6b4325'; g.fillRect(0, 13, 16, 3); g.fillStyle = '#b07a44'; g.fillRect(0, 0, 16, 2);
        g.fillStyle = '#5d566a'; g.fillRect(3, 0, 2, 16); g.fillRect(11, 0, 2, 16); K.circ(4, 8, 1.2, '#b8a6b6'); K.circ(12, 8, 1.2, '#b8a6b6');
      },
      plank(g) {
        g.fillStyle = '#8a5a32'; g.fillRect(0, 0, 16, 5); g.fillStyle = '#c99060'; g.fillRect(0, 0, 16, 1.5);
        g.fillStyle = '#ffffff'; g.fillRect(1, 0, 4, 1.2); g.fillStyle = '#5e3a1e'; g.fillRect(7, 5, 2, 4); g.fillStyle = 'rgba(0,0,0,0.25)'; g.fillRect(15, 0, 1, 5);
      },
    }),
  };
})();
