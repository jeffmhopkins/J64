#!/usr/bin/env node
// Bakes the N64 sprite sheets from the browser game's own drawing code.
//
//     node roms/clankers/tools/bake_sprites.mjs
//
// Loads web/lib (the drawing kit, Glitchies, map themes, title), the data files and the font
// into headless Chromium, draws every frame the N64 needs onto canvases, and writes:
//   assets/*.png        sprite strips (frames side by side), packed as RGBA32
//   assets/rgba16/*.png big pictures and hero sheets, packed as RGBA16 to save memory
//   src/n64/sprites.h   frame sizes and anchor points for each strip
// Re-run it after changing draw.js, a theme, or a world file, and commit the results.
// Needs Node and Playwright with Chromium (a dev-machine tool, not part of `make`).
import { createRequire } from 'module';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const require = createRequire(import.meta.url);
const { chromium } = require('playwright');

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROM = path.resolve(HERE, '..');
const WEB = path.join(ROM, 'web');
const DATA = path.join(ROM, 'data');
const ASSETS = path.join(ROM, 'assets');

const read = p => fs.readFileSync(path.join(WEB, p), 'utf8');
const themes = fs.readdirSync(path.join(WEB, 'themes')).filter(f => f.endsWith('.js')).sort().map(f => read('themes/' + f));
const engine = [read('lib/draw.js'), read('lib/ow.js'), read('lib/data.js'), read('lib/levelart.js'), ...themes, read('lib/bossart.js'), read('lib/cutart.js'), read('lib/houseart.js'), read('lib/furn34.js'), read('lib/housetop.js'), read('lib/title.js'), read('lib/glitchies.js')].join('\n');
const files = {};
(function walk(dir) {
  for (const f of fs.readdirSync(dir).sort()) {
    const p = path.join(dir, f);
    if (fs.statSync(p).isDirectory()) walk(p);
    else if (f.endsWith('.txt')) files[path.relative(DATA, p).split(path.sep).join('/')] = fs.readFileSync(p, 'utf8');
  }
})(DATA);
const font = fs.readFileSync(path.join(ASSETS, 'TitanOne.ttf')).toString('base64');

