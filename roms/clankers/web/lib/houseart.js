// The heroes' house: the yard, the rooms, the furniture and the little games inside (the paint
// mural, the aquarium, the garage race track, the piano). The browser game draws the house with
// these (houseScene in game.js) and tools/bake_sprites.mjs bakes the same pictures for the N64.
// Set CK.g first. Every piece of furniture is drawn standing on (0, 0), its bottom middle.
//
// The layout is shared with the N64 (src/core/ck_house.c has the same numbers): rooms side by
// side along one long floor, each with a wall colour, a stripe colour and a floor colour.
const HOUSEART = (() => {
  const K = CK, P = Math.PI;
  const hx = h => '#' + h.toString(16).padStart(6, '0');
  const LAYOUT = {
    W: 2040, FLOOR: 200, CEIL: 20, PILLAR: 8, DOOR_TOP: 120,
    // x0, x1, name, wall, stripe, floor
    rooms: [
      [0, 320, 'The yard', 0x8FD3FF, 0xBDE8FF, 0x5DBB63],
      [320, 560, 'Parts shop', 0xF3C77B, 0xE9B865, 0x8A5A3C],
      [560, 800, 'Kitchen', 0xBDE7D2, 0xA9DCC3, 0xD9CDB8],
      [800, 1040, 'Paint studio', 0xFFF1D6, 0xF6E1BA, 0x9C7552],
      [1040, 1280, 'Music room', 0xC9B8F0, 0xB8A3E9, 0x7A4E3A],
      [1280, 1520, 'Aquarium', 0x9FD8F0, 0x88CBE6, 0x5D7C99],
      [1520, 1760, 'Bedroom', 0xFFD3E2, 0xFFC1D6, 0x8C6A55],
      [1760, 2040, 'Garage', 0xB5BAC4, 0xA4AAB6, 0x6E7280],
    ],
    // x, kind, what Down does there
    spots: [
      [440, 'shop', 'Parts shop'],
      [612, 'snack', 'Have a snack'],
      [920, 'paint', 'Paint the mural'],
      [1160, 'piano', 'Play the piano'],
      [1400, 'aqua', 'Swim with the fish'],
      [1640, 'bed', 'Sleep and save'],
      [1900, 'cars', 'Race the cars'],
    ],
    // decorations that are only looked at: x, furniture frame
    decor: [[120, 'tree'], [262, 'mailbox'], [690, 'stove'], [752, 'table'], [1010, 'plant'], [1240, 'drum'], [1480, 'plant'], [1720, 'lamp'], [2000, 'tools']],
  };
  // furniture, by name; each fits in 96x96 standing on (0, 0)
  const F = {};
  F.tree = g => {
    K.rr(-6, -60, 12, 60, 4, '#8a5a3c');
    for (const [x, y, r] of [[-22, -66, 20], [20, -70, 22], [0, -80, 20], [-8, -56, 16], [14, -52, 16]]) K.circ(x, y, r, '#3f9d4e');
    for (const [x, y, r] of [[-16, -74, 10], [10, -84, 9], [24, -62, 8]]) K.circ(x, y, r, '#5cc46a');
    for (const [x, y] of [[-14, -60], [16, -80], [4, -66]]) K.circ(x, y, 3, '#ff5a5a');
  };
  F.mailbox = g => { K.rr(-2, -30, 4, 30, 1, '#7a5a40'); K.rr(-10, -42, 20, 14, 6, '#4f7ad1'); K.rr(7, -50, 3, 12, 1, '#e04f4f'); K.rr(7, -50, 8, 5, 1, '#e04f4f'); };
  F.counter = g => {
    // the parts shop counter, with a hat and a pair of shoes on show and a sign
    K.rr(-40, -34, 80, 34, 4, '#b07a44'); K.rr(-40, -38, 80, 7, 3, '#d99a5b'); K.rr(-34, -28, 68, 20, 3, '#9a6538');
    K.rr(-30, -54, 18, 12, 2, '#2b2833'); K.rr(-33, -44, 24, 4, 2, '#2b2833'); K.rr(-30, -48, 18, 3, 0, '#e04f4f');
    K.rr(10, -45, 10, 7, 3, '#e04f4f'); K.rr(22, -45, 10, 7, 3, '#e04f4f');
    K.rr(-26, -88, 52, 20, 4, '#ffd23f'); K.rr(-23, -85, 46, 14, 3, '#2b2230');
    K.nut(-12, -78, 4, 0.3, '#ffd23f'); K.nut(0, -78, 4, 0.8, '#ffd23f'); K.nut(12, -78, 4, 1.3, '#ffd23f');
  };
  F.fridge = g => {
    K.rr(-18, -76, 36, 76, 5, '#e8eef5'); K.rr(-18, -50, 36, 2, 0, '#b9c3cf'); K.rr(10, -70, 3, 14, 1, '#9aa1b0'); K.rr(10, -44, 3, 18, 1, '#9aa1b0');
    K.circ(-8, -64, 3, '#ff5a5a'); K.circ(0, -60, 3, '#ffd23f'); K.rr(-12, -40, 10, 12, 1, '#ffffff'); K.rr(-11, -38, 8, 2, 0, '#5ad1ff'); K.rr(-11, -34, 6, 2, 0, '#ff8fc7');
  };
  F.stove = g => {
    K.rr(-20, -40, 40, 40, 4, '#6f7684'); K.rr(-20, -44, 40, 6, 2, '#4a505c'); K.rr(-14, -30, 28, 20, 3, '#2b2833'); K.rr(-12, -28, 24, 4, 1, '#ff8a3d');
    K.rr(-15, -54, 18, 10, 3, '#9aa1b0'); K.rr(-17, -55, 22, 3, 1, '#7d8494'); K.rr(4, -50, 12, 3, 1, '#4a505c');
    g.globalAlpha = 0.6; K.circ(-8, -62, 3, '#ffffff'); K.circ(-4, -68, 2.5, '#ffffff'); g.globalAlpha = 1;
  };
  F.table = g => {
    K.rr(-26, -30, 52, 6, 2, '#c98a4b'); K.rr(-22, -24, 4, 24, 1, '#8a5a3c'); K.rr(18, -24, 4, 24, 1, '#8a5a3c');
    K.rr(-14, -36, 14, 6, 3, '#ffffff'); K.circ(-7, -37, 4, '#ff8a3d'); K.rr(4, -42, 8, 12, 2, '#5ad1ff'); K.rr(5, -40, 6, 3, 0, '#ffffff');
  };
  F.easel = g => {
    // the door to the paint studio's big wall, with a painted canvas beside it
    K.rr(-24, -84, 48, 84, 4, '#6a4a7a'); K.rr(-20, -80, 40, 80, 3, '#8f63a8'); K.circ(12, -40, 3, '#ffd23f');
    for (const [x, y, c] of [[-12, -70, '#ff5a5a'], [-4, -66, '#ffd23f'], [6, -72, '#49d17a'], [-8, -56, '#5ad1ff'], [4, -58, '#c28bff']]) K.circ(x, y, 4, c);
    K.rr(-30, -14, 14, 14, 3, '#9aa1b0'); K.rr(-30, -14, 14, 5, 2, '#ff5a5a');
  };
  F.piano = g => {
    K.rr(-38, -64, 76, 64, 4, '#2b2833'); K.rr(-34, -60, 68, 22, 3, '#3d3a48'); K.rr(-38, -36, 76, 6, 1, '#1b1426');
    for (let i = 0; i < 12; i++) K.rr(-34 + i * 5.6, -30, 5, 12, 1, '#fff6d8');
    for (const i of [0, 1, 3, 4, 5, 7, 8, 10]) K.rr(-30.5 + i * 5.6, -30, 3.2, 7, 0.5, '#1b1426');
    K.rr(-34, -18, 4, 18, 1, '#1b1426'); K.rr(30, -18, 4, 18, 1, '#1b1426');
    K.rr(-14, -76, 28, 12, 2, '#fff6d8'); for (let k = 0; k < 3; k++) K.rr(-11, -73 + k * 3, 22, 1, 0, '#9aa1b0');
    K.circ(-6, -71, 1.6, '#1b1426'); K.circ(5, -69, 1.6, '#1b1426');
  };
  F.tank = g => {
    // the fish tank on its stand (the fish swim in it while you watch: fish frames on top)
    K.rr(-40, -26, 80, 26, 3, '#6f4f3a'); K.rr(-36, -20, 72, 16, 2, '#5a3f2e');
    K.rr(-44, -90, 88, 64, 4, '#d8f2ff'); K.rr(-41, -84, 82, 56, 2, '#4fb3e3'); K.rr(-41, -84, 82, 8, 2, '#8fd3ff');
    K.rr(-41, -36, 82, 8, 2, '#e8c98a'); K.rr(-44, -92, 88, 5, 2, '#7d8494');
    for (const x of [-30, -16, 26]) { g.strokeStyle = '#2f9a5a'; g.lineWidth = 2.5; g.beginPath(); g.moveTo(x, -36); g.quadraticCurveTo(x - 4, -48, x + 1, -60); g.stroke(); }
    K.rr(14, -42, 12, 8, 3, '#8a7a6a'); K.rr(30, -44, 10, 10, 4, '#a89a88');
    g.globalAlpha = 0.5; K.rr(-38, -82, 6, 40, 3, '#ffffff'); g.globalAlpha = 1;
  };
  F.bed = g => {
    K.rr(-44, -26, 88, 20, 4, '#8a5a3c'); K.rr(-44, -50, 10, 50, 3, '#6f4630'); K.rr(36, -38, 8, 38, 3, '#6f4630');
    K.rr(-36, -32, 72, 10, 4, '#ffffff'); K.rr(-34, -40, 22, 10, 5, '#fff6d8');
    K.rr(-10, -36, 46, 14, 5, '#5ad1ff'); for (let k = 0; k < 4; k++) K.rr(-6 + k * 11, -36, 5, 14, 0, '#8fe0ff');
  };
  F.car = g => {
    // the parked go-kart: Down at it starts the race
    K.rr(-38, -26, 76, 16, 6, '#e04f4f'); K.rr(-20, -38, 34, 14, 6, '#e04f4f'); K.rr(-16, -36, 12, 9, 3, '#bff0ff'); K.rr(-1, -36, 12, 9, 3, '#bff0ff');
    K.rr(26, -22, 10, 5, 2, '#ffd23f'); K.rr(-38, -20, 6, 6, 2, '#ff8a3d');
    for (const x of [-22, 22]) { K.circ(x, -9, 9, '#2b2833'); K.circ(x, -9, 4, '#9aa1b0'); }
    K.rr(-30, -24, 10, 3, 1, '#ffffff');
  };
  F.plant = g => { K.rr(-9, -16, 18, 16, 3, '#c9683f'); for (const [a, l] of [[-0.6, 22], [0, 28], [0.6, 22], [-0.3, 18], [0.3, 18]]) { g.save(); g.translate(0, -16); g.rotate(a); K.rr(-3, -l, 6, l, 3, '#3f9d4e'); g.restore(); } };
  F.drum = g => { K.rr(-16, -26, 32, 26, 5, '#e04f4f'); K.rr(-16, -28, 32, 5, 2, '#fff6d8'); for (let k = 0; k < 4; k++) K.rr(-14 + k * 9, -22, 2, 20, 1, '#ffd23f'); K.rr(-20, -40, 2, 14, 1, '#c9b8c7'); K.rr(-26, -42, 14, 3, 1, '#ffd23f'); };
  F.lamp = g => { K.rr(-2, -50, 4, 50, 1, '#7d8494'); K.rr(-10, -4, 20, 4, 2, '#7d8494'); g.fillStyle = '#ffe9a8'; g.beginPath(); g.moveTo(-12, -48); g.lineTo(12, -48); g.lineTo(7, -66); g.lineTo(-7, -66); g.fill(); };
  F.tools = g => { K.rr(-26, -60, 52, 46, 3, '#8a7a6a'); for (let i = 0; i < 6; i++) K.circ(-20 + i * 8, -52, 1.2, '#5a4e42'); K.rr(-20, -48, 4, 18, 1, '#e04f4f'); K.rr(-12, -46, 12, 4, 1, '#9aa1b0'); K.rr(4, -50, 3, 20, 1, '#ffd23f'); K.rr(12, -44, 8, 8, 4, '#9aa1b0'); K.rr(-28, -14, 56, 14, 3, '#e04f4f'); K.rr(-28, -14, 56, 3, 1, '#ff8a8a'); };
  const NAMES = ['tree', 'mailbox', 'counter', 'fridge', 'stove', 'table', 'easel', 'piano', 'tank', 'bed', 'car', 'plant', 'drum', 'lamp', 'tools'];
  const SPOT_FURN = { shop: 'counter', snack: 'fridge', paint: 'easel', piano: 'piano', aqua: 'tank', bed: 'bed', cars: 'car' };
  function furn(g, name, x, y) { g.save(); g.translate(x, y); F[name](g); g.restore(); }

  // ---- the little creatures (drawn around (0, 0), facing right)
  const FISH = [['#ff8a3d', '#ffffff'], ['#ffd23f', '#5ad1ff'], ['#ff8fc7', '#c28bff']];
  function fish(g, kind, t) {
    const [a, b] = FISH[kind], w = Math.sin(t * 10) * 2;
    g.fillStyle = a; g.beginPath(); g.moveTo(-6, 0); g.lineTo(-12, -5 + w * 0.5); g.lineTo(-12, 5 + w * 0.5); g.fill();
    g.beginPath(); g.ellipse(0, 0, 8, 5, 0, 0, P * 2); g.fill();
    K.rr(-3, -5, 2, 10, 1, b); K.circ(4, -1, 1.6, '#ffffff'); K.circ(4.4, -1, 0.9, '#1b1426');
  }
  function shrimp(g, t) {
    g.strokeStyle = '#ff8a6b'; g.lineWidth = 3; g.lineCap = 'round'; g.beginPath(); g.arc(0, 0, 4, P * 0.15, P * 1.1); g.stroke();
    g.lineWidth = 1; g.beginPath(); g.moveTo(3, -2); g.lineTo(8, -6 + Math.sin(t * 12)); g.moveTo(3, -1); g.lineTo(8, -2 + Math.sin(t * 12 + 1)); g.stroke();
    K.circ(2.5, -3, 1, '#1b1426');
  }
  function crab(g, t, snap) {
    const s = snap ? 0.6 : 0;
    for (const d of [-1, 1]) for (let k = 0; k < 3; k++) { g.strokeStyle = '#c23a2a'; g.lineWidth = 1.4; g.beginPath(); g.moveTo(d * 4, 0); g.lineTo(d * (8 + k * 2), 4 + Math.sin(t * 14 + k) * 1.2); g.stroke(); }
    g.fillStyle = '#e8503a'; g.beginPath(); g.ellipse(0, -1, 8, 5, 0, 0, P * 2); g.fill();
    for (const d of [-1, 1]) { K.circ(d * 11, -6, 3.2, '#e8503a'); g.fillStyle = '#f7d6c6'; g.beginPath(); g.moveTo(d * 11, -6); g.arc(d * 11, -6, 3.4, -0.5 - s, 0.5 + s); g.fill(); }
    K.rr(-3.5, -9, 1.5, 4, 0.5, '#c23a2a'); K.rr(2, -9, 1.5, 4, 0.5, '#c23a2a'); K.circ(-2.8, -9, 1.6, '#ffffff'); K.circ(2.8, -9, 1.6, '#ffffff'); K.circ(-2.8, -9, 0.8, '#1b1426'); K.circ(2.8, -9, 0.8, '#1b1426');
  }
  function weed(g, t, h) { g.strokeStyle = '#2f9a5a'; g.lineWidth = 3; g.lineCap = 'round'; g.beginPath(); g.moveTo(0, 0); for (let y = 0; y > -h; y -= 6) g.lineTo(Math.sin(t * 2 + y * 0.15) * 3, y - 6); g.stroke(); }

  // ---- the aquarium swim: water on the left, a dry sandy beach with crabs on the right
  const AQUA = { WATER_TOP: 44, SEABED: 222, BEACH_X: 236, BEACH_Y: 182 };
  function aquaBg(g) {
    const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#bfe9ff'); gr.addColorStop(1, '#8fd3ff'); g.fillStyle = gr; g.fillRect(0, 0, 320, 240);
    const w = g.createLinearGradient(0, AQUA.WATER_TOP, 0, 240); w.addColorStop(0, '#3fa9de'); w.addColorStop(1, '#1d5f9e'); g.fillStyle = w; g.fillRect(0, AQUA.WATER_TOP, AQUA.BEACH_X + 10, 240 - AQUA.WATER_TOP);
    g.fillStyle = 'rgba(255,255,255,0.08)'; for (let i = 0; i < 6; i++) { g.beginPath(); g.moveTo(20 + i * 40, AQUA.WATER_TOP); g.lineTo(40 + i * 40, AQUA.WATER_TOP); g.lineTo(10 + i * 40, 230); g.lineTo(-10 + i * 40, 230); g.fill(); }
    g.fillStyle = '#e8c98a'; g.fillRect(0, AQUA.SEABED, 320, 240 - AQUA.SEABED);
    // the beach rises out of the water
    g.fillStyle = '#f0d49a'; g.beginPath(); g.moveTo(AQUA.BEACH_X - 30, AQUA.SEABED); g.lineTo(AQUA.BEACH_X, AQUA.BEACH_Y); g.lineTo(320, AQUA.BEACH_Y); g.lineTo(320, 240); g.lineTo(AQUA.BEACH_X - 30, 240); g.fill();
    K.rr(AQUA.BEACH_X, AQUA.BEACH_Y, 84, 3, 1, '#ffe2a8');
    for (const [x, y, r] of [[258, 182, 9], [300, 184, 12], [70, 222, 10], [160, 224, 8]]) { g.fillStyle = '#9a8f84'; g.beginPath(); g.ellipse(x, y, r, r * 0.7, 0, P, 0); g.fill(); }
    // a palm on the beach, a treasure chest on the seabed
    K.rr(300, 120, 6, 62, 3, '#8a5a3c'); for (const a of [-2.6, -2.0, -1.2, -0.5]) { g.save(); g.translate(303, 122); g.rotate(a); K.rr(0, -3, 26, 6, 3, '#3f9d4e'); g.restore(); }
    K.rr(108, 208, 22, 14, 3, '#a8672f'); K.rr(108, 206, 22, 6, 3, '#c98a4b'); K.rr(117, 210, 4, 4, 1, '#ffd23f');
    // the glass of the tank: a frame round the water
    K.rr(0, AQUA.WATER_TOP - 6, AQUA.BEACH_X + 10, 4, 1, 'rgba(255,255,255,0.5)');
  }
  // ---- the garage race: an oval track seen from above
  const TRACK = { cx: 160, cy: 130, ox: 150, oy: 100, ix: 92, iy: 48 };
  function trackBg(g) {
    g.fillStyle = '#5dbb63'; g.fillRect(0, 0, 320, 240);
    for (let i = 0; i < 60; i++) { const x = (i * 71) % 320, y = (i * 113) % 240; K.circ(x, y, 1.2, '#4fa856'); }
    const T = TRACK;
    g.fillStyle = '#e04f4f'; g.beginPath(); g.ellipse(T.cx, T.cy, T.ox + 4, T.oy + 4, 0, 0, P * 2); g.fill();
    g.fillStyle = '#ffffff'; for (let a = 0; a < P * 2; a += P / 16) { g.beginPath(); g.ellipse(T.cx, T.cy, T.ox + 4, T.oy + 4, 0, a, a + P / 32); g.lineTo(T.cx, T.cy); g.fill(); }
    g.fillStyle = '#5a5e6a'; g.beginPath(); g.ellipse(T.cx, T.cy, T.ox, T.oy, 0, 0, P * 2); g.fill();
    g.strokeStyle = 'rgba(255,255,255,0.5)'; g.lineWidth = 1.5; g.setLineDash([8, 8]); g.beginPath(); g.ellipse(T.cx, T.cy, (T.ox + T.ix) / 2, (T.oy + T.iy) / 2, 0, 0, P * 2); g.stroke(); g.setLineDash([]);
    g.fillStyle = '#e04f4f'; g.beginPath(); g.ellipse(T.cx, T.cy, T.ix, T.iy, 0, 0, P * 2); g.fill();
    g.fillStyle = '#5dbb63'; g.beginPath(); g.ellipse(T.cx, T.cy, T.ix - 4, T.iy - 4, 0, 0, P * 2); g.fill();
    // the finish line across the top of the track, checkered
    const y0 = T.cy - T.oy, y1 = T.cy - T.iy;
    for (let y = y0; y < y1; y += 4) for (let k = 0; k < 2; k++) { g.fillStyle = ((y - y0) / 4 + k) % 2 ? '#ffffff' : '#1b1426'; g.fillRect(T.cx - 4 + k * 4, y, 4, 4); }
    // a little stand of babies' flags in the middle
    K.rr(120, 118, 80, 22, 4, '#c98a4b'); K.rr(124, 122, 72, 14, 3, '#e8b46a');
    for (let i = 0; i < 5; i++) { K.rr(128 + i * 16, 104, 2, 16, 1, '#7d8494'); K.rr(130 + i * 16, 104, 9, 6, 1, ['#ff5a5a', '#ffd23f', '#49d17a', '#5ad1ff', '#c28bff'][i]); }
  }
  // a car from above, pointing right (+x), in white so it can be tinted; the driver's head on top
  function carTop(g, hex) {
    const c = hex == null ? '#ffffff' : hx(hex);
    K.rr(-11, -8, 4, 4, 1, '#1b1426'); K.rr(5, -8, 4, 4, 1, '#1b1426'); K.rr(-11, 4, 4, 4, 1, '#1b1426'); K.rr(5, 4, 4, 4, 1, '#1b1426');
    K.rr(-12, -6, 24, 12, 4, c); K.rr(6, -4, 6, 8, 2, c); K.rr(-12, -5, 3, 10, 1, '#d8d8d8');
    K.rr(1, -4, 4, 8, 1, '#bff0ff');
  }
  // ---- the piano keyboard for the music game: 2 octaves from C4, 14 white keys
  const KEYS = { x: 20, y: 150, w: 20, h: 80, bw: 12, bh: 48, n: 14 };
  return { LAYOUT, F, NAMES, SPOT_FURN, furn, fish, shrimp, crab, weed, FISH, AQUA, aquaBg, TRACK, trackBg, carTop, KEYS, hx };
})();
