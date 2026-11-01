// World map theme: Junkyard Hills. Paints the ground, the features and props that world.txt lists,
// and the home, boss and exit panels. Registered on THEMES; the world loader merges it into the world.
THEMES.junkyard = (W) => {
  const K = CK, P = Math.PI, T = 16, C = c => c * T + 8;
  const rng = s => () => (s = (s * 16807) % 2147483647) / 2147483647;
  const feat = n => W.features.find(f => f.name === n);
  const river = feat('river'), next = feat('next-world'), RIVER = river ? +river.args.col : -9;
  const PROPS = {
    pile(g, c, r, seed) {
      const x = C(c), y = C(r) + 5, w = 15, h = 9, q = rng(seed);
      g.fillStyle = 'rgba(40,20,20,0.25)'; g.beginPath(); g.ellipse(x, y + 1, 8, 2.5, 0, 0, P * 2); g.fill();
      g.fillStyle = '#857585'; g.beginPath(); g.ellipse(x, y, w / 2, h, 0, P, 0); g.fill();
      g.fillStyle = '#9b8a99'; g.beginPath(); g.ellipse(x - 1.5, y - 1.5, w * 0.3, h * 0.65, 0, P, 0); g.fill();
      const k = q();
      if (k < 0.4) { K.circ(x + 2, y - 5, 2.6, '#2b2833'); K.circ(x + 2, y - 5, 1, '#5b5666'); }
      else if (k < 0.7) K.gear(x - 2, y - 6, 3, 7, q() * 3, '#c9b8c7', '#857585');
      else { g.fillStyle = '#c96a4a'; g.fillRect(x - 3, y - 6, 6, 2.5); }
    },
    car(g, c, r) {
      const x = C(c), y = C(r);
      g.fillStyle = 'rgba(40,20,20,0.25)'; g.beginPath(); g.ellipse(x, y + 6, 8, 2, 0, 0, P * 2); g.fill();
      K.rr(x - 7.5, y - 1, 15, 6, 2.5, '#5f86a8'); K.rr(x - 4.5, y - 5.5, 9, 5, 2, '#5f86a8');
      K.rr(x - 3.5, y - 4.5, 3, 3, 0.5, '#c6e2f2'); K.rr(x + 0.5, y - 4.5, 3, 3, 0.5, '#c6e2f2');
      K.circ(x - 4.5, y + 5, 2, '#2b2833'); K.circ(x + 4.5, y + 5, 2, '#2b2833');
    },
    tyres(g, c, r) { const x = C(c), y = C(r) + 5; for (let i = 0; i < 3; i++) { K.rr(x - 5, y - 3 - i * 3, 10, 3.6, 1.8, '#2b2833'); K.rr(x - 2.5, y - 2.4 - i * 3, 5, 1.4, 0.7, '#4a4652'); } },
    barrel(g, c, r) { const x = C(c), y = C(r); K.rr(x - 4, y - 5, 8, 11, 2, '#c75b4a'); g.fillStyle = '#9b4033'; g.fillRect(x - 4, y - 2, 8, 1); g.fillRect(x - 4, y + 2, 8, 1); },
  };
  return {
    propKinds: Object.keys(PROPS),
    featureKinds: ['river', 'next-world', 'fence', 'windmill', 'wanderer'],
    road: '#ecc791',
    drawGround(g) {
      for (let c = 0; c < 20; c++) for (let r = 0; r < 15; r++) { g.fillStyle = (c + r) % 2 ? '#d29a60' : '#d8a26a'; g.fillRect(c * T, r * T, T, T); }
      const q = rng(5);
      for (let i = 0; i < 220; i++) { g.fillStyle = q() > 0.5 ? 'rgba(130,75,40,0.35)' : 'rgba(255,225,170,0.35)'; g.fillRect((q() * 20 | 0) * T + q() * 15, q() * 240, 1, 1); }
      if (next) {
        const nc = +next.args.col;
        for (let c = nc; c < 20; c++) for (let r = 0; r < 15; r++) { g.fillStyle = (c + r) % 2 ? '#8f95a8' : '#979db0'; g.fillRect(c * T, r * T, T, T); g.fillStyle = 'rgba(0,0,0,0.12)'; g.fillRect(c * T, r * T + T - 1, T, 1); g.fillRect(c * T + T - 1, r * T, 1, T); }
        K.rr(C(nc) + 4, 18, 58, 38, 3, '#5d6378'); g.fillStyle = '#5d6378'; g.beginPath(); g.moveTo(C(nc) + 4, 20); for (let i = 0; i < 5; i++) { g.lineTo(C(nc) + 4 + i * 11.6 + 5.8, 10); g.lineTo(C(nc) + 4 + (i + 1) * 11.6, 20); } g.fill();
        g.fillRect(C(nc + 2) + 6, 0, 7, 20);
        for (let i = 0; i < 4; i++) K.rr(C(nc) + 10 + i * 12, 30, 7, 8, 1, '#ffd37a');
        K.gear(C(nc + 1), 48, 6, 8, 0.3, '#ffd23f', '#5d6378');
      }
      if (river) {
        const bridge = +river.args.bridge;
        g.fillStyle = '#6d4a36'; g.fillRect(RIVER * T - 2, 0, T + 4, 240);
        g.fillStyle = '#4f86b0'; g.fillRect(RIVER * T, 0, T, 240);
        g.fillStyle = '#6fa6cf'; for (let r = 0; r < 15; r++) g.fillRect(RIVER * T + 3 + (r % 2) * 6, r * T + 6, 5, 1);
        g.fillStyle = '#8a5a32'; g.fillRect(RIVER * T - 3, bridge * T + 2, T + 6, 12);
        g.fillStyle = '#6b4325'; for (let i = 0; i < 6; i++) g.fillRect(RIVER * T - 2 + i * 3.5, bridge * T + 2, 1, 12);
        g.fillStyle = '#5e3a1e'; g.fillRect(RIVER * T - 3, bridge * T + 1, T + 6, 1.5); g.fillRect(RIVER * T - 3, bridge * T + 13.5, T + 6, 1.5);
      }
      let seed = 0;
      for (const pr of W.props) if (PROPS[pr.name]) PROPS[pr.name](g, pr.col, pr.row, pr.name === 'pile' ? ++seed : 0);
      for (const f of W.features.filter(f => f.name === 'fence')) {
        const x0 = +f.args.from * T + 8, x1 = (+f.args.to + 1) * T + 10, y = C(+f.args.row) + 2;
        g.fillStyle = '#7b6a62'; for (let x = x0; x < x1; x += 6) g.fillRect(x, y, 2, 8); g.fillRect(x0, y + 3, x1 - x0, 1.5);
      }
    },
    drawHome(g, n) {
      K.rr(n.x - 9, n.y - 11, 18, 13, 2, '#c75b4a'); g.fillStyle = '#9b4033';
      g.beginPath(); g.moveTo(n.x - 11, n.y - 10); g.lineTo(n.x, n.y - 18); g.lineTo(n.x + 11, n.y - 10); g.fill();
      K.rr(n.x - 6, n.y - 6, 12, 8, 1, '#e8d6b8'); g.fillStyle = '#c9b391'; for (let i = 0; i < 3; i++) g.fillRect(n.x - 6, n.y - 5 + i * 2.6, 12, 1);
      K.circ(n.x, n.y - 13, 1.6, '#ffd23f');
    },
    drawExit(g, n, t, ok) {
      g.save(); g.globalAlpha = ok ? 1 : 0.6;
      K.rr(n.x - 8, n.y - 8, 16, 16, 3, ok ? '#49d17a' : '#8c8496'); K.rr(n.x - 6.5, n.y - 6.5, 13, 6, 2, 'rgba(255,255,255,0.3)');
      g.font = '9px "Titan One", sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillStyle = '#2b2230'; g.fillText('W' + (W.num + 1), n.x, n.y + 1); g.textBaseline = 'alphabetic';
      g.restore();
    },
    drawBoss(g, n, t, fixed) {
      const bx = n.x + 2, by = n.y - 8, sw = fixed ? Math.sin(t * 1.2) * 0.12 : Math.sin(t * 2.4) * 0.35;
      g.fillStyle = '#d4892f'; g.fillRect(bx - 2.5, by - 34, 5, 34);
      g.strokeStyle = '#a5651f'; g.lineWidth = 1; for (let y = by - 34; y < by - 2; y += 5) { g.beginPath(); g.moveTo(bx - 2.5, y); g.lineTo(bx + 2.5, y + 5); g.stroke(); }
      g.fillStyle = '#d4892f'; g.fillRect(bx - 22, by - 36, 30, 3.5);
      K.rr(bx - 5, by - 43, 10, 8, 2, fixed ? '#49d17a' : '#e04f4f');
      K.circ(bx - 2, by - 39, 1.3, '#fff'); K.circ(bx + 2, by - 39, 1.3, '#fff');
      g.save(); g.translate(bx - 19, by - 33); g.rotate(sw);
      g.strokeStyle = '#3a3346'; g.beginPath(); g.moveTo(0, 0); g.lineTo(0, 12); g.stroke();
      K.arc(0, 16, 4, P * 1.05, P * 1.95, 2.5, '#e04f4f'); g.restore();
      if (!fixed && Math.floor(t * 6) % 3 === 0) { g.strokeStyle = '#bff6ff'; g.beginPath(); g.moveTo(bx + 6, by - 42); g.lineTo(bx + 9, by - 39); g.lineTo(bx + 7, by - 38); g.lineTo(bx + 10, by - 35); g.stroke(); }
    },
    drawLive(g, t) {
      if (river) {
        const bridge = +river.args.bridge; g.fillStyle = 'rgba(255,255,255,0.5)';
        for (let i = 0; i < 5; i++) { const y = ((t * 18 + i * 48) % 240); if (y > bridge * T - 2 && y < (bridge + 1) * T + 2) continue; g.fillRect(RIVER * T + 5, y, 4, 1); }
      }
      if (next) { const nc = +next.args.col; for (let i = 0; i < 3; i++) { const k = (t * 0.4 + i / 3) % 1; g.globalAlpha = 0.6 * (1 - k); K.circ(C(nc + 2) + 10 + k * 8, 4 - k * 12, 3 + k * 5, '#d9dbe4'); } g.globalAlpha = 1; }
      for (const f of W.features.filter(f => f.name === 'windmill')) { const [c, r] = f.pos; g.fillStyle = '#7b6a62'; g.fillRect(C(c) - 1, C(r) - 4, 2, 14); K.gear(C(c), C(r) - 4, 6, 8, t * 1.5, '#b8a6b6', '#7b6a62'); }
      for (const f of W.features.filter(f => f.name === 'wanderer')) {
        const [c, r] = f.pos, bx = C(c) + Math.sin(t * 0.8) * 9, by = C(r) - 2;
        const B = 0xFF7A59, cc = K.shade(B, 1), d = K.shade(B, 0.22);
        K.circ(bx, by + 6, 2.5, '#2b2833'); K.circ(bx, by + 1, 5, cc); K.circ(bx - 1.8, by, 1.2, '#fff'); K.circ(bx + 1.8, by, 1.2, '#fff');
        K.circ(bx - 1.8, by + 0.2, 0.6, d); K.circ(bx + 1.8, by + 0.2, 0.6, d); g.fillStyle = cc; g.fillRect(bx - 0.5, by - 7, 1, 3); K.circ(bx, by - 7, 1.1, '#ff3b3b');
      }
    },
  };
};
