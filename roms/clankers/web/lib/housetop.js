// The heroes' house, seen from above (three-quarter top-down, orthographic): a four-bedroom
// family house with a two-car garage, decorated, with every thing the house games need placed
// where it belongs. The furniture art is in furn34.js. Everything is in world pixels.
// houseScene in game.js walks it; tools/bake_sprites.mjs writes the same plan into
// src/core/ck_house_map.c for the N64 and bakes the furniture and the static picture of it.
//
//   back garden (patio, pond, apple tree, swing, veg patch)
//   +---------+-------------+-----------+------------+-------+
//   | Kitchen | Family room | Bedroom 2 | Main bed   |Ensuite|
//   +---------+-------------+-----------+------------+-------+
//   | Dining  |                 hallway                       |
//   +---------+--------+------+---------+----------+----------+
//   | Laundry | Living | Foyer| Bathroom| Bedroom 3| Nursery  |   (Bedroom 4)
//   +---------+--------+------+---------+----------+----------+
//   Garage beside the laundry; front garden, path, driveway, gate to the map.
const HOUSETOP = (() => {
  const K = CK, L = HOUSEART.LAYOUT, P = Math.PI, A = FURN34.A;
  const FONT = '"Titan One", "Arial Black", Impact, sans-serif';
  const hx = h => '#' + h.toString(16).padStart(6, '0');
  const WORLD = { W: 2160, H: 1400 };
  const FACE = 64, CAP = 10, WT = 8;
  const GRASS = 0x5DBB63;

  // ---- the plan: name, x, y, w, h, wall, stripe, floor, floor kind
  const R = (name, x, y, w, h, wall, stripe, floor, kind) => ({ name, x, y, w, h, fy: y + FACE, wall, stripe, floor, kind });
  const rooms = [
    R('Kitchen', 560, 300, 300, 280, 0xBDE7D2, 0xA9DCC3, 0xD9CDB8, 'check'),
    R('Family room', 860, 300, 420, 280, 0xFFE2B8, 0xF5D3A0, 0x8C6A55, 'wood'),
    R('Bedroom 2', 1280, 300, 240, 280, 0x9FD8F0, 0x88CBE6, 0x5D7C99, 'carpet'),
    R('Main bedroom', 1520, 300, 280, 280, 0xFFD3E2, 0xFFC1D6, 0xC9B49A, 'carpet'),
    R('Ensuite', 1800, 300, 160, 280, 0xE8F4F8, 0xD6ECF2, 0xF2F6F8, 'tile'),
    R('Dining room', 560, 580, 300, 240, 0xFFF1D6, 0xF6E1BA, 0x9C7552, 'wood'),
    R('Hallway', 860, 580, 1100, 100, 0xF2E6D0, 0xE8D8BC, 0xA8794F, 'wood'),
    R('Laundry', 560, 820, 300, 200, 0xD8EEF7, 0xC5E4F0, 0xE0E4EA, 'check'),
    R('Garage', 200, 640, 360, 380, 0xB5BAC4, 0xA4AAB6, 0x6E7280, 'concrete'),
    R('Living room', 860, 680, 320, 340, 0xC9B8F0, 0xB8A3E9, 0x7A4E3A, 'wood'),
    R('Foyer', 1180, 680, 140, 340, 0xF2E6D0, 0xE8D8BC, 0xC9C0B0, 'stone'),
    R('Bathroom', 1320, 680, 140, 340, 0xE8F4F8, 0xD6ECF2, 0xF2F6F8, 'tile'),
    R('Bedroom 3', 1460, 680, 250, 340, 0xFFF1D6, 0xF6E1BA, 0xB88A64, 'wood'),
    R('Nursery', 1710, 680, 250, 340, 0xD8F5E0, 0xC5EBD0, 0xE0D0F0, 'carpet'),
  ];
  const room = n => rooms.find(r => r.name === n);
  const FRONT = 1020;                                   // the front wall of the house
  const FACADE = { x0: 200, x1: 1960, y0: FRONT + WT / 2, y1: FRONT + WT / 2 + FACE };
  // doorways. across: in a wall running across the screen (x, wall y, half width): you walk up
  // through the back wall of the room below. down: in a wall running down the screen (wall x,
  // middle y, half height). out: in the front of the house (x, half width).
  const ACROSS = [
    [1070, 300, 26],                                                        // family room patio doors
    [710, 580, 34], [800, 820, 18],                                         // kitchen to dining, dining to laundry
    [1070, 580, 40], [1400, 580, 18], [1580, 580, 18],                      // hallway to family room, bedroom 2, main bedroom
    [1020, 680, 34], [1250, 680, 40], [1390, 680, 18], [1520, 680, 18], [1770, 680, 18],   // hallway to living, foyer, bathroom, bedroom 3, nursery
  ];
  const DOWN = [[860, 440, 30], [860, 630, 26], [560, 930, 24], [1800, 460, 24], [1180, 870, 30], [1960, 630, 22]];
  const OUT = [[1250, 20, 'door'], [380, 132, 'garage']];

  // ---- things. y is the front edge on the floor; foot is the footprint [half width, depth].
  // kind (via use()) is the SPOT_* from the house today that A starts there.
  const things = [];
  const put = (art, x, y, foot, o = {}) => things.push({ art, x, y, foot, ...o });
  const wall = (n, d = 0) => room(n).fy + d;
  const use = kind => { const k = L.spots.findIndex(s => s[1] === kind); return { spot: k, kind, label: L.spots[k][2] }; };
  // kitchen: fridge, sink, stove along the back, an island with stools
  put('fridge', 596, wall('Kitchen', 18), [16, 16], use('snack'));
  put('counter', 676, wall('Kitchen', 18), [38, 16]);
  put('stove', 758, wall('Kitchen', 18), [22, 16], use('stove'));
  put('plant', 832, wall('Kitchen', 12), [10, 8]);
  put('island', 710, wall('Kitchen', 140), [46, 30]);
  // dining room
  put('hutch', 612, wall('Dining room', 16), [30, 14]);
  put('table6', 710, wall('Dining room', 118), [58, 48]);
  put('plant', 832, wall('Dining room', 12), [10, 8]);
  // laundry
  put('washer', 600, wall('Laundry', 20), [17, 18], { arg: '#5ad1ff' }); put('washer', 640, wall('Laundry', 20), [17, 18], { arg: '#ff8a3d' });
  put('basket', 700, wall('Laundry', 90), [11, 8]); put('shoeRack', 600, wall('Laundry', 110), [20, 10]); put('coatRack', 840, wall('Laundry', 14), [9, 8]);
  // garage: the family car and the go-kart, nose to the door
  put('car', 290, 990, [32, 92], { arg: '#3a6fc0' });
  put('kart', 466, 980, [24, 56], use('cars'));
  put('shelf', 260, wall('Garage', 14), [29, 12]); put('bench', 470, wall('Garage', 20), [37, 18]); put('bikes', 370, wall('Garage', 30), [18, 10]);
  // family room: TV and sofa, the game corner with the cabinets and the pinball table
  put('tv', 960, wall('Family room', 20), [36, 16]);
  put('sofaBack', 960, wall('Family room', 120), [46, 30], { arg: '#c0504a' });
  put('blast', 1140, wall('Family room', 20), [16, 18], use('blast')); put('bolt', 1185, wall('Family room', 20), [16, 18], use('bolt')); put('jetrun', 1230, wall('Family room', 20), [16, 18], use('jetrun'));
  put('pinball', 1240, wall('Family room', 166), [20, 62], use('pinball'));
  put('beanbag', 1130, wall('Family room', 120), [16, 10], { arg: '#ff8fc7' }); put('beanbag', 1175, wall('Family room', 135), [16, 10], { arg: '#5ad1ff' });
  put('plant', 890, wall('Family room', 12), [10, 8]);
  // bedroom 2: a drummer's room
  put('bed', 1320, wall('Bedroom 2', 82), [28, 84], { arg: ['#49d17a', '#8fe8a8', '#2f9a5a'] });
  put('drums', 1450, wall('Bedroom 2', 150), [34, 38], use('drums'));
  put('desk', 1470, wall('Bedroom 2', 20), [28, 16], { arg: '#49d17a' });
  // main bedroom: the big bed is where you sleep and save
  put('bigBed', 1650, wall('Main bedroom', 82), [44, 84], { ...use('bed'), arg: ['#c28bff', '#d8b8ff', '#9a6ad1'] });
  put('nightstand', 1592, wall('Main bedroom', 16), [11, 12]); put('nightstand', 1708, wall('Main bedroom', 16), [11, 12]);
  put('dresser', 1762, wall('Main bedroom', 16), [27, 14]);
  put('wardrobe', 1550, wall('Main bedroom', 160), [25, 16]);
  // ensuite
  put('tub', 1904, wall('Ensuite', 22), [38, 20]); put('vanity', 1828, wall('Ensuite', 18), [21, 14]); put('toilet', 1910, wall('Ensuite', 170), [11, 22]);
  // hallway
  put('console', 1150, wall('Hallway', 12), [30, 10]); put('plant', 1480, wall('Hallway', 12), [10, 8]); put('plant', 1700, wall('Hallway', 12), [10, 8]); put('plant', 1920, wall('Hallway', 12), [10, 8]);
  // living room: the piano and the big fish tank
  put('piano', 920, wall('Living room', 34), [33, 34], use('piano'));
  put('tank', 1110, wall('Living room', 22), [38, 20], use('aqua'));
  put('sofa', 1000, wall('Living room', 260), [44, 40], { arg: '#3a6fc0' });
  put('coffee', 1000, wall('Living room', 190), [23, 16]);
  put('armchair', 1120, wall('Living room', 210), [22, 30], { arg: '#49a07a' });
  put('plant', 1156, wall('Living room', 150), [10, 8]);
  // foyer
  put('coatRack', 1200, wall('Foyer', 20), [9, 8]); put('shoeRack', 1294, wall('Foyer', 130), [20, 10]); put('plant', 1200, wall('Foyer', 250), [10, 8]);
  // bathroom
  put('toilet', 1346, wall('Bathroom', 30), [11, 22]); put('vanity', 1432, wall('Bathroom', 18), [21, 14]); put('tub', 1390, wall('Bathroom', 250), [38, 20]);
  // bedroom 3: the painter's room, with the easel
  put('bed', 1664, wall('Bedroom 3', 82), [28, 84], { arg: ['#ff8fc7', '#ffc1d6', '#e06a9a'] });
  put('paintShelf', 1576, wall('Bedroom 3', 14), [24, 12]);
  put('easel', 1580, wall('Bedroom 3', 190), [22, 14], use('paint'));
  put('desk', 1500, wall('Bedroom 3', 230), [28, 16], { arg: '#ff8fc7' });
  // nursery (bedroom 4): where the rescued babies sleep
  put('crib', 1830, wall('Nursery', 26), [26, 18], { arg: '#ffc1d6' }); put('crib', 1904, wall('Nursery', 26), [26, 18], { arg: '#bfe9ff' });
  put('toyChest', 1760, wall('Nursery', 180), [20, 14]); put('blocks', 1880, wall('Nursery', 190), [12, 10]);
  // back garden
  put('pond', 660, 200, [50, 56], { ...use('pond'), flat: true });
  put('tree', 380, 470, [8, 6], use('tree'));
  put('patioTable', 1180, 240, [26, 14]); put('bbq', 1290, 236, [14, 8]);
  put('swing', 1640, 200, [42, 8]); put('trampoline', 1860, 200, [40, 18]);
  put('gardenBench', 900, 200, [26, 12]);
  // front garden
  put('mailbox', 1310, 1140, [6, 6], use('mail'));
  put('sign', 1150, 1340, [24, 6], use('back'));
  put('tree', 1720, 1220, [8, 6]); put('tree', 760, 1220, [8, 6]);
  for (const [x, y] of [[90, 200], [2070, 200], [2070, 560], [100, 860], [2060, 960], [560, 1300], [1900, 1320], [400, 360], [1500, 1300]]) put('bush', x, y, [16, 10]);
  for (const [x, y] of [[620, 1110], [900, 1110], [1060, 1110], [1420, 1110], [1600, 1110], [1820, 1110], [1480, 250], [1980, 250]]) put('flowers', x, y, [0, 0], { flat: true });
  // the sandbox in the back garden, by the pond (last, so the other looks keep their numbers)
  put('sandbox', 440, 236, [36, 26], use('sand'));

  // every different look (art and colours) once: the N64 bakes a sprite for each
  const ARTS = [];
  for (const th of things) {
    const key = th.art + (th.arg !== undefined ? ' ' + JSON.stringify(th.arg) : '');
    let k = ARTS.findIndex(a => a.key === key);
    if (k < 0) { k = ARTS.length; ARTS.push({ key, art: th.art, arg: th.arg, foot: th.foot, flat: !!th.flat }); }
    th.ai = k;
  }

  // ---- where you can stand
  const { W, H } = WORLD;
  const walk = [];
  for (const r of rooms) walk.push([r.x + 10, r.fy + 4, r.x + r.w - 10, r.y + r.h - 6]);
  for (const [x, y, hw] of ACROSS) walk.push([x - hw + 4, y - 14, x + hw - 4, y + FACE + 10]);
  for (const [x, y, hh] of DOWN) walk.push([x - 16, y - hh + 4, x + 16, y + hh - 4]);
  for (const [x, hw] of OUT) walk.push([x - hw + 4, FRONT - 10, x + hw - 4, FACADE.y1 + 10]);
  walk.push([24, 76, W - 24, 292], [24, 76, 192, H - 30], [1968, 76, W - 24, H - 30], [24, FACADE.y1 + 6, W - 24, H - 30], [208, 76, 552, 632], [1220, H - 40, 1280, H - 6]);
  const inRect = (x, y, [a, b, c, d]) => x >= a && x <= c && y >= b && y <= d;
  function solid(x, y) {
    if (!walk.some(r => inRect(x, y, r))) return true;
    for (const th of things) if (th.foot[0] && Math.abs(x - th.x) < th.foot[0] + 4 && y > th.y - th.foot[1] - 2 && y < th.y + 5) return true;
    return false;
  }
  const roomAt = (x, y) => rooms.find(r => x > r.x && x < r.x + r.w && y > r.y && y < r.y + r.h);
  // the thing with a spot whose footprint is close to (x, y), or null; here(th) can rule one out
  function near(x, y, here) {
    let best = null, bd = 1e9;
    for (const th of things) {
      if (th.spot === undefined || (here && !here(th))) continue;
      const dx = Math.max(0, Math.abs(x - th.x) - th.foot[0]), dy = y < th.y - th.foot[1] ? th.y - th.foot[1] - y : Math.max(0, y - th.y);
      if (dx > 12 || dy > 16) continue;
      const d = dx + dy; if (d < bd) { bd = d; best = th; }
    }
    return best;
  }

  // where the heroes come in from the map, the gate back out, and the most two heroes can be apart
  const START = [[1238, 1120], [1262, 1134]];
  const GATE = { x: 1250, hw: 30, y: H - 14 };
  const LEASH = [280, 140];
  // where the babies wander: the back garden, then a room each (more babies go round again)
  const AREAS = [[700, 100, 1500, 280], ...['Nursery', 'Nursery', 'Family room', 'Bedroom 2', 'Bedroom 3', 'Living room', 'Kitchen'].map(n => room(n)).map(r => [r.x + 20, r.fy + 30, r.x + r.w - 20, r.y + r.h - 12])];
  // The camera for a w x h screen, given the middle of the party: it follows (a little above the
  // feet, so you see where you are going), but indoors it stays inside the room you are in.
  function camera(mx, my, w, h) {
    const cl = (v, a, b) => Math.max(a, Math.min(b, v));
    let cx = cl(Math.round(mx - w / 2), 0, W - w), cy = cl(Math.round(my - 44 - h / 2), 0, H - h);
    const rm = roomAt(mx, my);
    if (rm) {
      const fit = (c, lo, hi) => hi < lo ? Math.round((lo + hi) / 2) : cl(c, lo, hi);
      cx = cl(fit(cx, rm.x - 8, rm.x + rm.w + 8 - w), 0, W - w);
      if (rm.h > 160) cy = cl(fit(cy, rm.y - 8, rm.y + rm.h + 8 - h), 0, H - h);
    }
    return [cx, cy];
  }

  function text(g, s, x, y, size, col = '#fff6d8', align = 'center', outline = '#1b1426') {
    s = String(s).replace(/(?<=[^ ]) (?=[^ ])/g, '  ');
    g.font = `${size}px ${FONT}`; g.textAlign = align; g.textBaseline = 'alphabetic';
    if (outline) { g.lineJoin = 'round'; g.strokeStyle = outline; g.lineWidth = Math.max(2, size / 3); g.strokeText(s, x, y); }
    g.fillStyle = col; g.fillText(s, x, y);
  }
  const textW = (g, s, size) => { g.font = `${size}px ${FONT}`; return Math.ceil(g.measureText(String(s).replace(/(?<=[^ ]) (?=[^ ])/g, '  ')).width); };
  const shadow = (g, x, y, rx, ry = rx * 0.35, a = 0.22) => { g.fillStyle = `rgba(20,12,30,${a})`; g.beginPath(); g.ellipse(x, y, rx, ry, 0, 0, P * 2); g.fill(); };
  const hash = (a, b) => { let h = (a * 374761393 + b * 668265263) | 0; h = (h ^ (h >>> 13)) * 1274126177; return ((h ^ (h >>> 16)) >>> 0) / 4294967296; };

  // ---- the garden
  function garden(g) {
    g.fillStyle = hx(GRASS); g.fillRect(0, 0, W, H);
    g.fillStyle = '#4fa856'; for (let k = 0; k < 2400; k++) { const x = hash(k, 21) * W, y = hash(k, 22) * H; g.fillRect(x, y, 1, 3); g.fillRect(x + 2, y + 1, 1, 2); }
    g.fillStyle = 'rgba(255,255,255,0.05)'; for (let x = 0; x < W; x += 48) g.fillRect(x, FACADE.y1, 24, H - FACADE.y1);
    g.fillStyle = '#2f7a3c'; g.fillRect(0, 0, W, 30);
    for (let x = -10; x < W; x += 22) { K.circ(x, 22, 16, '#3f9d4e'); K.circ(x + 11, 14, 12, '#4fae5a'); }
    for (let x = 6; x < W - 6; x += 14) { K.rr(x, 34, 8, 30, 2, '#fff6d8'); g.fillStyle = '#fff6d8'; g.beginPath(); g.moveTo(x, 36); g.lineTo(x + 4, 30); g.lineTo(x + 8, 36); g.fill(); }
    K.rr(0, 44, W, 4, 1, '#e8dcc0'); K.rr(0, 56, W, 4, 1, '#e8dcc0'); g.fillStyle = 'rgba(20,12,30,0.2)'; g.fillRect(0, 64, W, 6);
    for (let y = 70; y < H; y += 20) { K.circ(6, y, 13, '#3f9d4e'); K.circ(W - 6, y, 13, '#3f9d4e'); }
    // patio outside the family room
    for (let y = 160; y < 292; y += 18) for (let x = 1000; x < 1340; x += 18) { K.rr(x, y, 16, 16, 3, '#c9c0b0'); K.rr(x + 1, y + 1, 14, 5, 2, '#ddd5c7'); }
    // vegetable patch
    K.rr(1400, 100, 160, 70, 4, '#8a5a3c'); for (let r = 0; r < 3; r++) for (let k = 0; k < 8; k++) { K.circ(1412 + k * 19, 116 + r * 20, 5, '#49a04a'); if ((k + r) % 3 === 0) K.circ(1412 + k * 19, 120 + r * 20, 2.5, '#ff8a3d'); }
    // stepping stones round both sides
    for (let y = 300; y < 1080; y += 26) { for (const sx of [140, 2010]) { const x = sx + Math.sin(y * 0.02) * 10; K.rr(x - 9, y, 18, 11, 4, '#c9c0b0'); K.rr(x - 8, y, 16, 4, 3, '#ddd5c7'); } }
    // driveway, front path, the gate
    g.fillStyle = '#9aa1b0'; g.fillRect(244, FACADE.y1, 272, H - FACADE.y1); g.fillStyle = '#8a91a0'; for (let y = FACADE.y1 + 30; y < H; y += 40) g.fillRect(244, y, 272, 2); g.fillRect(378, FACADE.y1, 4, H - FACADE.y1);
    for (let y = FACADE.y1 + 14; y < H - 10; y += 18) { const x = 1250 + Math.sin(y * 0.05) * 4; K.rr(x - 10, y, 20, 11, 4, '#c9c0b0'); K.rr(x - 9, y, 18, 4, 3, '#ddd5c7'); }
    for (let x = 6; x < W - 6; x += 14) if ((x < 1220 || x > 1280) && (x < 236 || x > 524)) K.rr(x, H - 26, 8, 26, 2, '#fff6d8');
    K.rr(0, H - 18, 236, 4, 1, '#e8dcc0'); K.rr(524, H - 18, 696, 4, 1, '#e8dcc0'); K.rr(1282, H - 18, W - 1282, 4, 1, '#e8dcc0');
    text(g, '↓ Map', 1250, H - 6, 8, '#fff6d8');
  }

  // ---- floors
  const RUGS = {
    'Family room': [[900, 120, 150, 100, '#c9683f', '#e8a060']],
    'Main bedroom': [[1590, 140, 120, 60, '#9a6ad1', '#c28bff']],
    'Bedroom 2': [[1390, 120, 120, 80, '#3a6fc0', '#5ad1ff']],
    'Bedroom 3': [[1530, 150, 110, 80, '#e8dcc0', '#fff6d8']],
    'Nursery': [[1760, 110, 150, 110, '#ffd23f', '#fff0a0']],
    'Living room': [[920, 150, 170, 110, '#3f9d9a', '#6fc9c0']],
    'Foyer': [[1222, 220, 56, 40, '#c0504a', '#e06a5a']],
    'Bathroom': [[1366, 150, 48, 30, '#5ad1ff', '#bfefff']], 'Ensuite': [[1846, 120, 52, 30, '#ff8fc7', '#ffc1d6']],
    'Dining room': [[620, 40, 180, 110, '#9a3a3a', '#c0504a']],
  };
  function floor(g, r) {
    const { x, fy, w } = r, h = r.y + r.h - fy, fc = r.floor, top = fy;
    g.fillStyle = hx(fc); g.fillRect(x, top, w, h);
    g.save(); g.beginPath(); g.rect(x, top, w, h); g.clip();
    if (r.kind === 'wood') {
      g.fillStyle = 'rgba(0,0,0,0.13)'; for (let y = top, k = 0; y < top + h; y += 12, k++) { g.fillRect(x, y, w, 1); for (let xx = x + (k * 17) % 40; xx < x + w; xx += 40) g.fillRect(xx, y, 1, 12); }
      g.fillStyle = 'rgba(255,255,255,0.06)'; for (let y = top + 2; y < top + h; y += 12) g.fillRect(x, y, w, 2);
    } else if (r.kind === 'check') {
      for (let y = top, a = 0; y < top + h; y += 16, a++) for (let xx = x, b = 0; xx < x + w; xx += 16, b++) if ((a + b) % 2) { g.fillStyle = K.shade(fc, 0.88); g.fillRect(xx, y, 16, 16); }
    } else if (r.kind === 'tile') {
      g.fillStyle = 'rgba(90,110,130,0.18)'; for (let y = top; y < top + h; y += 10) g.fillRect(x, y, w, 1); for (let xx = x; xx < x + w; xx += 10) g.fillRect(xx, top, 1, h);
    } else if (r.kind === 'stone') {
      g.fillStyle = 'rgba(0,0,0,0.12)'; for (let y = top, k = 0; y < top + h; y += 20, k++) { g.fillRect(x, y, w, 1); for (let xx = x + (k % 2) * 14; xx < x + w; xx += 28) g.fillRect(xx, y, 1, 20); }
    } else if (r.kind === 'carpet') {
      g.fillStyle = 'rgba(255,255,255,0.07)'; for (let k = 0; k < (w * h) / 60; k++) g.fillRect(x + hash(k, r.x) * w, top + hash(k, r.y + 1) * h, 1, 1);
    } else if (r.kind === 'concrete') {
      for (let k = 0; k < 120; k++) { g.fillStyle = 'rgba(0,0,0,0.08)'; g.fillRect(x + hash(k, 5) * w, top + hash(k, 6) * h, 2, 2); }
      g.fillStyle = 'rgba(27,20,38,0.25)'; g.beginPath(); g.ellipse(x + 80, top + 200, 14, 6, 0.3, 0, P * 2); g.fill();
      g.fillStyle = '#ffd23f'; g.fillRect(x + 178, top + 60, 3, h - 60);
    }
    for (const [rx, ry, rw, rh, c1, c2] of RUGS[r.name] || []) { K.rr(rx, top + ry, rw, rh, 8, c1); K.rr(rx + 5, top + ry + 5, rw - 10, rh - 10, 6, c2); }
    if (r.name === 'Hallway') { K.rr(x + 30, top + 8, w - 60, 22, 4, '#c0504a'); K.rr(x + 34, top + 12, w - 68, 14, 3, '#e06a5a'); for (let xx = x + 50; xx < x + w - 50; xx += 30) K.rr(xx, top + 17, 10, 4, 2, '#ffd23f'); }
    g.restore();
    const gr = g.createLinearGradient(0, top, 0, top + 14); gr.addColorStop(0, 'rgba(20,12,30,0.28)'); gr.addColorStop(1, 'rgba(20,12,30,0)'); g.fillStyle = gr; g.fillRect(x, top, w, 14);
  }

  // ---- back walls and what hangs on them
  const win = (g, x, y, w = 40) => { K.rr(x - w / 2, y, w, 32, 3, '#fff6d8'); K.rr(x - w / 2 + 3, y + 3, w - 6, 26, 2, '#8fd3ff'); g.fillStyle = 'rgba(255,255,255,0.5)'; g.fillRect(x - w / 2 + 6, y + 5, 4, 20); g.fillStyle = '#fff6d8'; g.fillRect(x - 1, y + 3, 2, 26); K.rr(x - w / 2 - 3, y + 31, w + 6, 4, 2, '#c98a4b'); };
  const pic = (g, x, y, w, h, c) => { K.rr(x - w / 2, y, w, h, 2, '#8a5a3c'); K.rr(x - w / 2 + 3, y + 3, w - 6, h - 6, 1, c); };
  const WALL_ART = {
    'Kitchen': g => { win(g, 676, 314); for (let k = 0; k < 6; k++) K.rr(716 + k * 10, 318, 8, 8, 1, k % 2 ? '#ffffff' : '#5ad1ff'); },
    'Family room': g => { win(g, 900, 316, 36); g.fillStyle = '#5ad1ff'; g.fillRect(1100, 352, 180, 2); },
    'Bedroom 2': g => { win(g, 1400, 316); K.rr(1300, 316, 30, 40, 2, '#1b1426'); K.circ(1315, 330, 7, '#ffd23f'); text(g, 'ROCK', 1315, 350, 6, '#ff5a5a', 'center', null); },
    'Main bedroom': g => { win(g, 1600, 316); win(g, 1700, 316); pic(g, 1650, 318, 30, 22, '#ff8fc7'); },
    'Ensuite': g => { win(g, 1840, 318, 28); for (let k = 0; k < 8; k++) K.rr(1806 + k * 18, 352, 16, 8, 1, k % 2 ? '#bfe9ff' : '#ffffff'); },
    'Dining room': g => { pic(g, 770, 592, 40, 30, '#e8b46a'); K.circ(770, 607, 8, '#ff5a5a'); },
    'Hallway': g => { for (const [x, c] of [[920, '#5ad1ff'], [1160, '#ffd23f'], [1320, '#ff8fc7'], [1490, '#49d17a'], [1700, '#c28bff'], [1880, '#ff8a3d']]) pic(g, x, 594, 26, 22, c); },
    'Laundry': g => { K.rr(700, 836, 60, 6, 1, '#8a5a3c'); for (let k = 0; k < 4; k++) K.rr(704 + k * 14, 842, 10, 14, 2, ['#ff8fc7', '#5ad1ff', '#ffd23f', '#ffffff'][k]); },
    'Living room': g => { pic(g, 1010, 694, 46, 32, '#8fd3ff'); K.circ(1000, 714, 6, '#3f9d4e'); K.rr(1020, 706, 10, 14, 1, '#fff6d8'); },
    'Foyer': g => { K.circ(1290, 708, 8, '#fff6d8'); K.circ(1290, 708, 6, '#ffffff'); K.rr(1289.5, 703, 1, 5, 0, '#2b2230'); },
    'Bathroom': g => { for (let k = 0; k < 7; k++) K.rr(1322 + k * 20, 732, 18, 8, 1, k % 2 ? '#bfe9ff' : '#ffffff'); },
    'Bedroom 3': g => { K.rr(1590, 694, 60, 34, 2, '#fff6d8'); for (const [px, py, c, pr] of [[1604, 704, '#ffd23f', 6], [1620, 716, '#5ad1ff', 8], [1636, 704, '#ff5a5a', 5], [1640, 720, '#49d17a', 4]]) K.circ(px, py, pr, c); },
    'Nursery': g => { for (let k = 0; k < 9; k++) { const x = 1724 + k * 26; g.fillStyle = ['#ff8fc7', '#5ad1ff', '#ffd23f'][k % 3]; g.beginPath(); g.moveTo(x, 692); g.lineTo(x + 20, 692); g.lineTo(x + 10, 704); g.fill(); } K.circ(1900, 722, 8, '#fff6d8'); K.circ(1903, 720, 7, '#d8f5e0'); },
    'Garage': g => { K.rr(220, 656, 30, 40, 2, '#e04f4f'); K.rr(224, 660, 22, 18, 1, '#fff6d8'); },
  };
  function backWall(g, r) {
    const { x, y, w } = r, wy = y + CAP;
    g.fillStyle = hx(r.wall); g.fillRect(x, y, w, FACE);
    g.fillStyle = hx(r.stripe); for (let xx = x + 12; xx < x + w; xx += 24) g.fillRect(xx, wy, 10, FACE - CAP);
    if (r.kind === 'tile') { g.fillStyle = 'rgba(90,110,130,0.15)'; for (let yy = wy + 30; yy < y + FACE; yy += 8) g.fillRect(x, yy, w, 1); }
    g.fillStyle = '#6f4630'; g.fillRect(x, y + FACE - 6, w, 6);
    if (WALL_ART[r.name]) WALL_ART[r.name](g);
    for (const [dx, dy, hw] of ACROSS) if (dy === y && dx > x && dx < x + w) {
      const beyond = rooms.find(o => o.y + o.h === y && dx > o.x && dx < o.x + o.w);
      g.fillStyle = hx(beyond ? beyond.floor : GRASS); g.fillRect(dx - hw, y, hw * 2, FACE);
      g.fillStyle = 'rgba(20,12,30,0.22)'; g.fillRect(dx - hw, y, hw * 2, FACE);
      K.rr(dx - hw - 3, y, 4, FACE, 1, '#8a5a3c'); K.rr(dx + hw - 1, y, 4, FACE, 1, '#8a5a3c');
    }
  }
  const PLATE = { Kitchen: 100, 'Family room': -60, 'Bedroom 2': -20, 'Main bedroom': 70, Ensuite: 20, 'Dining room': 20, Laundry: 40, Garage: 0, 'Living room': -100, Foyer: 0, Bathroom: -40, 'Bedroom 3': 0, Nursery: -80 };
  function plate(g, r) {
    if (r.name === 'Hallway') return;
    const mid = r.x + r.w / 2 + (PLATE[r.name] || 0), w = textW(g, r.name, 8) + 16; K.rr(mid - w / 2, r.y + 2, w, 15, 3, '#6f4630'); K.rr(mid - w / 2 + 2, r.y + 4, w - 4, 11, 2, '#c98a4b'); text(g, r.name, mid, r.y + 13, 8, '#fff6d8', 'center', '#3a2618');
  }
  function facade(g) {
    const { x0, x1, y0, y1 } = FACADE;
    g.fillStyle = '#e8b46a'; g.fillRect(x0 - WT / 2, y0, x1 - x0 + WT, FACE);
    g.fillStyle = '#c98a4b'; for (let y = y0 + 6; y < y1; y += 8) g.fillRect(x0 - WT / 2, y, x1 - x0 + WT, 1);
    g.fillStyle = 'rgba(20,12,30,0.25)'; g.fillRect(x0 - WT / 2, y1 - 4, x1 - x0 + WT, 4);
    for (const wx of [620, 720, 940, 1080, 1390, 1550, 1640, 1790, 1890]) { win(g, wx, y0 + 10, 36); K.rr(wx - 20, y0 + 10, 4, 32, 1, '#3a6fc0'); K.rr(wx + 16, y0 + 10, 4, 32, 1, '#3a6fc0'); }
    for (const [x, hw, kind] of OUT) {
      if (kind === 'door') {
        g.fillStyle = hx(room('Foyer').floor); g.fillRect(x - hw, y0, hw * 2, FACE); g.fillStyle = 'rgba(20,12,30,0.2)'; g.fillRect(x - hw, y0, hw * 2, FACE);
        K.rr(x - hw - 4, y0, 5, FACE, 1, '#6f4630'); K.rr(x + hw - 1, y0, 5, FACE, 1, '#6f4630'); K.rr(x - hw - 6, y0 - 2, hw * 2 + 12, 6, 2, '#6f4630');
        K.rr(x + hw + 2, y0 + 5, 9, FACE - 7, 1, '#e04f4f'); K.circ(x + hw + 8, y0 + 34, 1.5, '#ffd23f');
        K.rr(x - 26, y1 - 2, 52, 8, 2, '#c9c0b0'); K.rr(x - 18, y1 + 1, 36, 6, 2, '#c9683f');
        K.circ(x - hw - 14, y0 + 24, 3, '#ffe9a8'); K.circ(x + hw + 18, y0 + 24, 3, '#ffe9a8');
        K.rr(x + 32, y0 + 16, 20, 10, 2, '#fff6d8'); text(g, '64', x + 42, y0 + 24, 7, '#3a2618', 'center', null);   // the house number
      } else {
        g.fillStyle = '#4a505c'; g.fillRect(x - hw, y0, hw * 2, FACE); g.fillStyle = hx(room('Garage').floor); g.fillRect(x - hw + 3, y0 + 14, hw * 2 - 6, FACE - 14);
        g.fillStyle = '#9aa1b0'; g.fillRect(x - hw, y0, hw * 2, 14); g.fillStyle = '#7d8494'; for (let y = y0 + 3; y < y0 + 14; y += 4) g.fillRect(x - hw, y, hw * 2, 1);
        K.rr(x - 3, y0, 6, FACE, 1, '#6f4630');
      }
    }
  }
  function caps(g) {
    const hcap = (x0, x1, y) => { if (x1 <= x0) return; K.rr(x0 - WT / 2, y - WT / 2, x1 - x0 + WT, WT, 1, '#6f4630'); g.fillStyle = '#8a5a3c'; g.fillRect(x0 - WT / 2, y - WT / 2, x1 - x0 + WT, 2); };
    const vcap = (x, y0, y1) => { if (y1 <= y0) return; K.rr(x - WT / 2, y0 - WT / 2, WT, y1 - y0 + WT, 1, '#6f4630'); g.fillStyle = '#8a5a3c'; g.fillRect(x - WT / 2 + 1, y0 - WT / 2, 2, y1 - y0 + WT); };
    const across = (x0, x1, y) => { const holes = [...ACROSS.filter(([, dy]) => dy === y).map(([dx, , hw]) => [dx, hw]), ...(y === FRONT ? OUT.map(([dx, hw]) => [dx, hw]) : [])].filter(([dx]) => dx > x0 && dx < x1).sort((a, b) => a[0] - b[0]); let s = x0; for (const [dx, hw] of holes) { hcap(s, dx - hw - WT / 2, y); s = dx + hw + WT / 2; } hcap(s, x1, y); };
    const down = (x, y0, y1) => { const holes = DOWN.filter(([dx, dy]) => dx === x && dy > y0 && dy < y1).sort((a, b) => a[1] - b[1]); let s = y0; for (const [, dy, hh] of holes) { vcap(x, s, dy - hh - WT / 2); s = dy + hh + WT / 2; } vcap(x, s, y1); };
    for (const r of rooms) { across(r.x, r.x + r.w, r.y); across(r.x, r.x + r.w, r.y + r.h); down(r.x, r.y, r.y + r.h); down(r.x + r.w, r.y, r.y + r.h); }
  }
  function pond(g, x, y) {
    g.fillStyle = '#3f9d4e'; g.beginPath(); g.ellipse(x, y, 56, 30, 0, 0, P * 2); g.fill();
    g.fillStyle = '#3a8fc0'; g.beginPath(); g.ellipse(x, y, 50, 26, 0, 0, P * 2); g.fill();
    g.fillStyle = '#2f78a8'; g.beginPath(); g.ellipse(x + 5, y + 5, 36, 16, 0, 0, P * 2); g.fill();
    g.fillStyle = '#7fc8ea'; g.beginPath(); g.ellipse(x - 14, y - 10, 16, 3, 0, 0, P * 2); g.fill();
    for (const [dx, dy] of [[16, -6], [-22, 8], [26, 10]]) { K.circ(x + dx, y + dy, 5.5, '#49d17a'); g.fillStyle = '#3a8fc0'; g.beginPath(); g.moveTo(x + dx, y + dy); g.arc(x + dx, y + dy, 6, -0.3, 0.3); g.fill(); }
    K.circ(x + 17, y - 7, 2, '#ff8fc7');
    for (const [dx, h] of [[-52, 18], [-48, 24], [-44, 15]]) { K.rr(x + dx, y - h, 2, h, 1, '#2f7a3c'); K.rr(x + dx - 1, y - h - 6, 4, 8, 2, '#8a5a3c'); }
    K.rr(x + 50, y + 18, 6, 6, 1, '#6f4630'); g.strokeStyle = '#8a5a3c'; g.lineWidth = 2; g.beginPath(); g.moveTo(x + 53, y + 20); g.lineTo(x + 36, y - 30); g.stroke();
    g.strokeStyle = '#fff6d8'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 36, y - 30); g.lineTo(x + 22, y + 2); g.stroke(); K.circ(x + 22, y + 2, 2, '#ff5a5a');
  }
  // tall pieces against a back wall are drawn a little smaller so they don't poke over the wall top
  const SCALE = { fridge: 0.82, wardrobe: 0.82, piano: 0.82, blast: 0.74, bolt: 0.74, jetrun: 0.74, bookshelf: 0.82, tank: 0.82, pinball: 0.8, bench: 0.85, shelf: 0.85, hutch: 0.85, dresser: 0.9 };
  // one look from ARTS with its shadow (unless bare), its front-bottom middle at (0, 0)
  function drawArt(g, a, t, bare) {
    if (a.art === 'pond') { pond(g, 0, -30); return; }
    const sc = SCALE[a.art] || 1;
    if (a.foot[0] && !bare) shadow(g, 0, -a.foot[1] * 0.4, a.foot[0] + 4, a.foot[1] * 0.5 + 3, 0.14);
    g.save(); g.scale(sc, sc);
    A[a.art](a.arg !== undefined ? a.arg : t);
    g.restore();
  }
  function drawThing(g, th, t) {
    const sc = SCALE[th.art] || 1;
    g.save(); g.translate(th.x, th.y); drawArt(g, ARTS[th.ai], t); g.restore();
    if (th.art === 'tank') for (let k = 0; k < 3; k++) { const u = t * (0.3 + k * 0.1) + k * 2, fx = th.x + Math.sin(u) * 28 * sc, fy = th.y - (72 - k * 12) * sc; g.save(); g.translate(fx, fy); g.scale(Math.cos(u) > 0 ? 0.6 : -0.6, 0.6); HOUSEART.fish(g, k, t); g.restore(); }
  }
  function hero(g, m, x, y, st, t) {
    const Rb = m.hero === 'robot', s = Rb ? 0.074 : 0.071, hex = m.hex, acc = m.acc;
    shadow(g, x, y, Rb ? 10 : 9);
    if (st.moving && st.dir !== 'down') {
      const pose = K.pose(m.hero, 'run', Rb ? t * 2.2 : t * 2);
      if (Rb) K.robotSide(x, y, s, hex, Object.assign(pose, { dir: st.face, acc })); else K.ostrichSide(x, y, s, hex, Object.assign(pose, { dir: st.face, acc }));
    } else if (st.moving) {
      const w = Math.sin(t * 12);
      if (Rb) K.robot(x, y, s, hex, { t, legL: w * 14, legR: w * 14, legLen: 54 - Math.abs(w) * 6, armL: 22 - w * 18, armR: -22 - w * 18, bob: Math.abs(w) * 4, acc });
      else K.ostrich(x, y, s, hex, { t, legL: w * 12, legR: -w * 12, bob: Math.abs(w) * 8, acc });
    } else if (Rb) K.robot(x, y, s, hex, { t, bob: Math.sin(t * 3) * 3, acc });
    else K.ostrich(x, y, s, hex, { t, bob: Math.abs(Math.sin(t * 2.6)) * 8, acc });
  }
  // everything that never moves: the garden, the floors, the walls and the front of the house
  function drawStatic(g) {
    K.g = g;
    garden(g);
    for (const r of rooms) { floor(g, r); backWall(g, r); }
    facade(g); caps(g);
  }
  // The house as the camera at (cx, cy) sees it, with actors ({ y, draw(g) }) sorted in among the
  // furniture by where their feet are. bg is drawStatic's picture made once (or it is drawn
  // here); show(th) can hide a thing.
  function draw(g, cx, cy, t, actors = [], bg = null, show = null, w = 320, h = 240) {
    K.g = g; g.save(); g.translate(-cx, -cy);
    if (bg) g.drawImage(bg, cx, cy, w, h, cx, cy, w, h); else drawStatic(g);
    K.g = g;
    const list = [];
    for (const th of things) if (!show || show(th)) list.push({ y: th.flat ? -1e4 + th.y : th.y, draw: () => drawThing(g, th, t) });
    for (const a of actors) list.push(a);
    list.sort((p, q) => p.y - q.y);
    for (const o of list) o.draw(g);
    for (const r of rooms) plate(g, r);
    g.restore();
  }
  function prompt(g, label, x, y) { const s = 'A  ' + label, w = textW(g, s, 8) + 14; K.rr(x - w / 2, y, w, 15, 5, 'rgba(27,20,38,0.86)'); text(g, s, x, y + 11, 8, '#fff6d8', 'center', null); }

  return { WORLD, FACE, CAP, WT, rooms, ACROSS, DOWN, OUT, FACADE, walk, things, ARTS, SCALE, START, GATE, LEASH, AREAS, PLATE,
           draw, drawStatic, drawArt, drawThing, plate, hero, solid, near, roomAt, camera, prompt, text, textW, shadow };
})();
