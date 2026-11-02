// How a level looks in each world: the far backdrop, the hills in front of it, and the tiles.
// The browser game draws with these, and tools/bake_sprites.mjs bakes the same pictures for the
// N64, so both builds match. A world's theme (world.txt "theme:") picks the entry in LTHEMES.
//
//   far(g, boss)   a 640x240 picture that scrolls at a fifth of the camera speed
//   mid(g)         the hills in front, also 640 wide; only y 160..240 is kept, scrolls at half speed
//   tile(g, i)     one 16x16 tile at (0,0), by frame number:
//                    0-3 ground top (four looks)   4-7 ground inside   8 girder   9 plank
//                    10 crate   11 crate with a prize   12 door   13 door with its stripe
//                    14-17 conveyor belt, four steps of it moving right (left belts play them backwards)
//   debris         the two colours a broken crate scatters
// Set CK.g to the context before calling (the bake and the game both do).
const LTHEMES = {};
const LEVELART = (() => {
  const K = CK, P = Math.PI, TS = 16;
  const FONT = '"Titan One", "Arial Black", Impact, sans-serif';
  const HASH = [12, 1, 6, 15].map(h => h * 105);   // the four ground looks
  function qmark(g, x, y, col, outline) {
    g.font = `9px ${FONT}`; g.textAlign = 'center'; g.textBaseline = 'alphabetic';
    g.lineJoin = 'round'; g.strokeStyle = outline; g.lineWidth = 3; g.strokeText('?', x, y); g.fillStyle = col; g.fillText('?', x, y);
  }
  // The wooden crates, the lever door and the belt look the same in every world unless a theme says otherwise.
  function crate(g, prize) {
    K.rr(0.5, 0.5, TS - 1, TS - 1, 1.5, '#b07a44'); g.strokeStyle = '#7d5128'; g.lineWidth = 1.5; g.strokeRect(2, 2, TS - 4, TS - 4);
    g.beginPath(); g.moveTo(2.5, 2.5); g.lineTo(TS - 2.5, TS - 2.5); g.stroke();
    g.fillStyle = '#d9a46a'; g.fillRect(1, 1, TS - 2, 1);
    if (prize) { g.fillStyle = '#ffd23f'; g.fillRect(1, 6, TS - 2, 4); qmark(g, 8, 12, '#fff6d8', '#7d5128'); }
  }
  function door(g, stripe) {
    K.rr(1, 0, TS - 2, TS, 1, '#7d8397'); g.fillStyle = '#5d6378'; g.fillRect(3, 3, TS - 6, TS - 6);
    if (stripe) { g.fillStyle = '#ffcf3a'; g.fillRect(1, 0, TS - 2, 3); }
  }
  function belt(g, k, body = '#3a3346', tread = '#5d566a', roller = '#9aa1b0') {
    K.rr(0, 1, TS, TS - 2, 3, body);
    g.save(); g.beginPath(); g.rect(0, 1, TS, 4); g.clip();
    g.fillStyle = tread; for (let x = -8 + k * 2; x < TS + 8; x += 8) { g.beginPath(); g.moveTo(x, 1); g.lineTo(x + 3, 1); g.lineTo(x + 5, 5); g.lineTo(x + 2, 5); g.fill(); }
    g.restore();
    g.fillStyle = '#ffcf3a'; g.fillRect(0, 1, TS, 1);
    for (const cx of [4, 12]) { K.circ(cx, 10, 3, roller); g.save(); g.translate(cx, 10); g.rotate(k * P / 4); g.fillStyle = body; g.fillRect(-0.6, -2.4, 1.2, 4.8); g.restore(); }
    g.fillStyle = 'rgba(0,0,0,0.3)'; g.fillRect(0, TS - 2, TS, 1);
  }
  // Ground built from one recipe: body colour, a few specks, and a lighter top band.
  function ground(g, i, c, extraTop, extraIn) {
    const hash = HASH[i % 4];
    g.fillStyle = c.body; g.fillRect(0, 0, TS, TS);
    g.fillStyle = c.speck; if (hash % 3 === 0) g.fillRect(hash % 11, (hash % 7) + 5, 3, 2); if (hash % 5 === 0) g.fillRect((hash % 9) + 3, 10, 2, 2);
    if (extraIn) extraIn(g, i % 4, hash);
    if (i < 4) {
      g.fillStyle = c.top; g.fillRect(0, 0, TS, 5); g.fillStyle = c.rim; g.fillRect(0, 0, TS, 1.5);
      g.fillStyle = c.under; g.fillRect(0, 5, TS, 1.5);
      if (extraTop) extraTop(g, i % 4, hash);
    }
  }
  function tiles(spec) {
    return (g, i) => {
      if (i < 8) return spec.ground(g, i);
      if (i === 8) return spec.girder(g);
      if (i === 9) return spec.plank(g);
      if (i < 12) return (spec.crate || crate)(g, i === 11);
      if (i < 14) return (spec.door || door)(g, i === 13);
      return (spec.belt || belt)(g, i - 14);
    };
  }
  return { crate, door, belt, ground, tiles, TILE_FRAMES: 18 };
})();