// ------------------------------------------------------------------------------------------
// Everything below runs inside the page.
function bakeAll() {
  const K = CK, P = Math.PI, out = [];
  const COLS = [0x5AD1FF, 0xFFD23F, 0xFF7A59, 0x49F08A, 0xC28BFF, 0xFF8FC7, 0x7FA7C9, 0xF5F1E8];
  const FONT = '"Titan One", "Arial Black", Impact, sans-serif';
  // A strip of n frames, each fw x fh; draw(g, i) draws frame i with (0,0) at the anchor.
  function strip(name, fw, fh, ax, ay, n, draw, fmt = 'rgba32', cols = n) {
    const rows = Math.ceil(n / cols), cv = document.createElement('canvas');
    cv.width = fw * cols; cv.height = fh * rows;
    const g = cv.getContext('2d');
    for (let i = 0; i < n; i++) {
      g.save(); g.translate((i % cols) * fw, Math.floor(i / cols) * fh);
      g.beginPath(); g.rect(0, 0, fw, fh); g.clip();
      g.translate(ax, ay); K.g = g; draw(g, i); g.restore();
    }
    out.push({ name, fw, fh, ax, ay, n, cols, fmt, png: cv.toDataURL('image/png') });
  }
  const text = (g, s, x, y, size, col, outline) => {
    g.font = `${size}px ${FONT}`; g.textAlign = 'center'; g.textBaseline = 'alphabetic';
    if (outline) { g.lineJoin = 'round'; g.strokeStyle = outline; g.lineWidth = Math.max(2, size / 3); g.strokeText(s, x, y); }
    g.fillStyle = col; g.fillText(s, x, y);
  };

  // ---- heroes: one sheet per hero and colour, 42 frames of 64x56, feet at (32, 44)
  // Frame list (the N64 renderer uses the same numbers, see src/n64/draw.c):
  //  0-3 idle   4-11 run   12 jump   13-16 jet (robot) / flap (ostrich)   17-18 wobble
  //  19 crouch  20 slide   21-23 B action   24 shoot   25-26 cheer   27 dead   28 in a bubble
  //  29-30 jetpack zone    31 spare   32-35 walk toward us   36-39 walk away   40-41 idle from behind
  function heroFrame(hero, hex, i) {
    const R = hero === 'robot', rs = 0.074, os = 0.071;
    const side = o => R ? K.robotSide(0, 0, rs, hex, Object.assign(o, { dir: 1 })) : K.ostrichSide(0, 0, os, hex, Object.assign(o, { dir: 1 }));
    const front = o => R ? K.robot(0, 0, rs, hex, o) : K.ostrich(0, 0, os, hex, o);
    if (i < 4) { const t = R ? i * 0.52 : i * 0.3 + 0.05; return front(R ? { t: t + 0.3, bob: Math.sin(t * 3) * 3 } : { t: t + 0.3, bob: Math.abs(Math.sin(t * 2.6)) * 8, flick: Math.sin(t * 2.6) * 6 }); }
    if (i < 12) { const k = i - 4, f = k / 8 * P * 2, t = R ? f / 11 : f / 10; return side(K.pose(hero, 'run', t)); }
    if (i === 12) return side(K.pose(hero, 'jump', 0.3));
    if (i < 17) {
      const k = i - 13;
      if (R) return side(K.pose('robot', 'jet', 0.3 + (k % 2) * 0.05));
      return side(K.pose('ostrich', 'flap', k / 4 * (P * 2 / 13)));
    }
    if (i < 19) return side(K.pose(hero, 'wobble', i === 17 ? P / 24 : 3 * P / 24));
    if (i === 19) return side(R ? { t: 0.3, sy: 0.62, sx: 1.12, bob: 20, legF: -35, kneeF: 70, legB: 30, kneeB: 70 } : { t: 0.3, sy: 0.5, sx: 1.1, legF: -60, kneeF: 140, legB: -40, kneeB: 130, lean: 14 });
    if (i === 20) return side(R ? { t: 0.3, rot: -1.15, legF: -95, kneeF: 10, legB: -80, kneeB: 20, armF: -170, armB: -150, eyes: { happy: 0.6 } } : { t: 0.3, rot: 0.95, sy: 0.9, legF: 95, kneeF: 20, legB: 80, kneeB: 10, beakOpen: 0.6, flick: 20 });
    if (i < 24) {
      const actT = [19, 14, 9][i - 21], k = Math.sin((22 - actT) / 22 * P);
      if (R) return side({ t: 0.3, grab: true, reach: k * 330, clawOpen: actT > 12 ? 0.7 : 0.05, legF: 0, legB: 0 });
      return side({ t: 0.3, legF: -95 * Math.min(1, k * 2), legLenF: 92 + k * 330, kneeF: 0, toeF: -90, legB: 10 * k, rot: -0.12 * k, lean: -12 * k, beakOpen: k > 0.5 ? 0.7 : 0 });
    }
    if (i === 24) return side(R ? { t: 0.3, armF: -90, eyes: { happy: 0.3 } } : { t: 0.3, beakOpen: 0.9, lean: -6 });
    if (i < 27) { const t = 0.3 + (i - 25) * (R ? P / 18 : P / 12); return front(R ? { t, armL: 150 + Math.sin(t * 9) * 20, armR: -150 - Math.sin(t * 9) * 20, eyes: { happy: 0.8 }, mouth: 'o' } : { t, bob: Math.abs(Math.sin(t * 6)) * 14, eyes: { happy: 0.8 }, beakOpen: 0.8 }); }
    if (i === 27) return R ? K.robot(0, 0, rs, hex, { t: 0.3, eyes: { open: 0.15 }, mouth: 'wavy', armL: 160, armR: -160, stars: true }) : K.ostrich(0, 0, os, hex, { t: 0.3, eyes: { open: 0.15 }, beakOpen: 0.6, stars: true });
    if (i === 28) return R ? K.robot(0, 0, rs, hex, { t: 0.3, eyes: { happy: 0.4 } }) : K.ostrich(0, 0, os, hex, { t: 0.3 });
    if (i < 31) {
      const t = 0.3 + (i - 29) * 0.07, fy = 4;   // the jetpack zone draws the hero 4 px above its feet
      if (R) { K.rr(-9, fy - 22, 6, 11, 2, '#9aa1b0'); K.flame(-6, fy - 10, 30, 14, t); K.robotSide(0, fy, 0.074, hex, Object.assign(K.pose('robot', 'jet', t), { dir: 1, rot: 0.15 })); }
      else { K.rr(-10, fy - 22, 7, 11, 2, '#9aa1b0'); K.flame(-6.5, fy - 10, 30, 14, t); K.ostrichSide(0, fy, 0.071, hex, Object.assign(K.pose('ostrich', 'jump', t), { dir: 1, rot: 0.2 })); }
      return;
    }
    if (i < 40) {   // the house's walks up and down the screen: a quarter of a stride a frame
      const k = (i - 32) % 4, t = k / 4 * P * 2 / (R ? 11 : 10);
      return front(K.pose(hero, i < 36 ? 'walkFront' : 'walkBack', t));
    }
    if (i < 42) { const t = 0.3 + (i - 40) * (R ? 0.52 : 0.6); return front(R ? { t, back: true, bob: Math.sin(t * 3) * 3 } : { t, back: true, bob: Math.abs(Math.sin(t * 2.6)) * 8 }); }
    return front({ t: 0.3 });
  }
  const HERO_FRAMES = 42, isBack = i => i >= 36 && i < 42;
  for (const hero of ['robot', 'ostrich'])
    for (let c = 0; c < COLS.length; c++)
      strip(`hero_${hero}_${c}`, 64, 56, 32, 44, HERO_FRAMES, (g, i) => heroFrame(hero, COLS[c], i), 'rgba16', 8);

  // ---- accessories from the parts shop. The hero views call K.onWear(kind, where) as they draw:
  // 'head' (hats, face and neck pieces hang off the head's top middle), 'body' (the torso box) and
  // 'foot' (each foot). Where those land in every hero frame goes into sprites.h as ACC_AT; each
  // item is drawn once per view (robot front, side and back, then the ostrich's) into a strip
  // per slot, anchored at the same point, and the N64 places, turns and stretches it to match.
  // Items that cycle colour are drawn white (K.plain) so the N64 can tint them.
  const ACC_AT = [], REF = {};
  const pt = (m, x, y, s0) => { const q = m.transformPoint(new DOMPoint(x, y)); return { x: q.x, y: q.y, rot: Math.atan2(m.b, m.a), sx: Math.hypot(m.a, m.b) / s0, sy: Math.hypot(m.c, m.d) / s0 }; };
  for (const hero of ['robot', 'ostrich']) {
    const s0 = hero === 'robot' ? 0.074 : 0.071, rows = [];
    for (let i = 0; i < HERO_FRAMES; i++) {
      const cv = document.createElement('canvas'); cv.width = 64; cv.height = 56;
      const g = cv.getContext('2d'); g.translate(32, 44); K.g = g;
      const base = g.getTransform().inverse(), at = { head: null, body: null, feet: [], arms: [], side: 0 };
      K.onWear = (kind, a) => {
        const m = base.multiply(g.getTransform()), side = isBack(i) ? 2 : a.side ? 1 : 0, key = hero + side;
        REF[key] = REF[key] || {};
        if (kind === 'head') { at.head = pt(m, a.cx, a.top, s0); at.side = side; REF[key].head = REF[key].head || a; }
        else if (kind === 'body') { at.body = pt(m, a.x + a.w / 2, a.y + a.h / 2, s0); REF[key].body = REF[key].body || a; }
        else if (kind === 'arm' && at.arms.length < 2) {
          // a sleeve: the same stroke K.wearSleeve draws, from the shoulder 72% down the arm
          const r = a.deg * Math.PI / 180, l = a.ln * 0.72, p0 = m.transformPoint(new DOMPoint(a.x, a.y)), p1 = m.transformPoint(new DOMPoint(a.x - Math.sin(r) * l, a.y + Math.cos(r) * l));
          at.arms.push({ x0: p0.x, y0: p0.y, x1: p1.x, y1: p1.y, w: a.w * 1.3 * Math.hypot(m.a, m.b), far: a.far ? 1 : 0 });
        }
        else if (kind === 'foot' && at.feet.length < 2) { at.feet.push(Object.assign(pt(m, a.x, a.y, s0), { far: a.far ? 1 : 0 })); REF[key].foot = REF[key].foot || a; }
      };
      heroFrame(hero, COLS[0], i);
      K.onWear = null;
      rows.push(at);
    }
    ACC_AT.push(rows);
  }
  // one strip per slot: frame = view * (items in the slot) + item, view = hero * 3 + side (0 front, 1 side, 2 back)
  const SLOT_FRAME = { head: [32, 20, 16, 15], face: [28, 16, 14, 0], neck: [28, 24, 14, 0], body: [18, 16, 9, 8], feet: [12, 10, 6, 5] };
  for (const slot of K.ACC_SLOTS) {
    const items = K.ACC.filter(it => it.slot === slot), [fw, fh, ax, ay] = SLOT_FRAME[slot];
    strip('acc_' + slot, fw, fh, ax, ay, 6 * items.length, (g, i) => {
      const it = items[i % items.length], view = Math.floor(i / items.length), hero = view < 3 ? 'robot' : 'ostrich', R = REF[hero + (view % 3)];
      if (!R) return;
      K.back = view % 3 === 2;
      if (K.back && slot === 'face') { K.back = false; return; }   // no face pieces from behind
      const s0 = hero === 'robot' ? 0.074 : 0.071;
      K.plain = !!it.rainbow;
      g.scale(s0, s0);
      if (slot === 'body') { const b = R.body; g.translate(-(b.x + b.w / 2), -(b.y + b.h / 2)); K.wearBody(it.id, b, 0.3, !!b.side); }
      else if (slot === 'feet') { const f = R.foot; g.translate(-f.x, -f.y); K.wearFoot(it.id, f.x, f.y, f.s, 0.3, f.side, false); }
      else {
        const a = R.head; g.translate(-a.cx, -a.top);
        if (slot === 'head') K.wearHead(it.id, a.cx, a.top, a.w, 0.3, a.side);
        else if (slot === 'face') K.wearFace(it.id, a.eyes, a.side);
        else K.wearNeck(it.id, a.neck[0], a.neck[1], a.neck[2], a.side ? 1 : 0);
      }
      K.plain = false; K.back = false;
    }, 'rgba16', items.length);
  }
  out.accAt = ACC_AT;

  // ---- Glitchies: 8 frames per row of 32x36, feet at (16, 32)
  // rows: bumble, sprocket, grumble (frames 0-5 glitchy, 6-7 happy), then the shut Grumble Crate
  // (0-3 shut with dizzy stars, 4 kicked along the ground, 5 kicked in the air)
  strip('glitch', 32, 36, 16, 32, 32, (g, i) => {
    const row = Math.floor(i / 8), k = i % 8;
    if (row < 3) { const gl = k < 6; GLITCH[row].draw(g, 0, 0, 0.085, gl ? k / 12 + (k % 3) * 0.6 : 100 + (k - 6) * 1.7, gl); return; }
    if (k > 5) return;
    const e = { w: 14, h: 14, y: -14, state: k < 4 ? 'shell' : 'kicked', shellT: 0, dir: 1, ground: k === 4 };
    shutCrate(g, e, -7, k / 4 * P / 2);
  }, 'rgba32', 8);
  function shutCrate(g, e, x, t) {
    const cx = x + e.w / 2, by = e.y + e.h;
    if (e.state === 'kicked' && e.ground) { g.globalAlpha = 0.5; g.fillStyle = '#fff6d8'; for (let k = 1; k <= 3; k++) g.fillRect(cx - e.dir * (6 + k * 4), by - 3 - k * 3, 4, 1.5); g.globalAlpha = 1; }
    K.rr(cx - 7, by - 12, 14, 12, 2, '#c08850'); g.strokeStyle = '#8a5a30'; g.lineWidth = 1; g.strokeRect(cx - 5.5, by - 10.5, 11, 9);
    K.rr(cx - 8, by - 14, 16, 4, 1.5, '#d9a46a'); g.fillStyle = '#8a5a30'; g.fillRect(cx - 8, by - 10.5, 16, 1);
    g.strokeStyle = '#2a1d12'; g.lineWidth = 1.1;
    for (const ex of [-3, 3]) { g.beginPath(); g.moveTo(cx + ex - 1.5, by - 8.5); g.lineTo(cx + ex + 1.5, by - 5.5); g.moveTo(cx + ex + 1.5, by - 8.5); g.lineTo(cx + ex - 1.5, by - 5.5); g.stroke(); }
    if (e.state !== 'kicked') for (let k = 0; k < 2; k++) { const a = t * 4 + k * P; K.star(cx + Math.cos(a) * 8, by - 17 + Math.sin(a) * 2, 1.8, '#fff6a0'); }
  }

  // ---- level tiles, 16x16, one strip per world's look (tiles_0 junkyard, 1 factory, 2 mountain,
  //      3 moon; the order of THEME_* in src/core/ck.h). Frames as in web/lib/levelart.js:
  //      0-3 ground top, 4-7 ground inside, 8 girder, 9 plank, 10 crate, 11 crate with a prize,
  //      12 door, 13 door with its stripe, 14-17 conveyor belt moving right
  const THEME_ORDER = ['junkyard', 'factory', 'mountain', 'moon'];
  THEME_ORDER.forEach((th, k) => strip('tiles_' + k, 16, 16, 0, 0, LEVELART.TILE_FRAMES, (g, i) => LTHEMES[th].tile(g, i)));

  // ---- things in a level. Anchors put (0,0) at the entity's top-left corner, like the game's boxes.
  strip('bolt', 14, 14, 7, 7, 1, () => K.nut(0, 0, 4.2, 0, '#ffd23f'));   // anchored at the centre, so it can spin
  // The gear is a plain hexagon on the N64: fine teeth turn into a blurry sun at this size on a TV.
  strip('gear', 20, 20, 10, 10, 1, (g) => {
    const hex = (r) => { g.beginPath(); for (let k = 0; k < 6; k++) { const a = k * Math.PI / 3; g[k ? 'lineTo' : 'moveTo'](Math.cos(a) * r, Math.sin(a) * r); } g.closePath(); };
    hex(9); g.fillStyle = '#7a5600'; g.fill();
    hex(7.6); g.fillStyle = '#ffd23f'; g.fill();
    g.fillStyle = '#fff1a8'; g.beginPath(); g.moveTo(-3.8, -6.6); g.lineTo(3.8, -6.6); g.lineTo(2.6, -4.6); g.lineTo(-2.6, -4.6); g.closePath(); g.fill();
    g.fillStyle = '#7a5600'; g.beginPath(); g.arc(0, 0, 2.8, 0, Math.PI * 2); g.fill();
  });
  strip('power', 16, 16, 2, 2, 4, (g, i) => { K.rr(0, 0, 12, 12, 5, '#e8e4f0'); K.rr(1, 1, 10, 10, 4, '#3b2a5c'); powerIcon(g, ['mega', 'rainbow', 'bubble', 'magnet'][i], 6, 6, 0); });
  strip('picon', 14, 14, 7, 7, 5, (g, i) => powerIcon(g, ['mega', 'rainbow', 'bubble', 'magnet', 'gun'][i], 0, 0, 0));
  strip('tyre', 16, 10, 0, 1, 3, (g, i) => { const sq = i / 2, hh = 9 * (1 - sq * 0.4); K.rr(0, 9 - hh, 16, hh, hh / 2, '#2b2833'); K.rr(4, 9 - hh + hh * 0.3, 8, hh * 0.4, 2, '#4a4652'); g.fillStyle = '#5b5666'; g.fillRect(2, 9 - hh + 1, 12, 1); });
  strip('lever', 24, 26, 7, 12, 2, (g, i) => {
    K.rr(-1, 8, 12, 6, 1.5, '#5d566a');
    g.save(); g.translate(5, 10); g.rotate(i ? 0.7 : -0.7); g.fillStyle = '#9aa1b0'; g.fillRect(-1, -12, 2, 12); K.circ(0, -12, 3, i ? '#49d17a' : '#e04f4f'); g.restore();
  });
  strip('sign', 20, 24, 4, 3, 1, g => {
    g.fillStyle = '#7d5128'; g.fillRect(5, 8, 2, 12); K.rr(-3, -2, 18, 13, 2, '#d9a46a'); g.strokeStyle = '#7d5128'; g.lineWidth = 1; g.strokeRect(-2.5, -1.5, 17, 12);
    g.fillStyle = '#2b2230'; g.beginPath(); g.moveTo(1, 2); g.lineTo(11, 2); g.lineTo(6, 8.5); g.fill();
  });
  strip('flag', 20, 40, 1, 3, 3, (g, i) => {
    const on = i > 0, px = 2, top = 0, wave = i === 1 ? 2 : -2;
    g.fillStyle = '#5d6378'; g.fillRect(px, top, 2, 34); K.circ(px + 1, top, 2.2, on ? '#ffd23f' : '#9aa0b4');
    g.fillStyle = on ? '#49d17a' : '#8a8fa3'; g.beginPath(); g.moveTo(px + 2, top + 2);
    if (on) { g.lineTo(px + 15, top + 6 + wave); g.lineTo(px + 2, top + 12); } else { g.lineTo(px + 6, top + 14); g.lineTo(px + 2, top + 13); }
    g.fill(); if (on) K.nut(px + 7, top + 7 + wave / 2, 2.2, 0, '#fff6d8');
  });
  strip('hatch', 26, 8, 1, 1, 2, (g, i) => {
    g.fillStyle = '#2b2833'; g.beginPath(); g.ellipse(12, 3, 12, 3.5, 0, 0, P * 2); g.fill();
    g.fillStyle = i ? '#1b1426' : '#7d8397'; g.beginPath(); g.ellipse(12, 2.5, 10, 2.6, 0, 0, P * 2); g.fill();
    if (!i) for (let k = 0; k < 6; k++) { const a = k / 6 * P * 2; K.circ(12 + Math.cos(a) * 8, 2.5 + Math.sin(a) * 2, 0.8, '#ffd23f'); }
  });
  strip('arrow', 14, 9, 1, 1, 1, g => { g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(0, 0); g.lineTo(12, 0); g.lineTo(6, 7); g.closePath(); g.fill(); g.strokeStyle = '#2b2230'; g.lineWidth = 1; g.stroke(); });
  strip('pad', 26, 10, 1, 2, 1, g => {
    K.rr(0, 0, 24, 6, 2, '#3a8de0'); K.rr(2, 1, 20, 2, 1, '#8fd3ff');
    g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(13, -1); g.lineTo(9, 3.5); g.lineTo(12, 3.5); g.lineTo(10, 7); g.lineTo(15, 2); g.lineTo(12, 2); g.fill();
  });
  strip('button', 18, 10, 2, 2, 2, (g, i) => {
    if (i) { K.rr(0, 4, 14, 2, 1, '#5d566a'); return; }
    K.rr(-1, 3, 16, 4, 1, '#5d566a'); g.fillStyle = '#ff4d4d'; g.beginPath(); g.ellipse(7, 4, 6, 5, 0, P, 0); g.fill();
  });
  // the crane boss: tower and arm (without the cab), the cab's faces, the magnet
  strip('crane', 160, 196, -140, 0, 1, g => {
    const tx = 286;
    g.fillStyle = '#d4892f'; g.fillRect(tx - 6, 24, 12, 168); g.strokeStyle = '#a5651f'; g.lineWidth = 1.5;
    for (let y = 24; y < 190; y += 10) { g.beginPath(); g.moveTo(tx - 6, y); g.lineTo(tx + 6, y + 10); g.stroke(); }
    g.fillStyle = '#d4892f'; g.fillRect(150, 18, tx - 144, 8); g.fillStyle = '#a5651f'; g.fillRect(150, 24, tx - 144, 2);
    K.circ(160, 24, 3, '#5d566a');
  });
  strip('cab', 36, 28, 18, 0, 3, (g, i) => {
    const fixed = i === 2, ex = 0, ey = 12;
    K.rr(-16, 2, 32, 24, 5, fixed ? '#49d17a' : '#e04f4f');
    if (i === 1) { text(g, 'X', ex - 7, ey + 5, 9, '#fff'); text(g, 'X', ex + 7, ey + 5, 9, '#fff'); }
    else { K.eye(ex - 7, ey, 9, 11, '#2b2230', fixed ? '#49d17a' : '#e04f4f', { angry: fixed ? 0 : 1, happy: fixed ? 0.5 : 0 }); K.eye(ex + 7, ey, 9, 11, '#2b2230', fixed ? '#49d17a' : '#e04f4f', { angry: fixed ? 0 : -1, happy: fixed ? 0.5 : 0 }); }
    K.mouth(fixed ? 'smile' : 'zig', ex, ey + 6, 5, 1.5, '#2b2230');
  });
  strip('magnet', 28, 20, 14, 10, 1, g => { K.arc(0, 2, 9, P * 1.05, P * 1.95, 6, '#e04f4f'); g.fillStyle = '#e8e4f0'; g.fillRect(-12, 0, 6, 4); g.fillRect(6, 0, 6, 4); });
  // the bosses of Worlds 2 to 4 (web/lib/bossart.js), one frame per face:
  // 0 glitchy, 1 ouch, 2 fixed, 3 dizzy
  strip('boss_stamper', 52, 34, 26, 2, 4, (g, i) => BOSSART.stamper(g, i));
  strip('boss_rock', 44, 44, 22, 22, 1, g => BOSSART.boulderRock(g));
  strip('boss_rockface', 32, 24, 16, 12, 4, (g, i) => BOSSART.boulderFace(g, i));
  strip('boss_hauler', 80, 40, 40, 20, 4, (g, i) => BOSSART.hauler(g, i));

  // ---- baby bots, one strip per colour any level or world uses: 0-1 waving, 2-3 cheering
  const babies = new Set();
  for (const [f, txt] of Object.entries(DATA)) {
    if (f.endsWith('world.txt')) { const m = /^babies:\s*(.*)$/m.exec(txt); if (m) m[1].trim().split(/\s+/).forEach(h => babies.add(h.toUpperCase())); }
    else { const m = /^baby:\s*(\S+)/m.exec(txt); if (m) babies.add(m[1].toUpperCase()); }
  }
  for (const hex of [...babies].sort())
    strip('baby_' + hex, 24, 26, 12, 23, 4, (g, i) => {
      const h = parseInt(hex, 16), cheer = i > 1, t = 0.3 + (i % 2) * 0.26;
      K.baby(0, 0, 0.06, h, { t, armL: cheer ? 150 : 30, armR: cheer ? -150 : -150 + (i % 2 ? -20 : 20), mouth: cheer ? 'o' : 'smile', eyes: { happy: 0.5 } });
    });

  // ---- white shapes the N64 tints: a soft dot, a ring, a star, a pixel (for lines and confetti)
  strip('dot', 16, 16, 8, 8, 1, () => K.circ(0, 0, 7.5, '#ffffff'));
  strip('ring', 24, 24, 12, 12, 1, g => { g.strokeStyle = '#ffffff'; g.lineWidth = 1.5; g.beginPath(); g.arc(0, 0, 10.5, 0, P * 2); g.stroke(); });
  strip('star', 20, 20, 10, 10, 1, () => K.star(0, 0, 9.5, '#ffffff'));
  strip('px', 4, 4, 2, 2, 1, g => { g.fillStyle = '#fff'; g.fillRect(0, 0, 4, 4); });

  // ---- HUD bits
  strip('battery', 14, 8, 0, 0, 2, (g, i) => {
    const full = i === 0, x = 0, y = 0;
    K.rr(x, y, 11, 7, 1.5, full ? '#2b2833' : 'rgba(43,40,51,0.6)'); K.rr(x + 11, y + 2, 2, 3, 0.5, full ? '#2b2833' : 'rgba(43,40,51,0.6)');
    if (full) { K.rr(x + 1.5, y + 1.5, 8, 4, 1, '#49f08a'); g.fillStyle = '#fff'; g.globalAlpha = 0.5; g.fillRect(x + 2, y + 2, 7, 1); g.globalAlpha = 1; }
    else { g.strokeStyle = '#ff6b6b'; g.lineWidth = 1; g.beginPath(); g.moveTo(x + 3, y + 1.5); g.lineTo(x + 8, y + 5.5); g.stroke(); }
  });
  strip('clock', 12, 12, 6, 6, 1, g => { K.circ(0, 0, 4.2, '#d9d2e6'); g.strokeStyle = '#2b2230'; g.lineWidth = 1; g.beginPath(); g.moveTo(0, 0); g.lineTo(0, -2.7); g.moveTo(0, 0); g.lineTo(2, 0); g.stroke(); });

  // ---- the jetpack zone
  strip('cloud', 52, 24, 0, 8, 1, () => { K.rr(0, 0, 50, 14, 7, 'rgba(255,255,255,0.8)'); K.rr(12, -8, 28, 14, 7, 'rgba(255,255,255,0.8)'); });
  strip('pipe', 30, 16, 2, 0, 1, g => { g.fillStyle = '#7d9a8a'; g.fillRect(2, 0, 22, 16); g.fillStyle = '#5f7a6c'; g.fillRect(18, 0, 4, 16); g.fillStyle = '#a5c4b2'; g.fillRect(5, 0, 3, 16); });
  strip('pipelip', 32, 10, 3, 0, 1, g => { K.rr(-2, 0, 30, 10, 2, '#8fb09c'); g.fillStyle = '#c0703a'; g.fillRect(6, 3, 6, 3); K.circ(3, 5, 1.3, '#5f7a6c'); K.circ(23, 5, 1.3, '#5f7a6c'); });
  strip('pillar', 26, 28, 0, 0, 1, () => { K.rr(1, 14, 24, 13, 2, '#b07a44'); K.rr(0, 0, 26, 13, 6, '#2b2833'); K.rr(8, 4, 10, 5, 2, '#4a4652'); });
  strip('jetexit', 44, 132, 8, 12, 1, g => { K.rr(-6, 60 - 60, 40, 120, 8, '#2b2833'); K.rr(0, 8, 28, 104, 6, '#1b1426'); text(g, 'EXIT', 14, -6, 9, '#ffd23f', '#1b1426'); });
  strip('jet_hills', 640, 48, 0, 0, 1, g => {
    g.fillStyle = '#9fb7a0'; g.beginPath(); g.moveTo(0, 48);
    for (let x = 0; x <= 640; x += 4) g.lineTo(x, 20 + Math.sin(x * P * 2 / 213.3333) * 8); g.lineTo(640, 48); g.fill();
  }, 'rgba16');
  // ---- map pieces drawn on top of the baked map picture, one per world (map_boss_1 ... map_boss_4)
  const worlds = [1, 2, 3, 4].map(n => DATA_FMT.worldForMap(DATA_FMT.parseWorld(DATA['w' + n + '/world.txt'])));
  const node = (world, kind) => world.nodes.find(n => n.kind === kind);
  worlds.forEach((world, k) => strip('map_boss_' + (k + 1), 48, 52, 24, 44, 2, (g, i) => world.drawBoss(g, { x: 0, y: 0 }, 0.4, i === 1)));
  strip('map_shop', 32, 24, 16, 20, 2, (g, i) => { g.globalAlpha = i ? 0.5 : 1; K.rr(-11, -9, 22, 14, 2, '#7a5aa6'); g.fillStyle = '#5b3f86'; g.beginPath(); g.moveTo(-14, -8); g.lineTo(0, -19); g.lineTo(14, -8); g.fill(); K.rr(-4, -3, 8, 8, 1, '#2b2230'); K.gear(0, -13, 4, 8, 0, '#ffd23f', '#5b3f86'); g.globalAlpha = 1; }, 'rgba32');
  worlds.forEach((world, k) => strip('map_exit_' + (k + 1), 40, 56, 20, 34, 2, (g, i) => world.drawExit(g, { x: 0, y: 0 }, 0, i === 0)));
  // the start building, drawn over the roads that lead out of it
  worlds.forEach((world, k) => strip('map_home_' + (k + 1), 40, 52, 20, 34, 1, g => world.drawHome(g, { x: 0, y: 0 }, 0)));

  // ---- full-screen pictures (RGBA16)
  function picture(name, w, h, draw) { strip(name, w, h, 0, 0, 1, draw, 'rgba16'); }
  // each world's level backdrop (web/lib/levelart.js): the far picture, its boss-room version,
  // and the hills in front (only y 160..240 is kept)
  THEME_ORDER.forEach((th, k) => picture('bg_far_' + k, 640, 240, g => LTHEMES[th].far(g, false)));
  THEME_ORDER.forEach((th, k) => picture('bg_far_boss_' + k, 640, 240, g => LTHEMES[th].far(g, true)));
  THEME_ORDER.forEach((th, k) => strip('bg_mid_' + k, 640, 80, 0, -160, 1, g => LTHEMES[th].mid(g), 'rgba16'));
  worlds.forEach((world, k) => picture('map_w' + (k + 1), 320, 240, g => {
    world.drawGround(g); world.drawLive(g, 0.6);   // the home building is map_home_N, drawn over the roads
  }));
  // ---- the films between worlds (web/lib/cutart.js): a backdrop each, and the things that move
  [1, 2, 3, 4].forEach(k => picture('cut_bg_' + k, 320, 240, g => CUTART.bg(g, k)));
  // ---- the house (web/lib/houseart.js): the furniture, the aquarium and race-track backdrops,
  // and the little creatures; the rooms themselves are plain boxes the N64 fills in
  // the house seen from above (web/lib/housetop.js): every look of furniture in HOUSETOP.ARTS
  // order, cropped to what it draws (the N64 adds the soft shadow and the fish in the tank), and
  // The house is baked at HZ, the size the camera shows it at when not zoomed out (CK_HOUSE_ZOOM):
  // the furniture, one cropped sprite per look, and the house's static picture cut into tiles
  const HZ = HOUSETOP.ZOOM;
  HOUSETOP.ARTS.forEach((a, i) => {
    const cv = document.createElement('canvas'); cv.width = 320; cv.height = 320;
    const g = cv.getContext('2d'); g.translate(160, 260); g.scale(HZ, HZ); K.g = g; HOUSETOP.drawArt(g, a, 0, true);
    const d = g.getImageData(0, 0, 320, 320).data;
    let x0 = 320, y0 = 320, x1 = -1, y1 = -1;
    for (let y = 0; y < 320; y++) for (let x = 0; x < 320; x++) if (d[(y * 320 + x) * 4 + 3] > 8) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
    if (x1 < 0) { x0 = 160; x1 = 161; y0 = 259; y1 = 260; }
    x0 -= 1; y0 -= 1; x1 += 2; y1 += 2;
    strip('hart_' + i, x1 - x0, y1 - y0, 160 - x0, 260 - y0, 1, g2 => { g2.scale(HZ, HZ); HOUSETOP.drawArt(g2, a, 0, true); }, 'rgba16');
  });
  {
    const W = HOUSETOP.WORLD.W, H = HOUSETOP.WORLD.H, bg = document.createElement('canvas'); bg.width = W; bg.height = H;
    const g = bg.getContext('2d'); HOUSETOP.drawStatic(g); K.g = g; for (const r of HOUSETOP.rooms) HOUSETOP.plate(g, r);
    // each tile 160 x 120 pixels at HZ: 240 x 180 of the house; then at half that, for when the
    // camera zooms out, each 480 x 360 of the house
    for (const [name, z] of [['hbg_', HZ], ['hbs_', HZ / 2]]) {
      const tw = 160 / z, th = 120 / z, cols = Math.ceil(W / tw);
      for (let ty = 0; ty * th < H; ty++) for (let tx = 0; tx < cols; tx++)
        strip(name + (ty * cols + tx), 160, 120, 0, 0, 1, g2 => { g2.imageSmoothingQuality = 'high'; g2.scale(z, z); g2.drawImage(bg, -tx * tw, -ty * th); }, 'rgba16');
    }
  }
  picture('aqua_bg', 320, 240, g => HOUSEART.aquaBg(g, true));
  picture('aqua_water', 320, 240, g => HOUSEART.aquaBg(g, false));
  strip('house_fish', 28, 16, 14, 8, 6, (g, i) => HOUSEART.fish(g, i >> 1, (i & 1) * 0.16), 'rgba32');
  strip('house_shrimp', 20, 16, 9, 10, 2, (g, i) => HOUSEART.shrimp(g, i * 0.13), 'rgba32');
  strip('house_crab', 32, 22, 16, 16, 2, (g, i) => HOUSEART.crab(g, 0, i === 1), 'rgba32');
  strip('cut_gate', CUTART.GATE.w, CUTART.GATE.h, 0, 0, 1, g => CUTART.gate(g));
  strip('cut_cage', 48, 76, 24, 72, 2, (g, i) => CUTART.cage(g, i === 1));
  strip('cut_rocket', 48, 100, 24, 98, 1, g => CUTART.rocket(g));
  strip('cut_flame', 28, 48, 14, 0, 2, (g, i) => CUTART.flame(g, i));
  strip('cut_float', 140, 84, 70, 80, 1, g => CUTART.float(g));
  picture('title', 320, 240, g => { const T = makeTitle(g, 320, 240, false); T.frame(3.5); });
  picture('select_bg', 320, 240, g => {
    const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#2b2350'); gr.addColorStop(1, '#5b3f86'); g.fillStyle = gr; g.fillRect(0, 0, 320, 240);
    for (let i = 0; i < 6; i++) { g.globalAlpha = 0.08; K.gear(30 + i * 60, 210 + (i % 2) * 20, 30, 12, 0, '#ffffff', null); } g.globalAlpha = 1;
  });
  picture('jet_sky', 320, 240, g => {
    const gr = g.createLinearGradient(0, 0, 0, 240); gr.addColorStop(0, '#7fc8ff'); gr.addColorStop(1, '#cdeaff'); g.fillStyle = gr; g.fillRect(0, 0, 320, 240);
    g.fillStyle = '#8a5233'; g.fillRect(0, 224, 320, 16); g.fillStyle = '#c98b55'; g.fillRect(0, 224, 320, 3);
  });

  // ---- the story intro (introScene in web/game.js). The scenery is baked at night, in the storm
  // and at dawn, and the N64 cross-fades them; everything that moves is a sprite on top.
  function mixHex(a, b, k) { const pa = [a >> 16, (a >> 8) & 255, a & 255], pb = [b >> 16, (b >> 8) & 255, b & 255]; return 'rgb(' + pa.map((v, i) => Math.round(v + (pb[i] - v) * k)).join(',') + ')'; }
  const hexOf = rgb => rgb.match(/\d+/g).reduce((a, v) => a * 256 + +v, 0);
  function introBg(g, storm, dawn) {
    const gr = g.createLinearGradient(0, 0, 0, 240);
    gr.addColorStop(0, mixHex(hexOf(mixHex(0x141a3d, 0x2a1440, storm)), 0x5b6fc0, dawn));
    gr.addColorStop(0.7, mixHex(0x3b2f6b, 0xf2a37a, dawn)); gr.addColorStop(1, mixHex(0x4a3a6b, 0xffcf8a, dawn));
    g.fillStyle = gr; g.fillRect(0, 0, 320, 240);
    if (dawn < 1) { K.circ(268, 40, 14, '#fff1c4'); K.circ(274, 36, 12, mixHex(0x141a3d, 0x2a1440, storm)); }
    else { const sy = 160; K.circ(30, sy, 26, 'rgba(255,240,200,0.7)'); K.gear(30, sy, 22, 12, 0, '#ffe7a3', null); K.circ(30, sy, 17, '#fff1c4'); }
    g.fillStyle = mixHex(0x2b2140, 0x8a5f96, dawn); g.beginPath(); g.moveTo(0, 240);
    for (let x = 0; x <= 320; x += 20) g.lineTo(x, 168 + Math.sin(x * 0.03) * 10 + Math.sin(x * 0.07) * 5); g.lineTo(320, 240); g.fill();
    g.fillStyle = mixHex(0x3a2630, 0x8a5233, dawn); g.fillRect(0, 196, 320, 44); g.fillStyle = mixHex(0x5a3a40, 0xc98b55, dawn); g.fillRect(0, 196, 320, 4);
    const wall = mixHex(0x4c3a63, 0xb07a9a, dawn);
    K.rr(66, 128, 108, 68, 3, wall); g.fillStyle = mixHex(0x2e2244, 0x7d4a6a, dawn); g.beginPath(); g.moveTo(58, 132); g.lineTo(120, 98); g.lineTo(182, 132); g.fill();
    K.gear(120, 118, 7, 8, 0, '#ffd23f', mixHex(0x2e2244, 0x7d4a6a, dawn));
    K.rr(88, 146, 64, 50, 2, '#ffcf7a'); g.fillStyle = 'rgba(255,190,90,0.25)'; g.beginPath(); g.moveTo(88, 196); g.lineTo(152, 196); g.lineTo(172, 206); g.lineTo(68, 206); g.fill();
    K.rr(94, 190, 52, 5, 2, '#3a8de0'); K.rr(96, 190.5, 48, 1.5, 1, '#8fd3ff');
  }
  picture('intro_night', 320, 240, g => introBg(g, 0, 0));
  picture('intro_storm', 320, 240, g => introBg(g, 1, 0));
  picture('intro_dawn', 320, 240, g => introBg(g, 0, 1));
  // babies: 0-3 asleep, 4-7 startled, in the order pink, green, orange, blue
  const introBabies = [0xFF8FC7, 0x49F08A, 0xFFB347, 0x8FD3FF];
  strip('intro_baby', 24, 26, 12, 23, 8, (g, i) => {
    const awake = i > 3, hex = introBabies[i % 4];
    K.baby(0, 0, 0.05, hex, { t: 1, eyes: awake ? { open: 1, happy: 0 } : { open: 0.08 }, mouth: awake ? 'o' : 'smile', armL: awake ? 150 : 30, armR: awake ? -150 : -30 });
  });
  // robot: 0 asleep, 1 half awake, 2-3 up and waving, 4 jetting
  strip('intro_robot', 40, 64, 20, 57, 5, (g, i) => {
    const sleepy = i < 2, eyes = sleepy ? { open: i ? 0.45 : 0.06 } : { happy: 0.2 };
    K.robot(0, 0, 0.13, 0x5AD1FF, { t: 1, bob: sleepy ? 6 : 0, eyes, armR: sleepy ? -20 : i === 3 ? -150 : -170, armL: sleepy ? 20 : 40, mouth: sleepy ? 'wavy' : 'smile', jet: i === 4 });
  }, 'rgba16');
  // ostrich: 0 asleep, 1 half awake, 2-3 up with wings down and up
  strip('intro_ostrich', 40, 64, 20, 57, 4, (g, i) => {
    const sleepy = i < 2, eyes = sleepy ? { open: i ? 0.45 : 0.06 } : { happy: 0.2 };
    K.ostrich(0, 0, 0.11, 0xFFD23F, { t: 1, eyes, bob: sleepy ? 14 : 0, flap: i === 3 ? 1 : 0, beakOpen: sleepy ? 0 : 0.4 });
  }, 'rgba16');
  // the Big Glitch storm cloud, anchored at its middle
  strip('intro_cloud', 140, 88, 70, 46, 1, () => {
    for (const [dx, dy, r] of [[-46, 6, 22], [-20, -8, 28], [14, -12, 30], [44, 2, 24], [0, 14, 26], [-34, 18, 18], [32, 18, 20]]) K.circ(dx, dy, r, '#5b3f86');
    for (const [dx, dy, r] of [[-24, -14, 16], [16, -18, 18]]) K.circ(dx, dy, r, '#7a58a8');
    K.eye(-14, 2, 14, 16, '#1b1426', '#5b3f86', { angry: 1 }); K.eye(14, 2, 14, 16, '#1b1426', '#5b3f86', { angry: -1 });
    K.mouth('zig', 0, 16, 10, 2, '#1b1426');
  });
  // a white sleepy "z" the N64 tints and sizes
  strip('zee', 16, 16, 8, 12, 1, g => text(g, 'z', 0, 0, 12, '#ffffff', null));
  // the two heroes, small and white, for the parts shop's grid (the N64 tints them the player's colour)
  strip('shop_hero', 24, 30, 12, 28, 2, (g, i) => { K.g = g; if (i) K.ostrich(0, 1, 0.062, 0xF5F1E8, { t: 0 }); else K.robot(0, 0, 0.07, 0xF5F1E8, { t: 0 }); }, 'rgba16');

  function powerIcon(g, kind, x, y, t) {
    if (kind === 'mega') { K.rr(x - 3.5, y - 4.5, 7, 10, 1.5, '#2b2833'); K.rr(x - 1.5, y - 6, 3, 2, 0.5, '#2b2833'); K.rr(x - 2.2, y - 3.2, 4.4, 7.4, 1, '#49f08a');
      g.fillStyle = '#ffd23f'; g.beginPath(); g.moveTo(x + 0.8, y - 3); g.lineTo(x - 1.6, y + 0.8); g.lineTo(x, y + 0.8); g.lineTo(x - 0.8, y + 3.6); g.lineTo(x + 1.8, y - 0.4); g.lineTo(x + 0.2, y - 0.4); g.fill(); }
    else if (kind === 'rainbow') K.star(x, y, 5.5, '#ffd23f');
    else if (kind === 'bubble') { g.globalAlpha = 0.35; K.circ(x, y, 4.5, '#bff0ff'); g.globalAlpha = 1; g.strokeStyle = '#8fd3ff'; g.lineWidth = 1.2; g.beginPath(); g.arc(x, y, 4.5, 0, P * 2); g.stroke(); K.circ(x - 1.6, y - 1.6, 1.2, '#fff'); K.circ(x + 4.5, y - 4, 1.3, '#bff0ff'); }
    else if (kind === 'gun') { K.rr(x - 5, y - 2, 8, 4, 1.5, '#9aa1b0'); K.rr(x - 4, y, 3, 4, 1, '#5d566a'); g.globalAlpha = 0.5; K.circ(x + 4, y - 3, 2.6, '#bff0ff'); g.globalAlpha = 1; K.circ(x + 3.4, y - 3.6, 0.8, '#fff'); }
    else if (kind === 'magnet') { g.strokeStyle = '#e04f4f'; g.lineWidth = 3; g.beginPath(); g.arc(x, y - 0.5, 3.5, 0, P); g.stroke(); g.fillStyle = '#e8e4f0'; g.fillRect(x - 5, y - 3.5, 3, 3); g.fillRect(x + 2, y - 3.5, 3, 3); }
  }
  // the plan of the house for src/core/ck_house_map.c (the N64 walks the same one)
  const T = HOUSETOP, R = a => '{ ' + a.join(', ') + ' }';
  const houseMap = [
    '// Generated by roms/clankers/tools/bake_sprites.mjs from web/lib/housetop.js. Do not edit; re-run the tool.',
    '// The house seen from above: its rooms, where you can stand, the things in it, where the babies play.',
    '#include "ck_internal.h"', '',
    `const int ck_td_world[2] = { ${T.WORLD.W}, ${T.WORLD.H} };`,
    `const int ck_td_start[2][2] = { ${T.START.map(R).join(', ')} };`,
    `const int ck_td_gate[3] = { ${T.GATE.x}, ${T.GATE.hw}, ${T.GATE.y} };`,
    `const int ck_td_leash[2] = { ${T.LEASH.join(', ')} };`,
    `const ck_troom ck_trooms[${T.rooms.length}] = {`, ...T.rooms.map(r => `    { ${r.x}, ${r.y}, ${r.w}, ${r.h}, "${r.name}" },`), '};',
    `const int ck_ntrooms = ${T.rooms.length};`,
    `const ck_trect ck_twalk[${T.walk.length}] = {`, ...T.walk.map(w => `    ${R(w)},`), '};',
    `const int ck_ntwalk = ${T.walk.length};`,
    '// x, y (the front edge on the floor), footprint half width and depth, look (HOUSETOP.ARTS), spot (SPOT_*, -1 none), flat on the ground',
    `const ck_thing ck_things[${T.things.length}] = {`, ...T.things.map(t => `    { ${t.x}, ${t.y}, ${t.foot[0]}, ${t.foot[1]}, ${t.ai}, ${t.spot === undefined ? -1 : t.spot}, ${t.flat ? 1 : 0} },`), '};',
    `const int ck_nthings = ${T.things.length};`,
    '// the looks: how much smaller they are drawn (tall things by a back wall), the footprint, fish in it, a soft shadow under it',
    `const ck_tart ck_tarts[${T.ARTS.length}] = {`, ...T.ARTS.map(a => `    { ${(T.SCALE[a.art] || 1).toFixed(2)}f, ${a.foot[0]}, ${a.foot[1]}, ${a.art === 'tank' ? 1 : 0}, ${a.foot[0] && a.art !== 'pond' ? 1 : 0} },   // ${a.key.replace(/"/g, "'")}`), '};',
    `const int ck_ntarts = ${T.ARTS.length};`,
    `const ck_trect ck_tareas[${T.AREAS.length}] = {`, ...T.AREAS.map(w => `    ${R(w.map(Math.round))},`), '};',
    `const int ck_ntareas = ${T.AREAS.length};`, '',
  ].join('\n');
  return { sheets: out, accAt: out.accAt, houseMap };
}

