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
    W: 1720, FLOOR: 200, CEIL: 20, PILLAR: 8, DOOR_TOP: 120,
    // x0, x1, name, wall, stripe, floor
    rooms: [
      [0, 400, 'The yard', 0x8FD3FF, 0xBDE8FF, 0x5DBB63],
      [400, 580, 'Bedroom', 0xFFD3E2, 0xFFC1D6, 0x8C6A55],
      [580, 760, 'Kitchen', 0xBDE7D2, 0xA9DCC3, 0xD9CDB8],
      [760, 940, 'Paint studio', 0xFFF1D6, 0xF6E1BA, 0x9C7552],
      [940, 1120, 'Music room', 0xC9B8F0, 0xB8A3E9, 0x7A4E3A],
      [1120, 1300, 'Aquarium', 0x9FD8F0, 0x88CBE6, 0x5D7C99],
      [1300, 1520, 'Garage', 0xB5BAC4, 0xA4AAB6, 0x6E7280],
      [1520, 1720, 'Arcade', 0x3A2E5C, 0x46386E, 0x2B2240],
    ],
    // x, kind, what A does there, in SPOT_* order (the bedroom is first so saving is quick)
    spots: [
      [615, 'snack', 'Open the fridge'],
      [850, 'paint', 'Paint the mural'],
      [1005, 'piano', 'Play the piano'],
      [1210, 'aqua', 'Swim with the fish'],
      [490, 'bed', 'Sleep and save'],
      [1400, 'cars', 'Race the cars'],
      [1085, 'drums', 'Play the drums'],
      [262, 'mail', 'Check the mail'],
      [120, 'tree', 'Pick an apple'],
      [44, 'back', 'Back a world'],
      [685, 'stove', 'Cook something'],
      [1580, 'pinball', 'Play pinball'],
      [1665, 'blast', 'Play Bug Blaster'],
      [345, 'pond', 'Go fishing'],
    ],
    // decorations that are only looked at: x, furniture frame
    decor: [[728, 'table'], [920, 'plant'], [1285, 'plant'], [555, 'lamp'], [1490, 'tools']],
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
  // the yard's signpost back to the last world
  F.sign = g => { K.rr(-2, -40, 4, 40, 1, '#8a5a3c'); K.rr(-20, -44, 36, 14, 3, '#c98a4b'); K.rr(-26, -42, 8, 10, 2, '#c98a4b'); K.rr(-17, -41, 30, 2, 1, '#e8b46a'); K.rr(-14, -37, 22, 2, 1, '#6f4630'); };
  F.tools = g => { K.rr(-26, -60, 52, 46, 3, '#8a7a6a'); for (let i = 0; i < 6; i++) K.circ(-20 + i * 8, -52, 1.2, '#5a4e42'); K.rr(-20, -48, 4, 18, 1, '#e04f4f'); K.rr(-12, -46, 12, 4, 1, '#9aa1b0'); K.rr(4, -50, 3, 20, 1, '#ffd23f'); K.rr(12, -44, 8, 8, 4, '#9aa1b0'); K.rr(-28, -14, 56, 14, 3, '#e04f4f'); K.rr(-28, -14, 56, 3, 1, '#ff8a8a'); };
  // the arcade's pinball table: legs, a sloping glass top with bumpers on it, and a lit backbox
  F.pinball = g => {
    for (const x of [-24, 20]) K.rr(x, -34, 4, 34, 1, '#2b2833');
    g.fillStyle = '#c23b6b'; g.beginPath(); g.moveTo(-30, -32); g.lineTo(30, -32); g.lineTo(26, -50); g.lineTo(-26, -50); g.fill();
    g.fillStyle = '#1b1426'; g.beginPath(); g.moveTo(-26, -35); g.lineTo(26, -35); g.lineTo(23, -48); g.lineTo(-23, -48); g.fill();
    K.circ(-10, -44, 3, '#ffd23f'); K.circ(8, -44, 3, '#5ad1ff'); K.circ(-1, -40, 3, '#ff8fc7'); K.rr(-8, -37, 6, 2, 1, '#fff6d8'); K.rr(3, -37, 6, 2, 1, '#fff6d8');
    K.rr(-26, -90, 52, 40, 4, '#c23b6b'); K.rr(-22, -86, 44, 32, 3, '#1b1426');
    K.circ(-11, -76, 5, '#ffd23f'); K.circ(0, -72, 6, '#ff5a5a'); K.circ(11, -76, 5, '#49d17a');
    for (let i = 0; i < 5; i++) K.circ(-16 + i * 8, -60, 1.6, i % 2 ? '#fff6d8' : '#ffd23f');
  };
  // the arcade cabinet: Bug Blaster, with little bugs on its screen
  F.arcade = g => {
    K.rr(-20, -94, 40, 94, 4, '#3a6fc0'); K.rr(-20, -94, 40, 13, 3, '#ffd23f'); K.rr(-14, -91, 28, 3, 1, '#ff5a5a');
    K.rr(-15, -78, 30, 26, 2, '#1b1426');
    for (let i = 0; i < 4; i++) { K.circ(-9 + i * 6, -70, 1.8, i % 2 ? '#c28bff' : '#49d17a'); K.circ(-9 + i * 6, -64, 1.8, '#ff5a5a'); }
    K.rr(-2, -57, 4, 3, 1, '#5ad1ff');
    K.rr(-23, -50, 46, 9, 2, '#2b4f8c'); K.rr(-12, -55, 2, 6, 1, '#9aa1b0'); K.circ(-11, -55, 3, '#ff5a5a'); K.circ(6, -46, 2.5, '#ffd23f'); K.circ(13, -46, 2.5, '#49d17a');
    K.rr(-8, -26, 16, 12, 2, '#2b4f8c'); K.rr(-2, -22, 4, 4, 1, '#ffd23f');
  };
  // the yard's pond: water lying on the grass, a lily pad, reeds, and a rod by the side
  F.pond = g => {
    g.fillStyle = '#3f9d4e'; g.beginPath(); g.ellipse(0, -5, 46, 9, 0, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#3a8fc0'; g.beginPath(); g.ellipse(0, -5, 42, 7, 0, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#7fc8ea'; g.beginPath(); g.ellipse(-8, -7, 18, 2.5, 0, 0, Math.PI * 2); g.fill();
    K.circ(16, -5, 4, '#49d17a'); K.circ(17, -6, 1.5, '#ff8fc7');
    for (const [x, h] of [[-44, 22], [-40, 28], [-36, 18]]) { K.rr(x, -h, 2, h, 1, '#2f7a3c'); K.rr(x - 1, -h - 6, 4, 8, 2, '#8a5a3c'); }
    g.strokeStyle = '#8a5a3c'; g.lineWidth = 2; g.beginPath(); g.moveTo(40, -2); g.lineTo(30, -56); g.stroke();
    g.strokeStyle = '#fff6d8'; g.lineWidth = 1; g.beginPath(); g.moveTo(30, -56); g.lineTo(22, -8); g.stroke();
    K.circ(22, -8, 2, '#ff5a5a');
  };
  const NAMES = ['tree', 'mailbox', 'counter', 'fridge', 'stove', 'table', 'easel', 'piano', 'tank', 'bed', 'car', 'plant', 'drum', 'lamp', 'tools', 'sign', 'pinball', 'arcade', 'pond'];
  const SPOT_FURN = { shop: 'counter', snack: 'fridge', paint: 'easel', piano: 'piano', aqua: 'tank', bed: 'bed', cars: 'car', drums: 'drum', mail: 'mailbox', tree: 'tree', back: 'sign', stove: 'stove', pinball: 'pinball', blast: 'arcade', pond: 'pond' };
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
  const AQUA = { W: 960, WATER_TOP: 44, SEABED: 222, BEACH_X: 876, BEACH_LX: 236, BEACH_Y: 182 };   // BEACH_LX: on the last screen
  // the aquarium's last screen, with the beach (beach = false: one of the plain water screens before it)
  function aquaBg(g, beach = true) {
    const BX = beach ? AQUA.BEACH_LX : 330;
    const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#bfe9ff'); gr.addColorStop(1, '#8fd3ff'); g.fillStyle = gr; g.fillRect(0, 0, 320, 240);
    const w = g.createLinearGradient(0, AQUA.WATER_TOP, 0, 240); w.addColorStop(0, '#3fa9de'); w.addColorStop(1, '#1d5f9e'); g.fillStyle = w; g.fillRect(0, AQUA.WATER_TOP, BX + 10, 240 - AQUA.WATER_TOP);
    g.fillStyle = 'rgba(255,255,255,0.08)'; for (let i = 0; i < (beach ? 6 : 8); i++) { g.beginPath(); g.moveTo(20 + i * 40, AQUA.WATER_TOP); g.lineTo(40 + i * 40, AQUA.WATER_TOP); g.lineTo(10 + i * 40, 230); g.lineTo(-10 + i * 40, 230); g.fill(); }
    g.fillStyle = '#e8c98a'; g.fillRect(0, AQUA.SEABED, 320, 240 - AQUA.SEABED);
    if (beach) {   // the beach rises out of the water, with a palm on it
      g.fillStyle = '#f0d49a'; g.beginPath(); g.moveTo(BX - 30, AQUA.SEABED); g.lineTo(BX, AQUA.BEACH_Y); g.lineTo(320, AQUA.BEACH_Y); g.lineTo(320, 240); g.lineTo(BX - 30, 240); g.fill();
      K.rr(BX, AQUA.BEACH_Y, 84, 3, 1, '#ffe2a8');
      for (const [x, y, r] of [[258, 182, 9], [300, 184, 12], [70, 222, 10], [160, 224, 8]]) { g.fillStyle = '#9a8f84'; g.beginPath(); g.ellipse(x, y, r, r * 0.7, 0, P, 0); g.fill(); }
      K.rr(300, 120, 6, 62, 3, '#8a5a3c'); for (const a of [-2.6, -2.0, -1.2, -0.5]) { g.save(); g.translate(303, 122); g.rotate(a); K.rr(0, -3, 26, 6, 3, '#3f9d4e'); g.restore(); }
    } else {       // rocks and a sunken bolt on the seabed (the treasure chests are drawn by the game: they open)
      for (const [x, y, r] of [[40, 222, 12], [210, 223, 9], [290, 222, 14]]) { g.fillStyle = '#9a8f84'; g.beginPath(); g.ellipse(x, y, r, r * 0.7, 0, P, 0); g.fill(); }
      K.rr(250, 214, 14, 6, 2, '#c9b8c7'); K.rr(255, 210, 4, 12, 1, '#c9b8c7');
    }
    // the glass of the tank: a frame along the water's top
    K.rr(0, AQUA.WATER_TOP - 6, Math.min(320, BX + 10), 4, 1, 'rgba(255,255,255,0.5)');
  }

  // ---- the garage race, top down: the road scrolls down, three lanes, things to dodge. Boxes and
  // circles only, so the N64 (draw_cars) draws the same picture.
  const RACE = { len: 9600, obst: 64, lane: 40, edge: 52, carY: 190 };
  // four tracks (ck_tracks in ck_house.c): the obstacle kinds behave the same and only look different
  const TRACKS = [
    { name: 'Green Hills', ground: 0x5DBB63, deco: 0x3F9D4E, road: 0x5A5E6A, kerbA: 0xE04F4F, kerbB: 0xFFFFFF, line: 0xFFF6D8 },
    { name: 'Desert Dash', ground: 0xE8C77A, deco: 0x6FAF4F, road: 0xA0805A, kerbA: 0xFF8A3D, kerbB: 0xFFF6D8, line: 0xFFE8B0 },
    { name: 'Snowy Pass', ground: 0xEEF6FF, deco: 0x3F7D5E, road: 0x7D8CA3, kerbA: 0x3A6FC0, kerbB: 0xFFFFFF, line: 0xFFFFFF },
    { name: 'Moon Run', ground: 0x2B2840, deco: 0x8F8AA8, road: 0x5A566E, kerbA: 0xFFD23F, kerbB: 0x3A3650, line: 0xC9C4E0 },
  ];
  const raceObst = (track, i, clock) => {
    let kind = ['cone', 'crate', 'oil', 'cone', 'car'][i % 5];
    if (i < 8 && kind === 'oil') kind = 'crate';   // the start is easy: no oil or slow cars yet
    if (i < 16 && kind === 'car') kind = 'cone';
    const t = Math.min(1, i / 48);   // the gaps shrink from about 260 px to 80 px, as on the N64
    const across = track === 1 || track === 2;   // tumbleweeds and penguins wander across the road
    let x = ((i * 7 + Math.floor(i / 4)) % 3 - 1) * RACE.lane;
    if (kind === 'car' && across) x = Math.sin(clock * (track === 2 ? 0.9 : 1.4) + i) * 44;
    return { d: 360 + i * 80 + 4400 * (1 - (1 - t) * (1 - t)) + (kind === 'car' && !across ? clock * 60 : 0), x, kind,
             w: kind === 'oil' ? 20 : kind === 'car' ? (across ? 14 : 18) : kind === 'crate' ? 18 : 12, h: kind === 'oil' ? 14 : kind === 'car' ? (across ? 14 : 28) : kind === 'crate' ? 18 : 12 };
  };
  const box = (g, x, y, w, h, c, a = 1) => { g.globalAlpha = a; g.fillStyle = typeof c === 'number' ? hx(c) : c; g.fillRect(x, y, w, h); g.globalAlpha = 1; };
  const dot = (g, x, y, r, c, a = 1) => { g.globalAlpha = a; g.fillStyle = hx(c); g.beginPath(); g.arc(x, y, r, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; };
  const ring = (g, x, y, r, c) => { g.strokeStyle = hx(c); g.lineWidth = 2; g.beginPath(); g.arc(x, y, r - 1, 0, Math.PI * 2); g.stroke(); };
  const fmod = (a, b) => ((a % b) + b) % b;
  function carTop(g, x, y, hex) {
    for (let k = 0; k < 4; k++) box(g, x + (k % 2 ? 7 : -10), y + (k < 2 ? -11 : 5), 3, 7, 0x1B1426);
    box(g, x - 8, y - 14, 16, 28, hex); box(g, x - 6, y - 7, 12, 5, 0xBFF0FF); box(g, x - 6, y + 6, 12, 3, 0x000000, 0.25);
    box(g, x - 7, y - 15, 3, 2, 0xFFE066); box(g, x + 4, y - 15, 3, 2, 0xFFE066);
  }
  // one obstacle in its track's look (race_thing in draw.c)
  function raceThing(g, track, kind, x, y, t) {
    const k = track * 4 + ['cone', 'crate', 'oil', 'car'].indexOf(kind);
    switch (k) {
      case 0: dot(g, x, y, 6, 0xFF8A3D); dot(g, x, y, 2.5, 0xFFFFFF); break;
      case 1: box(g, x - 9, y - 9, 18, 18, 0x8A5A3C); box(g, x - 7, y - 7, 14, 14, 0xC98A4B); box(g, x - 1, y - 7, 2, 14, 0x8A5A3C); break;
      case 2: dot(g, x, y, 10, 0x1B1426, 0.85); dot(g, x + 3, y - 2, 3, 0x5A4F78); break;
      case 3: carTop(g, x, y, 0x9AA1B0); break;
      case 4: dot(g, x, y, 6, 0x4F9A43); dot(g, x - 5, y - 3, 3, 0x6FAF4F); dot(g, x + 5, y + 2, 3, 0x6FAF4F); dot(g, x, y, 2, 0xFF8FC7); break;   // cactus
      case 5: dot(g, x, y, 10, 0x8C7A66); dot(g, x - 3, y - 3, 5, 0xB09C84); break;   // boulder
      case 6: dot(g, x, y, 11, 0xC9A35A); dot(g, x, y, 6, 0xB08A45); break;   // sand pit
      case 7: ring(g, x, y, 7, 0x9C7A4A); dot(g, x + Math.cos(t * 6) * 3, y + Math.sin(t * 6) * 3, 2.5, 0x8A6A3C); dot(g, x - Math.cos(t * 6) * 3, y - Math.sin(t * 6) * 3, 2.5, 0xB08A55); break;   // tumbleweed
      case 8: dot(g, x, y + 2, 7, 0xFFFFFF); dot(g, x, y - 6, 4.5, 0xFFFFFF); dot(g, x + 3, y - 6, 1.5, 0xFF8A3D); break;   // snowman
      case 9: box(g, x - 9, y - 9, 18, 18, 0x7FC4E6); box(g, x - 7, y - 7, 14, 14, 0xBFE8FF); box(g, x - 5, y - 5, 4, 4, 0xFFFFFF); break;   // ice block
      case 10: dot(g, x, y, 11, 0xA8DCF5, 0.95); dot(g, x - 3, y - 3, 3, 0xFFFFFF); break;   // ice patch
      case 11: dot(g, x, y, 7, 0x1B1426); dot(g, x, y + 1, 4, 0xFFFFFF); dot(g, x, y - 5, 1.8, 0xFF8A3D); break;   // penguin
      case 12: dot(g, x, y, 6, 0xA9A4BC); dot(g, x + 2, y + 1, 2, 0x6E6A80); break;   // moon rock
      case 13: box(g, x - 9, y - 9, 18, 18, 0xC0C4D0); box(g, x - 9, y - 2, 18, 4, 0xFFD23F); box(g, x - 6, y - 7, 4, 3, 0x5AD1FF); break;   // supply pod
      case 14: dot(g, x, y, 11, 0x8F8AA8); dot(g, x + 1, y + 1, 8, 0x3A3650); break;   // crater
      default: carTop(g, x, y, 0xE6E6F0); dot(g, x, y, 3, 0xFF5A5A); break;   // moon rover
    }
  }
  function raceView(g, track, x0, vw, dist, clock) {
    const T = TRACKS[track], cx = x0 + vw / 2, r = track === 3 ? 4 : 7;
    box(g, x0, 0, vw, 240, T.ground);
    for (let k = 0; k < 8; k++) { const y = fmod(k * 60 + dist, 480) - 120; dot(g, x0 + 8 + (k % 2) * 4, y, r, T.deco); dot(g, x0 + vw - 10 - (k % 3) * 3, y + 30, r, T.deco); }
    box(g, cx - 64, 0, 128, 240, T.road);
    for (let y = fmod(dist, 24) - 24; y < 240; y += 24) { box(g, cx - 66, y, 4, 12, T.kerbA); box(g, cx - 66, y + 12, 4, 12, T.kerbB); box(g, cx + 62, y, 4, 12, T.kerbA); box(g, cx + 62, y + 12, 4, 12, T.kerbB); }
    for (let y = fmod(dist, 40) - 40; y < 240; y += 40) { box(g, cx - 21, y, 2, 20, T.line); box(g, cx + 19, y, 2, 20, T.line); }
    const fy = RACE.carY - (RACE.len - dist);
    if (fy > -10 && fy < 250) for (let r = 0; r < 2; r++) for (let k = 0; k < 16; k++) box(g, cx - 64 + k * 8, fy - 8 + r * 4, 8, 4, (k + r) % 2 ? 0xFFFFFF : 0x1B1426);
    for (let i = 0; i < RACE.obst; i++) {
      const o = raceObst(track, i, clock), y = RACE.carY - (o.d - dist);
      if (y < -30 || y > 270) continue;
      raceThing(g, track, o.kind, cx + o.x, y, clock + i);
    }
  }
  // ---- the piano keyboard for the music game: 2 octaves from C4, 14 white keys
  const KEYS = { x: 20, y: 150, w: 20, h: 80, bw: 12, bh: 48, n: 14 };
  // the drum kit (ck_drums in ck_house.c): btn is the button that hits it; x, y, r on the screen
  const DRUMS = [
    ['Kick', 'A', 'a', 'drum', 160, 172, 34, 0xE04F4F], ['Snare', 'B', 'b', 'drum', 104, 160, 20, 0xFFF6D8],
    ['Hi-hat', 'C', 'c', 'cymbal', 62, 120, 22, 0xFFD23F], ['Crash', 'L', 'l', 'cymbal', 92, 72, 26, 0xFFD23F],
    ['Cowbell', 'R', 'r', 'cymbal', 232, 72, 18, 0xC9B8C7], ['Tom', '^', 'up', 'drum', 136, 112, 18, 0x5AD1FF],
    ['Tom', '<', 'left', 'drum', 184, 112, 18, 0x49D17A], ['Floor tom', 'v', 'down', 'drum', 226, 158, 24, 0xC28BFF],
    ['Clap', '>', 'right', 'hands', 276, 120, 18, 0xFF8FC7],
  ].map(([name, label, btn, kind, x, y, r, col]) => ({ name, label, btn, kind, x, y, r, col }));
  // one piece; lit 0..1 brightens it after a hit (the N64 draws the same boxes and circles)
  const drumPiece = (g, d, lit) => {
    const c = hx(d.col), s = 1 + lit * 0.12, r = d.r * s;
    g.fillStyle = '#5a5560'; g.fillRect(d.x - 1, d.y, 2, 200 - d.y);
    if (d.kind === 'drum') {
      g.fillStyle = '#2b2833'; g.beginPath(); g.arc(d.x, d.y, r + 3, 0, Math.PI * 2); g.fill();
      g.fillStyle = c; g.beginPath(); g.arc(d.x, d.y, r, 0, Math.PI * 2); g.fill();
      g.fillStyle = '#fff6e6'; g.beginPath(); g.arc(d.x, d.y, r * 0.62, 0, Math.PI * 2); g.fill();
    } else if (d.kind === 'cymbal') {
      g.fillStyle = '#2b2833'; g.fillRect(d.x - r - 2, d.y - 4 - lit * 3, r * 2 + 4, 8);
      g.fillStyle = c; g.fillRect(d.x - r, d.y - 2 - lit * 3, r * 2, 4);
    } else {
      g.fillStyle = c; g.beginPath(); g.arc(d.x - 6 + lit * 4, d.y, r * 0.6, 0, Math.PI * 2); g.arc(d.x + 6 - lit * 4, d.y, r * 0.6, 0, Math.PI * 2); g.fill();
    }
    if (lit > 0) { g.globalAlpha = lit * 0.5; g.fillStyle = '#ffffff'; g.beginPath(); g.arc(d.x, d.y, r + 4, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; }
    g.fillStyle = '#1b1426'; g.fillRect(d.x - 8, d.y + r + 4, 16, 13);
    g.fillStyle = '#fff6d8'; g.font = '10px sans-serif'; g.textAlign = 'center'; g.fillText(d.label, d.x, d.y + r + 14);
  };
  // food (ck_foods in ck_house.c): [kind 0 circle (w = radius) / 1 box, x, y, w, h, colour]
  const FOODS = [
    ['apple', [[0, 0, 0, 4, 0, 0xE04F4F], [0, -1.5, -1.5, 1.2, 0, 0xFF8A8A], [1, -0.5, -6, 1, 3, 0x6F4630], [1, 0.5, -6, 3, 1.5, 0x49D17A]]],
    ['banana', [[1, -5, -1, 10, 3, 0xFFE066], [1, -6, -2, 2, 2, 0x6F4630], [1, 4, -3, 2, 3, 0xFFE066]]],
    ['cheese', [[1, -5, -3, 10, 6, 0xFFD23F], [0, -2, -1, 1, 0, 0xE0A92B], [0, 2, 1, 1.2, 0, 0xE0A92B]]],
    ['carrot', [[1, -1.5, -3, 3, 9, 0xFF8A3D], [1, -2.5, -6, 2, 3, 0x49D17A], [1, 0.5, -6, 2, 3, 0x49D17A]]],
    ['cake', [[1, -5, -2, 10, 6, 0xFF8FC7], [1, -5, -3, 10, 2, 0xFFF6E6], [0, 0, -5, 1.5, 0, 0xE04F4F]]],
    ['ice cream', [[1, -2, 0, 4, 6, 0xC98A4B], [0, 0, -1, 3.5, 0, 0xFF8FC7], [0, -1, -2, 1, 0, 0xFFFFFF]]],
    ['milk', [[1, -3, -5, 6, 10, 0xFFFFFF], [1, -3, -1, 6, 3, 0x5AD1FF], [1, -2, -7, 4, 2, 0xFFFFFF]]],
    ['pizza', [[1, -5, -3, 10, 6, 0xE8B46A], [1, -5, -3, 10, 1.5, 0xC98A4B], [0, -2, 0, 1.2, 0, 0xE04F4F], [0, 2, 1, 1.2, 0, 0xE04F4F]]],
    ['cookie', [[0, 0, 0, 4, 0, 0xC98A4B], [0, -1, -1, 0.8, 0, 0x6F4630], [0, 1.5, 1, 0.8, 0, 0x6F4630]]],
    ['egg', [[0, 0, 0, 3.5, 0, 0xFFF6E6], [0, -1, -1, 1, 0, 0xFFFFFF]]],
    ['tomato', [[0, 0, 0, 4, 0, 0xE04F4F], [1, -1.5, -5, 3, 1.5, 0x49D17A], [0, -1.5, -1.5, 1, 0, 0xFF8A8A]]],
    ['mushroom', [[1, -1.5, -1, 3, 5, 0xFFF6E6], [1, -5, -4, 10, 3.5, 0xC98A4B], [0, -2, -3, 0.8, 0, 0xFFF6E6]]],
    ['bread', [[1, -4, -4, 8, 8, 0xE8B46A], [1, -4, -5, 8, 2, 0xC98A4B], [1, -3, -3, 6, 6, 0xFFE2A8]]],
    ['omelette', [[0, 0, 0, 5, 0, 0xFFE066], [0, -2, -1, 1.5, 0, 0xFFF6A0], [0, 2, 1, 1, 0, 0xE0A92B]]],
    ['soup', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xE04F4F], [0, -1.5, -2, 1, 0, 0xFF8A3D], [0, 1.5, -1, 1, 0, 0x49D17A]]],
    ['toastie', [[1, -5, -3, 10, 6, 0xC98A4B], [1, -5, -1, 10, 2, 0xFFD23F], [1, -4, -2.5, 8, 1, 0xE8B46A]]],
  ].map(([name, s]) => ({ name, s }));
  // the mailbox's letters (ck_letters)
  const LETTERS = [
    ['the babies', ['We love our new house!', 'Thank you for saving us.', 'Hugs and beeps!']],
    ['Clanky Crane', ['Sorry I swung at you.', 'My magnet was all glitchy.', 'Friends now?']],
    ['the garage', ['The cars are fuelled up.', 'Race a friend and', 'jump the cones!']],
    ['the fish', ['Blub blub blub.', '(That means: please', 'feed us more flakes.)']],
    ['the paint studio', ['The wall is waiting.', 'Fly up high and', 'paint us a picture!']],
    ['the parts shop', ['New hats in stock!', 'Bolts buy cool things.', 'See you soon!']],
  ].map(([from, lines]) => ({ from, lines }));
  const CHESTS = [119, 439];
  const FRIDGE_FOODS = 9;
  const COOK_INGR = [9, 10, 2, 11, 3, 12];   // egg, tomato, cheese, mushroom, carrot, bread (ck_cook_ingr)   // the aquarium's treasure chests (ck_aqua_chest_x)
  return { FOODS, FRIDGE_FOODS, COOK_INGR, LETTERS, CHESTS, DRUMS, drumPiece, LAYOUT, F, NAMES, SPOT_FURN, furn, fish, shrimp, crab, weed, FISH, AQUA, aquaBg, RACE, TRACKS, raceObst, raceView, carTop, KEYS, hx };
})();
