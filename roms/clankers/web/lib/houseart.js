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
    W: 1800, FLOOR: 200, CEIL: 20, PILLAR: 8, DOOR_TOP: 120,
    // x0, x1, name, wall, stripe, floor
    rooms: [
      [0, 400, 'The yard', 0x8FD3FF, 0xBDE8FF, 0x5DBB63],
      [400, 580, 'Bedroom', 0xFFD3E2, 0xFFC1D6, 0x8C6A55],
      [580, 760, 'Kitchen', 0xBDE7D2, 0xA9DCC3, 0xD9CDB8],
      [760, 940, 'Paint studio', 0xFFF1D6, 0xF6E1BA, 0x9C7552],
      [940, 1120, 'Music room', 0xC9B8F0, 0xB8A3E9, 0x7A4E3A],
      [1120, 1300, 'Aquarium', 0x9FD8F0, 0x88CBE6, 0x5D7C99],
      [1300, 1520, 'Garage', 0xB5BAC4, 0xA4AAB6, 0x6E7280],
      [1520, 1800, 'Arcade', 0x3A2E5C, 0x46386E, 0x2B2240],
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
      [1575, 'pinball', 'Play pinball'],
      [1660, 'blast', 'Play Bug Blaster'],
      [345, 'pond', 'Go fishing'],
      [1745, 'bolt', 'Play Bolt Ball'],
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
  // the arcade's third cabinet: Bolt Ball, two paddles and a ball on its screen, a bolt on top
  F.paddle = g => {
    K.rr(-20, -94, 40, 94, 4, '#2f9e6a'); K.rr(-20, -94, 40, 13, 3, '#1b1426'); K.rr(-16, -89, 10, 2, 1, '#5ad1ff'); K.rr(6, -89, 10, 2, 1, '#5ad1ff');
    g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(3, -93); g.lineTo(-4, -87); g.lineTo(0, -87); g.lineTo(-2, -82); g.lineTo(4, -89); g.lineTo(0, -89); g.fill();   // a bolt on the marquee
    K.rr(-15, -78, 30, 26, 2, '#1b1426');
    K.rr(-12, -72, 2, 9, 1, '#ffd23f'); K.rr(10, -68, 2, 9, 1, '#8fd3ff'); K.circ(2, -66, 1.8, '#fff6d8');
    for (let y = -76; y < -54; y += 5) K.rr(-0.5, y, 1, 2.5, 0, '#46386e');
    K.rr(-23, -50, 46, 9, 2, '#1f6e4a'); K.rr(-12, -55, 2, 6, 1, '#9aa1b0'); K.circ(-11, -55, 3, '#ffd23f'); K.rr(9, -55, 2, 6, 1, '#9aa1b0'); K.circ(10, -55, 3, '#5ad1ff');
    K.rr(-8, -26, 16, 12, 2, '#1f6e4a'); K.rr(-2, -22, 4, 4, 1, '#ffd23f');
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
  const NAMES = ['tree', 'mailbox', 'counter', 'fridge', 'stove', 'table', 'easel', 'piano', 'tank', 'bed', 'car', 'plant', 'drum', 'lamp', 'tools', 'sign', 'pinball', 'arcade', 'pond', 'paddle'];
  const SPOT_FURN = { shop: 'counter', snack: 'fridge', paint: 'easel', piano: 'piano', aqua: 'tank', bed: 'bed', cars: 'car', drums: 'drum', mail: 'mailbox', tree: 'tree', back: 'sign', stove: 'stove', pinball: 'pinball', blast: 'arcade', pond: 'pond', bolt: 'paddle' };
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
  const AQUA = { W: 1920, WATER_TOP: 44, SEABED: 222, BEACH_X: 1836, BEACH_LX: 236, BEACH_Y: 182, SECTIONS: ['Kelp Forest', 'Coral Reef', 'Shipwreck', 'Deep Blue', 'Rocky Caves'], SEC_W: 320,
    // the kinds of fish living in each section (ck_aqua_pop): 0-2 the classic fish, then angel, puffer, jelly, eel, clown, tang, ray
    POP: [[0, 1, 2, 0, 8, 1], [7, 8, 3, 7, 4, 3], [6, 0, 4, 6, 2, 1], [5, 5, 9, 5, 9, 8], [6, 4, 1, 2, 8, 3]] };   // BEACH_LX: on the last screen
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

  // ---- the garage race from behind the car: the road runs to a horizon and bends; strips of road
  // from the bottom up, things shrinking with distance. Boxes and circles only, so the N64
  // (race_view in draw.c) draws the same picture.
  const RACE = { len: 9600, obst: 36, lane: 40, road: 64, edge: 84, pads: 14, HZ: 96, ZC: 2880, CAR_Z: 26, PX0: 2.66, BEND: 0.0026 };
  // four tracks (ck_tracks in ck_house.c): the obstacle kinds behave the same and only look different
  const TRACKS = [
    { name: 'Green Hills', ground: 0x5DBB63, deco: 0x3F9D4E, road: 0x5A5E6A, kerbA: 0xE04F4F, kerbB: 0xFFFFFF, line: 0xFFF6D8, sky: 0x8FD3FF, hills: 0x3F9D4E, amp: 0.8 },
    { name: 'Desert Dash', ground: 0xE8C77A, deco: 0x6FAF4F, road: 0xA0805A, kerbA: 0xFF8A3D, kerbB: 0xFFF6D8, line: 0xFFE8B0, sky: 0xFFC27A, hills: 0xC9875A, amp: 0.7 },
    { name: 'Snowy Pass', ground: 0xEEF6FF, deco: 0x3F7D5E, road: 0x7D8CA3, kerbA: 0x3A6FC0, kerbB: 0xFFFFFF, line: 0xFFFFFF, sky: 0xBFD9F2, hills: 0x8FA8C8, amp: 1.0 },
    { name: 'Moon Run', ground: 0x2B2840, deco: 0x8F8AA8, road: 0x5A566E, kerbA: 0xFFD23F, kerbB: 0x3A3650, line: 0xC9C4E0, sky: 0x0F0A19, hills: 0x4A4560, amp: 0.6 },
  ];
  const clampv = (v, a, b) => Math.max(a, Math.min(b, v));
  // how hard the road bends at distance d (ck_race_curve)
  const raceCurve = (track, d) => {
    if (d < 500) return 0;
    const ramp = d < 1200 ? (d - 500) / 700 : 1;
    return TRACKS[track].amp * ramp * clampv(1.8 * Math.sin(d * 0.0011 + track * 1.9), -1, 1) * Math.abs(clampv(2.2 * Math.sin(d * 0.00031 + 0.7 + track), -1, 1));
  };
  // boost pads and ramps (ck_race_pad): 28 across, 30 long
  const racePad = (track, i) => ({ d: 800 + i * 620 + (track * 37) % 100, x: ((i * 2 + track) % 3 - 1) * RACE.lane, kind: i % 3 === 2 ? 'ramp' : 'boost' });
  const raceObst = (track, i, clock) => {
    let kind = ['cone', 'crate', 'oil', 'cone', 'car'][i % 5];
    if (i < 5 && kind === 'oil') kind = 'crate';   // the start is easy: no oil or slow cars yet
    if (i < 10 && kind === 'car') kind = 'cone';
    const t = Math.min(1, i / 30);   // the gaps shrink from about 390 to 150, as on the N64
    const across = track === 1 || track === 2;   // tumbleweeds and penguins wander across the road
    let x = ((i * 7 + Math.floor(i / 4)) % 3 - 1) * RACE.lane;
    if (kind === 'car' && across) x = Math.sin(clock * (track === 2 ? 0.9 : 1.4) + i) * 44;
    return { d: 400 + i * 150 + 3600 * (1 - (1 - t) * (1 - t)) + (kind === 'car' && !across ? clock * 60 : 0), x, kind,
             w: kind === 'oil' ? 20 : kind === 'car' ? (across ? 14 : 18) : kind === 'crate' ? 18 : 12, h: kind === 'oil' ? 14 : kind === 'car' ? (across ? 14 : 28) : kind === 'crate' ? 18 : 12 };
  };
  const box = (g, x, y, w, h, c, a = 1) => { g.globalAlpha = a; g.fillStyle = typeof c === 'number' ? hx(c) : c; g.fillRect(x, y, w, h); g.globalAlpha = 1; };
  const dot = (g, x, y, r, c, a = 1) => { if (r <= 0) return; g.globalAlpha = a; g.fillStyle = hx(c); g.beginPath(); g.arc(x, y, r, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; };
  const ring = (g, x, y, r, c, lw = 2) => { g.strokeStyle = hx(c); g.lineWidth = lw; g.beginPath(); g.arc(x, y, Math.max(0.1, r - lw / 2), 0, Math.PI * 2); g.stroke(); };
  const fmod = (a, b) => ((a % b) + b) % b;
  const shade = (c, f) => (Math.min(255, Math.floor((c >> 16 & 255) * f)) << 16) | (Math.min(255, Math.floor((c >> 8 & 255) * f)) << 8) | Math.min(255, Math.floor((c & 255) * f));
  // a car from behind standing on (x, y), s px per unit, leaning -1..1 (car_back)
  function carBack(g, x, y, s, hex, lean, boost, t) {
    const w = 22 * s, l = lean * 2 * s;
    box(g, x - w / 2, y - 1.5 * s, w, 3 * s, 0x000000, 0.35);
    box(g, x - w / 2, y - 6 * s, 5 * s, 6 * s, 0x1B1426); box(g, x + w / 2 - 5 * s, y - 6 * s, 5 * s, 6 * s, 0x1B1426);
    box(g, x - w / 2 + s + l * 0.3, y - 12 * s, w - 2 * s, 7 * s, hex);
    box(g, x - 7 * s + l, y - 17 * s, 14 * s, 6 * s, shade(hex, 0.8)); box(g, x - 5.5 * s + l, y - 16 * s, 11 * s, 3.5 * s, 0x8FD3FF);
    box(g, x - w / 2 + 2.5 * s + l * 0.3, y - 10 * s, 3.5 * s, 2 * s, 0xFF5A5A); box(g, x + w / 2 - 6 * s + l * 0.3, y - 10 * s, 3.5 * s, 2 * s, 0xFF5A5A);
    box(g, x - 4 * s + l * 0.3, y - 7 * s, 8 * s, 2 * s, 0x2B2833);
    if (boost) for (let k = 0; k < 2; k++) dot(g, x + (k ? 4 : -4) * s, y - 4 * s + Math.sin(t * 40 + k) * s, (2 + Math.abs(Math.sin(t * 30 + k * 2))) * s, k ? 0xFFD23F : 0xFF8A3D);
  }
  // one obstacle in its track's look, standing on (x, y), s px per unit (race_thing in draw.c)
  function raceThing(g, track, kind, x, y, s, t) {
    const k = track * 4 + ['cone', 'crate', 'oil', 'car'].indexOf(kind), B = (a, b, w, h, c, al) => box(g, x + a * s, y + b * s, w * s, h * s, c, al), D = (a, b, r, c) => dot(g, x + a * s, y + b * s, r * s, c);
    switch (k) {
      case 0: B(-6, -2, 12, 2, 0xFF8A3D); B(-4, -8, 8, 6, 0xFF8A3D); B(-3, -6, 6, 2, 0xFFFFFF); B(-2, -13, 4, 5, 0xFF8A3D); break;
      case 1: B(-9, -18, 18, 18, 0x8A5A3C); B(-7, -16, 14, 14, 0xC98A4B); B(-1, -16, 2, 14, 0x8A5A3C); break;
      case 2: B(-10, -3, 20, 4, 0x1B1426, 0.85); B(-4, -2.5, 6, 1.5, 0x5A4F78); break;
      case 3: carBack(g, x, y, s * 0.85, 0x9AA1B0, 0, false, t); break;
      case 4: B(-2.5, -18, 5, 18, 0x4F9A43); B(-8, -13, 3, 7, 0x6FAF4F); B(-8, -7, 6, 2.5, 0x6FAF4F); B(5, -15, 3, 6, 0x6FAF4F); B(2, -10, 6, 2.5, 0x6FAF4F); D(0, -18, 1.5, 0xFF8FC7); break;   // cactus
      case 5: D(0, -8, 10, 0x8C7A66); D(-3, -11, 5, 0xB09C84); break;   // boulder
      case 6: B(-11, -3, 22, 4, 0xB08A45); B(-7, -2, 14, 2, 0x9C7A3A); break;   // sand pit
      case 7: ring(g, x, y - 7 * s, 7 * s, 0x9C7A4A, 2 * s); D(Math.cos(t * 6) * 3, -7 + Math.sin(t * 6) * 3, 2.5, 0x8A6A3C); D(-Math.cos(t * 6) * 3, -7 - Math.sin(t * 6) * 3, 2.5, 0xB08A55); break;   // tumbleweed
      case 8: D(0, -6, 7, 0xFFFFFF); D(0, -16, 4.5, 0xFFFFFF); B(-1, -16, 3, 1.5, 0xFF8A3D); D(-1.5, -17.5, 0.8, 0x1B1426); D(1.5, -17.5, 0.8, 0x1B1426); break;   // snowman
      case 9: B(-9, -18, 18, 18, 0x7FC4E6); B(-7, -16, 14, 14, 0xBFE8FF); B(-5, -14, 4, 4, 0xFFFFFF); break;   // ice block
      case 10: B(-11, -3, 22, 4, 0xA8DCF5, 0.95); B(-5, -2.5, 4, 1.5, 0xFFFFFF); break;   // ice patch
      case 11: D(0, -8, 7, 0x1B1426); D(0, -7, 4.5, 0xFFFFFF); B(-1.5, -13, 3, 2, 0xFF8A3D); B(-4, -1.5, 3, 1.5, 0xFF8A3D); B(1, -1.5, 3, 1.5, 0xFF8A3D); break;   // penguin
      case 12: D(0, -5, 6, 0xA9A4BC); D(2, -4, 2, 0x6E6A80); break;   // moon rock
      case 13: B(-9, -18, 18, 18, 0xC0C4D0); B(-9, -11, 18, 4, 0xFFD23F); B(-6, -16, 4, 3, 0x5AD1FF); break;   // supply pod
      case 14: B(-11, -3, 22, 4, 0x8F8AA8); B(-8, -2, 16, 2.5, 0x3A3650); break;   // crater
      default: carBack(g, x, y, s * 0.85, 0xE6E6F0, 0, false, t); B(-0.5, -24, 1, 8, 0x8F8AA8); D(0, -24, 1.5, 0xFF5A5A); break;   // moon rover
    }
  }
  // where depth zr ahead of the camera and x across lands on the screen (race_project)
  function raceProject(track, a, camx, cx, vw, zr, x) {
    if (zr < 4) return null;
    const s = RACE.ZC / zr / (240 - RACE.HZ) * RACE.PX0 * vw / 320, dz = Math.max(0, zr - RACE.CAR_Z);
    return { s, y: RACE.HZ + RACE.ZC / zr, x: cx + (x + 0.5 * RACE.BEND * raceCurve(track, a + dz / 2) * dz * dz - camx) * s };
  }
  // one player's view; me is their car { d, x, sky }, others are the other cars { d, x, jz, hex, lean, boost }
  function raceView(g, track, x0, vw, me, others, clock, t) {
    const T = TRACKS[track], cx = x0 + vw / 2, a = me.d, camx = me.x * 0.7, cam = a - RACE.CAR_Z;
    box(g, x0, 0, vw, RACE.HZ, T.sky);
    if (track === 3) for (let k = 0; k < 24; k++) dot(g, x0 + fmod(k * 47 - me.sky * 0.3, vw), 6 + (k * 29) % 70, (k % 3) * 0.4 + 0.4, 0xFFFFFF);
    else for (let k = 0; k < 3; k++) dot(g, x0 + fmod(k * 70 + 20 - me.sky * 0.15, vw + 40) - 20, 22 + k * 9, 9, 0xFFFFFF, 0.8);
    for (let k = 0; k < 8; k++) { const r = 18 + (k * 7) % 14; dot(g, x0 + fmod(k * 61 - me.sky, vw + 80) - 40, RACE.HZ + r * 0.35, r, T.hills); }
    for (let y = 238; y >= RACE.HZ; y -= 2) {
      const zr = RACE.ZC / (y + 1 - RACE.HZ), z = cam + zr, p = raceProject(track, a, camx, cx, vw, zr, 0), band = Math.floor(z / 32) & 1;
      const hw = RACE.road * p.s, kw = 7 * p.s;
      box(g, x0, y, vw, 2, band ? T.ground : shade(T.ground, 0.9));
      box(g, p.x - hw - kw, y, kw, 2, band ? T.kerbA : T.kerbB); box(g, p.x + hw, y, kw, 2, band ? T.kerbA : T.kerbB);
      if (z > RACE.len && z < RACE.len + 16) for (let k = 0; k < 8; k++) box(g, p.x - hw + k * hw / 4, y, hw / 4, 2, (Math.floor(z / 8) + k) % 2 ? 0xFFFFFF : 0x1B1426);
      else {
        box(g, p.x - hw, y, hw * 2, 2, band ? T.road : shade(T.road, 1.08));
        if (band) { const lw = Math.max(1, 2 * p.s); box(g, p.x - 20 * p.s - lw / 2, y, lw, 2, T.line); box(g, p.x + 20 * p.s - lw / 2, y, lw, 2, T.line); }
      }
    }
    const list = [];
    for (let i = 0; i < RACE.obst; i++) { const o = raceObst(track, i, clock), zr = o.d - cam; if (zr > 6 && zr < 1600) list.push({ zr, x: o.x, what: 0, kind: o.kind, i }); }
    for (let i = 0; i < RACE.pads; i++) { const o = racePad(track, i), zr = o.d - cam; if (zr > 6 && zr < 1600) list.push({ zr, x: o.x, what: 1, kind: o.kind, i }); }
    for (const o of others) { const zr = o.d - cam; if (zr > 6 && zr < 1600) list.push({ zr, x: o.x, what: 2, o }); }
    list.sort((p, q) => q.zr - p.zr);
    for (const r of list) {
      const p = raceProject(track, a, camx, cx, vw, r.zr, r.x);
      if (!p || p.s * 20 < 1.5) continue;
      const s = p.s;
      if (r.what === 1) {
        const p2 = raceProject(track, a, camx, cx, vw, r.zr + 30, r.x), h = Math.max(1, p.y - p2.y);
        if (r.kind === 'boost') { box(g, p.x - 14 * s, p.y - h, 28 * s, h, 0xFF8A3D); for (let k = 0; k < 3; k++) box(g, p.x - 10 * s, p.y - h + h * (k * 0.33 + 0.08), 20 * s, Math.max(1, h * 0.14), 0xFFE066); }
        else { box(g, p.x - 14 * s, p.y - h - 6 * s, 28 * s, h + 6 * s, 0x8A6A4A); box(g, p.x - 14 * s, p.y - h - 6 * s, 28 * s, Math.max(1, 2 * s), 0xFFD23F); for (let k = 0; k < 4; k++) box(g, p.x - 12 * s + k * 7 * s, p.y - h - 4 * s, 3 * s, h + 3 * s, 0xA8865E); }
      } else if (r.what === 2) carBack(g, p.x, p.y - Math.floor(r.o.jz) * s, s, r.o.hex, r.o.lean, r.o.boost, t);
      else raceThing(g, track, r.kind, p.x, p.y, s, t + r.i);
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
    ['potato', [[0, 0, 0, 4.5, 0, 0xC9A35A], [0, -1.5, -1, 0.8, 0, 0x8A6A3C], [0, 1.5, 1.5, 0.7, 0, 0x8A6A3C]]],
    ['rice', [[1, -4, -4, 8, 9, 0xE8D9B8], [1, -4, -4, 8, 2, 0xC9B27A], [0, 0, 1, 1.5, 0, 0xFFFFFF]]],
    ['sausage', [[1, -5, -1.5, 10, 3.5, 0xC0504A], [0, -5, 0.25, 1.75, 0, 0xC0504A], [0, 5, 0.25, 1.75, 0, 0xC0504A], [1, -3, -1, 6, 1, 0xE07A6A]]],
    ['pasta', [[1, -3, -6, 1, 12, 0xFFE2A8], [1, -1, -6, 1, 12, 0xFFE2A8], [1, 1, -6, 1, 12, 0xFFE2A8], [1, 3, -6, 1, 12, 0xFFE2A8], [1, -4, -1, 9, 2, 0xE04F4F]]],
    // the stove's newer ingredients (FOOD_STRAWBERRY on)
    ['strawberry', [[0, 0, 1, 4, 0, 0xE0303F], [1, -3, -4, 6, 2, 0x49D17A], [0, -1.5, 0, 0.6, 0, 0xFFE066], [0, 1.5, 2, 0.6, 0, 0xFFE066]]],
    ['orange', [[0, 0, 0, 4.5, 0, 0xFF9A2A], [0, -1.5, -1.5, 1.2, 0, 0xFFC27A], [1, -0.5, -6, 1, 2, 0x6F4630]]],
    ['lemon', [[0, 0, 0, 4, 0, 0xFFE94A], [0, -4, 0, 1.5, 0, 0xFFE94A], [0, 4, 0, 1.5, 0, 0xFFE94A], [0, -1, -1.5, 1, 0, 0xFFF6A0]]],
    ['grapes', [[0, -2, -2, 2, 0, 0x9A4AD1], [0, 2, -2, 2, 0, 0x9A4AD1], [0, 0, 1, 2, 0, 0x9A4AD1], [0, 0, -4.5, 1.6, 0, 0xB06AE6], [1, -0.5, -7, 1, 2.5, 0x6F4630]]],
    ['cherry', [[0, -2.5, 2, 2.5, 0, 0xC0203A], [0, 2.5, 2, 2.5, 0, 0xC0203A], [1, -1.5, -5, 1, 5, 0x49A04A], [1, 1, -5, 1, 5, 0x49A04A]]],
    ['pear', [[0, 0, 2, 4, 0, 0xB6D84A], [0, 0, -2, 2.5, 0, 0xB6D84A], [1, -0.5, -6, 1, 2, 0x6F4630]]],
    ['pineapple', [[1, -3.5, -2, 7, 8, 0xE8B43A], [1, -3.5, 0, 7, 1, 0xA0782A], [1, -3.5, 3, 7, 1, 0xA0782A], [1, -2.5, -6, 1.5, 4, 0x49D17A], [1, 1, -6, 1.5, 4, 0x49D17A]]],
    ['blueberry', [[0, -2.5, 1, 2.2, 0, 0x3A5AD1], [0, 2.5, 1, 2.2, 0, 0x3A5AD1], [0, 0, -2.5, 2.2, 0, 0x4A6AE6], [0, 0, -2.5, 0.6, 0, 0x1B2A6A]]],
    ['watermelon', [[0, 0, 0, 5, 0, 0x3FA04A], [0, 0, 0, 4, 0, 0xFF5A6A], [0, -1.5, 0, 0.5, 0, 0x1B1426], [0, 1.5, 1, 0.5, 0, 0x1B1426]]],
    ['onion', [[0, 0, 1, 4, 0, 0xE0B8D0], [0, 0, 1, 2.5, 0, 0xF2D8E6], [1, -0.5, -5, 1, 2.5, 0x8A6A3C]]],
    ['pepper', [[1, -3.5, -3, 7, 8, 0xE0303F], [0, 0, -3, 3.5, 0, 0xE0303F], [1, -0.5, -6, 1.5, 2.5, 0x49A04A]]],
    ['broccoli', [[1, -1, 0, 2, 5, 0x8FD06A], [0, -2.5, -1.5, 2.2, 0, 0x2F8A3C], [0, 2.5, -1.5, 2.2, 0, 0x2F8A3C], [0, 0, -3.5, 2.2, 0, 0x3FA04A]]],
    ['peas', [[1, -5, -1.5, 10, 3.5, 0x5FB04A], [0, -2.5, 0, 1.3, 0, 0x9AE06A], [0, 0, 0, 1.3, 0, 0x9AE06A], [0, 2.5, 0, 1.3, 0, 0x9AE06A]]],
    ['corn', [[1, -2, -5, 4, 10, 0xFFD23F], [1, -1, -5, 0.6, 10, 0xE0A92B], [1, 0.6, -5, 0.6, 10, 0xE0A92B], [1, -3.5, -1, 1.5, 6, 0x6FBF4A], [1, 2, -1, 1.5, 6, 0x6FBF4A]]],
    ['lettuce', [[0, 0, 0, 4.5, 0, 0x6FCF5A], [0, -1.5, -1, 2.5, 0, 0x9AE67A], [0, 1.5, 1.5, 1.5, 0, 0x4FAF4A]]],
    ['cucumber', [[1, -5, -1.5, 10, 3.5, 0x3F8A3C], [0, -5, 0.25, 1.75, 0, 0x3F8A3C], [0, 5, 0.25, 1.75, 0, 0x3F8A3C], [1, -3, -1, 6, 0.8, 0x6FBF6A]]],
    ['pumpkin', [[0, 0, 1, 4.5, 0, 0xFF8A2A], [1, -0.5, -3, 1, 8, 0xE0701A], [1, -0.5, -5, 1.5, 2.5, 0x6F4630]]],
    ['beans', [[0, -2, 0, 1.8, 0, 0xB8402A], [0, 1, -1.5, 1.8, 0, 0xB8402A], [0, 1.5, 1.5, 1.8, 0, 0xB8402A], [0, -0.5, 2.5, 1.5, 0, 0xA0301A]]],
    ['chicken', [[0, 1, 0, 3.5, 0, 0xC9783A], [0, 1, -0.5, 2.5, 0, 0xE09A5A], [1, -5, -0.5, 3.5, 1.5, 0xFFF6E6], [0, -5, -0.8, 1, 0, 0xFFF6E6], [0, -5, 0.8, 1, 0, 0xFFF6E6]]],
    ['fish', [[0, 0, 0, 3.5, 0, 0x8FB8D8], [0, -4, 0, 2.2, 0, 0x6F98B8], [0, 2, -1, 0.7, 0, 0x1B1426]]],
    ['shrimp', [[0, 1, 0, 3, 0, 0xFF8A6A], [0, -1.5, 2, 2, 0, 0xFF8A6A], [0, -3, 3, 1.2, 0, 0xFFAA8A], [0, 2, -1, 0.6, 0, 0x1B1426]]],
    ['bacon', [[1, -5, -2.5, 10, 2, 0xD0505A], [1, -5, -0.5, 10, 1, 0xFFD8D0], [1, -5, 0.5, 10, 2, 0xD0505A]]],
    ['ham', [[0, 0, 0, 4.5, 0, 0xF09AA8], [0, 0, 0, 3, 0, 0xFFB8C0], [0, -1, -1, 0.8, 0, 0xFFFFFF]]],
    ['tofu', [[1, -4, -3, 8, 6, 0xF8F0DA], [1, -4, -3, 8, 1.2, 0xE0D6B8], [1, -1, -0.5, 2, 2, 0xE8E0C8]]],
    ['chocolate', [[1, -4.5, -3.5, 9, 7, 0x6A3A20], [1, -4, -3, 4, 3, 0x8A5030], [1, 0.5, -3, 4, 3, 0x8A5030], [1, -4, 0.5, 4, 3, 0x8A5030]]],
    ['honey', [[1, -3.5, -3, 7, 8, 0xFFB020], [1, -3.5, -5, 7, 2, 0xC98A4B], [1, -2, -1, 4, 2, 0xFFF6A0]]],
    ['yogurt', [[1, -3.5, -3, 7, 7, 0xFFFFFF], [1, -3.5, -4, 7, 1.5, 0xC28BFF], [0, 0, 0.5, 1.5, 0, 0xFF8FC7]]],
    ['cream', [[0, 0, 2, 4, 0, 0xFFFFFF], [0, 0, -1, 3, 0, 0xFFF6F0], [0, 0, -3.5, 1.8, 0, 0xFFFFFF], [0, 0.5, -5, 0.8, 0, 0xFFFFFF]]],
    ['butter', [[1, -5, -2, 10, 5, 0xFFE88A], [1, -5, -2, 10, 1.2, 0xFFF6C0], [1, -5, 2, 10, 1, 0xD8C060]]],
    ['jam', [[1, -3.5, -3, 7, 8, 0xC0203A], [1, -4, -5, 8, 2.5, 0xFFFFFF], [1, -2, 0, 4, 2.5, 0xFFF6E6]]],
    ['flour', [[1, -4, -4, 8, 9, 0xF2EAD8], [1, -4, -4, 8, 1.5, 0xC9B27A], [1, -2, 0, 4, 2.5, 0x5AD1FF]]],
    ['noodles', [[0, 0, 1, 4.5, 0, 0xFFFFFF], [1, -3, -2, 6, 1, 0xF2D27A], [1, -3.5, 0, 7, 1, 0xF2D27A], [1, -3, 2, 6, 1, 0xF2D27A], [1, 1, -6, 0.7, 6, 0x8A5A3C]]],
    ['tortilla', [[0, 0, 0, 5, 0, 0xF2D49B], [0, -2, -1.5, 0.8, 0, 0xC98A4B], [0, 1.5, 1.5, 0.8, 0, 0xC98A4B], [0, 2, -2, 0.6, 0, 0xC98A4B]]],
    ['oats', [[1, -3.5, -4, 7, 9, 0xE0C080], [1, -3.5, -4, 7, 2, 0x5AD1FF], [0, 0, 1, 1.5, 0, 0xC9A35A]]],
    ['nuts', [[0, -2, 0, 2.2, 0, 0xA0704A], [0, 2, -1, 2.2, 0, 0xB8844A], [0, 0, 2, 2, 0, 0x8A5A3C]]],
    ['sprinkles', [[1, -3, -4, 6, 9, 0xFFFFFF], [1, -3, -4, 6, 2, 0xFF8FC7], [1, -2, 0, 1, 2, 0xE04F4F], [1, 0, -1, 1, 2, 0x5AD1FF], [1, 1.5, 1, 1, 2, 0xFFD23F]]],
    ['Omelette', [[0, 0, 0, 5, 0, 0xFFE066], [0, -2, -1, 1.5, 0, 0xFFF6A0], [0, 2, 1, 1, 0, 0xE0A92B]]],
    ['Cheesy omelette', [[0, 0, 0, 5, 0, 0xFFE066], [0, -2, -1, 1.5, 0, 0xFFF6A0], [1, -3, 0, 3, 2, 0xFFB020], [1, 1, -2, 3, 2, 0xFFB020]]],
    ['Mushroom omelette', [[0, 0, 0, 5, 0, 0xFFE066], [0, -2, -1, 1.5, 0, 0xFFF6A0], [0, -2, 1.5, 1.3, 0, 0xC98A4B], [0, 2, -1, 1.3, 0, 0xC98A4B], [1, 1.6, -0.5, 0.8, 1.5, 0xFFF6E6]]],
    ['Tomato omelette', [[0, 0, 0, 5, 0, 0xFFE066], [0, -2, -1, 1.5, 0, 0xFFF6A0], [0, -2, 1.5, 1.3, 0, 0xE04F4F], [0, 2, -1, 1.3, 0, 0xE04F4F], [0, 1, 2, 1, 0, 0xE04F4F]]],
    ['Potato omelette', [[0, 0, 0, 5, 0, 0xF0C850], [0, -2, -1, 1.5, 0, 0xFFE066], [0, -2, 1.5, 1.3, 0, 0xC9A35A], [0, 2, -1, 1.3, 0, 0xC9A35A], [0, 1.5, 2, 1, 0, 0xC9A35A]]],
    ['Big breakfast', [[0, 0, 0, 6, 0, 0xFFFFFF], [0, -2, -1, 2.8, 0, 0xFFF6E6], [0, -2, -1, 1.3, 0, 0xFFB020], [1, 1, -3, 4, 1.8, 0xC0504A], [1, 1, 0, 4, 1.8, 0xC0504A], [0, -1, 3, 1.2, 0, 0xE04F4F]]],
    ['Toast', [[1, -5, -3, 10, 6, 0xC98A4B], [1, -4, -2.5, 8, 5, 0xE8B46A], [1, -2, -1.5, 4, 2.5, 0xFFF6A0]]],
    ['Cheese toastie', [[1, -5, -3, 10, 6, 0xC98A4B], [1, -4, -2.5, 8, 5, 0xE8B46A], [1, -5, -1, 10, 2, 0xFFD23F], [1, -4, 1, 2, 1.5, 0xFFD23F]]],
    ['Pizza toastie', [[1, -5, -3, 10, 6, 0xC98A4B], [1, -4, -2.5, 8, 5, 0xE8B46A], [1, -4, -2, 8, 4, 0xE04F4F], [0, -1.5, -0.5, 1.2, 0, 0xFFD23F], [0, 1.5, 0.5, 1.2, 0, 0xFFD23F], [0, 0, -1.5, 0.8, 0, 0x49D17A]]],
    ['Mushroom toast', [[1, -5, -3, 10, 6, 0xC98A4B], [1, -4, -2.5, 8, 5, 0xE8B46A], [0, -2, -1, 1.5, 0, 0xC98A4B], [0, 2, 0, 1.5, 0, 0xC98A4B], [0, 0, -2, 1, 0, 0xFFF6E6]]],
    ['Tomato soup', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xE04F4F], [0, -1.5, -2, 1, 0, 0xFFFFFF]]],
    ['Mushroom soup', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xC9A37A], [0, 1, -2, 1.3, 0, 0x8A6A4A], [0, -1.5, -1, 1, 0, 0x8A6A4A]]],
    ['Carrot soup', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFF8A3D], [0, -1, -2, 1, 0, 0x49D17A]]],
    ['Cheese soup', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFFD23F], [0, 1, -2, 1, 0, 0xFFF6A0], [0, -1.5, -1, 0.8, 0, 0xFFF6A0]]],
    ['Veggie stew', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xA0603A], [0, -2, -2, 1, 0, 0xFF8A3D], [0, 1, -1, 1, 0, 0x49D17A], [0, 2, -2.5, 1, 0, 0xC9A35A]]],
    ['Sausage stew', [[1, -5, -1, 10, 5, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xA0603A], [1, -3, -2.5, 3, 1.2, 0xC0504A], [1, 1, -1.5, 3, 1.2, 0xC0504A]]],
    ['Hot dog', [[1, -6, -1, 12, 4, 0xE8B46A], [1, -5, -2.5, 10, 2.5, 0xC0504A], [1, -4, -2, 8, 0.8, 0xFFD23F]]],
    ['Cheesy hot dog', [[1, -6, -1, 12, 4, 0xE8B46A], [1, -5, -2.5, 10, 2.5, 0xC0504A], [1, -5, -3, 10, 1.4, 0xFFB020], [1, -3, -1.8, 1.5, 1.5, 0xFFB020]]],
    ['Hot dog and ketchup', [[1, -6, -1, 12, 4, 0xE8B46A], [1, -5, -2.5, 10, 2.5, 0xC0504A], [1, -4, -2, 2, 0.8, 0xE04F4F], [1, -1, -2.6, 2, 0.8, 0xE04F4F], [1, 2, -2, 2, 0.8, 0xE04F4F]]],
    ['Spaghetti and sauce', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFE2A8], [1, -3, -2, 6, 0.8, 0xF0CC88], [0, 0, -2, 2.2, 0, 0xE04F4F], [0, 1, -2.5, 0.8, 0, 0x49D17A]]],
    ['Spaghetti and meatballs', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFE2A8], [1, -3, -2, 6, 0.8, 0xF0CC88], [0, 0, -2, 2.4, 0, 0xE04F4F], [0, -1.5, -1.5, 1.3, 0, 0x8A4A3A], [0, 1.5, -2, 1.3, 0, 0x8A4A3A]]],
    ['Mac and cheese', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFB020], [0, -1.5, -1.5, 0.9, 0, 0xFFE066], [0, 1.5, -0.5, 0.9, 0, 0xFFE066], [0, 0, -3, 0.9, 0, 0xFFE066]]],
    ['Mushroom pasta', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFE2A8], [1, -3, -2, 6, 0.8, 0xF0CC88], [0, -1.5, -1.5, 1.3, 0, 0xC98A4B], [0, 1.5, -1, 1.3, 0, 0xC98A4B]]],
    ['Buttered pasta', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFE2A8], [1, -3, -2, 6, 0.8, 0xF0CC88], [1, -1, -2.5, 2, 1.5, 0xFFF6A0]]],
    ['Egg fried rice', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xF5E08A], [0, -2, -2, 1, 0, 0xFFD23F], [0, 2, -1, 1, 0, 0x49D17A], [0, 0, -3, 0.8, 0, 0xFFF6E6]]],
    ['Sausage rice', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFFFFFF], [1, -3, -2.5, 3, 1.3, 0xC0504A], [1, 0.5, -1.5, 3, 1.3, 0xC0504A]]],
    ['Veggie rice', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFFFFFF], [0, -2, -2, 1, 0, 0xFF8A3D], [0, 2, -1, 1, 0, 0x49D17A], [0, 0, -3, 0.9, 0, 0xC98A4B]]],
    ['Rice pudding', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFFF0D8], [0, 0, -2, 1.3, 0, 0xFFE066], [0, 1.5, -1, 0.7, 0, 0xC98A4B]]],
    ['Rice bowl', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xFFFFFF], [0, 0, -3, 1.2, 0, 0xFFFFFF], [1, 1, -5, 0.6, 4, 0x8A5A3C]]],
    ['Apple pie', [[0, 0, 0, 5.5, 0, 0xC98A4B], [0, 0, -0.5, 4, 0, 0xE8B46A], [1, -3, -1, 6, 1, 0xC98A4B], [1, -0.5, -4, 1, 7, 0xC98A4B], [0, 0, -0.5, 1, 0, 0xE04F4F]]],
    ['Fruit pie', [[0, 0, 0, 5.5, 0, 0xC98A4B], [0, 0, -0.5, 4, 0, 0xE8B46A], [0, -1.5, -1.5, 1.2, 0, 0xE04F4F], [0, 1.5, 0, 1.2, 0, 0xFFE066], [0, -1, 1.5, 1.2, 0, 0xC28BFF], [0, 1.5, -2, 1, 0, 0xE04F4F]]],
    ['Banana pancakes', [[1, -5, 1, 10, 2.5, 0xE8B46A], [1, -5, -1.5, 10, 2.5, 0xC98A4B], [1, -5, -4, 10, 2.5, 0xE8B46A], [0, -2, -4.5, 1.2, 0, 0xFFE066], [0, 1.5, -4.5, 1.2, 0, 0xFFE066]]],
    ['Chips', [[1, -4, -1, 8, 6, 0xE04F4F], [1, -3, -5, 1.5, 5, 0xFFD23F], [1, -1, -6, 1.5, 6, 0xFFD23F], [1, 1, -5, 1.5, 5, 0xFFD23F], [1, 3, -4.5, 1.2, 4, 0xFFD23F]]],
    ['Cheesy chips', [[1, -4, -1, 8, 6, 0xE04F4F], [1, -3, -5, 1.5, 5, 0xFFD23F], [1, -1, -6, 1.5, 6, 0xFFD23F], [1, 1, -5, 1.5, 5, 0xFFD23F], [1, 3, -4.5, 1.2, 4, 0xFFD23F], [1, -4, -3.5, 8, 1.5, 0xFFB020]]],
    ['Sausage and chips', [[0, 0, 0, 6, 0, 0xFFFFFF], [1, -4, -3, 1.2, 5, 0xFFD23F], [1, -2.3, -3.5, 1.2, 5, 0xFFD23F], [1, -0.6, -3, 1.2, 5, 0xFFD23F], [1, 1, -1, 4.5, 2, 0xC0504A], [1, 1, 1.5, 4.5, 2, 0xC0504A]]],
    ['Banana bread', [[1, -5, -3, 10, 6, 0x8A5A3C], [1, -4, -2, 8, 4, 0xC98A4B], [0, -1.5, -2.5, 1, 0, 0xFFE066], [0, 1.5, -2.5, 1, 0, 0xFFE066]]],
    ['Carrot cake', [[1, -5, -2, 10, 6, 0xC98A4B], [1, -5, -3, 10, 2, 0xFFF6E6], [1, -0.5, -6, 1, 3, 0xFF8A3D], [1, -1.5, -7, 1, 1.5, 0x49D17A], [1, 0.5, -7, 1, 1.5, 0x49D17A]]],
    ['Melted cheese', [[0, 0, 1, 5.5, 0, 0xFFFFFF], [0, 0, 0, 4, 0, 0xFFB020], [0, -1, -1, 1.2, 0, 0xFFE066]]],
    ['Chicken curry', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xE0A030], [0, -2, -2, 1.3, 0, 0xE09A5A], [0, 2, -1, 1.3, 0, 0xE09A5A], [0, 0, -3, 0.8, 0, 0x49D17A]]],
    ['Roast chicken', [[0, 0, 1, 6, 0, 0xFFFFFF], [0, 0, 0, 4.5, 0, 0xC9783A], [0, -1.5, -1.5, 1.8, 0, 0xE09A5A], [1, 3.5, -3, 3, 1.5, 0xFFF6E6], [1, -6.5, -3, 3, 1.5, 0xFFF6E6]]],
    ['Fish supper', [[0, 0, 1, 6, 0, 0xFFFFFF], [1, -4.5, -2, 7, 4, 0xE0A040], [0, 3, 0, 1.8, 0, 0xE0A040], [1, -4, 2, 3, 1.5, 0x9AE06A], [0, 4, 2.5, 1, 0, 0xFFE94A]]],
    ['Shrimp skewers', [[1, -6, -0.5, 12, 1, 0xC98A4B], [0, -3, 0, 2, 0, 0xFF8A6A], [0, 0, 0, 2, 0, 0xFF8A6A], [0, 3, 0, 2, 0, 0xFF8A6A]]],
    ['Tacos', [[0, 0, 1, 5, 0, 0xF2D49B], [1, -4, -2, 8, 3, 0xF2D49B], [0, -1.5, -2, 1.3, 0, 0x6FCF5A], [0, 1, -2.5, 1.2, 0, 0xE0303F], [0, 2.5, -1.5, 1, 0, 0xFFD23F]]],
    ['Porridge', [[1, -6, 0, 12, 4, 0x5AD1FF], [0, 0, -1, 4.5, 0, 0xF2E0B0], [0, -1, -2, 1, 0, 0xC98A4B], [0, 1.5, -1, 0.8, 0, 0xFFB020]]],
    ['Chocolate cake', [[1, -5, -2, 10, 6, 0x6A3A20], [1, -5, -3, 10, 2, 0x8A5030], [1, -5, 0.5, 10, 1, 0xFFF6E6], [0, 0, -4.5, 1.5, 0, 0xC0203A]]],
    ['Sundae', [[1, -2.5, 1, 5, 5, 0xBFE8FF], [0, 0, -1, 4, 0, 0xFFF0F6], [0, -1.5, -2, 2, 0, 0xFF8FC7], [0, 1.5, -3, 1.6, 0, 0x8A5030], [0, 0, -5, 1.2, 0, 0xC0203A]]],
    ['Smoothie', [[1, -3, -4, 6, 10, 0xBFE8FF], [1, -2.5, -2, 5, 7.5, 0xFF8FC7], [1, 1, -8, 0.8, 5, 0xFFFFFF], [0, -1, -3, 0.8, 0, 0xFFFFFF]]],
    ['Pumpkin pie', [[0, 0, 0, 5.5, 0, 0xC98A4B], [0, 0, -0.5, 4, 0, 0xFF8A2A], [0, 0, -1.5, 1.2, 0, 0xFFFFFF]]],
    ['Fruit salad', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0xFFF0D8], [0, -2, -2, 1.3, 0, 0xE0303F], [0, 1.5, -1.5, 1.3, 0, 0xFF9A2A], [0, 0, -3, 1.2, 0, 0x9A4AD1]]],
    ['Garden salad', [[1, -6, 0, 12, 4, 0xFFFFFF], [0, 0, -1, 4.5, 0, 0x6FCF5A], [0, -2, -2, 1.2, 0, 0x9AE67A], [0, 2, -1.5, 1.1, 0, 0xE0303F]]],
    ['Stir-fry', [[0, 0, 1, 6, 0, 0x3A3F4A], [0, 0, 0, 4.5, 0, 0xD8A05A], [0, -2, -1, 1.2, 0, 0x2F8A3C], [0, 1.5, 0.5, 1.2, 0, 0xE0303F], [0, 0.5, -2, 1, 0, 0xFFD23F]]],
    ['Cookies', [[0, -2, 1, 3, 0, 0xC98A4B], [0, 2, -1, 3, 0, 0xD8A05A], [0, -2.5, 0.5, 0.7, 0, 0x6A3A20], [0, 2, -1.5, 0.7, 0, 0x6A3A20]]],
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
  const CHESTS = [119, 439, 1050, 1500];
  // ---- the aquarium's other fish and each section's plants and things (fancy_fish and aqua_decor in draw.c)
  const ln = (g, x0, y0, x1, y1, w, c, a = 1) => { g.globalAlpha = a; g.strokeStyle = hx(c); g.lineWidth = w; g.lineCap = 'butt'; g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke(); g.globalAlpha = 1; };
  const bx2 = (g, x, y, w, h, c, a = 1) => { g.globalAlpha = a; g.fillStyle = hx(c); g.fillRect(x, y, w, h); g.globalAlpha = 1; };
  const dt2 = (g, x, y, r, c, a = 1) => { if (r <= 0) return; g.globalAlpha = a; g.fillStyle = hx(c); g.beginPath(); g.arc(x, y, r, 0, Math.PI * 2); g.fill(); g.globalAlpha = 1; };
  const rg2 = (g, x, y, r, c, a = 1) => { g.globalAlpha = a; g.strokeStyle = hx(c); g.lineWidth = 1.5; g.beginPath(); g.arc(x, y, Math.max(0.1, r - 0.75), 0, Math.PI * 2); g.stroke(); g.globalAlpha = 1; };
  function fancyFish(g, kind, x, y, d, t) {
    switch (kind) {
      case 3: bx2(g, x - d * 8 - 2, y - 3, 4, 6, 0xFFB020); bx2(g, x - 2, y - 11, 4, 7, 0xFFE066); bx2(g, x - 2, y + 4, 4, 7, 0xFFE066); dt2(g, x, y, 5, 0xFFE066); bx2(g, x - 1, y - 6, 2, 12, 0x2B2230); dt2(g, x + d * 3, y - 1, 1.2, 0x1B1426); break;
      case 4: dt2(g, x - d * 7, y, 3, 0xE0A92B); for (let k = 0; k < 8; k++) dt2(g, x + Math.cos(k * 0.785) * 7, y + Math.sin(k * 0.785) * 7, 1.2, 0xC9A35A); dt2(g, x, y, 6, 0xFFD23F); dt2(g, x, y + 2, 3.5, 0xFFF6A0); dt2(g, x + d * 3, y - 2, 1.6, 0xFFFFFF); dt2(g, x + d * 3.4, y - 2, 0.8, 0x1B1426); break;
      case 5: for (let k = 0; k < 4; k++) { const tx = x - 4 + k * 2.7; ln(g, tx, y + 2, tx + Math.sin(t * 3 + k) * 2, y + 13, 1, 0xE6B8FF, 0.9); } dt2(g, x, y, 6, 0xC28BFF, 0.75); dt2(g, x - 2, y - 2, 1.5, 0xFFFFFF, 0.8); break;
      case 6: for (let k = 6; k >= 0; k--) dt2(g, x - d * k * 4, y + Math.sin(t * 5 - k * 0.9) * 2.5, 3.2 - k * 0.25, k % 2 ? 0x4F9A6E : 0x3F7D5E); dt2(g, x + d * 1.5, y - 1, 1, 0xFFE066); break;
      case 7: bx2(g, x - d * 9 - 2, y - 3, 4, 6, 0xFF8A3D); dt2(g, x, y, 5, 0xFF8A3D); bx2(g, x + d * 1.5 - 1, y - 4.5, 2, 9, 0xFFFFFF); bx2(g, x - d * 4 - 1, y - 4, 2, 8, 0xFFFFFF); dt2(g, x + d * 3.5, y - 1, 1, 0x1B1426); break;
      case 8: bx2(g, x - d * 9 - 2, y - 3, 4, 6, 0xFFD23F); dt2(g, x, y, 5, 0x3A6FC0); bx2(g, x - 3, y - 1, 6, 2, 0x1B2A5A); dt2(g, x + d * 3, y - 1.5, 1, 0xFFFFFF); break;
      default: { const fl = Math.sin(t * 3) * 2; ln(g, x - d * 5, y, x - d * 18, y + 1, 1.5, 0x3A4A6A); dt2(g, x - d, y - 5 + fl, 4, 0x5A6E8A); dt2(g, x - d, y + 5 - fl, 4, 0x5A6E8A); dt2(g, x, y, 5, 0x6E86A8); dt2(g, x + d * 3, y - 1.5, 0.9, 0x1B1426); }
    }
  }
  // drawn in world coordinates (the caller has shifted by the camera); cam picks which sections show
  function aquaDecor(g, cam, t) {
    const S = AQUA.SEABED, TOP = AQUA.WATER_TOP;
    if (cam < 340) [[40, 120], [95, 150], [150, 100], [205, 160], [260, 130], [300, 110]].forEach(([kx, kh], k) => {
      for (let y = 0; y < kh; y += 8) {
        const sw = Math.sin(t * 1.4 + k + y * 0.03) * y * 0.06, sw2 = Math.sin(t * 1.4 + k + (y + 8) * 0.03) * (y + 8) * 0.06;
        ln(g, kx + sw, S - y, kx + sw2, S - y - 8, 4, 0x2F8A4A);
        if (y % 16 === 8) dt2(g, kx + sw + (y % 32 ? 4 : -4), S - y, 3, 0x49D17A);
      }
    });
    if (cam > 0 && cam < 660) {
      [[360, 10, 0xFF8FC7], [470, 8, 0xFFB347], [590, 11, 0xC28BFF]].forEach(([x, r, c]) => { dt2(g, x, S - 4, r, c); rg2(g, x, S - 4, r * 0.6, 0xFFFFFF, 0.5); });
      [400, 520, 615].forEach((x, k) => { const c = k % 2 ? 0xFF8A3D : 0xE04F4F; ln(g, x, S, x, S - 30, 3, c); ln(g, x, S - 17, x - 10, S - 32, 3, c); ln(g, x, S - 22, x + 9, S - 36, 3, c); dt2(g, x, S - 31, 2, c); dt2(g, x - 10, S - 33, 2, c); dt2(g, x + 9, S - 37, 2, c); });
      for (let k = 0; k < 7; k++) { const a = -2.6 + k * 0.35; ln(g, 440, S, 440 + Math.cos(a) * 34, S + Math.sin(a) * 34, 2, 0x9C5AD1); dt2(g, 440 + Math.cos(a) * 34, S + Math.sin(a) * 34, 2, 0xC28BFF); }
      for (let k = 0; k < 6; k++) { const a = -2.4 + k * 0.34 + Math.sin(t * 2 + k) * 0.12; ln(g, 560, S - 4, 560 + Math.cos(a) * 16, S - 4 + Math.sin(a) * 16, 3, 0xFF8FC7); }
    }
    if (cam > 320 && cam < 980) {
      const x = 700;
      ln(g, x + 80, S - 44, x + 110, S - 130, 5, 0x6F4630); ln(g, x + 90, S - 100, x + 128, S - 90, 3, 0x6F4630); bx2(g, x + 96, S - 96, 22, 18, 0xE8DCC0, 0.75);
      bx2(g, x, S - 40, 150, 40, 0x5A3A22); bx2(g, x, S - 44, 150, 6, 0x6F4630); for (let k = 1; k < 5; k++) bx2(g, x, S - 40 + k * 8, 150, 1, 0x3A2618);
      for (let k = 0; k < 4; k++) { dt2(g, x + 20 + k * 30, S - 26, 4, 0x2B3A4A); rg2(g, x + 20 + k * 30, S - 26, 5, 0xC9A35A); }
      const ax = 900; ln(g, ax, S - 32, ax, S - 6, 3, 0x6E6A80); ln(g, ax - 8, S - 26, ax + 8, S - 26, 3, 0x6E6A80); rg2(g, ax, S - 36, 4, 0x6E6A80); ln(g, ax - 9, S - 9, ax, S - 4, 3, 0x6E6A80); ln(g, ax + 9, S - 9, ax, S - 4, 3, 0x6E6A80);
    }
    if (cam > 640 && cam < 1300) {
      const x = 960;
      bx2(g, x + 16, TOP, 288, S - TOP, 0x0A1840, 0.45);
      for (let k = 0; k < 4; k++) { bx2(g, x + k * 4, TOP, 4, S - TOP, 0x0A1840, 0.1 + k * 0.09); bx2(g, x + 304 + (3 - k) * 4, TOP, 4, S - TOP, 0x0A1840, 0.1 + k * 0.09); }
      for (let k = 0; k < 7; k++) { const wx = x + 30 + k * 40, h = 20 + (k * 13) % 22; ln(g, wx, S, wx, S - h, 3, 0x3A3650); dt2(g, wx, S - h, 3, k % 2 ? 0x5AD1FF : 0xFF8FC7, 0.5 + 0.5 * Math.sin(t * 2 + k)); }
    }
    if (cam > 960) {
      [[1320, 30], [1400, 22], [1520, 36]].forEach(([x, r], k) => { dt2(g, x, S + 6, r, k % 2 ? 0x7D798F : 0x6E6A80); dt2(g, x - r * 0.3, S - r * 0.4, r * 0.3, 0x8F8AA8); });
      K.star(1360, S - 30, 6, '#ff8a3d'); K.star(1478, S - 22, 5, '#ff8fc7');
      [1440, 1580].forEach(x => { for (let j = 0; j < 8; j++) ln(g, x, S - 5, x + Math.cos(j * 0.785) * 9, S - 5 + Math.sin(j * 0.785) * 9, 1, 0x1B1426); dt2(g, x, S - 5, 5, 0x2B2240); });
      for (let k = 0; k < 3; k++) bx2(g, 1556 + k * 7, S - 14 - k * 5, 5, 14 + k * 5, 0xFFD23F);
    }
  }

  const FRIDGE_FOODS = 9;
  const FOOD = Object.fromEntries(FOODS.map((f, i) => [f.name, i]));
  // the stove's shelf, a scrolling row for each player (ck_cook_ingr)
  const COOK_INGR = ['egg', 'tomato', 'cheese', 'mushroom', 'carrot', 'bread', 'potato', 'rice', 'sausage', 'pasta', 'apple', 'banana', 'strawberry', 'orange', 'lemon', 'grapes', 'cherry', 'pear', 'pineapple', 'blueberry', 'watermelon', 'onion', 'pepper', 'broccoli', 'peas', 'corn', 'lettuce', 'cucumber', 'pumpkin', 'beans', 'chicken', 'fish', 'shrimp', 'bacon', 'ham', 'tofu', 'chocolate', 'honey', 'yogurt', 'cream', 'butter', 'jam', 'flour', 'noodles', 'tortilla', 'oats', 'nuts', 'sprinkles'].map(n => FOOD[n]);
  // what a pan cooks into (ck_cook_dish): [food index + 1, its name]; the first rule that fits wins
  function cookDish(pan) {
    const has = n => pan.includes(FOOD[n]);
    const k = has('sausage') && has('bread') ? (has('cheese') ? 'Cheesy hot dog' : has('tomato') ? 'Hot dog and ketchup' : 'Hot dog')
      : has('pasta') ? (has('tomato') && has('sausage') ? 'Spaghetti and meatballs' : has('tomato') ? 'Spaghetti and sauce' : has('cheese') ? 'Mac and cheese' : has('mushroom') ? 'Mushroom pasta' : 'Buttered pasta')
      : has('rice') ? (has('egg') ? 'Egg fried rice' : has('sausage') ? 'Sausage rice' : has('carrot') || has('mushroom') ? 'Veggie rice' : has('banana') || has('apple') ? 'Rice pudding' : 'Rice bowl')
      : has('banana') && has('bread') ? 'Banana bread'
      : has('apple') ? (has('banana') ? 'Fruit pie' : 'Apple pie')
      : has('banana') ? 'Banana pancakes'
      : has('carrot') && has('egg') ? 'Carrot cake'
      : has('egg') ? (has('cheese') ? 'Cheesy omelette' : has('mushroom') ? 'Mushroom omelette' : has('tomato') ? 'Tomato omelette' : has('sausage') ? 'Big breakfast' : has('potato') ? 'Potato omelette' : 'Omelette')
      : has('bread') ? (has('cheese') && has('tomato') ? 'Pizza toastie' : has('cheese') ? 'Cheese toastie' : has('mushroom') ? 'Mushroom toast' : 'Toast')
      : has('potato') && !has('tomato') && !has('carrot') && !has('mushroom') ? (has('cheese') ? 'Cheesy chips' : has('sausage') ? 'Sausage and chips' : 'Chips')
      : has('chicken') ? (has('onion') || has('pepper') || has('tomato') || has('peas') ? 'Chicken curry' : 'Roast chicken')
      : has('fish') || has('shrimp') ? (has('fish') ? 'Fish supper' : 'Shrimp skewers')
      : has('tortilla') ? 'Tacos' : has('oats') ? 'Porridge' : has('chocolate') || has('flour') ? 'Chocolate cake'
      : has('cream') || has('sprinkles') ? 'Sundae' : has('yogurt') ? 'Smoothie' : has('pumpkin') ? 'Pumpkin pie'
      : has('strawberry') || has('orange') || has('lemon') || has('grapes') || has('cherry') || has('pear') || has('pineapple') || has('blueberry') || has('watermelon') ? 'Fruit salad' : has('lettuce') || has('cucumber') ? 'Garden salad'
      : has('onion') || has('pepper') || has('broccoli') || has('peas') || has('corn') || has('beans') || has('bacon') || has('ham') || has('tofu') || has('noodles') ? 'Stir-fry'
      : has('honey') || has('jam') || has('butter') || has('nuts') ? 'Cookies'
      : has('potato') ? 'Veggie stew' : has('sausage') ? 'Sausage stew' : has('tomato') ? 'Tomato soup' : has('mushroom') ? 'Mushroom soup' : has('carrot') ? 'Carrot soup' : pan.length === 1 ? 'Melted cheese' : 'Cheese soup';
    // which ingredients the dish's own name already covers; the rest go into the name and on top
    const CORE = {
      'Omelette': 'egg', 'Cheesy omelette': 'egg cheese', 'Mushroom omelette': 'egg mushroom', 'Tomato omelette': 'egg tomato', 'Potato omelette': 'egg potato', 'Big breakfast': 'egg sausage',
      'Toast': 'bread', 'Cheese toastie': 'bread cheese', 'Pizza toastie': 'bread cheese tomato', 'Mushroom toast': 'bread mushroom',
      'Tomato soup': 'tomato', 'Mushroom soup': 'mushroom', 'Carrot soup': 'carrot', 'Cheese soup': 'cheese', 'Veggie stew': 'potato carrot tomato mushroom', 'Sausage stew': 'sausage',
      'Hot dog': 'sausage bread', 'Cheesy hot dog': 'sausage bread cheese', 'Hot dog and ketchup': 'sausage bread tomato',
      'Spaghetti and sauce': 'pasta tomato', 'Spaghetti and meatballs': 'pasta tomato sausage', 'Mac and cheese': 'pasta cheese', 'Mushroom pasta': 'pasta mushroom', 'Buttered pasta': 'pasta',
      'Egg fried rice': 'rice egg', 'Sausage rice': 'rice sausage', 'Veggie rice': 'rice carrot mushroom', 'Rice pudding': 'rice banana apple', 'Rice bowl': 'rice',
      'Apple pie': 'apple', 'Fruit pie': 'apple banana', 'Banana pancakes': 'banana egg',
      'Chips': 'potato', 'Cheesy chips': 'potato cheese', 'Sausage and chips': 'potato sausage',
      'Banana bread': 'banana bread', 'Carrot cake': 'carrot egg', 'Melted cheese': 'cheese',
      'Chicken curry': 'chicken', 'Roast chicken': 'chicken', 'Fish supper': 'fish', 'Shrimp skewers': 'shrimp', 'Tacos': 'tortilla', 'Porridge': 'oats', 'Chocolate cake': 'chocolate flour', 'Sundae': 'cream', 'Smoothie': 'yogurt', 'Pumpkin pie': 'pumpkin', 'Fruit salad': '', 'Garden salad': 'lettuce', 'Stir-fry': '', 'Cookies': '',
    }[k].split(' ').map(n => FOOD[n]);
    const ADJ = ['eggy', 'saucy', 'cheesy', 'foresty', 'crunchy', 'crusty', 'spuddy', 'fluffy', 'meaty', 'noodly', 'appley', 'tropical', 'berry', 'zesty', 'lemony', 'grapey', 'cherry', 'pear', 'pineapple', 'blue', 'melony', 'oniony', 'peppery', 'green', 'pea', 'corny', 'leafy', 'cool', 'pumpkin', 'beany', 'chicken', 'fishy', 'shrimpy', 'bacon', 'hammy', 'tofu', 'chocolatey', 'honey', 'creamy', 'whipped', 'buttery', 'jammy', 'floury', 'slurpy', 'wrapped', 'oaty', 'nutty', 'sparkly'];
    const NOUN = ['egg', 'tomato', 'cheese', 'mushrooms', 'carrots', 'toast', 'mash', 'rice', 'sausage', 'pasta', 'apple', 'banana', 'strawberries', 'orange', 'lemon', 'grapes', 'cherries', 'pear', 'pineapple', 'blueberries', 'melon', 'onions', 'peppers', 'broccoli', 'peas', 'corn', 'lettuce', 'cucumber', 'pumpkin', 'beans', 'chicken', 'fish', 'shrimp', 'bacon', 'ham', 'tofu', 'chocolate', 'honey', 'yogurt', 'cream', 'butter', 'jam', 'dumplings', 'noodles', 'tortilla', 'oats', 'nuts', 'sprinkles'];
    let ex = [], most = 0, mostI = -1;
    COOK_INGR.forEach((f, i) => {
      const cnt = pan.filter(p => p === f).length;
      if (cnt > most) { most = cnt; mostI = i; }
      if (cnt && !CORE.includes(f)) ex.push(i);
    });
    // a doubled main ingredient makes it a "double" dish; a doubled extra makes that word "extra"
    const coreMost = most >= 2 && CORE.includes(COOK_INGR[mostI]);
    for (let e = 1; e < ex.length; e++) if (most >= 2 && ex[e] === mostI) { ex[e] = ex[0]; ex[0] = mostI; }
    const lead = !ex.length || most < 2 || coreMost || ex[0] !== mostI ? '' : most >= 3 ? 'super ' : 'extra ';
    let s = (coreMost ? (most >= 4 ? 'mega ' : most === 3 ? 'triple ' : 'double ') : '') + lead + (ex.length ? ADJ[ex[0]] + ' ' : '');
    s += s ? k[0].toLowerCase() + k.slice(1) : k;
    for (let e = 1; e < ex.length; e++) {   // more extras, while the name still fits a banner
      const add = (e === 1 ? ' with ' : ' and ') + NOUN[ex[e]];
      if (s.length + add.length > 38) break;
      s += add;
    }
    return [FOOD[k] + 1, s[0].toUpperCase() + s.slice(1), ex.slice().sort((a, b) => a - b)];   // extras as shelf indices (the N64's bit mask)
  }
   // the aquarium's treasure chests (ck_aqua_chest_x)
  return { FOODS, FOOD, FRIDGE_FOODS, COOK_INGR, cookDish, LETTERS, CHESTS, DRUMS, drumPiece, LAYOUT, F, NAMES, SPOT_FURN, furn, fish, shrimp, crab, weed, FISH, AQUA, aquaBg, fancyFish, aquaDecor, RACE, TRACKS, raceObst, raceCurve, racePad, raceProject, raceView, carBack, KEYS, hx };
})();
