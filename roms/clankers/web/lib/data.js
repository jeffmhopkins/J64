// Reading and writing the text formats in roms/clankers/data (see data/README.md).
// Shared by the game and the editors, so both always agree on the format.
const DATA_FMT = (() => {
  const ROWS = 15;
  // Characters a level map may use, with what the editor calls them. Order is the editor palette order.
  const LEGEND = [
    ['.', 'Empty'], ['#', 'Ground'], ['=', 'Girder'], ['-', 'Plank'], ['X', 'Crate'], ['?', 'Gear crate'], ['$', 'Bolt crate'],
    ['1', 'Mega Battery crate'], ['2', 'Rainbow crate'], ['3', 'Bubble crate'], ['4', 'Magnet crate'], ['D', 'Door'], ['L', 'Lever'],
    ['o', 'Bolt'], ['G', 'Golden gear'], ['b', 'Bumblebot'], ['s', 'Boing Sprocket'], ['g', 'Grumble Crate'], ['T', 'Tyre spring'],
    ['v', 'Down sign'], ['r', 'Raft (rides its track)'], ['~', 'Raft track, sideways'], ['|', 'Raft track, up and down'], ['k', 'Swinging hook'], ['H', 'Hatch in'], ['h', 'Hatch out'], ['@', 'Start'], ['P', 'Charging pad'],
  ];
  const KNOWN = new Set(LEGEND.map(l => l[0]));

  function splitHeader(text) {
    const lines = text.replace(/\r/g, '').split('\n'), meta = {};
    let i = 0;
    for (; i < lines.length; i++) {
      const ln = lines[i];
      if (ln.trim() === '---') { i++; break; }
      const m = /^\s*([\w-]+)\s*:\s*(.*)$/.exec(ln);
      if (m) meta[m[1]] = m[2].trim();
    }
    return { meta, rest: lines.slice(i) };
  }

  function parseLevel(text) {
    const { meta, rest } = splitHeader(text);
    let rows = rest.filter(r => r.length).slice(0, ROWS);
    const w = Math.max(20, ...rows.map(r => r.length));
    while (rows.length < ROWS) rows.unshift('');
    rows = rows.map(r => r.padEnd(w, '.').split('').map(c => KNOWN.has(c) ? c : '.'));
    return { meta, rows, w };
  }
  function levelText(meta, rows) {
    const keys = ['name', 'kind', 'song', 'baby', 'hatch', 'boss'];
    const head = keys.filter(k => meta[k]).map(k => k + ': ' + meta[k]);
    for (const k of Object.keys(meta)) if (!keys.includes(k) && meta[k]) head.push(k + ': ' + meta[k]);
    return head.join('\n') + '\n---\n' + rows.map(r => r.join('')).join('\n') + '\n';
  }

  // "a b c" with "quoted words" and key=value pairs
  function tokens(line) {
    const out = []; const re = /"([^"]*)"|(\S+)/g; let m;
    while ((m = re.exec(line))) out.push(m[1] !== undefined ? { q: m[1] } : m[2]);
    return out;
  }
  function parseWorld(text) {
    const W = { meta: {}, nodes: [], roads: [], features: [], props: [] };
    for (const raw of text.replace(/\r/g, '').split('\n')) {
      const line = raw.replace(/\s+#.*$/, '').trim();
      if (!line || line.startsWith('#')) continue;
      const kv = /^([\w-]+)\s*:\s*(.*)$/.exec(line);
      if (kv) { W.meta[kv[1]] = kv[2].trim(); continue; }
      const t = tokens(line), word = t[0], opts = {}, plain = [];
      for (const x of t.slice(1)) {
        if (typeof x === 'object') plain.push(x.q);
        else if (x.includes('=')) { const [k, v] = x.split('='); (opts[k] = opts[k] || []).push(v); }
        else plain.push(x);
      }
      const one = k => opts[k] && opts[k][0];
      if (word === 'node') W.nodes.push({ id: plain[0], col: +plain[1], row: +plain[2], kind: plain[3], label: plain[4] || plain[0], level: one('level'), gate: one('gate') ? +one('gate') : 0 });
      else if (word === 'road') W.roads.push({ a: plain[0], b: plain[1], via: (opts.via || []).map(v => v.split(',').map(Number)), gate: one('gate') ? +one('gate') : 0 });
      else if (word === 'feature') { const args = {}; for (const k in opts) args[k] = one(k); W.features.push({ name: plain[0], pos: plain.slice(1).map(Number), args }); }
      else if (word === 'prop') W.props.push({ name: plain[0], col: +plain[1], row: +plain[2] });
    }
    return W;
  }
  function worldText(W) {
    const L = [];
    for (const k of ['name', 'num', 'theme', 'gears', 'babies', 'boss-msg']) if (W.meta[k] !== undefined && W.meta[k] !== '') L.push(k + ': ' + W.meta[k]);
    for (const k of Object.keys(W.meta)) if (!['name', 'num', 'theme', 'gears', 'babies', 'boss-msg'].includes(k)) L.push(k + ': ' + W.meta[k]);
    L.push('', '# node <id> <col> <row> <kind> "<label>" [level=<file>] [gate=<gears needed>]');
    for (const n of W.nodes) L.push(['node', n.id, n.col, n.row, n.kind, '"' + n.label + '"', n.level ? 'level=' + n.level : '', n.gate ? 'gate=' + n.gate : ''].filter(Boolean).join(' '));
    L.push('', '# road <from> <to> [via=<col>,<row> ...] [gate=<gears needed>]');
    for (const r of W.roads) L.push(['road', r.a, r.b, ...r.via.map(v => 'via=' + v.join(',')), r.gate ? 'gate=' + r.gate : ''].filter(Boolean).join(' '));
    L.push('', '# scenery: feature <name> [args], prop <name> <col> <row>');
    for (const f of W.features) L.push(['feature', f.name, ...f.pos, ...Object.entries(f.args).map(([k, v]) => k + '=' + v)].join(' '));
    for (const p of W.props) L.push(['prop', p.name, p.col, p.row].join(' '));
    return L.join('\n') + '\n';
  }

  // A world file + its theme -> the object the map engine (OW.core) runs.
  function worldForMap(W) {
    const T = 16, C = c => c * T + 8, m = W.meta;
    const world = {
      num: +m.num || 1, name: m.name || 'New World', gearsMax: +m.gears || 9, grid: true, bossMsg: m['boss-msg'] || '',
      babyHexes: (m.babies || 'FF8FC7 49F08A FFB347 8FD3FF').split(/\s+/).map(h => parseInt(h, 16)),
      nodes: W.nodes.map(n => Object.assign({ id: n.id, x: C(n.col), y: C(n.row), kind: n.kind, label: n.label, level: n.level }, n.kind === 'level' ? { num: W.nodes.filter(q => q.kind === 'level').indexOf(n) + 1 } : {}, n.gate ? { gate: n.gate } : {})),
      edges: W.roads.map(r => { const o = {}; if (r.gate) o.gate = r.gate; if (r.via.length) o.pts = r.via.map(([c, rr]) => [C(c), C(rr)]); return Object.keys(o).length ? [r.a, r.b, o] : [r.a, r.b]; }),
      start: { beaten: [], at: (W.nodes.find(n => n.kind === 'home') || W.nodes[0] || {}).id, gears: 0, bolts: 0 },
      features: W.features, props: W.props,
    };
    const theme = (typeof THEMES !== 'undefined' && THEMES[m.theme]) ? THEMES[m.theme](world) : {};
    return Object.assign(world, theme);
  }

  return { ROWS, LEGEND, parseLevel, levelText, parseWorld, worldText, worldForMap };
})();
const THEMES = {};
