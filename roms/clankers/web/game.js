// ===================================================================================
// CLANKERS — the playable HTML version. One file, 320x240, fixed 60 Hz update.
// Built from: CK (characters), OW (map engine), DATA_FMT + DATA (levels and worlds as text), THEMES, makeTitle, GLITCH.
// ===================================================================================
(() => {
const K = CK, P = Math.PI, FONT = '"Titan One", "Arial Black", Impact, sans-serif';
const cv = document.getElementById('scr'), g = cv.getContext('2d'), W = 320, H = 240, TS = 16;
const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches;
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const store = {
  get(k, d) { try { const v = localStorage.getItem('clankers:' + k); return v ? JSON.parse(v) : d; } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem('clankers:' + k, JSON.stringify(v)); } catch (e) { /* storage blocked */ } },
};

// ----------------------------------------------------------------------------- AUDIO
const SND = (() => {
  let ac = null, master, musicG, sfxG, muted = store.get('muted', false), cur = null, timer = null;
  function ensure() {
    if (ac) return;
    try {
      ac = new (window.AudioContext || window.webkitAudioContext)();
      master = ac.createGain(); master.gain.value = muted ? 0 : 0.55; master.connect(ac.destination);
      musicG = ac.createGain(); musicG.gain.value = 0.32; musicG.connect(master);
      sfxG = ac.createGain(); sfxG.gain.value = 0.7; sfxG.connect(master);
    } catch (e) { ac = null; }
  }
  function resume() { ensure(); if (ac && ac.state === 'suspended') ac.resume(); }
  function tone(f, dur, type = 'square', vol = 0.3, when = 0, slide = 0, dest) {
    if (!ac) return; const t = ac.currentTime + when, o = ac.createOscillator(), gn = ac.createGain();
    o.type = type; o.frequency.setValueAtTime(f, t); if (slide) o.frequency.exponentialRampToValueAtTime(Math.max(30, f * slide), t + dur);
    gn.gain.setValueAtTime(0.0001, t); gn.gain.exponentialRampToValueAtTime(vol, t + 0.008); gn.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(gn); gn.connect(dest || sfxG); o.start(t); o.stop(t + dur + 0.03);
  }
  let nbuf = null;
  function noise(dur, vol = 0.2, when = 0, freq = 800, dest, type = 'bandpass') {
    if (!ac) return;
    if (!nbuf) { nbuf = ac.createBuffer(1, ac.sampleRate, ac.sampleRate); const d = nbuf.getChannelData(0); for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1; }
    const t = ac.currentTime + when, s = ac.createBufferSource(), f = ac.createBiquadFilter(), gn = ac.createGain();
    s.buffer = nbuf; f.type = type; f.frequency.value = freq; gn.gain.setValueAtTime(vol, t); gn.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    s.connect(f); f.connect(gn); gn.connect(dest || sfxG); s.start(t); s.stop(t + dur + 0.02);
  }
  const fx = {
    jump: () => tone(330, 0.16, 'square', 0.18, 0, 2.2),
    boost: () => { noise(0.3, 0.25, 0, 1800); tone(220, 0.22, 'sawtooth', 0.1, 0, 2.6); },
    flap: () => tone(520, 0.06, 'triangle', 0.14, 0, 1.5),
    bolt: () => { tone(988, 0.05, 'square', 0.12); tone(1319, 0.09, 'square', 0.12, 0.05); },
    gear: () => [523, 659, 784, 1047, 1319].forEach((f, i) => tone(f, 0.14, 'square', 0.16, i * 0.07)),
    reboot: () => { tone(180, 0.08, 'square', 0.18, 0, 3); tone(880, 0.1, 'triangle', 0.22, 0.08); tone(1175, 0.14, 'triangle', 0.22, 0.15); },
    wobble: () => { tone(420, 0.35, 'triangle', 0.25, 0, 0.35); tone(300, 0.3, 'square', 0.06, 0.05, 0.5); },
    spring: () => tone(140, 0.4, 'sine', 0.4, 0, 4.5),
    crate: () => { noise(0.2, 0.35, 0, 500); tone(110, 0.12, 'square', 0.12, 0, 0.5); },
    lever: () => { tone(140, 0.07, 'square', 0.2); tone(210, 0.1, 'square', 0.2, 0.07); },
    door: () => { noise(0.6, 0.18, 0, 220); tone(90, 0.5, 'sawtooth', 0.06, 0, 0.6); },
    grab: () => tone(700, 0.12, 'sawtooth', 0.08, 0, 0.45),
    kick: () => { noise(0.08, 0.35, 0, 1200); tone(170, 0.1, 'square', 0.18, 0, 0.5); },
    boing: () => tone(200, 0.45, 'sine', 0.35, 0, 3.2),
    button: () => { tone(440, 0.09, 'square', 0.2); tone(330, 0.16, 'square', 0.2, 0.09); },
    clank: () => { noise(0.35, 0.4, 0, 300); tone(80, 0.3, 'square', 0.15, 0, 0.7); },
    select: () => tone(660, 0.05, 'square', 0.13),
    confirm: () => { tone(523, 0.07, 'square', 0.16); tone(784, 0.12, 'square', 0.16, 0.07); },
    open: () => [392, 523, 659, 784].forEach((f, i) => tone(f, 0.14, 'triangle', 0.22, i * 0.09)),
    step: () => tone(260, 0.03, 'square', 0.05),
    hop: () => tone(500, 0.08, 'square', 0.12, 0, 1.8),
    nope: () => tone(160, 0.15, 'square', 0.12),
    sprout: () => [262, 330, 392, 523, 659].forEach((f, i) => tone(f, 0.08, 'triangle', 0.18, i * 0.05)),
    power: () => [523, 659, 784, 1047, 784, 1047, 1319].forEach((f, i) => tone(f, 0.1, 'square', 0.15, i * 0.06)),
    shrink: () => [784, 659, 523, 392].forEach((f, i) => tone(f, 0.1, 'square', 0.15, i * 0.07)),
    blow: () => { tone(500, 0.15, 'sine', 0.2, 0, 1.8); noise(0.1, 0.08, 0, 2500); },
    stomp: () => { noise(0.3, 0.45, 0, 180, undefined, 'lowpass'); tone(70, 0.25, 'square', 0.15, 0, 0.6); },
    hatch: () => { [784, 659, 523, 392, 262].forEach((f, i) => tone(f, 0.09, 'square', 0.15, i * 0.05)); },
    thunder: () => { noise(1.1, 0.5, 0, 160, undefined, 'lowpass'); noise(0.2, 0.3, 0, 2000); },
    whoosh: () => { noise(1.4, 0.35, 0, 600); tone(200, 1.2, 'sine', 0.08, 0, 3); },
    slide: () => { noise(0.35, 0.2, 0, 900, undefined, 'lowpass'); tone(300, 0.25, 'triangle', 0.1, 0, 0.6); },
    ouch: () => [523, 440, 349, 262, 196].forEach((f, i) => tone(f, 0.16, 'square', 0.16, i * 0.11)),
    pop: () => { tone(900, 0.06, 'sine', 0.25, 0, 0.5); noise(0.05, 0.15, 0, 3000); },
    tick: () => tone(1200, 0.04, 'square', 0.1),
    star: (k = 0) => { tone([784, 988, 1175][k], 0.18, 'triangle', 0.26); tone([1568, 1976, 2349][k], 0.12, 'square', 0.06, 0.04); },
    record: () => [523, 659, 784, 1047, 1319, 1568, 2093].forEach((f, i) => tone(f, 0.11, 'square', 0.13, i * 0.05)),
    flag: () => [392, 523, 659, 784, 1047].forEach((f, i) => tone(f, 0.12, 'triangle', 0.22, i * 0.06)),
  };
  // --- music: tiny chiptune sequencer. Tokens are eighth notes; '-' extends, '.' rests.
  const NOTE = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
  const hz = name => { const m = /^([A-G])(#?)(\d)$/.exec(name); if (!m) return 0; const n = NOTE[m[1]] + (m[2] ? 1 : 0) + (+m[3] + 1) * 12; return 440 * Math.pow(2, (n - 69) / 12); };
  function parse(str) { const out = []; for (const tok of str.replace(/\|/g, ' ').split(/\s+/).filter(Boolean)) { const len = 1 + (tok.match(/-/g) || []).length, n = tok.replace(/-/g, ''); out.push([n === '.' ? 0 : hz(n), len]); } return out; }
  // The songs come from data/songs.txt, the same file the N64 build plays.
  const SONGS = {};
  { let cur = null;
    for (const raw of (DATA['songs.txt'] || '').split('\n')) {
      const ln = raw.trim(); if (!ln || ln[0] === '#') continue;
      const m = /^song\s+(\S+)(.*)$/.exec(ln);
      if (m) { const bpm = /bpm=(\d+)/.exec(m[2]); cur = SONGS[m[1]] = { bpm: bpm ? +bpm[1] : 120, loop: /\bloop\b/.test(m[2]), tracks: [] }; continue; }
      const [wave, vol, ...notes] = ln.split(/\s+/); if (cur) cur.tracks.push([wave, +vol, notes.join(' ')]);
    } }
  function playSong(name) {
    ensure(); if (!ac) return; if (cur && cur.name === name) return; stopSong();
    const song = SONGS[name]; if (!song) return;
    const spb = 60 / song.bpm / 2;   // seconds per eighth
    const tracks = song.tracks.map(([type, vol, str]) => ({ type, vol, notes: type === 'drum' ? str.replace(/\|/g, ' ').split(/\s+/).filter(Boolean).map(t => [t, 1]) : parse(str), i: 0, next: ac.currentTime + 0.06 }));
    cur = { name, song, tracks, spb, done: false };
    timer = setInterval(() => {
      if (!cur) return; const ahead = ac.currentTime + 0.25; let allDone = true;
      for (const tr of cur.tracks) {
        while (tr.next < ahead) {
          if (tr.i >= tr.notes.length) { if (!song.loop) break; tr.i = 0; }
          const [n, len] = tr.notes[tr.i++], when = tr.next - ac.currentTime, dur = len * spb;
          if (tr.type === 'drum') {
            if (n === 'k') { tone(120, 0.12, 'sine', tr.vol * 2.2, when, 0.35, musicG); }
            else if (n === 's') noise(0.1, tr.vol * 1.2, when, 1800, musicG);
            else if (n === 'h') noise(0.04, tr.vol * 0.6, when, 7000, musicG, 'highpass');
          } else if (n) tone(n, dur * 0.92, tr.type, tr.vol, when, 0, musicG);
          tr.next += dur;
        }
        if (tr.i < tr.notes.length) allDone = false;
      }
      if (!song.loop && allDone) stopSong();
    }, 50);
  }
  function stopSong() { if (timer) clearInterval(timer); timer = null; cur = null; }
  function setMuted(m) { muted = m; store.set('muted', m); if (master) master.gain.value = m ? 0 : 0.55; }
  return { resume, fx: new Proxy(fx, { get: (o, k) => (a) => { if (ac && o[k]) try { o[k](a); } catch (e) { /* audio hiccup */ } } }), playSong, stopSong, setMuted, get muted() { return muted; }, get ready() { return !!ac; } };
})();

// ----------------------------------------------------------------------------- INPUT
const BTN = ['left', 'right', 'up', 'down', 'a', 'b', 'start'];
const KEYMAP = [
  { left: ['ArrowLeft'], right: ['ArrowRight'], up: ['ArrowUp'], down: ['ArrowDown'], a: ['z', 'Z', ' ', 'k', 'K'], b: ['x', 'X', 'j', 'J'], start: ['Enter'] },
  { left: ['a', 'A'], right: ['d', 'D'], up: ['w', 'W'], down: ['s', 'S'], a: ['g', 'G'], b: ['h', 'H'], start: ['Tab'] },
];
const keys = new Set(), touch = {}, latch = new Set();   // latch: keys/buttons pressed and released between two frames still count once
const pads = [0, 1].map(() => ({ cur: {}, prev: {} }));
let anyInputYet = false;
// Edge on Xbox: hand the controller to the game instead of using it as a mouse pointer
try { if ('gamepadInputEmulation' in navigator) navigator.gamepadInputEmulation = 'gamepad'; } catch (e) { /* not on this browser */ }
function onFirstInput() { if (!anyInputYet) { anyInputYet = true; } SND.resume(); if (scene && scene.song) SND.playSong(scene.song); }
window.addEventListener('keydown', e => {
  keys.add(e.key); latch.add(e.key); onFirstInput();
  if (['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', ' ', 'Tab'].includes(e.key)) e.preventDefault();
});
window.addEventListener('keyup', e => keys.delete(e.key));
window.addEventListener('blur', () => keys.clear());
const touchEl = document.getElementById('touch');
for (const b of touchEl.querySelectorAll('[data-btn]')) {
  const k = b.dataset.btn;
  const on = e => { e.preventDefault(); touch[k] = true; latch.add('touch:' + k); b.classList.add('held'); onFirstInput(); try { b.setPointerCapture(e.pointerId); } catch (err) { /* ok */ } };
  const off = e => { e.preventDefault(); touch[k] = false; b.classList.remove('held'); };
  b.addEventListener('pointerdown', on); b.addEventListener('pointerup', off); b.addEventListener('pointercancel', off); b.addEventListener('lostpointercapture', off);
  b.addEventListener('contextmenu', e => e.preventDefault());
}
cv.addEventListener('pointerdown', () => { onFirstInput(); cv.focus(); });
function pollPads() {
  let gps = [];
  try { gps = (navigator.getGamepads && navigator.getGamepads()) || []; } catch (e) { gps = []; }
  const real = [...gps].filter(Boolean);
  pads.forEach((p, i) => {
    p.prev = p.cur; const c = {};
    for (const b of BTN) c[b] = KEYMAP[i][b].some(k => keys.has(k) || latch.has(k));
    if (i === 0) for (const b of BTN) c[b] = c[b] || !!touch[b] || latch.has('touch:' + b);
    const gp = real[i];
    if (gp) {
      const ax = gp.axes[0] || 0, ay = gp.axes[1] || 0, bt = n => gp.buttons[n] && gp.buttons[n].pressed;
      c.left = c.left || ax < -0.45 || bt(14); c.right = c.right || ax > 0.45 || bt(15);
      c.up = c.up || ay < -0.5 || bt(12); c.down = c.down || ay > 0.5 || bt(13);
      c.a = c.a || bt(0); c.b = c.b || bt(1) || bt(2); c.start = c.start || bt(9);
      if (Object.values(c).some(Boolean)) onFirstInput();
    }
    p.cur = c;
  });
  latch.clear();
}
const held = (i, b) => !!pads[i].cur[b];
const hit = (i, b) => !!pads[i].cur[b] && !pads[i].prev[b];
const anyHit = b => hit(0, b) || hit(1, b);

// ----------------------------------------------------------------------------- SHARED STATE
const COLS = [0x5AD1FF, 0xFFD23F, 0xFF7A59, 0x49F08A, 0xC28BFF, 0xFF8FC7, 0x7FA7C9, 0xF5F1E8];
const CNAMES = ['Sky', 'Sunny', 'Tangerine', 'Lime', 'Grape', 'Bubblegum', 'Steel', 'Snow'];
const party = store.get('party', [{ hero: 'robot', col: 0, joined: true }, { hero: 'ostrich', col: 1, joined: false }]);
party[0].joined = true;
let little = store.get('little', true);
const fresh = () => ({ beaten: [], at: 'home', gears: 0, bolts: 0, got: {}, lives: 3, powers: [null, null] });
// Progress is kept per world ('w1', 'w2'...); 'world' is the world you are in.
let worldNum = 1, prog = fresh();
const saveProg = () => store.set('w' + worldNum, prog);
let particles = [];
let scene = null;
function go(s) { scene = s; if (s.enter) s.enter(); if (s.song && SND.ready) SND.playSong(s.song); }
const activeParty = () => party.filter(m => m.joined).map((m, i) => ({ hero: m.hero, hex: COLS[m.col], slot: party.indexOf(m) }));

// ----------------------------------------------------------------------------- PARTICLES
function puff(x, y, n = 6, col = 'rgba(255,248,230,0.9)') { for (let i = 0; i < n; i++) particles.push({ x, y, vx: (Math.random() - 0.5) * 2, vy: -Math.random() * 1.2, life: 26 + Math.random() * 10, max: 36, r: 2 + Math.random() * 2, col, kind: 'puff' }); }
function sparks(x, y, n = 8, col = '#ffe66b') { for (let i = 0; i < n; i++) { const a = Math.random() * P * 2, sp = 1 + Math.random() * 2; particles.push({ x, y, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp - 1, life: 30, max: 30, r: 2.5, col, kind: 'star' }); } }
function confetti(x, y, n = 40) { const c = ['#ff8fc7', '#5ad1ff', '#ffd23f', '#49f08a', '#c28bff', '#ff7a59']; for (let i = 0; i < n; i++) particles.push({ x: x + (Math.random() - 0.5) * 200, y: y - Math.random() * 60, vx: (Math.random() - 0.5) * 1.5, vy: Math.random() * 1.5, life: 120 + Math.random() * 60, max: 180, r: 2, col: c[i % c.length], kind: 'conf', rot: Math.random() * 6 }); }
function debris(x, y) { for (let i = 0; i < 8; i++) particles.push({ x: x + Math.random() * 12, y: y + Math.random() * 12, vx: (Math.random() - 0.5) * 3, vy: -1 - Math.random() * 3, life: 40, max: 40, r: 2.5, col: i % 2 ? '#b07a44' : '#7d5128', kind: 'chunk', g: 0.25 }); }
function stepParticles() {
  for (const p of particles) { p.x += p.vx; p.y += p.vy; p.vy += p.g || (p.kind === 'conf' ? 0.02 : 0); if (p.kind === 'puff') { p.vx *= 0.92; p.vy *= 0.92; } p.life--; if (p.kind === 'conf') p.rot += 0.2; }
  particles = particles.filter(p => p.life > 0);
}
function drawParticles(camX = 0) {
  for (const p of particles) {
    const a = clamp(p.life / p.max, 0, 1), x = p.x - camX;
    g.globalAlpha = p.kind === 'conf' ? 1 : a;
    if (p.kind === 'star') K.star(x, p.y, p.r * (0.5 + a), p.col);
    else if (p.kind === 'conf') { g.save(); g.translate(x, p.y); g.rotate(p.rot); g.fillStyle = p.col; g.fillRect(-1.5, -1, 3, 2); g.restore(); }
    else if (p.kind === 'chunk') { g.fillStyle = p.col; g.fillRect(x - 1.5, p.y - 1.5, 3, 3); }
    else K.circ(x, p.y, p.r * (1.5 - a * 0.5), p.col);
  }
  g.globalAlpha = 1;
}

// ----------------------------------------------------------------------------- TEXT HELPERS
function text(s, x, y, size = 10, col = '#fff6d8', align = 'center', outline = '#1b1426') {
  g.font = `${size}px ${FONT}`; g.textAlign = align; g.textBaseline = 'alphabetic';
  if (outline) { g.lineJoin = 'round'; g.strokeStyle = outline; g.lineWidth = Math.max(2, size / 3); g.strokeText(s, x, y); }
  g.fillStyle = col; g.fillText(s, x, y);
}
function panel(x, y, w, h, c = 'rgba(27,20,38,0.86)') { K.rr(x, y, w, h, 5, c); }

// ----------------------------------------------------------------------------- TITLE
const TITLE = makeTitle(g, W, H, reduce);
const titleScene = {
  song: 'title', t: 0,
  update() { this.t += 1 / 60; if (anyHit('start') || anyHit('a')) { SND.fx.confirm(); go(selectScene); } },
  draw() { K.g = g; TITLE.frame(this.t); },
};


// ----------------------------------------------------------------------------- INTRO STORY (plays before the title, any button skips)
function mixHex(a, b, k) { const pa = [a >> 16, (a >> 8) & 255, a & 255], pb = [b >> 16, (b >> 8) & 255, b & 255]; return 'rgb(' + pa.map((v, i) => Math.round(v + (pb[i] - v) * k)).join(',') + ')'; }
const introScene = {
  song: 'lullaby', t: 0, bolts: [], flash: 0,
  stars: Array.from({ length: 40 }, (_, i) => [(i * 97) % 320, (i * 53) % 120, (i % 3) + 1]),
  enter() { this.t = 0; },
  update() {
    const was = this.t; this.t += 1 / 60; const t = this.t;
    if (was < 4.5 && t >= 4.5) { this.song = 'storm'; SND.playSong('storm'); }
    if (was < 11 && t >= 11) { this.song = 'title'; SND.playSong('title'); }
    if (t > 4.8 && t < 10.5 && Math.random() < 0.025) { this.flash = 1; SND.fx.thunder(); this.bolt = { x: 120 + Math.random() * 160, life: 10 }; }
    if (was < 8 && t >= 8) SND.fx.whoosh();
    if (was < 11.6 && t >= 11.6) SND.fx.jump();
    if (was < 12 && t >= 12) SND.fx.boost();
    if (this.flash > 0) this.flash -= 0.08; if (this.bolt && --this.bolt.life <= 0) this.bolt = null;
    if (t > 15.2 || (t > 0.4 && (anyHit('start') || anyHit('a')))) { titleScene.t = 0; go(titleScene); }
  },
  draw() {
    const t = this.t; K.g = g;
    const dawn = clamp((t - 10.5) / 3, 0, 1), storm = clamp((t - 4.2) / 1.2, 0, 1) * (1 - clamp((t - 10.2) / 1.2, 0, 1));
    // sky
    const gr = g.createLinearGradient(0, 0, 0, H);
    gr.addColorStop(0, mixHex(mixHex(0x141a3d, 0x2a1440, storm).match(/\d+/g).reduce((a, v) => a * 256 + +v, 0), 0x5b6fc0, dawn));
    gr.addColorStop(0.7, mixHex(0x3b2f6b, 0xf2a37a, dawn)); gr.addColorStop(1, mixHex(0x4a3a6b, 0xffcf8a, dawn));
    g.fillStyle = gr; g.fillRect(0, 0, W, H);
    g.globalAlpha = 1 - dawn; for (const [x, y, r] of this.stars) { if (Math.sin(t * 2 + x) > -0.3) K.circ(x, y, r * 0.5, '#fff6d8'); } g.globalAlpha = 1;
    if (dawn < 1) { g.globalAlpha = 1 - dawn; K.circ(268, 40, 14, '#fff1c4'); K.circ(274, 36, 12, mixHex(0x141a3d, 0x2a1440, storm)); g.globalAlpha = 1; }
    if (dawn > 0) { K.circ(30, 200 - dawn * 40, 26, 'rgba(255,240,200,0.7)'); K.gear(30, 200 - dawn * 40, 22, 12, t * 0.2, '#ffe7a3', null); K.circ(30, 200 - dawn * 40, 17, '#fff1c4'); }
    // far junk hills
    g.fillStyle = mixHex(0x2b2140, 0x8a5f96, dawn); g.beginPath(); g.moveTo(0, H);
    for (let x = 0; x <= W; x += 20) g.lineTo(x, 168 + Math.sin(x * 0.03) * 10 + Math.sin(x * 0.07) * 5); g.lineTo(W, H); g.fill();
    // ground
    g.fillStyle = mixHex(0x3a2630, 0x8a5233, dawn); g.fillRect(0, 196, W, 44); g.fillStyle = mixHex(0x5a3a40, 0xc98b55, dawn); g.fillRect(0, 196, W, 4);
    // home garage
    const wall = mixHex(0x4c3a63, 0xb07a9a, dawn);
    K.rr(66, 128, 108, 68, 3, wall); g.fillStyle = mixHex(0x2e2244, 0x7d4a6a, dawn); g.beginPath(); g.moveTo(58, 132); g.lineTo(120, 98); g.lineTo(182, 132); g.fill();
    K.gear(120, 118, 7, 8, 0, '#ffd23f', mixHex(0x2e2244, 0x7d4a6a, dawn));
    K.rr(88, 146, 64, 50, 2, '#ffcf7a'); g.fillStyle = 'rgba(255,190,90,0.25)'; g.beginPath(); g.moveTo(88, 196); g.lineTo(152, 196); g.lineTo(172, 206); g.lineTo(68, 206); g.fill();
    K.rr(94, 190, 52, 5, 2, '#3a8de0'); K.rr(96, 190.5, 48, 1.5, 1, '#8fd3ff');
    // baby bots: asleep, then startled, then blown away
    const babies = [[104, 0xFF8FC7, [-30, -40]], [116, 0x49F08A, [360, -20]], [128, 0xFFB347, [60, -60]], [140, 0x8FD3FF, [300, -70]]];
    babies.forEach(([bx, hex, [tx, ty]], k) => {
      const fly = clamp((t - 8 - k * 0.15) / 2.2, 0, 1), shake = t > 5 && t < 8 ? Math.sin(t * 40 + k) * 1.2 : 0;
      if (fly >= 1) return;
      const e = fly * fly, x = bx + (tx - bx) * e + shake, y = 190 + (ty - 190) * e - Math.sin(fly * P) * 30;
      g.save(); g.translate(x, y); g.rotate(fly * 8 * (k % 2 ? 1 : -1)); const sc = 0.05 * (1 - fly * 0.5);
      K.baby(0, 0, sc, hex, { t, eyes: t < 4.8 ? { open: 0.08 } : { open: 1, happy: 0 }, mouth: t < 4.8 ? 'smile' : 'o', armL: t > 4.8 ? 150 : 30, armR: t > 4.8 ? -150 : -30 });
      g.restore();
    });
    if (t < 4.6) for (let k = 0; k < 3; k++) { const z = (t * 0.6 + k / 3) % 1; g.globalAlpha = 1 - z; text('z', 122 + z * 14 + k * 4, 176 - z * 26, 6 + z * 6, '#d9d2e6', 'center', null); } g.globalAlpha = 1;
    // heroes: dozing beside the garage, then up and ready
    const up = clamp((t - 11.4) / 0.5, 0, 1), hop = t > 11.4 && t < 12.6 ? Math.sin((t - 11.4) / 1.2 * P) * 22 : 0;
    const sleepy = t < 11.4, eyes = sleepy ? { open: t > 5 && t < 10.5 ? 0.45 : 0.06 } : { happy: 0.2 };
    K.robot(210, 196 - hop, 0.13, party[0].hero === 'robot' ? COLS[party[0].col] : 0x5AD1FF, { t, bob: sleepy ? 6 : 0, eyes, armR: up ? -160 + Math.sin(t * 8) * 10 : -20, armL: up ? 40 : 20, mouth: sleepy ? 'wavy' : 'smile', jet: t > 12 && t < 12.6 });
    K.ostrich(266, 196 - hop * 0.8, 0.11, 0xFFD23F, { t, eyes, bob: sleepy ? 14 : 0, flap: up ? Math.abs(Math.sin(t * 12)) : 0, beakOpen: sleepy ? 0 : 0.4 });
    if (t < 11 && t > 1) { for (let k = 0; k < 2; k++) { const z = (t * 0.5 + k / 2) % 1; g.globalAlpha = (1 - z) * (t < 4.6 || t > 10.5 ? 1 : 0); text('z', 226 + k * 46 + z * 10, 150 - z * 20, 6 + z * 5, '#d9d2e6', 'center', null); } g.globalAlpha = 1; }
    // the Big Glitch storm cloud
    if (storm > 0) {
      const cx = 380 - storm * 170 + Math.sin(t * 1.3) * 6, cy = 44 + Math.sin(t * 2) * 3;
      g.globalAlpha = Math.min(1, storm * 1.5);
      for (const [dx, dy, r] of [[-46, 6, 22], [-20, -8, 28], [14, -12, 30], [44, 2, 24], [0, 14, 26], [-34, 18, 18], [32, 18, 20]]) K.circ(cx + dx, cy + dy, r, '#5b3f86');
      for (const [dx, dy, r] of [[-24, -14, 16], [16, -18, 18]]) K.circ(cx + dx, cy + dy, r, '#7a58a8');
      K.eye(cx - 14, cy + 2, 14, 16, '#1b1426', '#5b3f86', { angry: 1 }); K.eye(cx + 14, cy + 2, 14, 16, '#1b1426', '#5b3f86', { angry: -1 });
      K.mouth('zig', cx, cy + 16, 10, 2, '#1b1426');
      if (t > 7.6 && t < 10.5) for (let k = 0; k < 8; k++) { const y = 60 + k * 16, o = (t * 300 + k * 70) % 380; g.strokeStyle = 'rgba(220,210,255,0.5)'; g.lineWidth = 1; g.beginPath(); g.moveTo(340 - o, y); g.lineTo(310 - o, y + 2); g.stroke(); }
      g.globalAlpha = 1;
      if (this.bolt) { g.strokeStyle = '#ffe66b'; g.lineWidth = 2.5; g.beginPath(); let x = this.bolt.x, y = cy + 20; g.moveTo(x, y); while (y < 196) { y += 14; x += (Math.random() - 0.5) * 18; g.lineTo(x, y); } g.stroke(); }
    }
    if (this.flash > 0) { g.fillStyle = 'rgba(255,255,255,' + (this.flash * 0.6) + ')'; g.fillRect(0, 0, W, H); }
    // captions
    const cap = t < 4.5 ? 'One night in Junkyard Hills...' : t < 8 ? '...a Big Glitch storm rolled in!' : t < 11 ? 'Whoosh! The baby bots blew far away!' : 'Time to bring them home!';
    const ct = t < 4.5 ? t : t < 8 ? t - 4.5 : t < 11 ? t - 8 : t - 11;
    g.globalAlpha = clamp(ct * 2, 0, 1); panel(30, 210, 260, 22); text(cap, W / 2, 225, 10, '#fff6d8', 'center', null); g.globalAlpha = 1;
    if (t > 14.4) { g.fillStyle = 'rgba(255,248,230,' + clamp((t - 14.4) / 0.8, 0, 1) + ')'; g.fillRect(0, 0, W, H); }
    if (Math.floor(t * 2) % 2 && t > 1) text('START to skip', W - 8, 14, 7, '#d9d2e6', 'right', null);
  },
};

// ----------------------------------------------------------------------------- PLAYER SELECT
const selectScene = {
  song: 'select', t: 0, ready: [false, false],
  enter() { this.ready = [false, false]; },
  update() {
    this.t += 1 / 60;
    for (let i = 0; i < 2; i++) {
      const m = party[i];
      if (!m.joined) { if (hit(i, 'a') || hit(i, 'start')) { m.joined = true; SND.fx.confirm(); } continue; }
      if (this.ready[i]) { if (hit(i, 'b')) { this.ready[i] = false; SND.fx.select(); } continue; }
      if (hit(i, 'left') || hit(i, 'right')) { m.hero = m.hero === 'robot' ? 'ostrich' : 'robot'; SND.fx.select(); }
      if (hit(i, 'up')) { m.col = (m.col + COLS.length - 1) % COLS.length; SND.fx.select(); }
      if (hit(i, 'down')) { m.col = (m.col + 1) % COLS.length; SND.fx.select(); }
      if (hit(i, 'a')) { this.ready[i] = true; SND.fx.confirm(); }
      if (hit(i, 'b') && i === 1) { m.joined = false; SND.fx.select(); }
    }
    const joined = party.filter(m => m.joined).length;
    const allReady = party.every((m, i) => !m.joined || this.ready[i]);
    if ((allReady && joined) || anyHit('start')) { store.set('party', party); SND.fx.open(); go(mapScene); }
  },
  draw() {
    const t = this.t;
    const gr = g.createLinearGradient(0, 0, 0, H); gr.addColorStop(0, '#2b2350'); gr.addColorStop(1, '#5b3f86'); g.fillStyle = gr; g.fillRect(0, 0, W, H);
    K.g = g; for (let i = 0; i < 6; i++) { g.globalAlpha = 0.08; K.gear(30 + i * 60, 210 + (i % 2) * 20, 30, 12, t * 0.2 * (i % 2 ? 1 : -1), '#ffffff', null); } g.globalAlpha = 1;
    text("WHO'S PLAYING?", W / 2, 26, 16);
    for (let i = 0; i < 2; i++) {
      const m = party[i], x = 18 + i * 152, y = 38, w = 132, h = 158;
      panel(x, y, w, h, m.joined ? 'rgba(27,20,38,0.7)' : 'rgba(27,20,38,0.4)');
      text('PLAYER ' + (i + 1), x + w / 2, y + 16, 10, i ? '#8fd3ff' : '#ffd23f');
      if (!m.joined) {
        text('Press A', x + w / 2, y + 74, 12); text('to join', x + w / 2, y + 90, 12);
        text(i ? 'keyboard G or controller 2' : '', x + w / 2, y + 112, 7, '#a49db4', 'center', null);
        continue;
      }
      // pedestal and hero
      K.rr(x + 30, y + 118, w - 60, 8, 4, 'rgba(255,255,255,0.12)');
      const hex = COLS[m.col], ready = this.ready[i];
      if (m.hero === 'robot') K.robot(x + w / 2, y + 120, 0.2, hex, { t, bob: Math.sin(t * 3) * 3, armR: ready ? -150 + Math.sin(t * 8) * 20 : -22, eyes: { happy: ready ? 0.7 : 0.45 } });
      else K.ostrich(x + w / 2, y + 120, 0.19, hex, { t, bob: Math.abs(Math.sin(t * 2.6)) * 8, eyes: { happy: ready ? 0.7 : 0.4 } });
      text(m.hero === 'robot' ? 'ROBOT' : 'ROBOT OSTRICH', x + w / 2, y + 138, 10);
      text('◀ ▶ hero   ▲ ▼ ' + CNAMES[m.col].toLowerCase(), x + w / 2, y + 151, 7, '#d9d2e6', 'center', null);
      if (ready) { K.rr(x + 30, y + 22, w - 60, 16, 5, '#49d17a'); text('READY!', x + w / 2, y + 34, 10, '#fff', 'center', '#1f6b3c'); }
    }
    const blink = Math.floor(t * 2) % 2;
    if (blink) text('A when ready · START to go', W / 2, 220, 9);
  },
};

// ----------------------------------------------------------------------------- MAP
let MAP = null;
function buildMap() {
  WORLD.start = { beaten: prog.beaten.slice(), at: prog.at || WORLD.start.at, gears: prog.gears || 0, bolts: prog.bolts || 0 };
  MAP = OW.core(WORLD, g, { party: activeParty() });
  MAP.init();
}
const mapScene = {
  song: 'map', t: 0,
  enter() { if (this.keep) this.keep = false; else buildMap(); },
  update() {
    this.t += 1 / 60;
    for (let i = 0; i < 2; i++) {
      for (const [b, k] of [['left', 'ArrowLeft'], ['right', 'ArrowRight'], ['up', 'ArrowUp'], ['down', 'ArrowDown']]) if (hit(i, b)) { if (MAP.step(k)) SND.fx.step(); }
      if (hit(i, 'a') || hit(i, 'start')) {
        const n = MAP.here(); if (!n) continue;
        prog.at = n.id; saveProg();
        if ((n.kind === 'level' || n.kind === 'boss' || n.kind === 'bonus') && LEVELS[n.id]) { SND.fx.confirm(); go(levelScene(n.id)); }
        else if (n.kind === 'shop') { SND.fx.confirm(); go(shopScene); }
        else if (n.kind === 'exit') {
          if (WORLD_FILES[worldNum + 1] || worldNum >= 4) { SND.fx.confirm(); go(cutScene(worldNum)); }
          else { SND.fx.nope(); MAP.say('World ' + (worldNum + 1) + ' is coming soon!'); }
        }
        else if (n.kind === 'home' && worldNum > 1) { SND.fx.confirm(); enterWorld(worldNum - 1, 'exit'); }
        else { MAP.say('Home sweet home'); }
      }
    }
    if (!MAP.walking()) { const n = MAP.here(); if (n && prog.at !== n.id) { prog.at = n.id; saveProg(); } }
  },
  draw() {
    K.g = g; MAP.frame(this.t); livesHud(4, H - 19, this.t);
    // three stars under every level panel: finished, got everything, star time
    const stars = prog.stars || {};
    for (const n of WORLD.nodes) if (n.kind === 'level' && MAP.st.beaten.has(n.id)) {
      const got = stars[n.id] || [true, false, false];
      for (let k = 0; k < 3; k++) { const sx = n.x - 9 + k * 9, sy = n.y + 14; K.star(sx, sy, 4, '#2b2230'); K.star(sx, sy, 3.1, got[k] ? '#ffd23f' : 'rgba(255,255,255,0.35)'); }
    }
    const n = !MAP.walking() && MAP.here();
    if (n && (n.kind === 'level' || n.kind === 'boss' || n.kind === 'bonus') && LEVELS[n.id]) {
      const best = (prog.best || {})[n.id], d = LEVELS[n.id], bits = [];
      if (d.time) bits.push('RACE ' + clock(d.time * 60));
      if (best != null) bits.push('BEST ' + clock(best));
      if (d.par && n.kind === 'level') bits.push('STAR TIME ' + clock(d.par * 60));
      if (bits.length) { const s = bits.join('   '), w = s.length * 5 + 16; panel(6, 24, w, 14); text(s, 6 + w / 2, 34, 7, '#fff6d8', 'center', null); }
    }
  },
};
function finishNode(id, res) {
  // back on the map, standing on the node we played, record and pave
  prog.at = id; buildMap();
  const before = MAP.st.beaten.has(id);
  MAP.beat(res);
  prog.beaten = [...MAP.st.beaten]; prog.gears = MAP.st.gears; prog.bolts = MAP.st.bolts; prog.at = id; saveProg();
  if (!before) SND.fx.open();
}

// ----------------------------------------------------------------------------- SHOP (placeholder)
const shopScene = {
  song: 'map', t: 0,
  update() { this.t += 1 / 60; if (anyHit('a') || anyHit('b') || anyHit('start')) { SND.fx.select(); go(mapScene); } },
  draw() {
    K.g = g; MAP.frame(this.t);
    g.fillStyle = 'rgba(15,10,25,0.6)'; g.fillRect(0, 0, W, H);
    panel(40, 50, 240, 140, '#3b2a5c');
    text('PARTS SHOP', W / 2, 78, 16, '#ffd23f');
    for (let i = 0; i < 3; i++) { K.rr(64 + i * 68, 92, 56, 50, 6, 'rgba(255,255,255,0.08)'); }
    // hats on the shelf: a party hat, a propeller cap, a light-up antenna
    g.fillStyle = '#ff8fc7'; g.beginPath(); g.moveTo(80, 132); g.lineTo(92, 102); g.lineTo(104, 132); g.fill(); K.circ(92, 101, 3, '#ffd23f');
    K.rr(146, 120, 40, 12, 6, '#5ad1ff'); g.fillStyle = '#3a3346'; g.fillRect(165, 108, 2, 12); K.rr(152, 104 + Math.sin(this.t * 20) * 0.5, 28, 4, 2, '#ffd23f');
    g.fillStyle = '#9aa1b0'; g.fillRect(235, 108, 3, 24); K.circ(236.5, 106, 6, ['#ff7a59', '#49f08a', '#5ad1ff'][Math.floor(this.t * 3) % 3]);
    text('Hats and antennas open soon!', W / 2, 164, 9, '#fff6d8', 'center', null);
    text('Press A to go back', W / 2, 180, 8, '#a49db4', 'center', null);
  },
};

// ----------------------------------------------------------------------------- FILMS BETWEEN WORLDS
// 1: into the Gear Factory, 2: up the mine lift, 3: the rocket to the Moon, 4: the parade home.
// The N64 plays the same films (src/core/ck_cut.c follows these times), A or START skips.
const CUT_LEN = [0, 12.6, 12.6, 12.6, 16.5];
const CUT_CAPS = [[],
  [[0, 'Junkyard Hills is fixed!'], [4, 'More baby bots blew into the Gear Factory...'], [8, "Let's go and find them!"]],
  [[0, 'The Gear Factory is humming again!'], [4, 'Next stop: Magnet Mountain!'], [8, 'Going up, up, up!']],
  [[0, 'Magnet Mountain is fixed!'], [4.5, '3...'], [5.5, '2...'], [6.5, '1...'], [7.5, 'Blast off to the Moon!']],
  [[0, 'Every baby bot is safe!'], [5, 'The Big Glitch is fixed for good!'], [10, 'Home together!  THE END']]];
const smooth = u => { u = clamp(u, 0, 1); return u * u * (3 - 2 * u); };
// Where everyone is at time t: [{ x, y, a (alpha), walk, baby, hex, hero }]
function cutCast(k, t, np, babies) {
  const out = [], G = CUTART.GROUND;
  const add = (x, y, a, walk, i, isBaby) => out.push({ x, y, a, walk, i, baby: isBaby });
  if (k === 1) {
    const lx = t < 0.8 ? -20 : t < 7.5 ? -20 + 30 * (t - 0.8) : 181 + 34 * (t - 7.5), walk = t > 0.8 && t < 10.6;
    const fade = x => clamp(1 - (x - 244) / 24, 0, 1);
    for (let i = 0; i < np; i++) { const x = lx - i * 18; add(x, G, fade(x), walk, i, false); }
    babies.forEach((h, j) => { const x = lx - np * 18 - 6 - j * 14; add(x, G, fade(x), walk, j, true); });
  } else if (k === 2) {
    const lx = t < 0.5 ? -20 : -20 + 90 * (t - 0.5), cy = t < 3.5 ? G : G - (G - CUTART.SHAFT.top) * smooth((t - 3.5) / 6);
    const out2 = t < 9.8 ? 0 : Math.min(102, 60 * (t - 9.8));
    for (let i = 0; i < np; i++) { const x = Math.min(lx - i * 16, 176 - i * 14); add(x + out2, cy, 1, (x < 176 - i * 14 && t > 0.5) || (out2 > 0 && out2 < 102), i, false); }
    babies.forEach((h, j) => { const x = Math.min(lx - np * 16 - 4 - j * 12, 152 - j * 7); add(x + out2, cy, 1, x < 152 - j * 7 && t > 0.5, j, true); });
  } else if (k === 3) {
    const lx = t < 0.5 ? -20 : -20 + 60 * (t - 0.5), X = CUTART.PAD.x, fade = x => clamp((X - x) / 14, 0, 1);
    for (let i = 0; i < np; i++) { const x = Math.min(lx - i * 16, X); add(x, G, fade(x), x < X, i, false); }
    babies.forEach((h, j) => { const x = Math.min(lx - np * 16 - 6 - j * 12, X); add(x, G, fade(x), x < X, j, true); });
  } else {
    const fx = cutFloatX(t), top = 180;
    for (let i = 0; i < np; i++) add(fx - 12 * (np - 1) + i * 24, top, 1, false, i, false);
    babies.forEach((h, j) => add(fx - 60 + j * 8, top, 1, false, j, true));
  }
  return out;
}
const cutFloatX = t => 200 + 220 * Math.pow(1 - clamp(t / 5, 0, 1), 2);
const cutCageY = t => t < 3.5 ? CUTART.GROUND : CUTART.GROUND - (CUTART.GROUND - CUTART.SHAFT.top) * smooth((t - 3.5) / 6);
const cutRocketY = t => t < 7.5 ? CUTART.GROUND : CUTART.GROUND - 45 * (t - 7.5) * (t - 7.5);
const cutGateOpen = t => clamp((t - 5.5) / 1.5, 0, 1) - clamp((t - 10.4) / 1, 0, 1);
function cutBabies(k) {
  const list = n => ((WORLD_FILES[n] && WORLD_FILES[n].meta.babies) || 'FF8FC7 49F08A FFB347 8FD3FF').split(/\s+/).map(h => parseInt(h, 16));
  return k < 4 ? list(k) : [1, 2, 3, 4].flatMap(list);
}
function cutScene(k) {
  const len = CUT_LEN[k], babies = cutBabies(k), team = activeParty();
  const bg = document.createElement('canvas'); bg.width = W; bg.height = H;
  { const b = bg.getContext('2d'), old = K.g; K.g = b; CUTART.bg(b, k); K.g = old; }
  const S = { song: k === 4 ? 'parade' : 'travel', t: 0 };
  const at = (was, when) => was < when && S.t >= when;
  function finish() { if (k < 4) enterWorld(k + 1, 'home'); else { titleScene.t = 0; go(titleScene); } }
  S.update = function () {
    const was = S.t; S.t += 1 / 60; const t = S.t;
    if (k === 1 && (at(was, 5.5) || at(was, 10.4))) SND.fx.door();
    if (k === 2 && (at(was, 3.5) || at(was, 9.5))) SND.fx.clank();
    if (k === 3) {
      for (const w of [4.5, 5.5, 6.5]) if (at(was, w)) SND.fx.tick();
      if (at(was, 7.5)) { SND.fx.boost(); SND.fx.whoosh(); }
      if (t > 6 && t < 9 && Math.floor(t * 60) % 4 === 0) puff(CUTART.PAD.x + (Math.random() - 0.5) * 30, CUTART.GROUND - 4, 2, 'rgba(240,240,250,0.9)');
    }
    if (k === 4 && Math.floor(t * 60) % 24 === 0) confetti(160, 0, 10);
    stepParticles();
    if (t >= len || (t > 1 && (anyHit('start') || anyHit('a')))) finish();
  };
  S.draw = function () {
    const t = S.t; K.g = g;
    g.drawImage(bg, 0, 0);
    const cast = cutCast(k, t, team.length, babies);
    if (k === 1) {   // the roller door rises into the wall
      const G = CUTART.GATE, u = cutGateOpen(t);
      g.save(); g.beginPath(); g.rect(G.x, G.y, G.w, G.h); g.clip(); g.translate(G.x, G.y - u * G.h); CUTART.gate(g); g.restore();
    }
    if (k === 2) { g.save(); g.translate(CUTART.SHAFT.x, cutCageY(t)); CUTART.cage(g, false); g.restore(); }
    if (k === 3) {
      const ry = cutRocketY(t), sh = t > 4.5 && t < 7.5 ? Math.sin(t * 50) : 0;
      if (t > 6.8) { g.save(); g.translate(CUTART.PAD.x + sh, ry); if (t < 7.5) g.scale(0.55, 0.55); CUTART.flame(g, Math.floor(t * 14) % 2); g.restore(); }
      g.save(); g.translate(CUTART.PAD.x + sh, ry); CUTART.rocket(g); g.restore();
    }
    if (k === 4) { g.save(); g.translate(cutFloatX(t), 210); CUTART.float(g); g.restore(); }
    // babies behind, heroes in front
    for (const c of cast) {
      if (!c.baby || c.a <= 0) continue;
      g.globalAlpha = c.a; const hop = c.walk || k === 4 ? Math.abs(Math.sin(t * (k === 4 ? 8 : 9) + c.i)) * (k === 4 ? 6 : 4) : 0;
      K.baby(c.x, c.y - hop, 0.05, babies[c.i], { t: t + c.i, armL: k === 4 ? 150 : 30, armR: k === 4 ? -150 : -150 + Math.sin(t * 6 + c.i) * 20, mouth: k === 4 ? 'o' : 'smile', eyes: { happy: 0.5 } });
    }
    for (const c of cast) {
      if (c.baby || c.a <= 0) continue;
      g.globalAlpha = c.a; const m = team[c.i], tt = t + c.i * 0.7;
      if (c.walk) { if (m.hero === 'robot') K.robotSide(c.x, c.y, 0.074, m.hex, Object.assign(K.pose('robot', 'run', tt * 2.2), { dir: 1 })); else K.ostrichSide(c.x, c.y, 0.071, m.hex, Object.assign(K.pose('ostrich', 'run', tt * 2), { dir: 1 })); }
      else if (k === 4 || (k === 2 && t > 9.5)) { if (m.hero === 'robot') K.robot(c.x, c.y, 0.074, m.hex, { t: tt, armL: 150 + Math.sin(tt * 9) * 20, armR: -150 - Math.sin(tt * 9) * 20, eyes: { happy: 0.8 }, mouth: 'o' }); else K.ostrich(c.x, c.y, 0.071, m.hex, { t: tt, bob: Math.abs(Math.sin(tt * 6)) * 14, eyes: { happy: 0.8 }, beakOpen: 0.8 }); }
      else { if (m.hero === 'robot') K.robot(c.x, c.y, 0.074, m.hex, { t: tt, bob: Math.sin(tt * 3) * 3 }); else K.ostrich(c.x, c.y, 0.071, m.hex, { t: tt, bob: Math.abs(Math.sin(tt * 2.6)) * 8 }); }
    }
    g.globalAlpha = 1;
    if (k === 2) { g.save(); g.translate(CUTART.SHAFT.x, cutCageY(t)); CUTART.cage(g, true); g.restore(); }
    drawParticles(0);
    // the caption strip
    const caps = CUT_CAPS[k]; let ci = 0; for (let i = 0; i < caps.length; i++) if (t >= caps[i][0]) ci = i;
    const ct = t - caps[ci][0];
    g.globalAlpha = clamp(ct * 3, 0, 1); panel(20, 210, 280, 22); text(caps[ci][1], W / 2, 225, 10, k === 4 && ci === 2 ? '#ffd23f' : '#fff6d8', 'center', null); g.globalAlpha = 1;
    if (t < 0.6) { g.fillStyle = 'rgba(0,0,0,' + (1 - t / 0.6) + ')'; g.fillRect(0, 0, W, H); }
    if (t > len - 0.8) { g.fillStyle = 'rgba(255,248,230,' + clamp((t - (len - 0.8)) / 0.8, 0, 1) + ')'; g.fillRect(0, 0, W, H); }
    if (t > 1 && Math.floor(t * 2) % 2) text('START to skip', W - 8, 14, 7, '#d9d2e6', 'right', null);
  };
  particles = [];
  return S;
}

// ---- power-ups: mega and bubble are kept (lost on a bump); rainbow and magnet run on a timer
const POWER = { M: 'mega', R: 'rainbow', B: 'bubble', N: 'magnet' };
const POWER_NAME = { mega: 'MEGA BATTERY!', rainbow: 'RAINBOW OVERDRIVE!', bubble: 'BUBBLE BLASTER!', magnet: 'MAGNET HELMET!' };
const hexStr = h => '#' + h.toString(16).padStart(6, '0');
const has = (p, k) => !!(p.stack && p.stack.includes(k));
const baseH = p => Math.round((p.hero === 'robot' ? 26 : 30) * (has(p, 'mega') ? 1.5 : 1));
let testPower = null;   // the level editor can start a play-test with a power-up
function powerIcon(kind, x, y, t) {
  if (kind === 'mega') { K.rr(x - 3.5, y - 4.5, 7, 10, 1.5, '#2b2833'); K.rr(x - 1.5, y - 6, 3, 2, 0.5, '#2b2833'); K.rr(x - 2.2, y - 3.2, 4.4, 7.4, 1, '#49f08a');
    g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(x + 0.8, y - 3); g.lineTo(x - 1.6, y + 0.8); g.lineTo(x, y + 0.8); g.lineTo(x - 0.8, y + 3.6); g.lineTo(x + 1.8, y - 0.4); g.lineTo(x + 0.2, y - 0.4); g.fill(); }
  else if (kind === 'rainbow') K.star(x, y, 5.5, hexStr(COLS[Math.floor(t * 10) % COLS.length]));
  else if (kind === 'bubble') { g.globalAlpha = 0.35; K.circ(x, y, 4.5, '#bff0ff'); g.globalAlpha = 1; g.strokeStyle = '#8fd3ff'; g.lineWidth = 1.2; g.beginPath(); g.arc(x, y, 4.5, 0, P * 2); g.stroke(); K.circ(x - 1.6, y - 1.6, 1.2, '#fff'); K.circ(x + 4.5, y - 4, 1.3, '#bff0ff'); }
  else if (kind === 'magnet') { g.strokeStyle = '#e04f4f'; g.lineWidth = 3; g.beginPath(); g.arc(x, y - 0.5, 3.5, 0, P); g.stroke(); g.fillStyle = '#e8e4f0'; g.fillRect(x - 5, y - 3.5, 3, 3); g.fillRect(x + 2, y - 3.5, 3, 3); }
}
// lives are robot batteries
function battery(x, y, full, t) {
  K.rr(x, y, 11, 7, 1.5, full ? '#2b2833' : 'rgba(43,40,51,0.6)'); K.rr(x + 11, y + 2, 2, 3, 0.5, full ? '#2b2833' : 'rgba(43,40,51,0.6)');
  if (full) { K.rr(x + 1.5, y + 1.5, 8, 4, 1, '#49f08a'); g.fillStyle = '#fff'; g.globalAlpha = 0.5; g.fillRect(x + 2, y + 2, 7, 1); g.globalAlpha = 1; }
  else { g.strokeStyle = '#ff6b6b'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 3, y + 1.5); g.lineTo(x + 8, y + 5.5); g.stroke(); }
}
const clock = f => { const sec = Math.floor(f / 60); return Math.floor(sec / 60) + ':' + String(sec % 60).padStart(2, '0'); };
function livesHud(x, y, t) { panel(x, y, 48, 15); const n = prog.lives ?? 3; for (let k = 0; k < 3; k++) battery(x + 3 + k * 15, y + 4, k < n, t); }

// ----------------------------------------------------------------------------- WORLDS AND LEVELS
// Each world is a folder of text files in roms/clankers/data (embedded at build time as DATA):
// w1/world.txt and its levels, w2/world.txt... The map, levels and look all follow the world you are in.
const WORLD_FILES = {};
for (let n = 1; DATA['w' + n + '/world.txt']; n++) WORLD_FILES[n] = DATA_FMT.parseWorld(DATA['w' + n + '/world.txt']);
const LAST_WORLD = Object.keys(WORLD_FILES).length;
let WORLD_FILE = null, WORLD = null;
function levelDef(text, theme) {
  const { meta, rows, w } = DATA_FMT.parseLevel(text);
  return { name: (meta.name || 'LEVEL').toUpperCase(), w, rows, song: meta.song, boss: meta.kind === 'boss', bonus: meta.kind === 'bonus', baby: parseInt(meta.baby || 'FF8FC7', 16), zone: meta.hatch, par: +meta.par || 0, time: +meta.time || 0,
    bossType: meta.boss || 'crane', gravity: meta.gravity === 'low' ? 0.2 : 0.3, theme: meta.theme || theme || 'junkyard' };
}
const LEVELS = {};
function setWorld(n) {
  worldNum = clamp(Math.round(+n) || 1, 1, LAST_WORLD); store.set('world', worldNum);
  prog = Object.assign(fresh(), store.get('w' + worldNum, {}));
  WORLD_FILE = WORLD_FILES[worldNum]; WORLD = DATA_FMT.worldForMap(WORLD_FILE);
  mapScene.song = shopScene.song = WORLD_FILE.meta.song || 'map';
  for (const k of Object.keys(LEVELS)) delete LEVELS[k];
  const dir = 'w' + worldNum + '/';
  for (const n of WORLD_FILE.nodes) if (n.level && DATA[dir + n.level + '.txt']) LEVELS[n.id] = levelDef(DATA[dir + n.level + '.txt'], WORLD_FILE.meta.theme);
}
setWorld(store.get('world', 1));
// Walk through a world's gate: World n+1 from the exit (after its film), or back to World n-1 from home.
function enterWorld(n, at) {
  setWorld(n);
  const node = WORLD_FILE.nodes.find(q => q.kind === at) || WORLD_FILE.nodes[0];
  prog.at = node.id; saveProg();
  buildMap(); MAP.say(WORLD.name.toUpperCase() + '!'); mapScene.keep = true; go(mapScene);
}
const CRATE = { 'X': '', '?': 'G', '$': 'o', '1': 'M', '2': 'R', '3': 'B', '4': 'N' };
function buildFromRows(L, def) {
  const rows = def.rows; let out = null;
  rows.forEach((r, y) => r.forEach((c, x) => { if (c === 'h') out = [x, y]; }));
  rows.forEach((r, y) => r.forEach((c, x) => {
    if (c === '#') L.ground(x, x, y, true); else if (c === '=') L.girder(x, x, y); else if (c === '-') L.plank(x, x, y); else if (c === 'D') L.door(x, y, y);
    else if (c === '<' || c === '>') L.belt(x, y, c);
    else if (c in CRATE) L.crate(x, y, CRATE[c]);
    else if (c === 'o') L.bolts(x, x, y); else if (c === 'G') L.gear(x, y);
    else if (c === 'b') L.enemy('bumble', x, y + 1); else if (c === 's') L.enemy('sprocket', x, y + 1); else if (c === 'g') L.enemy('grumble', x, y + 1);
    else if (c === 'T') L.tyre(x, y + 1); else if (c === 'L') L.lever(x, y + 1); else if (c === 'v') L.sign(x, y + 1);
    else if (c === 'P') L.pad(x, y + 1); else if (c === '@') L.start(x, y + 1); else if (c === 'F') L.flag(x, y + 1);
    else if (c === 'H') { if (out) L.hatch(x, y + 1, def.zone || 'jet1', out[0], out[1] + 1); }
    else if (c === 'k') L.hook(x, y);
    else if (c === 'C') L.crusher(x, y);
    else if (c === 'r') {   // follow the track of ~ (sideways) or | (up and down) that starts next to it
      let dx = 0, dy = 0, n = 0;
      if (rows[y][x + 1] === '~') dx = 1; else if (rows[y][x - 1] === '~') dx = -1; else if (rows[y - 1] && rows[y - 1][x] === '|') dy = -1; else if (rows[y + 1] && rows[y + 1][x] === '|') dy = 1;
      if (dx || dy) while (true) { const r2 = rows[y + dy * (n + 1)]; const c2 = r2 && r2[x + dx * (n + 1)]; if (c2 === (dx ? '~' : '|')) n++; else break; }
      L.raft(x, y, x + dx * n, y + dy * n);
    }
  }));
}

function buildLevel(id, defIn) {
  const def = defIn || LEVELS[id], w = def.w, h = 15;
  const tiles = Array.from({ length: h }, () => Array(w).fill('.'));
  const ents = [], contents = {};
  let start = [2 * TS, 12 * TS], gearN = 0;
  const L = {
    start(x, top) { start = [x * TS + 2, top * TS]; },
    ground(x0, x1, top, one) { for (let x = x0; x <= x1; x++) for (let y = top; y < (one ? top + 1 : h); y++) tiles[y][x] = '#'; },
    girder(x0, x1, y) { for (let x = x0; x <= x1; x++) tiles[y][x] = '='; },
    plank(x0, x1, y) { for (let x = x0; x <= x1; x++) tiles[y][x] = '-'; },
    belt(x, y, c) { tiles[y][x] = c; },
    crate(x, y, c) { tiles[y][x] = 'X'; if (c) contents[x + ',' + y] = c; },
    door(x, y0, y1) { for (let y = y0; y <= y1; y++) tiles[y][x] = 'D'; },
    bolts(x0, x1, y) { for (let x = x0; x <= x1; x++) { const b = pickup('bolt', x * TS + 8, y * TS + 8); b.orig = true; ents.push(b); } },
    gear(x, y) { const e = pickup('gear', x * TS + 8, y * TS + 8); e.id = gearN++; ents.push(e); },
    enemy(kind, x, top) { ents.push(enemy(kind, x * TS + 2, top * TS)); },
    tyre(x, top) { ents.push({ type: 'tyre', x: x * TS, y: top * TS - 9, w: 16, h: 9, sq: 0 }); },
    lever(x, top) { ents.push({ type: 'lever', x: x * TS + 3, y: top * TS - 14, w: 10, h: 14, on: false, t: 0 }); },
    hatch(x, top, zone, outX, outTop) { ents.push({ type: 'hatch', x: x * TS - 4, y: top * TS - 4, w: 24, h: 4, zone, out: { x: outX * TS + 8, y: outTop * TS } }); ents.push({ type: 'hatchOut', x: outX * TS - 4, y: outTop * TS - 4, w: 24, h: 4 }); },
    raft(x0, y0, x1, y1) { ents.push({ type: 'mover', kind: 'raft', ax: x0 * TS, ay: y0 * TS, bx: x1 * TS, by: y1 * TS, x: x0 * TS, y: y0 * TS, w: 32, h: 6, dx: 0, dy: 0, period: Math.max(2, Math.hypot(x1 - x0, y1 - y0) / 1.1) * 2 }); },
    crusher(x, y) {
      let gy = y + 2; while (gy < h && !SOLID[tiles[gy][x]] && !(def.rows[gy] && '#=X'.includes(def.rows[gy][x]))) gy++;
      ents.push({ type: 'mover', kind: 'crusher', home: y * TS, low: gy * TS - 32, x: x * TS, y: y * TS, w: 32, h: 32, dx: 0, dy: 0, state: 'up', timer: 60 + x % 40, vy: 0 });
    },
    hook(x, y) { ents.push({ type: 'mover', kind: 'hook', px: x * TS + 8, py: y * TS + 4, len: 56, x: x * TS - 8, y: y * TS + 60, w: 32, h: 6, dx: 0, dy: 0 }); },
    sign(x, top) { ents.push({ type: 'sign', x: x * TS + 2, y: top * TS - 20, w: 12, h: 20 }); },
    flag(x, top) { ents.push({ type: 'flag', x: x * TS + 2, y: top * TS - 34, w: 12, h: 34, on: false }); },
    pad(x, top) { ents.push({ type: 'pad', x: x * TS - 4, y: top * TS - 6, w: 24, h: 6 }); },
  };
  buildFromRows(L, def);
  ents.forEach((e, i) => { e.uid = i; });
  const boltTotal = ents.filter(e => e.orig).length + 3 * Object.values(contents).filter(c => c === 'o').length;
  return { id, def, w, h, tiles, ents, contents, start, doorT: -1, bolts: 0, boltTotal, boltGot: 0, foeTotal: ents.filter(e => e.type === 'enemy').length, entCount: ents.length, got: (prog.got[id] || [false, false, false]).slice(), gotNow: [false, false, false] };
}
function pickup(kind, x, y) { return { type: kind, x: x - 5, y: y - 5, w: 10, h: 10, vx: 0, vy: 0, fly: false, delay: 0, t: Math.random() * 6 }; }
function enemy(kind, x, groundY) {
  const dims = { bumble: [14, 16], sprocket: [14, 18], grumble: [14, 14] }[kind];
  return { type: 'enemy', kind, x, y: groundY - dims[1], w: dims[0], h: dims[1], vx: kind === 'bumble' ? -0.45 : 0, vy: 0, face: -1, state: 'glitchy', t: Math.random() * 3, hopT: 60 + Math.random() * 60, happyT: 0 };
}

const SOLID = { '#': 1, '=': 1, 'X': 1, 'D': 1, '<': 1, '>': 1 };   // < and > are conveyor belts
function tileAt(lv, tx, ty) { if (tx < 0 || tx >= lv.w) return '#'; if (ty < 0 || ty >= lv.h) return '.'; return lv.tiles[ty][tx]; }
const isSolid = (lv, tx, ty) => !!SOLID[tileAt(lv, tx, ty)];

function moveBody(lv, b, oneWay = true) {
  // x
  b.x += b.vx;
  let x0 = Math.floor(b.x / TS), x1 = Math.floor((b.x + b.w - 0.01) / TS), y0 = Math.floor(b.y / TS), y1 = Math.floor((b.y + b.h - 0.01) / TS);
  b.hitWall = 0;
  if (b.vx > 0) { for (let ty = y0; ty <= y1; ty++) if (isSolid(lv, x1, ty)) { b.x = x1 * TS - b.w - 0.01; b.vx = 0; b.hitWall = 1; break; } }
  else if (b.vx < 0) { for (let ty = y0; ty <= y1; ty++) if (isSolid(lv, x0, ty)) { b.x = (x0 + 1) * TS + 0.01; b.vx = 0; b.hitWall = -1; break; } }
  // y
  const prevBottom = b.y + b.h;
  b.y += b.vy; b.ground = false; b.bumped = null;
  x0 = Math.floor(b.x / TS); x1 = Math.floor((b.x + b.w - 0.01) / TS);
  if (b.vy > 0) {
    const ty = Math.floor((b.y + b.h) / TS);
    for (let tx = x0; tx <= x1; tx++) {
      const t = tileAt(lv, tx, ty);
      if (SOLID[t] || (oneWay && t === '-' && prevBottom <= ty * TS + 0.5)) { b.y = ty * TS - b.h; b.vy = 0; b.ground = true; break; }
    }
  } else if (b.vy < 0) {
    const ty = Math.floor(b.y / TS);
    for (let tx = x0; tx <= x1; tx++) if (isSolid(lv, tx, ty)) { b.y = (ty + 1) * TS; b.vy = 0; b.bumped = [tx, ty]; break; }
  }
}
const overlap = (a, b) => a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;

function makePlayer(m, i, x, y) {
  const stack = testPower ? [testPower] : [], h = baseH({ hero: m.hero, stack });   // power-ups never carry over between levels
  return { slot: m.slot, i, hero: m.hero, hex: m.hex, stack, rainbowT: 0, magnetT: 0, x: x + i * 14, y: y - h, w: 12, h,
    vx: 0, vy: 0, face: 1, ground: false, coyote: 0, boost: false, jetT: 0, flapping: false, flapT: 0, actT: 0, inv: 0, wob: 0,
    safeX: x, safeY: y - 30, anim: 0, idleT: 0, cheer: false };
}

// Each world's tiles, drawn once into a strip of 16x16 frames (the same strip the N64 bakes).
const TILE_SHEETS = {};
function tileSheet(theme) {
  if (!LTHEMES[theme]) theme = 'junkyard';
  if (TILE_SHEETS[theme]) return TILE_SHEETS[theme];
  const cv = document.createElement('canvas'); cv.width = TS * LEVELART.TILE_FRAMES; cv.height = TS;
  const b = cv.getContext('2d'), old = K.g; K.g = b;
  for (let i = 0; i < LEVELART.TILE_FRAMES; i++) { b.save(); b.translate(i * TS, 0); b.beginPath(); b.rect(0, 0, TS, TS); b.clip(); LTHEMES[theme].tile(b, i); b.restore(); }
  K.g = old;
  return (TILE_SHEETS[theme] = cv);
}
// Every boss is fixed by bouncing on three red buttons that pop up one after another.
const BOSS_BUTTONS = { crane: [[3, 12], [4.5, 8], [14.5, 8]], stamper: [[16, 12], [4.5, 8], [14.5, 8]], boulder: [[9.5, 12], [4.5, 8], [14.5, 8]], hauler: [[3, 12], [14.5, 8], [4.5, 8]] };
function makeBoss(type) {
  if (!BOSS_BUTTONS[type]) type = 'crane';
  const b = { type, hits: 0, shake: 0, ouch: 0, fixed: false, padShown: false, buttons: BOSS_BUTTONS[type], ang: 0 };
  if (type === 'stamper') Object.assign(b, { x: 160, y: 24, vy: 0, low: 0, state: 'move', timer: 150 });
  if (type === 'boulder') Object.assign(b, { x: 280, y: 174, rot: 0, dir: -1, state: 'wait', timer: 90 });
  if (type === 'hauler') Object.assign(b, { x: 160, y: 46, ft: 0, state: 'fly', timer: 150 });
  return b;
}

function levelScene(id, retry, defIn, snap) {
  const lv = buildLevel(id, defIn), def = lv.def;
  if (snap) {   // a lost try after the checkpoint flag: carry on from there, with what was already done still done
    lv.tiles = snap.tiles.map(r => r.slice()); lv.bolts = snap.bolts; lv.boltGot = snap.boltGot; lv.gotNow = snap.gotNow.slice(); lv.doorT = snap.doorT;
    lv.ents = lv.ents.filter(e => !snap.gone.includes(e.uid)); for (const e of lv.ents) if (e.type === 'flag' && e.uid === snap.flag) e.on = true;
    lv.start = [snap.x, snap.y];
  }
  const players = activeParty().map((m, i) => makePlayer(m, i, lv.start[0], lv.start[1]));
  for (const p of players) p.inv = 90;   // a moment of safety at the start
  const S = { song: def.song || (def.boss ? 'boss' : def.bonus ? 'bonus' : 'level'), t: 0, frame: 0, camX: 0, clearT: -1, dyingT: -1, nameT: 150, lv, players, result: null, timeF: snap ? snap.timeF : 0, snap };
  if (prog.lives == null || prog.lives < 1) { prog.lives = 3; saveProg(); }
  particles = [];
  if (retry) S.nameT = 120;
  // boss state
  const boss = def.boss ? makeBoss(def.bossType) : null, crane = boss && boss.type === 'crane' ? boss : null;
  // bonus state
  const shower = def.bonus ? { left: 30 * 60, done: -1 } : null;
  const speed = () => (little ? 1 : 1.5);
  const grav = def.gravity || 0.3;
  function beltUnder(p) {
    const ty = Math.floor((p.y + p.h + 1) / TS), a = tileAt(lv, Math.floor((p.x + 1) / TS), ty), b = tileAt(lv, Math.floor((p.x + p.w - 1) / TS), ty);
    const dir = (a === '>' || b === '>') ? 1 : (a === '<' || b === '<') ? -1 : 0;
    return dir * (little ? 0.8 : 1);
  }

  function loseLife(p) {
    if (p.dead || p.bubble || S.dyingT >= 0 || S.clearT >= 0) return;
    prog.lives = Math.max(0, (prog.lives ?? 3) - 1); p.stack = []; p.rainbowT = 0; p.magnetT = 0; saveProg();
    const buddy = players.find(q => q !== p && !q.dead && !q.bubble);
    if (buddy && prog.lives > 0) {
      // co-op: float back to your buddy in a bubble, the level carries on
      p.bubble = 1; p.vx = 0; p.vy = 0; p.wob = 0; p.x = clamp(p.x, S.camX + 8, S.camX + W - 20); p.y = Math.min(p.y, H - 50); SND.fx.ouch(); return;
    }
    for (const q of players) if (!q.bubble) { q.dead = q === p; }
    S.nameT = 0; p.dead = true; p.vy = p.y > H ? -9 : -5.5; p.vx = 0; if (p.y > H) p.y = H; S.dyingT = 0; SND.stopSong(); SND.fx.ouch();
  }
  // race levels: the clock ran out, so everyone flops and it costs one try
  function outOfTime() {
    prog.lives = Math.max(0, (prog.lives ?? 3) - 1); saveProg();
    for (const q of players) { q.dead = true; q.bubble = 0; q.vy = -5.5; q.vx = 0; q.stack = []; }
    S.nameT = 0; S.dyingT = 0; S.timeUp = true; SND.stopSong(); SND.fx.ouch();
  }
  S.foesLeft = () => lv.ents.filter(e => e.type === 'enemy' && !e.dead && e.state !== 'happy').length;
  function setH(p, h) { p.y += p.h - h; p.h = h; }
  function canStand(p, h) {
    const top = p.y + p.h - h, x0 = Math.floor(p.x / TS), x1 = Math.floor((p.x + p.w - 0.01) / TS);
    for (let ty = Math.floor(top / TS); ty <= Math.floor((p.y - 0.01) / TS); ty++) for (let tx = x0; tx <= x1; tx++) if (isSolid(lv, tx, ty)) return false;
    return true;
  }
  function respawn(p) {
    p.x = p.safeX; p.y = p.safeY - 4; p.vx = 0; p.vy = 0; p.inv = 60; p.boost = false;
    puff(p.x + p.w / 2, p.y + p.h, 8); SND.fx.boing();
  }
  // power-ups stack: Mega and Bubble together, plus Rainbow and Magnet on their timers. A bump knocks off the newest.
  function setStack(p, stack) {
    if (p.low) { p.low = false; p.slideT = 0; }
    p.stack = stack; setH(p, baseH(p));
  }
  function wobble(p, fromX) {
    if (p.rainbowT > 0) return;
    if (p.stack.length) { setStack(p, p.stack.slice(0, -1)); p.inv = 110; p.wob = 14; p.vx = (p.x + p.w / 2 < fromX ? -2 : 2); p.vy = -2; SND.fx.shrink(); puff(p.x + p.w / 2, p.y + p.h / 2, 8); return; }
    if (lv.bolts <= 0) { loseLife(p); return; }   // no bolts to drop: that bump costs a life
    p.vx = (p.x + p.w / 2 < fromX ? -2.4 : 2.4); p.vy = -2.6; p.wob = 26; p.inv = 100; SND.fx.wobble();
    const drop = Math.min(3, lv.bolts); lv.bolts -= drop;
    for (let k = 0; k < drop; k++) { const b = pickup('bolt', p.x + p.w / 2, p.y + 6); b.fly = true; b.vx = (k - 1) * 1.2 + (Math.random() - 0.5); b.vy = -3 - Math.random(); b.delay = 40; lv.ents.push(b); }
  }
  function reboot(e, p) {
    if (e.state !== 'glitchy') return; e.state = 'happy'; e.happyT = 0; e.vx = 0; SND.fx.reboot(); sparks(e.x + e.w / 2, e.y + 4, 8, '#bff6ff');
    const b = pickup('bolt', e.x + e.w / 2, e.y); b.fly = true; b.vy = -3; b.vx = (Math.random() - 0.5) * 1.5; b.delay = 10; lv.ents.push(b);
  }
  // Grumble Crates snap shut when stomped. Touch one to kick it, or press B next to it to pick it up and B again to throw.
  function shutCrate(e) { e.state = 'shell'; e.shellT = 0; e.vx = 0; e.kickT = 12; e.bounces = 0; SND.fx.stomp(); sparks(e.x + e.w / 2, e.y + 2, 5, '#ffe2a8'); }
  function kickCrate(e, dir, by) { e.state = 'kicked'; e.dir = dir; e.spd = 3.2; e.vx = 3.2 * dir; e.kickT = 14; e.by = by; e.bounces = 0; SND.fx.kick(); puff(e.x + e.w / 2, e.y + e.h, 4); }
  function wakeCrate(e) { e.state = 'glitchy'; e.vx = 0; e.vy = -2.5; e.hopT = 60; SND.fx.wobble(); puff(e.x + e.w / 2, e.y + e.h / 2, 5); }
  function breakCrate(tx, ty) {
    if (tileAt(lv, tx, ty) !== 'X') return; lv.tiles[ty][tx] = '.'; debris(tx * TS, ty * TS); SND.fx.crate();
    const c = lv.contents[tx + ',' + ty];
    if (c === 'o') for (let k = 0; k < 3; k++) { const b = pickup('bolt', tx * TS + 8, ty * TS + 8); b.orig = true; b.fly = true; b.vx = (k - 1) * 1.1; b.vy = -3.2; b.delay = 12; lv.ents.push(b); }
    if (POWER[c]) { lv.ents.push({ type: 'power', kind: POWER[c], x: tx * TS + 2, y: ty * TS + 2, w: 12, h: 12, vx: 0, vy: -2.2, rise: 18, dir: 1, t: 0 }); SND.fx.sprout(); }
    if (c === 'G') { const gg = pickup('gear', tx * TS + 8, ty * TS + 8); gg.id = lv.ents.filter(e => e.type === 'gear').length + lv.gotNow.filter(Boolean).length; gg.fly = true; gg.vy = -3.5; gg.delay = 14;
      // crates hold the level's gear slot that is not otherwise placed
      const placed = new Set(lv.ents.filter(e => e.type === 'gear').map(e => e.id)); for (let k = 0; k < 3; k++) if (!placed.has(k) && !lv.gotNow[k]) { gg.id = k; break; }
      lv.ents.push(gg); }
  }
  function openDoors() { if (lv.doorT >= 0) return; lv.doorT = 0; SND.fx.door(); }
  function actionBox(p) {
    if (p.hero === 'robot') return p.face > 0 ? { x: p.x + p.w, y: p.y + p.h - 18, w: 30, h: 10 } : { x: p.x - 30, y: p.y + p.h - 18, w: 30, h: 10 };
    return p.face > 0 ? { x: p.x + p.w - 2, y: p.y + p.h - 16, w: 18, h: 14 } : { x: p.x - 16, y: p.y + p.h - 16, w: 18, h: 14 };
  }
  function doAction(p) {
    const box = actionBox(p);
    for (const e of lv.ents) {
      if (!overlap(box, e)) continue;
      if (e.type === 'enemy') reboot(e, p);
      else if (e.type === 'lever' && !e.on) { e.on = true; SND.fx.lever(); openDoors(); }
      else if (e.type === 'bolt' && p.hero === 'robot' && e.delay <= 0) collect(e, p);
      else if (e.type === 'button') pressButton(e);
    }
    const x0 = Math.floor(box.x / TS), x1 = Math.floor((box.x + box.w) / TS), y0 = Math.floor(box.y / TS), y1 = Math.floor((box.y + box.h) / TS);
    for (let ty = y0; ty <= y1; ty++) for (let tx = x0; tx <= x1; tx++) breakCrate(tx, ty);
  }
  function collect(e, p) {
    if (e.dead) return; e.dead = true;
    if (e.type === 'bolt') { lv.bolts++; if (e.orig) lv.boltGot++; SND.fx.bolt(); sparks(e.x + 5, e.y + 5, 3, '#ffd23f'); }
    if (e.type === 'gear') { lv.gotNow[e.id] = true; SND.fx.gear(); sparks(e.x + 5, e.y + 5, 14, '#ffd23f'); }
  }
  function pressButton(e) {
    if (e.dead || !boss) return; e.dead = true; boss.hits++; boss.ouch = 50; boss.shake = 30; SND.fx.button(); SND.fx.clank(); sparks(e.x + 7, e.y, 10, '#ff8a8a');
    if (boss.hits >= 3) { boss.fixed = true; SND.fx.reboot(); setTimeout(() => SND.fx.open(), 400); confetti(160, 60, 30); }
  }
  function spawnButton() {
    if (!boss || boss.fixed) return; if (lv.ents.some(e => e.type === 'button' && !e.dead)) return;
    const [bx, by] = boss.buttons[boss.hits]; lv.ents.push({ type: 'button', x: bx * TS + 1, y: by * TS - 6, w: 14, h: 6, t: 0 });
  }
  // The bosses of Worlds 2 to 4 (the crane is updated with the other things further down).
  function landing(x0, x1, fromY) {   // the top of the first floor, girder or plank under x0..x1
    for (let ty = Math.max(0, Math.floor(fromY / TS)); ty < lv.h; ty++)
      for (let tx = Math.floor(x0 / TS); tx <= Math.floor(x1 / TS); tx++) { const t = tileAt(lv, tx, ty); if (SOLID[t] || t === '-') return ty * TS; }
    return lv.h * TS;
  }
  function nearestX(x) { let best = null, bd = 1e9; for (const p of players) { if (p.bubble || p.dead) continue; const d = Math.abs(p.x + p.w / 2 - x); if (d < bd) { bd = d; best = p; } } return best ? best.x + best.w / 2 : x; }
  function hitPlayers(box, fromX) { for (const p of players) if (!p.bubble && p.inv <= 0 && overlap(p, box)) wobble(p, fromX); }
  function updateBoss() {
    const b = boss, sp = (little ? 0.9 : 1.25) * (1 + b.hits * 0.15);
    if (b.type === 'stamper') {
      // slides along its rail over the nearest hero, shakes, then slams down to the floor
      if (b.fixed) { b.y += (24 - b.y) * 0.05; b.x += (160 - b.x) * 0.03; b.state = 'move'; return; }
      if (b.state === 'move') { b.x = clamp(b.x + clamp(nearestX(b.x) - b.x, -1.1 * sp, 1.1 * sp), 40, 280); if (--b.timer <= 0) { b.state = 'warn'; b.timer = little ? 50 : 36; } }
      else if (b.state === 'warn') { if (--b.timer <= 0) { b.state = 'drop'; b.vy = 1; b.low = landing(b.x - 23, b.x + 23, b.y + 30); } }
      else if (b.state === 'drop') {
        b.vy = Math.min(b.vy + 0.7, 9); b.y += b.vy;
        if (b.y + 30 >= b.low) { b.y = b.low - 30; b.state = 'down'; b.timer = 50; S.shake = 10; SND.fx.stomp(); puff(b.x - 20, b.low, 5); puff(b.x + 20, b.low, 5); }
        hitPlayers({ x: b.x - 24, y: b.y + 4, w: 48, h: 26 }, b.x);
      }
      else if (b.state === 'down') { if (--b.timer <= 0) b.state = 'rise'; }
      else if (b.state === 'rise') { b.y -= 1.2 * sp; if (b.y <= 24) { b.y = 24; b.state = 'move'; b.timer = little ? 170 : 120; } }
      if (b.state === 'down' || b.state === 'rise') for (const p of players) if (!p.bubble && overlap(p, { x: b.x - 24, y: b.y + 4, w: 48, h: 26 })) { p.x = p.x + p.w / 2 < b.x ? b.x - 24 - p.w - 0.5 : b.x + 24.5; p.vx = 0; }
    } else if (b.type === 'boulder') {
      // rolls along the floor, bumps the wall, sits dizzy for a moment, rolls back
      const r = 18, floor = 12 * TS;
      if (b.fixed) { b.x += (272 - b.x) * 0.04; b.rot += (272 - b.x) * 0.002; b.y = floor - r; return; }
      if (b.state === 'wait' || b.state === 'dizzy') { if (--b.timer <= 0) b.state = 'roll'; }
      else {
        const v = (little ? 1.2 : 1.7) * (1 + b.hits * 0.2) * b.dir;
        b.x += v; b.rot += v / r;
        if (b.x - r <= 16 || b.x + r >= W - 16) { b.x = clamp(b.x, 16 + r, W - 16 - r); b.dir = -b.dir; b.state = 'dizzy'; b.timer = little ? 80 : 50; S.shake = 8; SND.fx.stomp(); puff(b.x - b.dir * r, floor - 8, 6); }
      }
      b.y = floor - r;
      for (const p of players) {
        if (p.bubble || p.inv > 0) continue;
        const cx = clamp(b.x, p.x, p.x + p.w), cy = clamp(b.y, p.y, p.y + p.h);
        if ((cx - b.x) ** 2 + (cy - b.y) ** 2 < 15 * 15) wobble(p, b.x);
      }
    } else if (b.type === 'hauler') {
      // flies to and fro, stops, flickers a warning, then shines a beam straight down
      if (b.fixed) { b.y += (30 - b.y) * 0.03; b.x += (160 - b.x) * 0.03; b.state = 'fly'; return; }
      if (b.state === 'fly') { b.ft += 1 / 60 * sp; b.x = 160 + Math.sin(b.ft * 0.7) * 110; b.y = 46 + Math.sin(b.ft * 2.1) * 4; if (--b.timer <= 0) { b.state = 'warn'; b.timer = little ? 60 : 45; } }
      else if (b.state === 'warn') { if (--b.timer <= 0) { b.state = 'beam'; b.timer = 80; SND.fx.blow(); } }
      else if (b.state === 'beam') { if (S.frame % 6 === 0) S.shake = 3; hitPlayers({ x: b.x - 12, y: b.y + 14, w: 24, h: 12 * TS - b.y - 14 }, b.x); if (--b.timer <= 0) { b.state = 'fly'; b.timer = little ? 170 : 130; } }
    }
  }
  function startClear() {
    if (S.clearT >= 0) return; S.clearT = 0; SND.playSong('rescue');
    const kindLevel = !def.boss && !def.bonus, sweep = lv.boltGot >= lv.boltTotal && S.foesLeft() === 0;
    const fast = def.par ? S.timeF <= def.par * 60 : !!def.time;
    const best = (prog.best || {})[id];
    S.result = { time: S.timeF, sweep, fast, record: best != null && S.timeF < best, first: best == null, stars: kindLevel ? [true, sweep, fast] : null,
      had: (prog.stars || {})[id] || [false, false, false] };
    for (const p of players) { p.vx = 0; p.cheer = true; p.bubble = 0; }
    confetti(160, 20, 50);
  }
  function finish() {
    if (defIn) { prog.lives = 3; go(levelScene(id, false, defIn)); return; }   // editor play-test: just go again
    // count gears new to this save
    const before = prog.got[id] || [false, false, false];
    const now = before.map((b, k) => b || lv.gotNow[k]);
    const newG = now.filter(Boolean).length - before.filter(Boolean).length;
    prog.got[id] = now;
    prog.best = prog.best || {}; prog.stars = prog.stars || {};
    if (S.result && (prog.best[id] == null || S.result.time < prog.best[id])) prog.best[id] = S.result.time;
    if (S.result && S.result.stars) prog.stars[id] = S.result.stars.map((v, k) => v || S.result.had[k]);
    saveProg();
    finishNode(id, { bolts: lv.bolts, gears: newG });
    mapScene.keep = true; go(mapScene);   // map already built and paved by finishNode
  }

  S.comeBack = function (h, bolts) {
    lv.bolts += bolts; scene = S; SND.playSong(S.song);
    players.forEach((p, k) => { p.x = h.out.x - p.w / 2 + k * 10; p.y = h.out.y - p.h - 2; p.vx = 0; p.vy = -6.5; p.spring = true; p.inv = 60; p.safeX = p.x; p.safeY = h.out.y - p.h; });
    S.camX = clamp(h.out.x - W / 2, 0, Math.max(0, lv.w * TS - W)); puff(h.out.x, h.out.y, 10); SND.fx.boing();
  };
  S.update = function () {
    S.t += 1 / 60; S.frame++; if (S.nameT > 0) S.nameT--;
    if (S.clearT >= 0) {
      S.clearT++;
      if (S.clearT % 25 === 0) confetti(S.camX + 160, 0, 8);
      for (const p of players) { p.vy += grav; moveBody(lv, p); }
      S.camX += (clamp(players.reduce((a, p) => a + p.x, 0) / players.length - W / 2 + 6, 0, Math.max(0, lv.w * TS - W)) - S.camX) * 0.12;
      if (S.clearT > 330 || (S.clearT > 90 && (anyHit('a') || anyHit('start')))) finish();
      if (S.result && S.result.stars) for (let k = 0; k < 3; k++) if (S.clearT === 70 + k * 18 && S.result.stars[k]) SND.fx.star(k);
      if (S.result && S.result.record && S.clearT === 140) SND.fx.record();
      stepParticles(); return;
    }
    if (S.dyingT >= 0) {
      S.dyingT++;
      for (const p of players) if (p.dead && S.dyingT > 30) { p.vy += 0.3; p.y += p.vy; }
      if (S.dyingT === 150) {
        if (prog.lives <= 0 && defIn) { prog.lives = 3; go(levelScene(id, false, defIn)); }
        else if (prog.lives <= 0) {
          prog.lives = 3; prog.at = id; saveProg();
          buildMap(); MAP.say('Out of tries! Have another go'); mapScene.keep = true; go(mapScene);
        } else go(levelScene(id, true, defIn, S.snap));
      }
      stepParticles(); return;
    }
    if (anyHit('start') && S.frame > 20) { SND.fx.select(); go(pauseScene(S)); return; }
    S.timeF++;
    if (def.time && S.timeF >= def.time * 60) { outOfTime(); stepParticles(); return; }
    if (def.time && S.timeF >= (def.time - 10) * 60 && S.timeF % 60 === 0) SND.fx.tick();
    // --- moving platforms first, so riders can be carried by how far they moved
    for (const m of lv.ents) if (m.type === 'mover') {
      const ox = m.x, oy = m.y, sp = little ? 1 : 1.25;
      if (m.kind === 'raft') { const k = (1 - Math.cos(S.t * sp * P * 2 / m.period)) / 2; m.x = m.ax + (m.bx - m.ax) * k; m.y = m.ay + (m.by - m.ay) * k; }
      else if (m.kind === 'crusher') {
        m.timer--;
        if (m.state === 'up' && m.timer <= 0) { m.state = 'warn'; m.timer = little ? 48 : 34; }
        else if (m.state === 'warn' && m.timer <= 0) { m.state = 'drop'; m.vy = 1; }
        else if (m.state === 'drop') { m.vy = Math.min(m.vy + 0.7, 8); m.y += m.vy; if (m.y >= m.low) { m.y = m.low; m.state = 'down'; m.timer = 40; S.shake = 8; SND.fx.stomp(); puff(m.x + 4, m.y + 32, 4); puff(m.x + 28, m.y + 32, 4); } }
        else if (m.state === 'down' && m.timer <= 0) m.state = 'rise';
        else if (m.state === 'rise') { m.y -= 0.8 * sp; if (m.y <= m.home) { m.y = m.home; m.state = 'up'; m.timer = little ? 120 : 80; } }
      }
      else { m.ang = 0.75 * Math.sin(S.t * 1.3 * sp); m.x = m.px + Math.sin(m.ang) * m.len - m.w / 2; m.y = m.py + Math.cos(m.ang) * m.len; }
      m.dx = m.x - ox; m.dy = m.y - oy;
    }
    // --- players
    for (const p of players) {
      if (p.bubble) {
        const b = players.find(q => q !== p && !q.bubble) || p;
        p.bubble++; const tx = b.x - b.face * 4, ty = b.y - 26;
        p.x += (tx - p.x) * 0.05; p.y += (ty - p.y) * 0.05 + Math.sin(S.t * 4) * 0.3;
        if ((p.bubble > 50 && Math.hypot(tx - p.x, ty - p.y) < 14) || hit(p.slot, 'a') && p.bubble > 50) { p.bubble = 0; p.inv = 90; p.vy = -2; SND.fx.pop(); puff(p.x + p.w / 2, p.y + p.h / 2, 8, 'rgba(190,240,255,0.9)'); }
        continue;
      }
      const i = p.slot; p.anim += Math.abs(p.vx) * 0.055 + 0.004;
      if (p.inv > 0) p.inv--; if (p.jetT > 0) p.jetT--; if (p.actT > 0) p.actT--;
      if (p.rainbowT > 0 && --p.rainbowT === 0 && !players.some(q => q.rainbowT > 0)) SND.playSong(S.song);
      if (p.magnetT > 0) p.magnetT--;
      const max = (p.hero === 'robot' ? 1.75 : 2.2) * (p.rainbowT > 0 ? 1.35 : 1), acc = p.ground ? 0.32 : 0.2;
      let dir = 0;
      if (p.wob > 0) p.wob--; else dir = (held(i, 'right') ? 1 : 0) - (held(i, 'left') ? 1 : 0);
      if (dir) { p.vx = clamp(p.vx + dir * acc, -max, max); p.face = dir; }
      else if (p.wob <= 0) { p.vx *= p.ground ? 0.7 : 0.94; if (Math.abs(p.vx) < 0.06) p.vx = 0; }
      if (p.ground) { p.coyote = 7; p.boost = false; } else if (p.coyote > 0) p.coyote--;
      if (hit(i, 'down') && p.ground && !p.low && p.wob <= 0) {
        const h = lv.ents.find(e => e.type === 'hatch' && overlap(p, { x: e.x + 2, y: e.y - 6, w: e.w - 4, h: 12 }));
        if (h) { SND.fx.hatch(); for (const q of players) { q.bubble = 0; if (q.low) { setH(q, baseH(q)); q.low = false; } } scene = jetScene(S, h); SND.playSong('jet'); return; }
      }
      // --- Down: crouch when standing, slide when running. Low = 14 px tall, so you fit under 1-tile gaps.
      const fullH = baseH(p);
      if (held(i, 'down') && p.ground && p.wob <= 0 && !p.low) {
        if (Math.abs(p.vx) > 1.1) { p.slideT = 36; p.vx = p.face * 3.2; SND.fx.slide(); }
        setH(p, has(p, 'mega') ? 20 : 14); p.low = true;
      }
      if (p.low) {
        if (p.slideT > 0) { p.slideT--; p.vx = p.face * Math.max(0.9, Math.abs(p.vx) * 0.975); if (S.frame % 4 === 0) puff(p.x + p.w / 2 - p.face * 6, p.y + p.h, 1); }
        else p.vx = clamp(p.vx, -0.9, 0.9);
        if ((!held(i, 'down') && p.slideT <= 0) || !p.ground) { if (canStand(p, fullH)) { setH(p, fullH); p.low = false; p.slideT = 0; } else if (p.slideT <= 0 && !held(i, 'down') && dir) p.vx = dir * 0.9; }
      }
      if (hit(i, 'a') && p.wob <= 0 && p.low) { if (canStand(p, fullH)) { setH(p, fullH); p.low = false; p.slideT = 0; } }
      if (hit(i, 'a') && p.wob <= 0 && !p.low) {
        if (p.coyote > 0) { p.vy = p.hero === 'robot' ? -5.5 : -6.1; p.coyote = 0; p.ground = false; SND.fx.jump(); puff(p.x + p.w / 2, p.y + p.h, 3); }
        else if (p.hero === 'robot' && !p.boost) { p.vy = -4.8; p.boost = true; p.jetT = 22; SND.fx.boost(); }
      }
      if (!held(i, 'a') && p.vy < -2.2 && p.jetT <= 0 && !p.spring) p.vy = -2.2;
      p.flapping = false;
      if (p.hero === 'ostrich' && held(i, 'a') && p.vy > 0.6 && !p.ground) { p.vy = 0.6; p.flapping = true; if (p.flapT-- <= 0) { SND.fx.flap(); p.flapT = 14; } }
      p.vy = Math.min(p.vy + grav, grav < 0.3 ? 4.5 : 6);   // the Moon's low gravity floats you down slower
      if (p.vy >= 0) p.spring = false;
      if (p.actT <= 0) p.shoot = false;
      const near = !p.carry && hit(i, 'b') && p.actT <= 0 && p.wob <= 0 && !p.low && lv.ents.find(e => e.type === 'enemy' && e.state === 'shell' && !e.dead && overlap(e, { x: p.x - 10, y: p.y - 4, w: p.w + 20, h: p.h + 8 }));
      if (p.carry && hit(i, 'b')) {
        const e = p.carry; p.carry = null; e.x = p.face > 0 ? p.x + p.w + 1 : p.x - e.w - 1; e.y = Math.min(e.y, p.y + p.h - e.h);
        if (isSolid(lv, Math.floor((e.x + e.w / 2) / TS), Math.floor((e.y + e.h / 2) / TS))) e.x = p.x + p.w / 2 - e.w / 2;
        kickCrate(e, p.face, p.slot); e.vy = held(i, 'up') ? -6.5 : -2; if (held(i, 'up')) e.spd = 1.6;
        p.actT = 12; p.shoot = true;
      } else if (near) { p.carry = near; near.state = 'carried'; near.vx = 0; SND.fx.grab(); p.actT = 8; p.shoot = true; }
      else if (hit(i, 'b') && p.actT <= 0 && p.wob <= 0 && has(p, 'bubble')) {
        if (lv.ents.filter(e => e.type === 'shot' && e.owner === p.slot).length < 2) {
          lv.ents.push({ type: 'shot', owner: p.slot, x: p.face > 0 ? p.x + p.w : p.x - 10, y: p.y + p.h * 0.3, w: 10, h: 10, vx: p.face * 2.4 + p.vx * 0.4, life: 110, t: 0 });
          p.actT = 12; p.shoot = true; SND.fx.blow();
        }
      } else if (hit(i, 'b') && p.actT <= 0 && p.wob <= 0) { p.actT = 22; (p.hero === 'robot' ? SND.fx.grab : SND.fx.kick)(); }
      if (p.actT >= 6 && p.actT <= 18 && !p.shoot) doAction(p);
      if (p.onMover) { p.x += p.onMover.dx; p.y += p.onMover.dy; }
      const wasGround = p.ground || !!p.onMover, pvy = p.vy, prevBottom = p.y + p.h;
      const belt = p.ground ? beltUnder(p) : 0;   // a conveyor belt carries you along
      p.vx += belt; moveBody(lv, p); if (belt && !p.hitWall) p.vx -= belt;
      p.onMover = null;
      for (const m of lv.ents) if (m.type === 'mover' && m.kind === 'crusher' && overlap(p, m) && prevBottom > m.y + 4) {
        const out = p.x + p.w / 2 < m.x + 16 ? m.x - p.w - 0.5 : m.x + m.w + 0.5;
        if (m.state === 'drop' && p.y + p.h > m.y + m.h - 10) { p.x = out; if (p.inv <= 0) wobble(p, m.x + 16); else p.vx = 0; }
        else { p.x = out; p.vx = 0; }
      }
      if (p.vy >= 0 && !p.ground) for (const m of lv.ents) if (m.type === 'mover' && p.x + p.w > m.x + 1 && p.x < m.x + m.w - 1 && prevBottom <= m.y + 2 + Math.max(0, m.dy) && p.y + p.h >= m.y - 0.5) { p.y = m.y - p.h; p.vy = 0; p.ground = true; p.onMover = m; break; }
      if (has(p, 'mega')) {
        if (p.hitWall) { const tx = p.hitWall > 0 ? Math.floor((p.x + p.w + 1) / TS) : Math.floor((p.x - 1) / TS); for (let ty = Math.floor(p.y / TS); ty <= Math.floor((p.y + p.h - 1) / TS); ty++) breakCrate(tx, ty); }
        if (p.ground && !wasGround && pvy > 3) { S.shake = 10; SND.fx.stomp(); puff(p.x + p.w / 2, p.y + p.h, 6); }
      }
      if (p.bumped) { const [tx, ty] = p.bumped; breakCrate(tx, ty); }
      if (p.ground && !wasGround) puff(p.x + p.w / 2, p.y + p.h, 2);
      if (p.ground) {
        const l = Math.floor((p.x + 1) / TS), r = Math.floor((p.x + p.w - 1) / TS), ty = Math.floor((p.y + p.h + 1) / TS);
        const ok = t => t === '#' || t === '=' || t === '-' || t === '<' || t === '>';
        if (ok(tileAt(lv, l, ty)) && ok(tileAt(lv, r, ty))) { p.safeX = p.x; p.safeY = p.y; }
      }
      if (p.y > H + 30) loseLife(p);
      if (p.x < 0) { p.x = 0; p.vx = 0; }
      if (p.x > lv.w * TS - p.w) { p.x = lv.w * TS - p.w; p.vx = 0; }
    }
    // two players: a player left far behind hops over to their buddy
    if (players.length === 2 && !players.some(q => q.bubble)) {
      const [a, b] = players, dx = a.x - b.x;
      if (Math.abs(dx) > 250) { const lag = dx > 0 ? b : a, lead = dx > 0 ? a : b; puff(lag.x + 6, lag.y + 15, 6); lag.x = lead.x - lead.face * 14; lag.y = lead.y - 6; lag.vx = 0; lag.vy = -2; lag.inv = 40; SND.fx.hop(); puff(lag.x + 6, lag.y + 15, 6); }
    }
    // --- doors
    if (lv.doorT >= 0) { lv.doorT++; if (lv.doorT === 40) for (let y = 0; y < lv.h; y++) for (let x = 0; x < lv.w; x++) if (lv.tiles[y][x] === 'D') lv.tiles[y][x] = '.'; }
    // --- the boss: press its three red buttons while dodging it
    if (boss) {
      if (boss.shake > 0) boss.shake--; if (boss.ouch > 0) boss.ouch--;
      if (S.frame > 60) spawnButton();
      updateBoss();
      if (boss.fixed && !boss.padShown && boss.ouch <= 0) { boss.padShown = true; lv.ents.push({ type: 'pad', x: 9 * TS - 4, y: 12 * TS - 6, w: 24, h: 6, pop: 0 }); SND.fx.open(); }
    }
    if (crane) {
      const sp = little ? 0.9 : 1.25;
      crane.ang = crane.fixed ? crane.ang * 0.96 : 0.95 * Math.cos(S.t * sp);   // starts out at the side, away from the players
      crane.rope = crane.fixed ? Math.max(40, (crane.rope || 140) - 1.2) : 140;   // fixed: reel the magnet up out of the way
      const mx = 160 + Math.sin(crane.ang) * crane.rope, my = 24 + Math.cos(crane.ang) * crane.rope;
      crane.mx = mx; crane.my = my;
      if (!crane.fixed) for (const p of players) {
        const cx = clamp(mx, p.x, p.x + p.w), cy = clamp(my, p.y, p.y + p.h);
        if (!p.bubble && (cx - mx) ** 2 + (cy - my) ** 2 < 11 * 11 && p.inv <= 0) wobble(p, mx);
      }
    }
    // --- bolt shower
    if (shower) {
      if (shower.left > 0) {
        shower.left--;
        if (shower.left % 9 === 0) { const b = pickup('bolt', 20 + Math.random() * 280, -6); b.fly = true; b.vy = 0.5; b.vx = 0; b.delay = 0; b.rain = true; lv.ents.push(b); }
      } else if (shower.done < 0) { shower.done = 0; SND.fx.open(); } else if (++shower.done > 120) startClear();
    }
    // --- entities
    for (const e of lv.ents) {
      if (e.dead) continue;
      e.t = (e.t || 0) + 1 / 60;
      if (e.type === 'bolt' || e.type === 'gear') {
        if (e.delay > 0) e.delay--;
        const mag = e.delay <= 0 && players.find(p => p.magnetT > 0 && !p.bubble && Math.hypot(p.x + p.w / 2 - e.x - 5, p.y + p.h / 2 - e.y - 5) < 150);
        if (mag) { e.fly = false; e.x += (mag.x + mag.w / 2 - 5 - e.x) * 0.12; e.y += (mag.y + mag.h / 2 - 5 - e.y) * 0.12; }
        else if (e.fly) { e.vy = Math.min(e.vy + 0.2, 4); const pv = e.vy; moveBody(lv, e); if (e.ground) { e.vy = pv > 1.2 ? -pv * 0.4 : 0; e.vx *= 0.8; if (Math.abs(e.vy) < 0.3 && Math.abs(e.vx) < 0.1) e.fly = false; } if (e.y > H + 20) e.dead = true; }
        for (const p of players) if (!p.bubble && e.delay <= 0 && overlap(p, e)) collect(e, p);
      } else if (e.type === 'enemy') {
        updateEnemy(e);
        if (e.state === 'bubbled') for (const p of players) if (!p.bubble && overlap(p, e)) { e.bubT = 999; if (p.vy > 0) p.vy = -4; }
        if ((e.state === 'shell' || e.state === 'kicked') && e.kickT <= 0) for (const p of players) {
          if (p.bubble || p.carry || !overlap(p, e)) continue;
          const fromTop = p.vy > 0 && p.y + p.h - p.vy <= e.y + 5;
          if (e.state === 'kicked' && fromTop) { e.state = 'shell'; e.shellT = 0; e.vx = 0; e.kickT = 10; p.vy = -4.4; SND.fx.stomp(); puff(e.x + e.w / 2, e.y + e.h, 4); }
          else if (e.state === 'shell') { kickCrate(e, p.x + p.w / 2 < e.x + e.w / 2 ? 1 : -1, p.slot); if (fromTop) p.vy = -4.4; }
          break;
        }
        if (e.state === 'glitchy') for (const p of players) {
          if (p.bubble || !overlap(p, e)) continue;
          if (p.slideT > 0 || p.rainbowT > 0) { reboot(e, p); continue; }
          if (p.vy > 0 && p.y + p.h - p.vy <= e.y + 5) {
            if (e.kind === 'grumble') shutCrate(e); else reboot(e, p);
            p.vy = held(p.slot, 'a') ? (e.kind === 'sprocket' ? -8.6 : -6.2) : (e.kind === 'sprocket' ? -7.6 : -4.4); p.spring = e.kind === 'sprocket'; p.boost = false;
          } else if (p.inv <= 0) wobble(p, e.x + e.w / 2);
        }
      } else if (e.type === 'power') {
        e.t += 1 / 60;
        if (e.rise > 0) { e.rise--; e.y += e.vy * 0.5; if (e.rise === 0) { e.vy = 0; e.vx = 0.7 * e.dir; } }
        else {
          e.vy = Math.min(e.vy + 0.25, 4); const pv = e.vy; moveBody(lv, e);
          if (e.hitWall) e.dir *= -1; e.vx = 0.7 * e.dir;
          if (e.kind === 'rainbow' && e.ground) e.vy = -3.6;
          if (e.y > H + 20) e.dead = true;
          for (const p of players) if (!p.bubble && !e.dead && overlap(p, e)) {
            e.dead = true; SND.fx.power(); sparks(e.x + 6, e.y + 6, 14, '#fff6a0'); S.toast = { text: POWER_NAME[e.kind], t: 110 };
            if (e.kind === 'mega' || e.kind === 'bubble') setStack(p, [...p.stack.filter(k => k !== e.kind), e.kind]);
            else if (e.kind === 'rainbow') { p.rainbowT = 480; SND.playSong('rainbow'); }
            else p.magnetT = 1200;
          }
        }
      } else if (e.type === 'shot') {
        e.t++; e.x += e.vx; e.vx *= 0.985; e.y += Math.sin(e.t * 0.2) * 0.4 - 0.06;
        const tx = Math.floor((e.x + 5) / TS), ty = Math.floor((e.y + 5) / TS);
        let pop = --e.life <= 0;
        if (tileAt(lv, tx, ty) === 'X') { breakCrate(tx, ty); pop = true; } else if (isSolid(lv, tx, ty)) pop = true;
        for (const en of lv.ents) if (en.type === 'enemy' && en.state === 'glitchy' && overlap(e, en)) { en.state = 'bubbled'; en.bubT = 0; en.vx = 0; pop = true; SND.fx.blow(); break; }
        if (pop) { e.dead = true; puff(e.x + 5, e.y + 5, 3, 'rgba(190,240,255,0.9)'); }
      } else if (e.type === 'tyre') {
        if (e.sq > 0) e.sq -= 0.08;
        for (const p of players) if (p.vy > 0 && p.x + p.w > e.x + 1 && p.x < e.x + e.w - 1 && p.y + p.h >= e.y && p.y + p.h - p.vy <= e.y + 6) {
          p.y = e.y - p.h; p.vy = held(p.slot, 'a') ? -9.4 : -8.4; p.spring = true; p.boost = false; e.sq = 1; SND.fx.spring();
        }
      } else if (e.type === 'lever') {
        if (!e.on) for (const p of players) if (overlap(p, { x: e.x, y: 0, w: e.w, h: e.y + e.h })) { e.on = true; SND.fx.lever(); openDoors(); }   // bumping it counts too
      } else if (e.type === 'button') {
        for (const p of players) if (p.vy > 0 && overlap(p, e) && p.y + p.h - p.vy <= e.y + 4) { pressButton(e); p.vy = -5; }
      } else if (e.type === 'flag') {
        if (!e.on) for (const p of players) if (!p.bubble && overlap(p, e)) {
          e.on = true; SND.fx.flag(); sparks(e.x + 6, e.y + 4, 14, '#9dffc0'); S.toast = { text: 'CHECKPOINT!', t: 100 };
          const here = new Set(lv.ents.filter(q => !q.dead).map(q => q.uid)), gone = [];
          for (let u = 0; u < lv.entCount; u++) if (!here.has(u)) gone.push(u);
          S.snap = { x: e.x + 6, y: e.y + e.h, flag: e.uid, timeF: S.timeF, bolts: lv.bolts, boltGot: lv.boltGot, gotNow: lv.gotNow.slice(), doorT: lv.doorT, tiles: lv.tiles.map(r => r.slice()), gone };
          break;
        }
      } else if (e.type === 'pad') {
        for (const p of players) if (!p.bubble && overlap(p, { x: e.x + 4, y: 0, w: e.w - 8, h: e.y + 6 })) startClear();   // the whole column above the pad counts
      }
    }
    lv.ents = lv.ents.filter(e => !e.dead || e.type === 'button');
    // --- camera
    const lead = players.reduce((s, p) => s + p.x, 0) / players.length + (players.length === 1 ? players[0].face * 24 : 0);
    const target = clamp(lead - W / 2 + 6, 0, Math.max(0, lv.w * TS - W));
    S.camX += (target - S.camX) * 0.12;
    if (S.shake > 0) S.shake--; if (S.toast && --S.toast.t <= 0) S.toast = null;
    stepParticles();
  };
  function updateEnemy(e) {
    const sp = speed();
    e.vy = Math.min(e.vy + 0.3, 5);
    if (e.state === 'bubbled') { e.bubT++; e.y -= 0.5; e.x += Math.sin(e.bubT * 0.15) * 0.4; if (e.bubT > 70) { e.state = 'glitchy'; SND.fx.pop(); reboot(e); } return; }
    if (e.state === 'happy') { e.happyT++; e.vx = 0; if (e.happyT === 24) { e.dead = true; puff(e.x + e.w / 2, e.y + e.h / 2, 10); sparks(e.x + e.w / 2, e.y + e.h / 2, 8, '#bff6ff'); SND.fx.pop(); } return; }
    if (e.kickT > 0) e.kickT--;
    if (e.state === 'carried') {
      const c = players.find(q => q.carry === e);
      if (!c || c.bubble || c.dead || c.wob > 0 || c.low) { if (c) c.carry = null; e.state = 'shell'; e.shellT = Math.min(e.shellT, 420); e.kickT = 20; e.vy = -1; return; }
      e.x = c.x + c.w / 2 - e.w / 2; e.y = c.y - e.h + 3; e.vy = 0; e.shellT = Math.min(e.shellT, 300); return;
    }
    if (e.state === 'shell') {
      e.vx = 0; moveBody(lv, e); if (e.y > H + 30) e.dead = true;
      if (++e.shellT > 600) wakeCrate(e);   // wiggles from 480, then pops back open
      return;
    }
    if (e.state === 'kicked') {
      if (e.ground) e.spd = 3.2; e.vx = e.spd * e.dir;
      const pdir = e.dir; moveBody(lv, e);
      if (e.hitWall) {
        const tx = pdir > 0 ? Math.floor((e.x + e.w + 1) / TS) : Math.floor((e.x - 1) / TS);
        for (let ty = Math.floor(e.y / TS); ty <= Math.floor((e.y + e.h - 1) / TS); ty++) breakCrate(tx, ty);
        e.dir = -pdir; SND.fx.clank(); puff(pdir > 0 ? e.x + e.w : e.x, e.y + e.h / 2, 3);
        if (++e.bounces > 6) { reboot(Object.assign(e, { state: 'glitchy' })); return; }
      }
      if (e.bumped) breakCrate(e.bumped[0], e.bumped[1]);
      for (const o of lv.ents) {
        if (o === e || o.dead || !overlap(o, e)) continue;
        if (o.type === 'enemy' && (o.state === 'glitchy' || o.state === 'shell' || o.state === 'kicked')) { o.state = 'glitchy'; reboot(o); SND.fx.clank(); }
        else if (o.type === 'lever' && !o.on) { o.on = true; SND.fx.lever(); openDoors(); }
        else if (o.type === 'button') pressButton(o);
      }
      if (e.y > H + 30 || Math.abs(e.x - S.camX - W / 2) > W * 1.5) e.dead = true;
      return;
    }
    if (e.kind === 'bumble') {
      e.vx = 0.45 * sp * e.face;
      // turn at walls and ledges
      const ahead = e.face > 0 ? e.x + e.w + 1 : e.x - 1, below = Math.floor((e.y + e.h + 2) / TS);
      if (!isSolid(lv, Math.floor(ahead / TS), below) && tileAt(lv, Math.floor(ahead / TS), below) !== '-') e.face *= -1;
    } else if (e.kind === 'sprocket') {
      e.vx = 0; if (e.ground && --e.hopT <= 0) { e.vy = -3.6; e.hopT = 80 / sp; }
    } else if (e.kind === 'grumble') {
      let near = null, nd = 1e9; for (const p of players) { const d = Math.abs(p.x - e.x); if (d < nd) { nd = d; near = p; } }
      if (near && nd < 120) e.face = near.x > e.x ? 1 : -1;
      e.vx = (nd < 120 ? 0.4 : 0.2) * sp * e.face;
      const ahead = e.face > 0 ? e.x + e.w + 1 : e.x - 1, below = Math.floor((e.y + e.h + 2) / TS);
      if (!isSolid(lv, Math.floor(ahead / TS), below)) { e.face *= -1; e.vx = 0; }
    }
    moveBody(lv, e);
    if (e.hitWall) e.face *= -1;
    if (e.y > H + 30) e.dead = true;
  }

  // ------------------------------------------------------------------- drawing a level
  // the backdrops and tiles come from the world's look (lib/levelart.js and themes/)
  const LT = LTHEMES[def.theme] || LTHEMES.junkyard, sheet = tileSheet(def.theme);
  const bgFar = document.createElement('canvas'); bgFar.width = 640; bgFar.height = 240;
  const bgMid = document.createElement('canvas'); bgMid.width = 640; bgMid.height = 240;
  for (const [cv, paint] of [[bgFar, b => LT.far(b, def.boss)], [bgMid, b => LT.mid(b)]]) { const b = cv.getContext('2d'), old = K.g; K.g = b; paint(b); K.g = old; }
  bgMid.getContext('2d').clearRect(0, 0, 640, 160);   // the N64 keeps only the bottom 80 rows, so the browser does too
  const tile = (i, x, y) => g.drawImage(sheet, i * TS, 0, TS, TS, x, y, TS, TS);
  function drawTiles() {
    const x0 = Math.max(0, Math.floor(S.camX / TS)), x1 = Math.min(lv.w - 1, x0 + 21), beltK = Math.floor(S.frame / 4) % 4;
    for (let ty = 0; ty < lv.h; ty++) for (let tx = x0; tx <= x1; tx++) {
      const t = lv.tiles[ty][tx]; if (t === '.') continue;
      const x = Math.round(tx * TS - S.camX), y = ty * TS, hash = (tx * 73856093 ^ ty * 19349663) >>> 0;
      if (t === '#') {
        tile((SOLID[tileAt(lv, tx, ty - 1)] ? 4 : 0) + hash % 4, x, y);
        if (!SOLID[tileAt(lv, tx - 1, ty)] && tx > 0) { g.fillStyle = 'rgba(0,0,0,0.15)'; g.fillRect(x, y, 2, TS); }
        if (!SOLID[tileAt(lv, tx + 1, ty)] && tx < lv.w - 1) { g.fillStyle = 'rgba(0,0,0,0.2)'; g.fillRect(x + TS - 2, y, 2, TS); }
      } else if (t === '=') tile(8, x, y);
      else if (t === '-') tile(9, x, y);
      else if (t === '>' || t === '<') tile(14 + (t === '>' ? beltK : 3 - beltK), x, y);
      else if (t === 'X') {
        const prize = lv.contents[tx + ',' + ty] && lv.contents[tx + ',' + ty] !== 'o';
        tile(prize ? 11 : 10, x, y);
        if (prize) {
          if (Math.sin(S.t * 4 + tx) > 0.6) K.star(x + 13, y + 3, 2.2, '#fff6a0');
          if (players.some(p => Math.abs(p.x + p.w / 2 - (tx * TS + 8)) < 48 && Math.abs(p.y + p.h - (ty + 1) * TS) < 40)) {
            const by = y - 12 + Math.sin(S.t * 5) * 1.5; K.circ(x + 8, by, 6, '#e0564f'); text('B', x + 8, by + 3.5, 8, '#fff', 'center', null);
          }
        }
      } else if (t === 'D') {
        const drop = lv.doorT >= 0 ? Math.min(1, lv.doorT / 40) * TS * (lv.h) : 0;
        g.save(); g.beginPath(); g.rect(x, 0, TS, 12 * TS); g.clip(); tile(ty % 3 === 0 ? 13 : 12, x, y + drop); g.restore();
      }
    }
  }
  // A Grumble Crate that has been stomped shut: lid down, dizzy eyes, wiggling before it pops open again.
  function drawShutCrate(e, x) {
    const wig = e.state === 'shell' && e.shellT > 480 ? Math.sin(e.shellT * 1.3) * 1.2 : 0, cx = x + e.w / 2 + wig, by = e.y + e.h;
    if (e.state === 'kicked' && e.ground) { g.globalAlpha = 0.5; g.fillStyle = '#fff6d8'; for (let k = 1; k <= 3; k++) g.fillRect(cx - e.dir * (6 + k * 4), by - 3 - k * 3, 4, 1.5); g.globalAlpha = 1; }
    K.rr(cx - 7, by - 12, 14, 12, 2, '#c08850'); g.strokeStyle = '#8a5a30'; g.lineWidth = 1; g.strokeRect(cx - 5.5, by - 10.5, 11, 9);
    K.rr(cx - 8, by - 14, 16, 4, 1.5, '#d9a46a'); g.fillStyle = '#8a5a30'; g.fillRect(cx - 8, by - 10.5, 16, 1);
    g.strokeStyle = '#2a1d12'; g.lineWidth = 1.1;
    for (const ex of [-3, 3]) { g.beginPath(); g.moveTo(cx + ex - 1.5, by - 8.5); g.lineTo(cx + ex + 1.5, by - 5.5); g.moveTo(cx + ex + 1.5, by - 8.5); g.lineTo(cx + ex - 1.5, by - 5.5); g.stroke(); }
    if (e.state !== 'kicked') for (let k = 0; k < 2; k++) { const a = S.t * 4 + k * P; K.star(cx + Math.cos(a) * 8, by - 17 + Math.sin(a) * 2, 1.8, '#fff6a0'); }
  }
  function drawEnts() {
    for (const e of lv.ents) {
      if (e.dead && e.type !== 'button') continue;
      const x = e.x - S.camX; if (x < -40 || x > W + 40) continue;
      if (e.type === 'bolt') { const sp = Math.cos(e.t * 5); g.save(); g.translate(x + 5, e.y + 5 + (e.fly ? 0 : Math.sin(e.t * 3) * 1)); g.scale(Math.abs(sp) * 0.8 + 0.2, 1); K.nut(0, 0, 4.2, 0, '#ffd23f'); g.restore(); }
      else if (e.type === 'gear') { const gl = 0.5 + 0.5 * Math.sin(e.t * 4); g.globalAlpha = 0.35 * gl; K.circ(x + 5, e.y + 5, 11, '#fff3a0'); g.globalAlpha = 1; K.gear(x + 5, e.y + 5 + Math.sin(e.t * 2) * 1.5, 7, 10, e.t * 1.5, '#ffd23f', '#b8860b'); }
      else if (e.type === 'enemy' && (e.state === 'shell' || e.state === 'kicked' || e.state === 'carried')) drawShutCrate(e, x);
      else if (e.type === 'enemy') {
        const gl = e.state === 'glitchy', s = 0.085;
        const idx = { bumble: 0, sprocket: 1, grumble: 2 }[e.kind];
        g.save(); if (!gl) { const k = e.happyT / 24; g.translate(x + e.w / 2, e.y + e.h); g.scale(1 + k * 0.5, Math.max(0.1, 1 - k * 0.9)); g.translate(-(x + e.w / 2), -(e.y + e.h)); }
        GLITCH[idx].draw(g, x + e.w / 2, e.y + e.h, s, e.t + (gl ? 0 : 100), gl || e.state === 'bubbled');
        g.restore();
        if (e.state === 'bubbled') { const cx = x + e.w / 2, cy = e.y + e.h / 2, r = 13 + Math.sin(e.bubT * 0.3) * 0.8; g.globalAlpha = 0.25; K.circ(cx, cy, r, '#bff0ff'); g.globalAlpha = 1; g.strokeStyle = '#dff6ff'; g.lineWidth = 1.5; g.beginPath(); g.arc(cx, cy, r, 0, P * 2); g.stroke(); K.circ(cx - 5, cy - 6, 2, '#fff'); }
      } else if (e.type === 'power') {
        const glow = 0.5 + 0.5 * Math.sin(e.t * 6); g.globalAlpha = 0.3 + glow * 0.3; K.circ(x + 6, e.y + 6, 10, '#fff3a0'); g.globalAlpha = 1;
        K.rr(x, e.y, 12, 12, 5, '#e8e4f0'); K.rr(x + 1, e.y + 1, 10, 10, 4, '#3b2a5c'); powerIcon(e.kind, x + 6, e.y + 6, S.t);
      } else if (e.type === 'shot') {
        g.globalAlpha = 0.3; K.circ(x + 5, e.y + 5, 5.5, '#bff0ff'); g.globalAlpha = 1; g.strokeStyle = '#dff6ff'; g.lineWidth = 1.2; g.beginPath(); g.arc(x + 5, e.y + 5, 5.5, 0, P * 2); g.stroke(); K.circ(x + 3, e.y + 3, 1.3, '#fff');
      } else if (e.type === 'flag') {
        const px = x + 2, top = e.y, wave = Math.sin(S.t * 6) * 2;
        g.fillStyle = '#5d6378'; g.fillRect(px, top, 2, e.h); K.circ(px + 1, top, 2.2, e.on ? '#ffd23f' : '#9aa0b4');
        g.fillStyle = e.on ? '#49d17a' : '#8a8fa3'; g.beginPath(); g.moveTo(px + 2, top + 2);
        if (e.on) { g.lineTo(px + 15, top + 6 + wave); g.lineTo(px + 2, top + 12); } else { g.lineTo(px + 6, top + 14); g.lineTo(px + 2, top + 13); }
        g.fill(); if (e.on) K.nut(px + 7, top + 7 + wave / 2, 2.2, S.t, '#fff6d8');
      } else if (e.type === 'hatch' || e.type === 'hatchOut') {
        g.fillStyle = '#2b2833'; g.beginPath(); g.ellipse(x + 12, e.y + 3, 12, 3.5, 0, 0, P * 2); g.fill();
        g.fillStyle = e.type === 'hatch' ? '#7d8397' : '#1b1426'; g.beginPath(); g.ellipse(x + 12, e.y + 2.5, 10, 2.6, 0, 0, P * 2); g.fill();
        if (e.type === 'hatch') {
          for (let k = 0; k < 6; k++) { const a = k / 6 * P * 2; K.circ(x + 12 + Math.cos(a) * 8, e.y + 2.5 + Math.sin(a) * 2, 0.8, '#ffd23f'); }
          const near = players.some(p => Math.abs(p.x + p.w / 2 - (x + S.camX + 12)) < 40);
          if (near || Math.sin(S.t * 4) > 0) { const ay = e.y - 16 + Math.sin(S.t * 6) * 2; g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(x + 6, ay); g.lineTo(x + 18, ay); g.lineTo(x + 12, ay + 7); g.fill(); g.strokeStyle = '#2b2230'; g.lineWidth = 1; g.stroke(); }
        }
      } else if (e.type === 'mover') {
        if (e.kind === 'crusher') {
          const sh = e.state === 'warn' ? Math.sin(S.frame * 1.8) * 1.5 : 0, cx = x + sh;
          g.fillStyle = '#4a4652'; g.fillRect(cx + 13, 0, 6, e.y); g.fillStyle = '#6e6880'; g.fillRect(cx + 14, 0, 2, e.y);
          K.rr(cx, e.y, 32, 32, 3, '#7d8397'); K.rr(cx + 2, e.y + 2, 28, 4, 1.5, '#a9b0c0');
          g.save(); g.beginPath(); g.rect(cx, e.y + 24, 32, 8); g.clip(); for (let k = -1; k < 6; k++) { g.fillStyle = k % 2 ? '#2b2833' : (e.state === 'warn' && S.frame % 8 < 4 ? '#fff6a0' : '#ffcf3a'); g.beginPath(); g.moveTo(cx + k * 8, e.y + 32); g.lineTo(cx + k * 8 + 8, e.y + 24); g.lineTo(cx + k * 8 + 16, e.y + 24); g.lineTo(cx + k * 8 + 8, e.y + 32); g.fill(); } g.restore();
          const mad = e.state === 'warn' || e.state === 'drop';
          K.eye(cx + 10, e.y + 14, 7, 8, '#2b2230', '#7d8397', { angry: mad ? 1 : 0, open: e.state === 'down' ? 0.3 : 1 }); K.eye(cx + 22, e.y + 14, 7, 8, '#2b2230', '#7d8397', { angry: mad ? -1 : 0, open: e.state === 'down' ? 0.3 : 1 });
          continue;
        }
        if (e.kind === 'hook') {
          const px = e.px - S.camX; g.strokeStyle = '#3a3346'; g.lineWidth = 1.5; g.beginPath(); g.moveTo(px, e.py); g.lineTo(x + e.w / 2, e.y - 10); g.stroke();
          K.circ(px, e.py, 3, '#d4892f'); K.arc(x + e.w / 2, e.y - 6, 4, P * 1.1, P * 1.9, 2, '#9aa1b0');
          g.strokeStyle = '#5d566a'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + e.w / 2, e.y - 10); g.lineTo(x + 3, e.y); g.moveTo(x + e.w / 2, e.y - 10); g.lineTo(x + e.w - 3, e.y); g.stroke();
        } else {
          g.strokeStyle = 'rgba(60,40,50,0.35)'; g.lineWidth = 2; g.setLineDash([3, 3]); g.beginPath(); g.moveTo(e.ax - S.camX + 16, e.ay + 3); g.lineTo(e.bx - S.camX + 16, e.by + 3); g.stroke(); g.setLineDash([]);
        }
        K.rr(x, e.y, e.w, e.h, 2, '#7a8a96'); g.fillStyle = '#a9b8c2'; g.fillRect(x + 1, e.y, e.w - 2, 2); K.circ(x + 4, e.y + 3.5, 1.2, '#4a4652'); K.circ(x + e.w - 4, e.y + 3.5, 1.2, '#4a4652');
        g.fillStyle = '#ffcf3a'; for (let k = 0; k < 3; k++) g.fillRect(x + 9 + k * 6, e.y + 2.5, 3, 2);
      } else if (e.type === 'sign') {
        g.fillStyle = '#7d5128'; g.fillRect(x + 5, e.y + 8, 2, 12); K.rr(x - 3, e.y - 2, 18, 13, 2, '#d9a46a'); g.strokeStyle = '#7d5128'; g.lineWidth = 1; g.strokeRect(x - 2.5, e.y - 1.5, 17, 12);
        g.fillStyle = '#2b2230'; g.beginPath(); g.moveTo(x + 1, e.y + 2); g.lineTo(x + 11, e.y + 2); g.lineTo(x + 6, e.y + 8.5); g.fill();
      } else if (e.type === 'tyre') {
        const sq = Math.max(0, e.sq), hh = 9 * (1 - sq * 0.4);
        K.rr(x, e.y + 9 - hh, 16, hh, hh / 2, '#2b2833'); K.rr(x + 4, e.y + 9 - hh + hh * 0.3, 8, hh * 0.4, 2, '#4a4652');
        g.fillStyle = '#5b5666'; g.fillRect(x + 2, e.y + 9 - hh + 1, 12, 1);
      } else if (e.type === 'lever') {
        K.rr(x - 1, e.y + 8, 12, 6, 1.5, '#5d566a');
        const a = e.on ? 0.7 : -0.7; g.save(); g.translate(x + 5, e.y + 10); g.rotate(a); g.fillStyle = '#9aa1b0'; g.fillRect(-1, -12, 2, 12); K.circ(0, -12, 3, e.on ? '#49d17a' : '#e04f4f'); g.restore();
        if (!e.on && Math.sin(S.t * 5) > 0.3) K.star(x + 12, e.y - 2, 2, '#fff');
      } else if (e.type === 'button') {
        if (e.dead) { K.rr(x, e.y + 4, 14, 2, 1, '#5d566a'); continue; }
        K.rr(x - 1, e.y + 3, 16, 4, 1, '#5d566a'); const pulse = 0.5 + 0.5 * Math.sin(S.t * 8);
        g.fillStyle = '#ff4d4d'; g.beginPath(); g.ellipse(x + 7, e.y + 4, 6, 5, 0, P, 0); g.fill(); g.globalAlpha = 0.4 * pulse; K.circ(x + 7, e.y + 1, 10, '#ff8a8a'); g.globalAlpha = 1;
        text('!', x + 7, e.y - 6 - pulse * 2, 8, '#ffd23f');
      } else if (e.type === 'pad') {
        K.rr(x, e.y, e.w, e.h, 2, '#3a8de0'); K.rr(x + 2, e.y + 1, e.w - 4, 2, 1, '#8fd3ff');
        g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(x + 13, e.y - 1); g.lineTo(x + 9, e.y + 3.5); g.lineTo(x + 12, e.y + 3.5); g.lineTo(x + 10, e.y + 7); g.lineTo(x + 15, e.y + 2); g.lineTo(x + 12, e.y + 2); g.fill();
        if (Math.sin(S.t * 6) > 0.5) K.star(x + 4 + (Math.floor(S.t * 3) % 3) * 8, e.y - 18, 2, '#bff6ff');
        const hop = S.clearT >= 0 ? Math.abs(Math.sin(S.t * 8)) * 10 : Math.abs(Math.sin(S.t * 3)) * 2;
        K.baby(x + 12, e.y - hop, 0.06, def.baby, { t: S.t, armL: S.clearT >= 0 ? 150 : 30, armR: S.clearT >= 0 ? -150 : -150 + Math.sin(S.t * 6) * 20, mouth: S.clearT >= 0 ? 'o' : 'smile', eyes: { happy: 0.5 } });
      }
    }
  }
  function drawCrane() {
    if (!crane) return;
    const sh = crane.shake > 0 ? Math.sin(crane.shake * 1.7) * 2 : 0, tx = 286 + sh;
    g.fillStyle = '#d4892f'; g.fillRect(tx - 6, 24, 12, 168); g.strokeStyle = '#a5651f'; g.lineWidth = 1.5;
    for (let y = 24; y < 190; y += 10) { g.beginPath(); g.moveTo(tx - 6, y); g.lineTo(tx + 6, y + 10); g.stroke(); }
    g.fillStyle = '#d4892f'; g.fillRect(150, 18, tx - 144, 8); g.fillStyle = '#a5651f'; g.fillRect(150, 24, tx - 144, 2);
    // cab with a face
    K.rr(tx - 16, 2, 32, 24, 5, crane.fixed ? '#49d17a' : '#e04f4f');
    const ex = tx, ey = 12;
    if (crane.ouch > 0) { text('X', ex - 7, ey + 5, 9, '#fff', 'center', null); text('X', ex + 7, ey + 5, 9, '#fff', 'center', null); }
    else { K.eye(ex - 7, ey, 9, 11, '#2b2230', crane.fixed ? '#49d17a' : '#e04f4f', { angry: crane.fixed ? 0 : 1, happy: crane.fixed ? 0.5 : 0 }); K.eye(ex + 7, ey, 9, 11, '#2b2230', crane.fixed ? '#49d17a' : '#e04f4f', { angry: crane.fixed ? 0 : -1, happy: crane.fixed ? 0.5 : 0 }); }
    K.mouth(crane.fixed ? 'smile' : 'zig', ex, ey + 6, 5, 1.5, '#2b2230');
    if (!crane.fixed && Math.floor(S.t * 8) % 3 === 0) { g.strokeStyle = '#bff6ff'; g.lineWidth = 1; g.beginPath(); g.moveTo(tx + 18, 4); g.lineTo(tx + 22, 9); g.lineTo(tx + 19, 10); g.lineTo(tx + 24, 15); g.stroke(); }
    // rope and magnet
    const mx = crane.mx ?? 160, my = crane.my ?? 164;
    g.strokeStyle = '#3a3346'; g.lineWidth = 1.5; g.beginPath(); g.moveTo(160, 24); g.lineTo(mx, my - 8); g.stroke();
    K.circ(160, 24, 3, '#5d566a');
    g.save(); g.translate(mx, my); g.rotate(-crane.ang);
    K.arc(0, 2, 9, P * 1.05, P * 1.95, 6, '#e04f4f'); g.fillStyle = '#e8e4f0'; g.fillRect(-12, 0, 6, 4); g.fillRect(6, 0, 6, 4);
    g.restore();
    if (!crane.fixed) { const k = (S.t * 1.5) % 1; g.globalAlpha = 0.5 * (1 - k); K.arc(mx, my, 12 + k * 8, P * 0.15, P * 0.85, 1, '#ff6b6b'); g.globalAlpha = 1; }
  }
  function drawBoss() {
    if (!boss) return;
    const b = boss, face = b.ouch > 0 ? 1 : b.fixed ? 2 : (b.type === 'boulder' && b.state === 'dizzy') ? 3 : 0;
    const zap = (x, y) => { if (!b.fixed && Math.floor(S.t * 8) % 3 === 0) { g.strokeStyle = '#bff6ff'; g.lineWidth = 1; g.beginPath(); g.moveTo(x, y); g.lineTo(x + 4, y + 5); g.lineTo(x + 1, y + 6); g.lineTo(x + 6, y + 11); g.stroke(); } };
    if (b.type === 'crane') drawCrane();
    else if (b.type === 'stamper') {
      // the rail it hangs from, and its piston
      g.fillStyle = '#3a3346'; g.fillRect(24, 14, 272, 6); g.fillStyle = '#5d6378'; g.fillRect(24, 14, 272, 2);
      K.rr(b.x - 12, 10, 24, 12, 3, '#7d8397');
      g.fillStyle = '#9aa1b0'; g.fillRect(b.x - 3, 20, 6, Math.max(0, b.y - 20)); g.fillStyle = '#5d6378'; g.fillRect(b.x + 1, 20, 2, Math.max(0, b.y - 20));
      const sh = b.state === 'warn' ? Math.sin(S.frame * 1.8) * 1.5 : 0;
      g.save(); g.translate(b.x + sh, b.y); BOSSART.stamper(g, face); g.restore();
      if (b.state === 'warn' && S.frame % 8 < 4) { g.globalAlpha = 0.5; g.fillStyle = '#ff6b6b'; g.fillRect(b.x - 24, landing(b.x - 23, b.x + 23, b.y + 30) - 2, 48, 2); g.globalAlpha = 1; }
      zap(b.x + 26, b.y + 2);
    } else if (b.type === 'boulder') {
      g.fillStyle = 'rgba(30,20,30,0.3)'; g.beginPath(); g.ellipse(b.x, 12 * TS, 18, 4, 0, 0, P * 2); g.fill();
      g.save(); g.translate(b.x, b.y); g.rotate(b.rot); BOSSART.boulderRock(g); g.restore();
      g.save(); g.translate(b.x, b.y); BOSSART.boulderFace(g, face); g.restore();
      if (face === 3) for (let k = 0; k < 3; k++) { const a = S.t * 5 + k * P * 2 / 3; K.star(b.x + Math.cos(a) * 14, b.y - 24 + Math.sin(a) * 3, 2.2, '#fff6a0'); }
      zap(b.x + 16, b.y - 22);
    } else if (b.type === 'hauler') {
      if (b.state === 'warn' && S.frame % 6 < 3) { g.fillStyle = 'rgba(191,246,255,0.7)'; g.fillRect(b.x - 1, b.y + 14, 2, 12 * TS - b.y - 14); }
      if (b.state === 'beam') { const w = 24 + Math.sin(S.frame * 0.6) * 2; g.globalAlpha = 0.35; g.fillStyle = '#bff6ff'; g.fillRect(b.x - w / 2, b.y + 14, w, 12 * TS - b.y - 14); g.globalAlpha = 0.7; g.fillRect(b.x - 4, b.y + 14, 8, 12 * TS - b.y - 14); g.globalAlpha = 1; }
      const tilt = b.fixed ? 0 : Math.sin(S.t * 2.2) * 0.08;
      g.save(); g.translate(b.x, b.y); g.rotate(tilt); BOSSART.hauler(g, face); g.restore();
      zap(b.x + 30, b.y - 14);
    }
    // hits so far
    for (let i = 0; i < 3; i++) K.circ(140 + i * 20, 220, 5, i < b.hits ? '#49d17a' : 'rgba(255,255,255,0.25)');
  }
  function drawPlayer(p) {
    if (p.inv > 0 && p.wob <= 0 && !p.dead && !p.bubble && Math.floor(p.inv / 4) % 2 === 0 && S.clearT < 0) return;
    if (p.dead) {
      const fx = p.x + p.w / 2 - S.camX, fy = p.y + p.h, o = { t: S.t, eyes: { open: 0.15 }, mouth: 'wavy', armL: 160, armR: -160, stars: true };
      if (p.hero === 'robot') K.robot(fx, fy, 0.074, p.hex, o); else K.ostrich(fx, fy, 0.071, p.hex, { t: S.t, eyes: { open: 0.15 }, beakOpen: 0.6, stars: true });
      return;
    }
    if (p.bubble) {
      const fx = p.x + p.w / 2 - S.camX, cy = p.y + p.h / 2;
      if (p.hero === 'robot') K.robot(fx, p.y + p.h, 0.074, p.hex, { t: S.t, eyes: { happy: 0.4 } }); else K.ostrich(fx, p.y + p.h, 0.071, p.hex, { t: S.t });
      g.globalAlpha = 0.28; K.circ(fx, cy, 21, '#bff0ff'); g.globalAlpha = 1; g.strokeStyle = 'rgba(220,250,255,0.9)'; g.lineWidth = 1.5; g.beginPath(); g.arc(fx, cy, 21, 0, P * 2); g.stroke();
      K.circ(fx - 8, cy - 9, 3, 'rgba(255,255,255,0.8)');
      return;
    }
    const mega = has(p, 'mega') ? 1.5 : 1, hexNow = p.rainbowT > 0 ? COLS[Math.floor(S.t * 12 + p.i * 3) % COLS.length] : p.hex;
    if (p.rainbowT > 0 && S.frame % 3 === 0) particles.push({ x: p.x + p.w / 2 + (Math.random() - 0.5) * 10, y: p.y + Math.random() * p.h, vx: 0, vy: -0.3, life: 20, max: 20, r: 2, col: hexStr(COLS[Math.floor(Math.random() * 8)]), kind: 'star' });
    const fx = p.x + p.w / 2 - S.camX, fy = p.y + p.h, hero = p.hero, dir = p.face, rs = 0.074 * mega, os = 0.071 * mega;
    const side = hero === 'robot' ? (o) => K.robotSide(fx, fy, rs, hexNow, Object.assign(o, { dir })) : (o) => K.ostrichSide(fx, fy, os, hexNow, Object.assign(o, { dir }));
    const front = hero === 'robot' ? (o) => K.robot(fx, fy, rs, hexNow, o) : (o) => K.ostrich(fx, fy, os, hexNow, o);
    const t = S.t + p.i * 0.7;
    // power-up auras, drawn behind the hero
    const aura = (col, r, a) => { const gr = g.createRadialGradient(fx, fy - p.h / 2, 2, fx, fy - p.h / 2, r); gr.addColorStop(0, col.replace('A', a)); gr.addColorStop(1, col.replace('A', 0)); g.fillStyle = gr; g.fillRect(fx - r, fy - p.h / 2 - r, r * 2, r * 2); };
    if (has(p, 'mega')) { aura('rgba(120,255,160,A)', p.h * 0.95, 0.45 + 0.2 * Math.sin(t * 6)); if (S.frame % 6 === 0) particles.push({ x: p.x + Math.random() * p.w, y: p.y + p.h * Math.random(), vx: 0, vy: -0.6, life: 22, max: 22, r: 1.8, col: '#9dffc0', kind: 'star' }); }
    if (p.rainbowT > 0) aura('rgba(' + [255, 200, 120].map((v, k) => Math.round(160 + 95 * Math.sin(t * 8 + k * 2))).join(',') + ',A)', p.h * 0.9, 0.5);
    if (p.magnetT > 0) { const k = (t * 1.2) % 1; g.globalAlpha = 0.5 * (1 - k); g.strokeStyle = '#ff6b6b'; g.lineWidth = 1.2; g.beginPath(); g.arc(fx, fy - p.h / 2, 8 + k * 22, 0, P * 2); g.stroke(); g.globalAlpha = 1; }
    if (S.clearT >= 0) { front(hero === 'robot' ? { t, armL: 150 + Math.sin(t * 9) * 20, armR: -150 - Math.sin(t * 9) * 20, eyes: { happy: 0.8 }, mouth: 'o' } : { t, bob: Math.abs(Math.sin(t * 6)) * 14, eyes: { happy: 0.8 }, beakOpen: 0.8 }); }
    else if (p.low && p.slideT > 0) side(hero === 'robot' ? { t, rot: -1.15, legF: -95, kneeF: 10, legB: -80, kneeB: 20, armF: -170, armB: -150, eyes: { happy: 0.6 } } : { t, rot: 0.95, sy: 0.9, legF: 95, kneeF: 20, legB: 80, kneeB: 10, beakOpen: 0.6, flick: 20 });
    else if (p.low) side(hero === 'robot' ? { t, sy: 0.62, sx: 1.12, bob: 20, legF: -35, kneeF: 70, legB: 30, kneeB: 70 } : { t, sy: 0.5, sx: 1.1, legF: -60, kneeF: 140, legB: -40, kneeB: 130, lean: 14 });
    else if (p.wob > 0) side(K.pose(hero, 'wobble', t));
    else if (p.actT > 0 && p.shoot) side(hero === 'robot' ? { t, armF: -90, eyes: { happy: 0.3 } } : { t, beakOpen: 0.9, lean: -6 });
    else if (p.actT > 0) {
      const k = Math.sin((22 - p.actT) / 22 * P);
      if (hero === 'robot') side({ t, grab: true, reach: k * 330, clawOpen: p.actT > 12 ? 0.7 : 0.05, legF: p.ground ? 0 : -30, legB: p.ground ? 0 : 20 });
      else side({ t, legF: -100 * k, legB: 10 * k, rot: -0.12 * k, lean: -12 * k, beakOpen: k > 0.5 ? 0.7 : 0 });
    }
    else if (!p.ground) {
      if (hero === 'robot') side(p.jetT > 0 ? K.pose('robot', 'jet', t) : K.pose('robot', 'jump', t));
      else side(p.flapping ? K.pose('ostrich', 'flap', t) : K.pose('ostrich', 'jump', t));
    }
    else if (Math.abs(p.vx) > 0.25) side(K.pose(hero, 'run', p.anim * 1.6 / (hero === 'robot' ? 1 : 1.1)));
    else front(hero === 'robot' ? { t, bob: Math.sin(t * 3) * 3 } : { t, bob: Math.abs(Math.sin(t * 2.6)) * 8, flick: Math.sin(t * 2.6) * 6 });
    if (p.magnetT > 0 && (p.magnetT > 180 || Math.floor(p.magnetT / 8) % 2)) { g.save(); g.translate(fx, fy - p.h - 4); g.scale(1.3 * mega, 1.3 * mega); powerIcon('magnet', 0, 0, S.t); g.restore(); }
    if (has(p, 'bubble') && S.clearT < 0) {
      // Bubble Blaster: three soap bubbles circle the hero
      const cy = fy - p.h * 0.5, rx = 11 * mega, ry = p.h * 0.28, br = 3.4 * mega;
      for (let k = 0; k < 3; k++) {
        const a = t * 2.6 + k * P * 2 / 3, bx = fx + Math.cos(a) * rx, by = cy + Math.sin(a) * ry, front = Math.sin(a) > 0;
        g.globalAlpha = front ? 1 : 0.55;
        g.globalAlpha *= 0.35; K.circ(bx, by, br, '#c8f2ff'); g.globalAlpha = front ? 1 : 0.55;
        g.strokeStyle = ['#8fd3ff', '#ff8fc7', '#c28bff'][k]; g.lineWidth = 1.1; g.beginPath(); g.arc(bx, by, br, 0, P * 2); g.stroke();
        K.circ(bx - br * 0.35, by - br * 0.35, br * 0.28, '#ffffff');
        g.globalAlpha = 1;
      }
    }
    if (players.length > 1) { const lx = fx, ly = fy - p.h - (p.magnetT > 0 ? 18 : 12); g.fillStyle = p.i ? '#8fd3ff' : '#ffd23f'; g.beginPath(); g.moveTo(lx - 3, ly); g.lineTo(lx + 3, ly); g.lineTo(lx, ly + 3); g.fill(); text(String(p.slot + 1), lx, ly - 1, 7, p.i ? '#8fd3ff' : '#ffd23f'); }
  }
  function hud() {
    panel(4, 4, 62, 15); K.nut(13, 11.5, 4, S.t, '#ffd23f'); text('×' + lv.bolts, 20, 16, 9, lv.bolts ? '#fff6d8' : '#ff9a8a', 'left', null);
    livesHud(W - 52, 4, S.t);
    players.forEach((p, k) => { const kinds = [...p.stack, p.rainbowT > 0 && 'rainbow', p.magnetT > 0 && 'magnet'].filter(Boolean); if (!kinds.length) return;
      const x0 = W - 56 - (k + 1) * (kinds.length * 12 + 8); panel(x0, 4, kinds.length * 12 + 4, 15); kinds.forEach((kd, j) => powerIcon(kd, x0 + 8 + j * 12, 11.5, S.t)); });
    if (!shower && S.clearT < 0) {
      const left = def.time ? Math.max(0, def.time * 60 - S.timeF) : 0, warn = def.time && left < 600;
      panel(128, 4, 46, 15); K.circ(137, 11.5, 4.2, warn ? '#ff8a8a' : '#d9d2e6'); g.strokeStyle = '#2b2230'; g.lineWidth = 1; g.beginPath(); g.moveTo(137, 11.5); g.lineTo(137, 8.8); g.moveTo(137, 11.5); g.lineTo(139, 11.5); g.stroke();
      text(clock(def.time ? left : S.timeF), 158, 16, 9, warn && Math.floor(S.t * 4) % 2 ? '#ff8a8a' : '#fff6d8', 'center', null);
    }
    if (def.time && S.frame < 200 && S.clearT < 0 && S.dyingT < 0) { const k = Math.min(1, S.frame / 15); panel(W / 2 - 70, 150, 140, 22, '#ffd23f'); text('RACE THE CLOCK!', W / 2, 165, 10 * k, '#2b2230', 'center', null); }
    if (S.timeUp && S.dyingT > 10) { panel(W / 2 - 60, 60, 120, 22, '#ff8a8a'); text("TIME'S UP!", W / 2, 76, 12, '#2b2230', 'center', null); }
    if (S.toast) { g.globalAlpha = Math.min(1, S.toast.t / 20); text(S.toast.text, W / 2, 50, 13, '#ffd23f'); g.globalAlpha = 1; }
    if (!def.boss && !def.bonus) {
      panel(70, 4, 50, 15);
      for (let k = 0; k < 3; k++) { const have = lv.got[k] || lv.gotNow[k]; K.gear(82 + k * 13, 11.5, 5, 9, have ? S.t : 0, have ? '#ffd23f' : 'rgba(255,255,255,0.22)', have ? '#b8860b' : 'rgba(27,20,38,0.86)'); }
    }
    if (shower && shower.left > 0) { panel(128, 4, 64, 15); text(Math.ceil(shower.left / 60) + 's', 160, 16, 10, shower.left < 300 ? '#ff8a8a' : '#fff6d8'); }
    if (S.nameT > 0) { g.globalAlpha = Math.min(1, S.nameT / 30); panel(W / 2 - 90, 92, 180, retry ? 44 : 30); text(retry ? 'TRY AGAIN!' : def.name, W / 2, 112, 11, '#ffd23f');
      if (retry) text(prog.lives + (prog.lives > 1 ? ' tries left' : ' try left'), W / 2, 128, 8, '#fff6d8', 'center', null); g.globalAlpha = 1; }
    if (S.dyingT > 40) { const k = Math.min(1, (S.dyingT - 40) / 20); g.globalAlpha = k; panel(W / 2 - 80, 96, 160, 34); text(prog.lives > 0 ? 'OOPS!' : 'OUT OF TRIES', W / 2, 118, 13, '#ffd23f'); g.globalAlpha = 1; }
    if (boss && !boss.fixed && S.frame < 260) text('Bounce on the red buttons!', W / 2, 140, 9, '#fff6d8');
    if (shower && S.frame < 180) text('Catch the bolts!', W / 2, 140, 10, '#fff6d8');
    if (S.clearT >= 0) {
      const k = Math.min(1, S.clearT / 20);
      panel(W / 2 - 80, 60 - (1 - k) * 30, 160, 34, '#ffd23f');
      text(def.boss ? 'FIXED!' : def.bonus ? 'BONUS!' : 'RESCUED!', W / 2, 84 - (1 - k) * 30, 18, '#2b2230', 'center', null);
      if (S.clearT > 40) { const nG = lv.gotNow.filter((v, k2) => v && !(prog.got[id] || [])[k2]).length; text('+' + lv.bolts + ' bolts' + (nG ? '   +' + nG + ' golden gear' + (nG > 1 ? 's' : '') : ''), W / 2, 112, 9); }
      const R = S.result;
      if (R && S.clearT > 55) {
        text('TIME ' + clock(R.time) + (def.par ? '   (star time ' + clock(def.par * 60) + ')' : ''), W / 2, 126, 8, '#fff6d8', 'center', null);
        if (R.stars) for (let k = 0; k < 3; k++) {
          const at = 70 + k * 18, pop = S.clearT < at ? 0 : Math.min(1, (S.clearT - at) / 8), got = R.stars[k];
          const sx = W / 2 - 36 + k * 36, sy = 150, r = 9 * (got ? 0.6 + pop * 0.4 + (pop < 1 ? 0.3 * Math.sin(pop * P) : 0) : 0.8);
          if (S.clearT >= at || !got) K.star(sx, sy, r + 1.5, got ? '#7a4a10' : 'rgba(27,20,38,0.6)');
          if (got && S.clearT >= at) K.star(sx, sy, r, '#ffd23f'); else K.star(sx, sy, r - 1, 'rgba(255,255,255,0.25)');
          text(['DONE', 'GOT IT ALL', 'SPEEDY'][k], sx, sy + 18, 6, got && S.clearT >= at ? '#fff6d8' : 'rgba(255,246,216,0.4)', 'center', null);
        }
        if (R.record && S.clearT > 140) { const b = 1 + 0.08 * Math.sin(S.t * 10); g.save(); g.translate(W / 2, 194); g.scale(b, b); panel(-58, -11, 116, 18, '#ff8fc7'); text('NEW RECORD!', 0, 3, 11, '#2b2230', 'center', null); g.restore(); }
      }
    }
  }
  S.draw = function () {
    const cx = S.camX;
    g.drawImage(bgFar, -((cx * 0.2) % 320), 0); g.drawImage(bgFar, 320 - ((cx * 0.2) % 320), 0, 320, 240, 320 - ((cx * 0.2) % 320) + 0, 0, 320, 240);
    const mo = (cx * 0.5) % 640; g.drawImage(bgMid, -mo, 0); g.drawImage(bgMid, 640 - mo, 0);
    K.g = g;
    g.save(); if (S.shake > 0) g.translate(0, Math.sin(S.shake * 2.3) * 2);
    drawBoss();
    drawTiles();
    drawEnts();
    for (const p of players) drawPlayer(p);
    drawParticles(cx);
    g.restore();
    hud();
  };
  return S;
}


// ----------------------------------------------------------------------------- JETPACK SIDE ZONES (from the jpanel Jetpack game)
// No gravity: stick or A goes up, down goes down, letting go hovers. A bump knocks bolts loose and nothing worse.
const ZONES = {
  jet1: { name: 'JUNK PIPES', len: 1400, speed: 1.15, sky: ['#7fc8ff', '#cdeaff'], build(Z) {
    const gapsY = [120, 90, 150, 110, 70, 140, 100, 160];
    gapsY.forEach((gy, k) => {
      const x = 360 + k * 130;
      if (k % 3 === 2) Z.pillar(x, gy + 34);       // a junk pillar from the ground only
      else Z.pipes(x, gy, 78);
      for (let j = 0; j < 3; j++) Z.bolt(x + 10 + j * 14, gy + (k % 3 === 2 ? -10 : 0));
      Z.bolt(x + 75, (gy + gapsY[(k + 1) % gapsY.length]) / 2);
    });
  } },
};
function jetScene(parent, hatch) {
  const z = ZONES[hatch.zone], obs = [], items = [];
  const Z = {
    pipes(x, gy, gap) { obs.push({ x, y: -10, w: 26, h: gy - gap / 2 + 10, kind: 'pipeT' }, { x, y: gy + gap / 2, w: 26, h: 260, kind: 'pipeB' }); },
    pillar(x, top) { obs.push({ x, y: top, w: 26, h: 260, kind: 'pillar' }); },
    bolt(x, y) { items.push({ x: x - 5, y: y - 5, w: 10, h: 10, t: Math.random() * 6 }); },
  };
  z.build(Z);
  const ps = parent.players.map((p, k) => ({ src: p, x: 50, y: 90 + k * 40, w: 12, h: p.h - 4, vy: 0, inv: 40, face: 1 }));
  const J = { song: 'jet', t: 0, scroll: -40, bolts: 0, endT: -1, nameT: 110 };
  J.update = function () {
    J.t += 1 / 60; if (J.nameT > 0) J.nameT--; if (J.endT < 0) parent.timeF++;
    if (J.endT >= 0) { if (++J.endT > 50) parent.comeBack(hatch, J.bolts); stepParticles(); return; }
    J.scroll += z.speed * (little ? 1 : 1.3);
    for (const p of ps) {
      const i = p.src.slot, up = held(i, 'up') || held(i, 'a'), dn = held(i, 'down');
      p.vy += (((dn ? 1 : 0) - (up ? 1 : 0)) * 1.9 - p.vy) * 0.18;
      p.y = clamp(p.y + p.vy, 14, 224 - p.h);
      const dx = (held(i, 'right') ? 1 : 0) - (held(i, 'left') ? 1 : 0); if (dx) p.face = 1;
      p.x = clamp(p.x + dx * 1.3, 16, 220);
      if (p.inv > 0) p.inv--;
      const wb = { x: p.x + J.scroll, y: p.y, w: p.w, h: p.h };
      if (p.inv <= 0) for (const o of obs) if (overlap(wb, o)) {
        p.inv = 80; p.vy = p.y > o.y ? 2.5 : -2.5; SND.fx.wobble(); puff(p.x + 6, p.y + 10, 6);
        const drop = Math.min(3, J.bolts); J.bolts -= drop; for (let k = 0; k < drop; k++) sparks(p.x + 6 + J.scroll, p.y + 8, 2, '#ffd23f');
        break;
      }
      for (const it of items) if (!it.dead && overlap(wb, it)) { it.dead = true; J.bolts++; SND.fx.bolt(); sparks(it.x + 5, it.y + 5, 3, '#ffd23f'); }
      if (S_frame() % 3 === 0) particles.push({ x: p.x + J.scroll - 2, y: p.y + p.h, vx: -0.6, vy: 0.8, life: 16, max: 16, r: 1.5, col: 'rgba(255,200,120,0.9)', kind: 'puff' });
    }
    if (J.scroll > z.len) { J.endT = 0; SND.fx.open(); }
    stepParticles();
  };
  let fr = 0; const S_frame = () => fr++;
  J.draw = function () {
    K.g = g; const sc = J.scroll;
    const gr = g.createLinearGradient(0, 0, 0, H); gr.addColorStop(0, z.sky[0]); gr.addColorStop(1, z.sky[1]); g.fillStyle = gr; g.fillRect(0, 0, W, H);
    for (let k = 0; k < 5; k++) { const cx = ((k * 110 - sc * 0.2) % 550 + 550) % 550 - 60; K.rr(cx, 30 + (k % 3) * 26, 50, 14, 7, 'rgba(255,255,255,0.8)'); K.rr(cx + 12, 22 + (k % 3) * 26, 28, 14, 7, 'rgba(255,255,255,0.8)'); }
    g.fillStyle = '#9fb7a0'; g.beginPath(); g.moveTo(0, H); for (let x = 0; x <= W; x += 16) g.lineTo(x, 200 + Math.sin((x + sc * 0.5) * 0.03) * 8); g.lineTo(W, H); g.fill();
    g.fillStyle = '#8a5233'; g.fillRect(0, 224, W, 16); g.fillStyle = '#c98b55'; g.fillRect(0, 224, W, 3);
    for (const o of obs) {
      const x = o.x - sc; if (x < -40 || x > W + 10) continue;
      if (o.kind === 'pillar') {
        for (let y = o.y; y < 224; y += 14) { const k = Math.floor((y - o.y) / 14) % 2; if (k) K.rr(x + 1, y, 24, 13, 2, '#b07a44'); else { K.rr(x, y, 26, 13, 6, '#2b2833'); K.rr(x + 8, y + 4, 10, 5, 2, '#4a4652'); } }
      } else {
        const top = o.kind === 'pipeT', lipY = top ? o.y + o.h - 10 : o.y;
        K.rr(x + 2, o.y, 22, o.h, 2, '#7d9a8a'); g.fillStyle = '#5f7a6c'; g.fillRect(x + 18, o.y, 4, o.h); g.fillStyle = '#a5c4b2'; g.fillRect(x + 5, o.y, 3, o.h);
        K.rr(x - 2, lipY, 30, 10, 2, '#8fb09c'); g.fillStyle = '#c0703a'; g.fillRect(x + 6, lipY + 3, 6, 3); K.circ(x + 3, lipY + 5, 1.3, '#5f7a6c'); K.circ(x + 23, lipY + 5, 1.3, '#5f7a6c');
      }
    }
    for (const it of items) { if (it.dead) continue; const x = it.x - sc; if (x < -10 || x > W + 10) continue; it.t += 1 / 60; g.save(); g.translate(x + 5, it.y + 5); g.scale(Math.abs(Math.cos(it.t * 5)) * 0.8 + 0.2, 1); K.nut(0, 0, 4.2, 0, '#ffd23f'); g.restore(); }
    const endX = z.len + 260 - sc; if (endX < W + 30) { K.rr(endX - 6, 60, 40, 120, 8, '#2b2833'); K.rr(endX, 68, 28, 104, 6, '#1b1426'); text('EXIT', endX + 14, 54, 9, '#ffd23f'); }
    for (const p of ps) {
      if (p.inv > 0 && Math.floor(p.inv / 4) % 2 === 0) continue;
      const fx = p.x + p.w / 2, fy = p.y + p.h + 4, src = p.src, t = J.t + src.i;
      if (src.hero === 'robot') { K.rr(fx - 9, fy - 22, 6, 11, 2, '#9aa1b0'); K.flame(fx - 6, fy - 10, 30, 14, t); K.robotSide(fx, fy, 0.074, src.hex, Object.assign(K.pose('robot', 'jet', t), { dir: 1, rot: 0.15 })); }
      else { K.rr(fx - 10, fy - 22, 7, 11, 2, '#9aa1b0'); K.flame(fx - 6.5, fy - 10, 30, 14, t); K.ostrichSide(fx, fy, 0.071, src.hex, Object.assign(K.pose('ostrich', 'jump', t), { dir: 1, rot: 0.2 })); }
    }
    drawParticles(sc);
    panel(4, 4, 62, 15); K.nut(13, 11.5, 4, J.t, '#ffd23f'); text('×' + (parent.lv.bolts + J.bolts), 20, 16, 9, '#fff6d8', 'left', null);
    if (J.nameT > 0) { g.globalAlpha = Math.min(1, J.nameT / 30); panel(W / 2 - 90, 92, 180, 44); text(z.name, W / 2, 112, 11, '#ffd23f'); text('▲ / A to fly up   ▼ to go down', W / 2, 128, 8, '#fff6d8', 'center', null); g.globalAlpha = 1; }
    if (J.endT >= 0) { g.fillStyle = 'rgba(0,0,0,' + Math.min(1, J.endT / 40) + ')'; g.fillRect(0, 0, W, H); }
  };
  particles = [];
  return J;
}

function pauseScene(level) {
  return {
    song: null, t: 0,
    update() { this.t += 1 / 60; if (anyHit('start') || anyHit('a')) { SND.fx.select(); scene = level; } if (anyHit('b')) { SND.fx.select(); go(mapScene); } },
    draw() { level.draw(); g.fillStyle = 'rgba(15,10,25,0.6)'; g.fillRect(0, 0, W, H); panel(80, 80, 160, 70); text('PAUSED', W / 2, 106, 14, '#ffd23f'); text('A: keep playing   B: back to map', W / 2, 130, 8, '#fff6d8', 'center', null);
      if (level.lv && !level.lv.def.boss && !level.lv.def.bonus) text('Bolts ' + level.lv.boltGot + '/' + level.lv.boltTotal + '    Glitchies left ' + level.foesLeft(), W / 2, 144, 7, '#d9d2e6', 'center', null); },
  };
}

// ----------------------------------------------------------------------------- MAIN LOOP
let acc = 0, last = performance.now(), failed = null;
function loop(nowMs) {
  if (failed) return;
  try {
    acc += Math.min(100, nowMs - last); last = nowMs;
    while (acc >= 1000 / 60) { pollPads(); scene.update(); acc -= 1000 / 60; }
    scene.draw();
  } catch (err) {
    failed = err; console.error(err);
    g.fillStyle = 'rgba(20,10,20,0.9)'; g.fillRect(8, 80, 304, 80);
    g.fillStyle = '#ffd23f'; g.font = '10px sans-serif'; g.textAlign = 'left';
    g.fillText('Game error (please screenshot this):', 16, 100);
    g.fillStyle = '#fff'; g.fillText(String(err && err.message || err).slice(0, 58), 16, 118);
    const st = String(err && err.stack || '').split('\n'); g.fillText((st[1] || '').trim().slice(0, 58), 16, 134); g.fillText((st[2] || '').trim().slice(0, 58), 16, 150);
    return;
  }
  requestAnimationFrame(loop);
}

// ----------------------------------------------------------------------------- PAGE CONTROLS
const muteB = document.getElementById('mute'), diffB = document.getElementById('diff'), touchB = document.getElementById('touchToggle');
function syncButtons() {
  muteB.textContent = SND.muted ? 'Sound off' : 'Sound on'; muteB.setAttribute('aria-pressed', SND.muted);
  diffB.textContent = little ? 'Little mode' : 'Big mode'; diffB.setAttribute('aria-pressed', little);
}
muteB.addEventListener('click', () => { SND.resume(); SND.setMuted(!SND.muted); syncButtons(); });
diffB.addEventListener('click', () => { little = !little; store.set('little', little); syncButtons(); });
const coarse = matchMedia('(pointer: coarse)').matches;
let touchOn = store.get('touch', coarse);
function syncTouch() { touchEl.classList.toggle('on', touchOn); touchB.setAttribute('aria-pressed', touchOn); }
touchB.addEventListener('click', () => { touchOn = !touchOn; store.set('touch', touchOn); syncTouch(); });
document.getElementById('full').addEventListener('click', () => { const el = document.getElementById('wrap'); try { const r = (el.requestFullscreen || el.webkitRequestFullscreen).call(el); if (r && r.catch) r.catch(() => {}); } catch (e) { /* not available */ } });
document.getElementById('wipe').addEventListener('click', () => { for (let n = 1; n <= LAST_WORLD; n++) store.set('w' + n, fresh()); setWorld(1); if (scene === mapScene) buildMap(); });
syncButtons(); syncTouch();

window.__ck = { get scene() { return scene; }, go, introScene, buildLevel, levelScene, mapScene, get prog() { return prog; }, LEVELS, setWorld, enterWorld, cutScene, get world() { return worldNum; } };
// The level editor runs this game in a frame and sends the level it is editing to play-test.
window.addEventListener('message', e => {
  const d = e.data;
  if (!d || d.type !== 'clankers:play' || typeof d.text !== 'string') return;
  try {
    if (d.party) d.party.forEach((m, i) => Object.assign(party[i], m));
    prog.lives = 3; testPower = d.power || null;
    const wf = WORLD_FILES[parseInt(String(d.world || '').replace(/\D/g, ''), 10)];   // the folder the level is in picks its look
    go(levelScene('test', false, levelDef(d.text, wf ? wf.meta.theme : WORLD_FILE.meta.theme))); SND.resume();
  } catch (err) { console.error(err); }
});
const params = new URLSearchParams(location.search); if (window.CLANKERS_TEST) params.set('test', '1');
if (params.has('test')) document.body.classList.add('test');
const startScene = {
  song: null, t: 0,
  update() { this.t += 1 / 60; if (anyInputYet && this.t > 0.2) { introScene.t = 0; go(introScene); } },
  draw() {
    const t = this.t; K.g = g; g.fillStyle = '#141a3d'; g.fillRect(0, 0, W, H);
    for (let i = 0; i < 40; i++) { const x = (i * 97) % 320, y = (i * 53) % 200; if (Math.sin(t * 2 + i) > -0.3) K.circ(x, y, ((i % 3) + 1) * 0.5, '#fff6d8'); }
    K.robot(118, 168, 0.11, COLS[party[0].col], { t, bob: Math.sin(t * 3) * 3, armR: -150 + Math.sin(t * 6) * 25, eyes: { happy: 0.6 } });
    K.ostrich(204, 168, 0.1, COLS[party[1].col], { t, bob: Math.abs(Math.sin(t * 2.6)) * 8, flick: Math.sin(t * 2.6) * 6 });
    g.fillStyle = '#3a2630'; g.fillRect(0, 168, W, 72);
    text('CLANKERS', W / 2, 62, 26, '#ffd23f');
    const k = 0.5 + 0.5 * Math.sin(t * 4); g.globalAlpha = 0.6 + 0.4 * k;
    panel(W / 2 - 100, 186, 200, 26, '#ffd23f'); text('TAP OR PRESS ANY BUTTON', W / 2, 203, 10, '#2b2230', 'center', null); g.globalAlpha = 1;
    text('turn the sound on!', W / 2, 228, 7, '#d9d2e6', 'center', null);
  },
};
function boot() { if (params.has('test')) { scene = { song: null, update() {}, draw() { g.fillStyle = '#15131c'; g.fillRect(0, 0, W, H); text('Waiting for a level...', W / 2, H / 2, 10); } }; } else go(anyInputYet ? introScene : startScene); requestAnimationFrame(t => { last = t; loop(t); }); }
(document.fonts && document.fonts.load ? document.fonts.load('10px "Titan One"').catch(() => {}) : Promise.resolve()).then(boot, boot);
})();