// ------------------------------------------------------------------------------------------
const browser = await chromium.launch();
const page = await browser.newPage();
await page.setContent('<!doctype html><html><body></body></html>');
await page.evaluate(async (b64) => {
  const bin = Uint8Array.from(atob(b64), c => c.charCodeAt(0));
  const f = new FontFace('Titan One', bin.buffer);
  await f.load(); document.fonts.add(f);
}, font);
await page.addScriptTag({ content: engine + '\nconst DATA = ' + JSON.stringify(files) + ';\n' + bakeAll.toString() });
const { sheets, accAt, houseMap } = await page.evaluate(() => bakeAll());
await browser.close();

fs.mkdirSync(path.join(ASSETS, 'rgba16'), { recursive: true });
for (const s of sheets) {
  const dir = s.fmt === 'rgba16' ? path.join(ASSETS, 'rgba16') : ASSETS;
  fs.writeFileSync(path.join(dir, s.name + '.png'), Buffer.from(s.png.split(',')[1], 'base64'));
}
const id = n => 'SPR_' + n.toUpperCase().replace(/[^A-Z0-9]/g, '_');
const lines = [
  '// Generated by roms/clankers/tools/bake_sprites.mjs. Do not edit; re-run the tool.',
  '// Each strip: frame size, frames per row, frame count, and the anchor inside a frame.',
  '#ifndef CK_SPRITES_H', '#define CK_SPRITES_H', '',
  'typedef struct { const char *file; short fw, fh, cols, n, ax, ay; } spr_info;', '',
  'enum {', ...sheets.map(s => `    ${id(s.name)},`), '    SPR_COUNT', '};', '',
  'static const spr_info SPR_INFO[SPR_COUNT] = {',
  ...sheets.map(s => `    [${id(s.name)}] = { "rom:/${s.name}.sprite", ${s.fw}, ${s.fh}, ${s.cols}, ${s.n}, ${s.ax}, ${s.ay} },`),
  '};', '',
  '// Where each hero frame (robot, ostrich) wears its accessories, from the feet anchor: the head\'s',
  '// top middle (hats, face and neck pieces), the middle of the body, and up to two feet (the far',
  '// one marked), each with its turn in radians and stretch; side is 0 front, 1 side, 2 back. Arms are the',
  '// sleeve strokes of body items: shoulder, end, width (the far arm marked).',
  'typedef struct { float x, y, rot, sx, sy; } acc_pt;',
  'typedef struct { float x0, y0, x1, y1, w; unsigned char far; } acc_arm;',
  'typedef struct { acc_pt head, body, foot[2]; unsigned char side, has_body, nfeet, far[2]; unsigned char narms; acc_arm arm[2]; } acc_at;',
  `static const acc_at ACC_AT[2][${accAt[0].length}] = {`,
  ...accAt.map(rows => '    {\n' + rows.map(a => { const P = (p) => `{ ${p.x.toFixed(2)}f, ${p.y.toFixed(2)}f, ${p.rot.toFixed(3)}f, ${p.sx.toFixed(3)}f, ${p.sy.toFixed(3)}f }`, Z = { x: 0, y: -30, rot: 0, sx: 1, sy: 1 };
    const f = [a.feet[0] || Z, a.feet[1] || Z];
    return `        { ${P(a.head || Z)}, ${P(a.body || Z)}, { ${P(f[0])}, ${P(f[1])} }, ${a.side}, ${a.body ? 1 : 0}, ${a.feet.length}, { ${a.feet[0]?.far ? 1 : 0}, ${a.feet[1]?.far ? 1 : 0} }, ${a.arms.length}, { ${[0, 1].map(k => { const r = a.arms[k] || { x0: 0, y0: 0, x1: 0, y1: 0, w: 0, far: 0 }; return `{ ${r.x0.toFixed(2)}f, ${r.y0.toFixed(2)}f, ${r.x1.toFixed(2)}f, ${r.y1.toFixed(2)}f, ${r.w.toFixed(2)}f, ${r.far} }`; }).join(', ')} } },`; }).join('\n') + '\n    },'),
  '};', '', '#endif', '',
];
fs.writeFileSync(path.join(ROM, 'src', 'n64', 'sprites.h'), lines.join('\n'));
fs.writeFileSync(path.join(ROM, 'src', 'core', 'ck_house_map.c'), houseMap);
console.log(`baked ${sheets.length} sprite strips into ${path.relative(process.cwd(), ASSETS)}`);
