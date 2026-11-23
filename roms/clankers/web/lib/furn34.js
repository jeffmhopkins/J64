// The house furniture, drawn for the three-quarter top-down view (web/lib/housetop.js places it;
// tools/bake_sprites.mjs bakes each piece for the N64).
// Each thing is a set of boxes: a front face you look at, and its top, squashed to V of its
// real depth. Origin (0, 0) is the middle of the thing's front edge, on the floor. Colours are
// the ones the side-view furniture in houseart.js already uses.
const FURN34 = (() => {
  const K = CK, P = Math.PI, V = 0.6;
  const g = () => K.g;
  // a box w wide, d deep, h tall, standing with its front-bottom middle at (x, y)
  const box = (x, y, w, d, h, front, top, r = 2) => { K.rr(x - w / 2, y - h, w, h, r, front); K.rr(x - w / 2, y - h - d * V, w, d * V + 1, r, top); };
  const ell = (x, y, rx, ry, c) => { const G = g(); G.fillStyle = c; G.beginPath(); G.ellipse(x, y, rx, Math.max(0.1, ry), 0, 0, P * 2); G.fill(); };
  const ring = (x, y, rx, ry, c, w = 1.5) => { const G = g(); G.strokeStyle = c; G.lineWidth = w; G.beginPath(); G.ellipse(x, y, rx, ry, 0, 0, P * 2); G.stroke(); };
  // a round thing (pot, drum) r across, h tall
  const cyl = (x, y, r, h, side, top) => { ell(x, y, r, r * 0.45, side); K.rr(x - r, y - h, r * 2, h, 0, side); ell(x, y - h, r, r * 0.45, top); };
  const line = (x0, y0, x1, y1, c, w = 2) => { const G = g(); G.strokeStyle = c; G.lineWidth = w; G.lineCap = 'round'; G.beginPath(); G.moveTo(x0, y0); G.lineTo(x1, y1); G.stroke(); };

  const A = {};
  // ---- kitchen
  A.fridge = () => {
    box(0, 0, 36, 24, 74, '#e8eef5', '#f8fbff', 4);
    K.rr(-18, -48, 36, 2, 0, '#b9c3cf'); K.rr(10, -68, 3, 14, 1, '#9aa1b0'); K.rr(10, -42, 3, 18, 1, '#9aa1b0');
    K.circ(-8, -64, 3, '#ff5a5a'); K.circ(0, -60, 3, '#ffd23f'); K.rr(-12, -40, 10, 12, 1, '#ffffff'); K.rr(-11, -38, 8, 2, 0, '#5ad1ff'); K.rr(-11, -34, 6, 2, 0, '#ff8fc7');
  };
  A.stove = t => {
    box(0, 0, 42, 26, 38, '#6f7684', '#3d4250');
    K.rr(-14, -28, 28, 18, 3, '#2b2833'); K.rr(-12, -26, 24, 3, 1, '#ff8a3d');
    for (let k = 0; k < 4; k++) K.circ(-13 + k * 8.7, -34, 1.6, '#9aa1b0');
    for (const [x, y] of [[-10, -49], [10, -49], [-10, -42], [10, -42]]) ring(x, y, 6, 2.6, '#9aa1b0');
    ell(9, -42, 4.5, 2, '#ff8a3d');
    cyl(-10, -46, 8, 9, '#9aa1b0', '#c9ced8'); ell(-10, -55, 5.5, 2.2, '#e0e4ea'); K.rr(-2, -54, 9, 2, 1, '#4a505c');
    const G = g(), u = (t || 0) % 1.5 / 1.5; G.globalAlpha = 0.6 * (1 - u); K.circ(-10, -62 - u * 12, 3, '#ffffff'); K.circ(-6, -68 - u * 12, 2.5, '#ffffff'); G.globalAlpha = 1;
  };
  A.counter = () => {
    box(0, 0, 76, 26, 34, '#d8d0c0', '#b9c3cf');
    for (let k = 0; k < 3; k++) { K.rr(-35 + k * 24, -30, 22, 26, 2, '#e8e0d0'); K.circ(-24 + k * 24 + (k === 1 ? 6 : 0), -17, 1.4, '#9aa1b0'); }
    ell(12, -42, 13, 4.2, '#5a6a7a'); ell(12, -41, 10, 3, '#7d8a98');
    K.rr(10, -58, 3, 12, 1, '#9aa1b0'); K.rr(10, -58, 9, 3, 1, '#9aa1b0');
    K.rr(-30, -47, 16, 6, 2, '#ffffff'); K.circ(-22, -46, 2.5, '#49d17a');   // a chopping board with a lime
  };
  A.table = () => {
    const chairBack = (x, y) => { K.rr(x - 8, y - 12 - 8 - 18, 16, 18, 3, '#8a5a3c'); K.rr(x - 6, y - 36, 12, 3, 1, '#c98a4b'); box(x, y, 16, 14, 12, '#8a5a3c', '#c98a4b'); };
    const chairSide = (x, y, d) => { box(x, y, 14, 16, 12, '#8a5a3c', '#c98a4b'); K.rr(x + d * 6 - 2, y - 30, 4, 20, 2, '#8a5a3c'); };
    chairBack(0, -40);
    chairSide(-40, -10, -1); chairSide(40, -10, 1);
    box(0, 0, 66, 46, 22, '#8a5a3c', '#c98a4b', 3);
    K.rr(-30, -48, 60, 24, 3, '#d99a5b');
    for (const [x, c] of [[-16, '#ff8a3d'], [16, '#49d17a']]) { ell(x, -36, 9, 4, '#ffffff'); ell(x, -36.5, 5, 2.2, c); }
    cyl(0, -38, 3, 7, '#5ad1ff', '#bfefff');
    box(0, 16, 16, 14, 12, '#8a5a3c', '#c98a4b'); K.rr(-8, -2, 16, 14, 3, '#6f4630'); K.rr(-6, 0, 12, 3, 1, '#8a5a3c');   // the near chair, its back to us
  };

  // ---- bedroom
  // a bed, headboard to the wall. c: [blanket, stripe, edge]; wide: a double with two pillows
  A.bed = (c, wide) => {
    if (!Array.isArray(c)) c = ['#5ad1ff', '#8fe0ff', '#3fb3e0'];
    const w = wide ? 84 : 52;
    box(0, -78, w + 4, 6, 34, '#6f4630', '#8a5a3c', 3);
    box(0, 0, w + 2, 112, 14, '#8a5a3c', '#ffffff', 4);
    if (wide) { K.rr(-36, -80, 32, 14, 6, '#fff6d8'); K.rr(4, -80, 32, 14, 6, '#fff6d8'); } else K.rr(-18, -80, 36, 14, 6, '#fff6d8');
    K.rr(-w / 2 - 1, -60, w + 2, 46, 4, c[0]); for (let x = -w / 2 + 3; x < w / 2 - 4; x += 12) K.rr(x, -60, 5, 46, 0, c[1]);
    K.rr(-w / 2 - 1, -63, w + 2, 6, 3, '#ffffff');
    K.rr(-w / 2 - 1, -14, w + 2, 4, 2, c[2]);
  };
  A.bigBed = c => A.bed(c, true);
  A.lamp = () => {
    ell(0, -1, 9, 3.4, '#7d8494'); K.rr(-1.5, -54, 3, 52, 1, '#7d8494');
    const G = g(); G.fillStyle = '#ffe9a8'; G.beginPath(); G.moveTo(-12, -50); G.lineTo(12, -50); G.lineTo(7, -66); G.lineTo(-7, -66); G.fill();
    ell(0, -50, 12, 3, '#f2d27a'); ell(0, -66, 7, 2.4, '#fff6d8');
  };
  A.wardrobe = () => { box(0, 0, 50, 26, 80, '#c98a4b', '#d99a5b', 3); K.rr(-1, -78, 2, 76, 0, '#8a5a3c'); K.circ(-5, -42, 2, '#ffd23f'); K.circ(5, -42, 2, '#ffd23f'); K.rr(-22, -8, 44, 4, 1, '#8a5a3c'); };
  A.rugRound = (c1, c2) => { ell(0, 0, 44, 22, c1); ell(0, 0, 36, 17, c2); };

  // ---- music room
  A.piano = () => {
    box(0, -18, 76, 22, 60, '#2b2833', '#3d3a48', 3);
    K.rr(-34, -72, 68, 20, 3, '#3d3a48');
    box(0, -30, 76, 16, 4, '#1b1426', '#fff6d8', 1);                  // the keys, from above
    const ky = -30 - 4 - 16 * V;
    for (let i = 1; i < 14; i++) K.rr(-38 + i * 5.43, ky, 0.8, 16 * V, 0, '#c9c4b0');
    for (const i of [0, 1, 3, 4, 5, 7, 8, 10, 11, 12]) K.rr(-35.5 + i * 5.43, ky, 3, 16 * V * 0.6, 0.5, '#1b1426');
    K.rr(-34, -30, 4, 12, 1, '#1b1426'); K.rr(30, -30, 4, 12, 1, '#1b1426');
    K.rr(-12, -88, 24, 12, 2, '#fff6d8'); for (let k = 0; k < 3; k++) K.rr(-9, -85 + k * 3, 18, 1, 0, '#9aa1b0');
    box(0, 0, 38, 12, 14, '#4a3020', '#6f4630', 2);                   // the bench
  };
  A.drums = t => {
    const lit = Math.max(0, Math.sin((t || 0) * 3)) * 0;
    box(0, -40, 14, 12, 14, '#2b2833', '#4a4a55', 4);                // stool behind
    line(-32, -2, -32, -50, '#c9b8c7'); line(32, -2, 32, -56, '#c9b8c7');
    ell(-32, -52, 13, 4, '#e0a92b'); ell(-32, -53, 11, 3, '#ffd23f');
    ell(32, -58, 12, 4, '#e0a92b'); ell(32, -59, 10, 3, '#ffd23f');
    cyl(-24, -6, 9, 10, '#e04f4f', '#fff6d8'); ring(-24, -16, 9, 4, '#c0303a', 1.2);   // snare
    cyl(24, -4, 10, 16, '#e04f4f', '#fff6d8');                                         // floor tom
    K.circ(0, -16, 16, '#c0303a'); K.circ(0, -16, 13, '#fff6d8'); K.circ(0, -16, 5 + lit, '#ffd23f');   // bass drum, facing us
    cyl(-10, -34, 8, 6, '#e04f4f', '#fff6d8'); cyl(10, -34, 8, 6, '#e04f4f', '#fff6d8');               // two toms on top
    line(-14, -24, -4, -30, '#c9b8c7', 1.5); line(14, -24, 4, -30, '#c9b8c7', 1.5);
  };

  // ---- paint studio
  A.easel = () => {
    line(0, -18, 0, -66, '#6f4630', 3);
    line(-18, 2, -4, -66, '#8a5a3c', 3); line(18, 2, 4, -66, '#8a5a3c', 3);
    box(0, -30, 44, 4, 36, '#fff6d8', '#e8dcc0', 2);
    for (const [x, y, c, r] of [[-10, -56, '#ff5a5a', 5], [4, -60, '#ffd23f', 6], [12, -48, '#49d17a', 4], [-6, -42, '#5ad1ff', 5], [8, -38, '#c28bff', 3]]) K.circ(x, y, r, c);
    K.rr(-22, -30, 44, 3, 1, '#8a5a3c');
    cyl(-26, 6, 5, 8, '#9aa1b0', '#ff5a5a'); cyl(-14, 10, 5, 8, '#9aa1b0', '#5ad1ff'); cyl(26, 8, 5, 8, '#9aa1b0', '#ffd23f');
    ell(14, 14, 10, 4, '#e8c98a'); for (const [x, c] of [[9, '#ff5a5a'], [14, '#49d17a'], [19, '#c28bff']]) K.circ(x, 13, 1.6, c);   // the palette on the floor
  };
  A.paintShelf = () => { box(0, 0, 48, 18, 54, '#9c7552', '#b88a64', 2); for (let r = 0; r < 3; r++) { K.rr(-22, -50 + r * 17, 44, 2, 0, '#6f4630'); for (let k = 0; k < 5; k++) cyl(-17 + k * 8.5, -36 + r * 17, 3, 9, ['#ff5a5a', '#ffd23f', '#49d17a', '#5ad1ff', '#c28bff'][(k + r) % 5], '#fff6d8'); } };

  // ---- living room
  A.tank = t => {
    box(0, 0, 90, 30, 24, '#6f4f3a', '#8a5f45', 3);
    K.rr(-36, -20, 72, 14, 2, '#5a3f2e');
    K.rr(-45, -86, 90, 62, 4, '#d8f2ff'); K.rr(-42, -82, 84, 56, 2, '#4fb3e3'); K.rr(-42, -34, 84, 8, 2, '#e8c98a');
    for (const x of [-30, -16, 28]) { const G = g(); G.strokeStyle = '#2f9a5a'; G.lineWidth = 2.5; G.beginPath(); G.moveTo(x, -34); G.quadraticCurveTo(x - 4 + Math.sin((t || 0) * 2 + x) * 2, -46, x + 1, -58); G.stroke(); }
    K.rr(12, -40, 12, 7, 3, '#8a7a6a'); K.rr(28, -42, 10, 9, 4, '#a89a88');
    K.rr(-46, -86 - 18, 92, 20, 4, '#d8f2ff'); K.rr(-43, -86 - 16, 86, 16, 3, '#8fd3ff');   // the water's top, seen from above
    const G = g(); G.globalAlpha = 0.5; K.rr(-40, -80, 6, 40, 3, '#ffffff'); K.rr(-30, -98, 24, 3, 1, '#ffffff'); G.globalAlpha = 1;
  };
  A.sofa = () => {
    box(0, -32, 84, 12, 30, '#3a6fc0', '#5a8fe0', 6);
    box(0, 0, 78, 36, 14, '#2b4f8c', '#5a8fe0', 4);
    K.rr(-1, -34, 2, 20, 0, '#3a6fc0');
    for (const x of [-38, 38]) box(x, 0, 12, 46, 22, '#2b4f8c', '#3a6fc0', 4);
    K.rr(-30, -40, 14, 10, 4, '#ffd23f'); K.rr(16, -40, 14, 10, 4, '#ff8fc7');
  };
  A.coffee = () => { box(0, 0, 46, 26, 12, '#8a5a3c', '#c98a4b', 3); cyl(10, -20, 3.2, 5, '#ffffff', '#6f4630'); K.rr(-16, -24, 14, 8, 1, '#e04f4f'); K.rr(-15, -23, 12, 2, 0, '#fff6d8'); };
  A.bookshelf = () => {
    box(0, 0, 54, 18, 82, '#6f4630', '#8a5a3c', 3); K.rr(-24, -78, 48, 74, 2, '#4a3020');
    const cols = ['#e04f4f', '#ffd23f', '#5ad1ff', '#49d17a', '#c28bff', '#ff8a3d', '#fff6d8'];
    for (let r = 0; r < 3; r++) { K.rr(-24, -54 + r * 24, 48, 3, 0, '#6f4630'); for (let k = 0, x = -22; x < 20; k++) { const w = 4 + (k * 7 + r * 3) % 4, h = 14 + (k * 5 + r) % 6; K.rr(x, -54 + r * 24 - h, w, h, 1, cols[(k + r * 2) % 7]); x += w + 1; } }
  };

  // ---- arcade: three cabinets in their own colours, a pinball table, beanbags
  function cabinet(body, top, marquee, panel, screen) {
    box(0, 0, 38, 28, 90, body, top, 4);
    K.rr(-19, -90, 38, 13, 3, marquee);
    K.rr(-15, -74, 30, 24, 2, '#1b1426'); screen();
    box(0, -36, 42, 16, 6, panel, K.shade(parseInt(panel.slice(1), 16), 1.3), 2);
    K.rr(-12, -52, 2, 6, 1, '#9aa1b0'); K.circ(-11, -52, 3, '#ff5a5a'); K.circ(6, -46, 2.2, '#ffd23f'); K.circ(12, -46, 2.2, '#49d17a');
    K.rr(-7, -24, 14, 11, 2, panel); K.rr(-2, -21, 4, 4, 1, '#ffd23f');
  }
  A.blast = () => cabinet('#3a6fc0', '#4f86d8', '#ffd23f', '#2b4f8c', () => { for (let i = 0; i < 4; i++) { K.circ(-9 + i * 6, -68, 1.8, i % 2 ? '#c28bff' : '#49d17a'); K.circ(-9 + i * 6, -62, 1.8, '#ff5a5a'); } K.rr(-2, -55, 4, 3, 1, '#5ad1ff'); });
  A.bolt = () => cabinet('#2f9e6a', '#3fb87e', '#1b1426', '#1f6e4a', () => { K.rr(-12, -70, 2, 9, 1, '#ffd23f'); K.rr(10, -66, 2, 9, 1, '#8fd3ff'); K.circ(2, -63, 1.8, '#fff6d8'); for (let y = -72; y < -52; y += 5) K.rr(-0.5, y, 1, 2.5, 0, '#46386e'); });
  A.jetrun = () => cabinet('#e0564f', '#f07068', '#ffd23f', '#a83a34', () => { K.rr(-15, -74, 30, 24, 2, '#7fc8ff'); K.rr(-15, -56, 30, 6, 1, '#8a5233'); K.rr(4, -74, 6, 9, 1, '#7d9a8a'); K.rr(4, -62, 6, 6, 1, '#7d9a8a'); K.circ(-6, -64, 3, '#5ad1ff'); K.circ(-11, -61, 1.6, '#ff8a3d'); });
  A.pinball = () => {
    for (const x of [-20, 16]) K.rr(x, -14, 4, 14, 1, '#2b2833');
    box(0, -12, 44, 96, 10, '#8a2a4b', '#c23b6b', 3);
    const y0 = -22 - 96 * V + 4, y1 = -24;
    K.rr(-18, y0, 36, y1 - y0, 3, '#1b1426');
    for (const [x, y, c] of [[-8, y0 + 12, '#ffd23f'], [8, y0 + 12, '#5ad1ff'], [0, y0 + 22, '#ff8fc7']]) { K.circ(x, y, 4.5, c); K.circ(x, y, 2, '#fff6d8'); }
    K.rr(13, y0 + 4, 2, y1 - y0 - 8, 1, '#46386e'); K.circ(14, y1 - 6, 2, '#c9ced8');
    const G = g(); G.save(); G.translate(-6, y1 - 5); G.rotate(0.35); K.rr(-1, -1.5, 9, 3, 1.5, '#ffd23f'); G.restore(); G.save(); G.translate(6, y1 - 5); G.rotate(P - 0.35); K.rr(-1, -1.5, 9, 3, 1.5, '#ffd23f'); G.restore();
    // the lit backbox standing at the far end
    box(0, y0 - 2, 44, 10, 36, '#c23b6b', '#a02a5a', 4);
    K.rr(-18, y0 - 34, 36, 26, 3, '#1b1426'); K.circ(-9, y0 - 24, 4, '#ffd23f'); K.circ(0, y0 - 20, 5, '#ff5a5a'); K.circ(9, y0 - 24, 4, '#49d17a');
    for (let i = 0; i < 5; i++) K.circ(-12 + i * 6, y0 - 12, 1.3, i % 2 ? '#fff6d8' : '#ffd23f');
  };
  A.beanbag = c => { ell(0, -6, 17, 10, K.shade(parseInt(c.slice(1), 16), 0.75)); ell(0, -9, 15, 8, c); ell(-4, -12, 6, 3, K.shade(parseInt(c.slice(1), 16), 1.25)); };

  // ---- garage
  A.kart = () => {
    // the go-kart from above, nose toward us (and the garage door)
    for (const [x, y] of [[-22, -50], [22, -50]]) box(x, y, 12, 18, 10, '#1b1426', '#2b2833', 3);
    box(0, -54, 44, 6, 8, '#2b2833', '#4a4a55', 2);                                      // the spoiler
    box(0, -2, 34, 76, 9, '#b83838', '#e04f4f', 8);
    K.rr(-3, -52, 6, 46, 2, '#fff6d8');
    box(0, -30, 18, 14, 9, '#1b1426', '#2b2833', 4);                                     // seat
    ring(0, -20, 6, 2.6, '#2b2833', 2); K.rr(-0.8, -20, 1.6, 4, 0, '#2b2833');         // steering wheel
    for (const [x, y] of [[-21, -6], [21, -6]]) box(x, y, 11, 14, 9, '#1b1426', '#2b2833', 3);
    K.circ(-9, -6, 2.2, '#ffd23f'); K.circ(9, -6, 2.2, '#ffd23f');
    K.circ(0, -40, 5, '#fff6d8'); K.rr(-1, -43, 2, 6, 0, '#e04f4f');                    // its number, 1
  };
  A.bench = () => {
    K.rr(-30, -96, 60, 42, 3, '#8a7a6a'); for (let i = 0; i < 7; i++) for (let j = 0; j < 4; j++) K.circ(-25 + i * 8, -90 + j * 10, 1, '#5a4e42');
    K.rr(-22, -88, 4, 18, 1, '#e04f4f'); K.rr(-14, -86, 12, 4, 1, '#9aa1b0'); K.rr(4, -90, 3, 20, 1, '#ffd23f'); K.rr(12, -84, 8, 8, 4, '#9aa1b0');
    box(0, 0, 74, 28, 30, '#8a5a3c', '#c98a4b', 2); K.rr(-34, -26, 68, 3, 0, '#6f4630');
    box(20, -32, 22, 12, 10, '#e04f4f', '#ff8a8a', 2); K.rr(17, -50, 6, 3, 1, '#2b2833');
    box(-20, -34, 12, 10, 8, '#4a505c', '#7d8494', 2);                                   // the vise
  };
  A.shelf = () => {
    box(0, 0, 58, 20, 68, '#7d8494', '#9aa1b0', 2); K.rr(-26, -64, 52, 60, 1, '#4a505c');
    for (let r = 0; r < 3; r++) { K.rr(-27, -44 + r * 21, 54, 3, 0, '#9aa1b0'); for (let k = 0; k < 3; k++) if ((k + r) % 4) box(-17 + k * 17, -44 + r * 21, 14, 8, 12, '#c9a35a', '#e0c080', 1); }
    cyl(-14, 0, 7, 16, '#e04f4f', '#ff8a8a'); cyl(14, 0, 6, 10, '#3a6fc0', '#5a8fe0');   // a petrol can and a paint pot
  };

  // ---- the yard
  A.tree = () => {
    ell(0, -2, 10, 4, '#6f4630'); K.rr(-6, -40, 12, 40, 4, '#8a5a3c');
    for (const [x, y, r] of [[-22, -62, 20], [20, -64, 22], [0, -80, 24], [-10, -46, 16], [14, -46, 16]]) K.circ(x, y, r, '#3f9d4e');
    for (const [x, y, r] of [[-14, -78, 12], [8, -90, 11], [24, -70, 9], [-26, -60, 8]]) K.circ(x, y, r, '#5cc46a');
    for (const [x, y] of [[-14, -58], [16, -76], [4, -64], [-24, -72], [22, -52]]) { K.circ(x, y, 3.2, '#ff5a5a'); K.circ(x - 1, y - 1, 1, '#ff9a9a'); }
  };
  A.mailbox = () => {
    K.rr(-2, -30, 4, 30, 1, '#7a5a40');
    K.rr(-9, -42, 18, 14, 4, '#3a5fb0'); K.rr(-9, -50, 18, 12, 6, '#4f7ad1'); K.rr(-6, -38, 12, 6, 2, '#2b4f8c');
    K.rr(9, -54, 3, 14, 1, '#e04f4f'); K.rr(9, -54, 8, 5, 1, '#e04f4f');
  };
  A.sign = () => {
    for (const x of [-14, 12]) K.rr(x, -34, 3, 34, 1, '#6f4630');
    K.rr(-24, -46, 48, 4, 2, '#e8b46a'); K.rr(-24, -42, 48, 16, 3, '#c98a4b');
    const G = g(); G.fillStyle = '#6f4630'; G.beginPath(); G.moveTo(-16, -34); G.lineTo(-8, -40); G.lineTo(-8, -37); G.lineTo(14, -37); G.lineTo(14, -31); G.lineTo(-8, -31); G.lineTo(-8, -28); G.fill();
  };
  A.gardenBench = () => { for (const x of [-20, 18]) K.rr(x, -10, 3, 10, 1, '#6f4630'); K.rr(-26, -34, 52, 14, 3, '#8a5a3c'); for (let k = 0; k < 2; k++) K.rr(-24, -32 + k * 6, 48, 3, 1, '#c98a4b'); box(0, -6, 52, 16, 4, '#8a5a3c', '#c98a4b', 2); };
  A.plant = () => { cyl(0, 0, 9, 14, '#c9683f', '#6f4630'); for (const [x, y, r] of [[0, -26, 10], [-9, -20, 7], [9, -20, 7], [-4, -32, 6]]) K.circ(x, y, r, '#3f9d4e'); K.circ(3, -30, 4, '#5cc46a'); };
  A.bush = () => { for (const [x, y, r] of [[-10, -10, 11], [10, -10, 11], [0, -18, 12]]) K.circ(x, y, r, '#3f9d4e'); K.circ(-3, -22, 5, '#5cc46a'); K.circ(8, -14, 4, '#5cc46a'); };
  A.flowers = () => { for (let k = 0; k < 9; k++) { const x = -20 + (k % 5) * 10, y = -4 - Math.floor(k / 5) * 7; K.rr(x - 0.5, y, 1, 5, 0, '#2f7a3c'); K.circ(x, y, 2.6, ['#ff5a5a', '#ffd23f', '#ff8fc7', '#ffffff'][k % 4]); } };
  A.coatRack = () => { ell(0, -1, 8, 3, '#6f4630'); K.rr(-1.5, -58, 3, 56, 1, '#8a5a3c'); for (const d of [-1, 1]) line(0, -54, d * 8, -60, '#8a5a3c', 2); K.rr(-9, -56, 8, 18, 3, '#e04f4f'); K.rr(3, -54, 7, 14, 3, '#ffd23f'); };


  // ---- more of a house
  A.nightstand = () => { box(0, 0, 22, 18, 20, '#8a5a3c', '#c98a4b', 2); K.rr(-8, -16, 16, 6, 1, '#6f4630'); K.circ(0, -13, 1.2, '#ffd23f'); ell(4, -26, 5, 2.5, '#7d8494'); K.rr(3, -40, 2, 14, 0, '#7d8494'); const G = g(); G.fillStyle = '#ffe9a8'; G.beginPath(); G.moveTo(-3, -38); G.lineTo(11, -38); G.lineTo(8, -48); G.lineTo(0, -48); G.fill(); };
  A.dresser = () => { box(0, 0, 54, 22, 40, '#c98a4b', '#d99a5b', 3); for (let r = 0; r < 3; r++) { K.rr(-24, -36 + r * 11, 48, 9, 1, '#d99a5b'); K.circ(-8, -31.5 + r * 11, 1.3, '#6f4630'); K.circ(8, -31.5 + r * 11, 1.3, '#6f4630'); } K.rr(-20, -66, 40, 18, 3, '#8a5a3c'); K.rr(-17, -63, 34, 12, 2, '#bfe9ff'); cyl(18, -48, 4, 6, '#ff8fc7', '#ffc1d6'); };
  A.desk = c => { box(0, 0, 56, 26, 26, '#8a5a3c', '#c98a4b', 2); K.rr(-24, -22, 18, 18, 1, '#6f4630'); box(10, -32, 22, 4, 16, '#2b2833', '#4a4a55', 2); K.rr(1, -46, 18, 11, 1, c || '#5ad1ff'); box(-12, -30, 14, 8, 2, '#fff6d8', '#ffffff', 1); box(0, 18, 18, 14, 12, '#2b2833', '#4a4a55', 4); K.rr(-9, 0, 18, 14, 4, '#3d3a48'); };
  A.crib = c => { box(0, 0, 52, 30, 30, '#fff6d8', '#ffffff', 3); K.rr(-24, -46, 48, 16, 2, c || '#ffc1d6'); K.rr(-18, -46, 14, 6, 3, '#ffffff'); for (let x = -24; x <= 24; x += 6) K.rr(x - 1, -30, 2, 28, 1, '#e8dcc0'); K.rr(-26, -32, 52, 3, 1, '#e8dcc0'); line(0, -48, 0, -70, '#c9b8c7', 1.5); for (let k = 0; k < 4; k++) K.circ(-9 + k * 6, -66 + (k % 2) * 4, 2.5, ['#ffd23f', '#5ad1ff', '#ff8fc7', '#49d17a'][k]); };
  A.toyChest = () => { box(0, 0, 40, 22, 20, '#e04f4f', '#ff8a8a', 3); K.rr(-20, -24, 40, 4, 1, '#ffd23f'); K.circ(-8, -32, 4, '#5ad1ff'); K.rr(4, -36, 8, 8, 1, '#49d17a'); K.circ(14, -30, 3, '#ffd23f'); };
  A.blocks = () => { box(-8, 0, 10, 10, 10, '#e04f4f', '#ff8a8a', 1); box(4, -2, 10, 10, 10, '#5ad1ff', '#bfefff', 1); box(-2, -12, 10, 10, 10, '#ffd23f', '#fff0a0', 1); };
  A.sofaBack = c => { c = c || '#c0504a'; const d = K.shade(parseInt(c.slice(1), 16), 0.8); box(0, 0, 90, 30, 16, d, c, 5); box(0, -16, 90, 8, 22, c, K.shade(parseInt(c.slice(1), 16), 1.15), 6); for (const x of [-42, 42]) box(x, 0, 10, 38, 26, d, c, 4); K.rr(-20, -40, 12, 6, 3, '#ffd23f'); };
  A.armchair = c => { c = c || '#49a07a'; const d = K.shade(parseInt(c.slice(1), 16), 0.75); box(0, -24, 40, 10, 26, c, K.shade(parseInt(c.slice(1), 16), 1.15), 6); box(0, 0, 36, 28, 12, d, c, 4); for (const x of [-18, 18]) box(x, 0, 8, 32, 18, d, c, 3); };
  A.tv = () => { box(0, 0, 70, 22, 18, '#4a3020', '#6f4630', 2); K.rr(-28, -14, 24, 10, 1, '#3a2618'); K.rr(4, -14, 24, 10, 1, '#3a2618'); K.rr(-30, -64, 60, 38, 3, '#1b1426'); K.rr(-27, -61, 54, 32, 2, '#3a6fc0'); K.circ(-8, -48, 6, '#ffd23f'); K.rr(4, -52, 14, 10, 2, '#49d17a'); K.rr(-4, -28, 8, 4, 1, '#1b1426'); box(-22, -32, 12, 8, 4, '#9aa1b0', '#c9ced8', 1); };
  A.island = () => { box(0, 0, 90, 40, 34, '#5a8f7a', '#e8e0d0', 3); for (let k = 0; k < 3; k++) K.rr(-42 + k * 29, -30, 26, 26, 2, '#6fa58f'); ell(-20, -48, 8, 3.5, '#ffffff'); K.circ(-20, -49, 3, '#ff8a3d'); cyl(18, -46, 6, 4, '#c98a4b', '#e8b46a'); for (const x of [-30, 0, 30]) { line(x, 10, x, 2, '#4a505c', 2); ell(x, 0, 7, 3, '#e04f4f'); } };
  A.table6 = () => {
    const chair = (x, y, back) => { if (back === 'up') { K.rr(x - 8, y - 38, 16, 18, 3, '#8a5a3c'); } box(x, y, 16, 14, 12, '#8a5a3c', '#c98a4b'); if (back === 'down') { K.rr(x - 8, y - 18, 16, 14, 3, '#6f4630'); } };
    for (const x of [-26, 26]) chair(x, -46, 'up'); chair(-58, -18); chair(58, -18);
    box(0, 0, 104, 50, 22, '#8a5a3c', '#c98a4b', 3); K.rr(-48, -50, 96, 26, 3, '#d99a5b');
    for (const [x, c] of [[-30, '#ff8a3d'], [0, '#49d17a'], [30, '#e04f4f']]) { ell(x, -38, 9, 4, '#ffffff'); ell(x, -38.5, 5, 2.2, c); }
    cyl(-14, -40, 3, 10, '#fff6d8', '#ffd23f'); cyl(14, -40, 3, 10, '#fff6d8', '#ffd23f');
    for (const x of [-26, 26]) { box(x, 16, 16, 14, 12, '#8a5a3c', '#c98a4b'); K.rr(x - 8, -2, 16, 14, 3, '#6f4630'); }
  };
  A.hutch = () => { box(0, 0, 60, 22, 34, '#8a5a3c', '#c98a4b', 2); box(0, -34, 54, 10, 46, '#8a5a3c', '#a06a40', 2); K.rr(-24, -78, 48, 40, 2, '#bfe9ff'); for (let r = 0; r < 2; r++) for (let k = 0; k < 4; k++) { ell(-18 + k * 12, -66 + r * 18, 4, 4, '#ffffff'); ell(-18 + k * 12, -66 + r * 18, 2, 2, '#5ad1ff'); } };
  A.washer = c => { box(0, 0, 34, 28, 40, '#f2f6f8', '#ffffff', 4); K.circ(0, -20, 11, '#c9ced8'); K.circ(0, -20, 8, c || '#5ad1ff'); K.circ(-3, -23, 3, '#bfefff'); K.rr(-14, -38, 28, 5, 1, '#c9ced8'); K.circ(10, -36, 1.5, '#49d17a'); };
  A.basket = () => { cyl(0, 0, 11, 14, '#c9a35a', '#a07a3a'); K.circ(-4, -16, 5, '#ff8fc7'); K.circ(4, -16, 5, '#5ad1ff'); K.circ(0, -19, 4, '#ffffff'); };
  A.shoeRack = () => { box(0, 0, 40, 16, 16, '#8a5a3c', '#c98a4b', 2); for (const [x, c] of [[-12, '#e04f4f'], [0, '#3a6fc0'], [12, '#ffd23f']]) K.rr(x - 5, -26, 10, 5, 2, c); };
  A.toilet = () => { K.rr(-10, -46, 20, 18, 3, '#ffffff'); K.rr(-8, -48, 16, 4, 2, '#e8eef5'); ell(0, -18, 10, 8, '#e8eef5'); ell(0, -18, 7, 5.5, '#ffffff'); ell(0, -18, 4.5, 3, '#bfe9ff'); K.rr(-6, -12, 12, 12, 3, '#e8eef5'); };
  A.vanity = () => { box(0, 0, 42, 22, 30, '#8a5a3c', '#e8eef5', 2); ell(0, -38, 11, 4, '#bfe9ff'); K.rr(-1.5, -50, 3, 8, 1, '#9aa1b0'); K.rr(-16, -82, 32, 26, 6, '#c9ced8'); K.rr(-14, -80, 28, 22, 5, '#d8f2ff'); const G = g(); G.globalAlpha = 0.6; K.rr(-10, -78, 4, 16, 2, '#ffffff'); G.globalAlpha = 1; };
  A.tub = () => { box(0, 0, 76, 34, 22, '#ffffff', '#e8eef5', 6); K.rr(-34, -40, 68, 14, 6, '#8fd3ff'); K.circ(-20, -34, 3, '#ffffff'); K.circ(-14, -36, 2, '#ffffff'); K.circ(18, -34, 4, '#ffd23f'); K.circ(21, -36, 1.5, '#ff8a3d'); K.rr(26, -62, 3, 20, 1, '#9aa1b0'); K.rr(22, -62, 10, 3, 1, '#9aa1b0'); };
  A.towels = () => { K.rr(-14, -50, 28, 3, 1, '#9aa1b0'); K.rr(-12, -48, 10, 22, 2, '#ff8fc7'); K.rr(2, -48, 10, 22, 2, '#5ad1ff'); };
  A.car = c => {
    // the family car, from above, nose toward us
    c = c || '#3a6fc0'; const d = K.shade(parseInt(c.slice(1), 16), 0.75), l = K.shade(parseInt(c.slice(1), 16), 1.2);
    for (const [x, y] of [[-30, -88], [30, -88], [-30, -18], [30, -18]]) box(x, y, 10, 22, 12, '#1b1426', '#2b2833', 3);
    box(0, -2, 62, 150, 16, d, c, 12);
    K.rr(-26, -76, 52, 34, 8, '#8fd3ff'); K.rr(-22, -72, 44, 26, 6, l);   // the roof and its windows
    K.rr(-26, -42, 52, 8, 3, '#8fd3ff'); K.rr(-26, -84, 52, 6, 3, '#8fd3ff');
    K.circ(-22, -8, 3, '#ffd23f'); K.circ(22, -8, 3, '#ffd23f'); K.rr(-10, -9, 20, 4, 1, '#2b2833');
  };
  A.bikes = () => { for (const [x, c] of [[-10, '#e04f4f'], [10, '#49d17a']]) { ring(x, -10, 3, 8, '#2b2833', 2); ring(x, -36, 3, 8, '#2b2833', 2); K.rr(x - 1, -36, 2, 26, 1, c); K.rr(x - 6, -40, 12, 2, 1, '#2b2833'); } };
  A.console = () => { box(0, 0, 60, 16, 24, '#8a5a3c', '#c98a4b', 2); K.rr(-26, -54, 20, 24, 3, '#ffd23f'); K.rr(-23, -51, 14, 18, 2, '#8fd3ff'); cyl(16, -34, 5, 12, '#5ad1ff', '#3f9d4e'); K.circ(16, -48, 6, '#ff8fc7'); K.circ(12, -44, 4, '#ffd23f'); };
  A.patioTable = () => { line(0, -22, 0, -70, '#9aa1b0', 2); for (const x of [-22, 22]) box(x, 4, 14, 12, 10, '#ffffff', '#e8eef5', 3); ell(0, -16, 22, 10, '#ffffff'); ell(0, -18, 20, 8, '#e8eef5'); const G = g(); G.fillStyle = '#e04f4f'; G.beginPath(); G.ellipse(0, -74, 34, 14, 0, 0, P * 2); G.fill(); G.fillStyle = '#fff6d8'; for (let k = 0; k < 4; k++) { G.beginPath(); G.moveTo(0, -74); G.ellipse(0, -74, 34, 14, 0, k * P / 2, k * P / 2 + P / 4); G.fill(); } };
  A.bbq = () => { for (const x of [-10, 10]) line(x, 0, x * 0.6, -20, '#2b2833', 2); cyl(0, -18, 14, 10, '#2b2833', '#4a4a55'); for (let k = -2; k <= 2; k++) K.rr(-11, -29 + k * 2, 22, 0.8, 0, '#9aa1b0'); K.rr(-8, -31, 8, 3, 1.5, '#c0504a'); K.rr(2, -29, 8, 3, 1.5, '#c0504a'); };
  A.swing = () => { for (const x of [-34, 34]) { line(x - 8, 0, x, -70, '#e04f4f', 3); line(x + 8, 0, x, -70, '#e04f4f', 3); } line(-34, -70, 34, -70, '#e04f4f', 4); for (const x of [-14, 14]) { line(x, -70, x - 1, -24, '#c9b8c7', 1); line(x + 8, -70, x + 7, -24, '#c9b8c7', 1); box(x + 3, -20, 14, 6, 3, '#ffd23f', '#fff0a0', 1); } };
  A.trampoline = () => { for (const x of [-30, 30]) line(x, 0, x, -10, '#2b2833', 2); ell(0, -12, 40, 18, '#3a6fc0'); ell(0, -12, 34, 14, '#2b2840'); ell(-8, -16, 12, 4, '#46386e'); };
  A.sandbox = () => {   // the sandbox: a wooden frame full of sand, a castle, a bucket and a spade
    box(0, 0, 72, 44, 8, '#8a5a3c', '#c98a4b', 3);
    K.rr(-32, -33, 64, 23, 2, '#e8c27a');
    for (const [x, y, r] of [[-20, -18, 7], [-8, -26, 5], [14, -14, 6]]) ell(x, y, r, r * 0.45, '#d8ae62');
    for (const [x, y] of [[-24, -28], [4, -22], [22, -27], [-14, -13], [26, -16]]) K.rr(x, y, 1.5, 1.5, 0, '#c99a52');
    K.rr(-22, -30, 14, 9, 1, '#d8ae62'); K.rr(-24, -34, 5, 5, 0, '#d8ae62'); K.rr(-12, -34, 5, 5, 0, '#d8ae62'); K.rr(-17, -38, 4, 8, 0, '#e8c27a');
    K.rr(-16, -45, 1, 8, 0, '#6f4630'); K.rr(-15, -45, 6, 4, 0, '#e04f4f');
    cyl(16, -22, 6, 9, '#3a8fe0', '#5ad1ff'); ell(16, -31, 4, 1.6, '#e8c27a'); line(10, -30, 16, -36, '#2b2833', 1);
    line(2, -14, 10, -26, '#8a5a3c', 2); K.rr(-1, -15, 6, 4, 1.5, '#e04f4f');
  };
  A.rugOval = c => { ell(0, 0, 40, 18, c[0]); ell(0, 0, 32, 13, c[1]); };
  return { A, box, ell, cyl, ring, V };
})();
