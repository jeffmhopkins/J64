#!/usr/bin/env python3
"""Builds the browser version of Clankers into one self-contained HTML file.

    python3 roms/clankers/web/build.py [--out FILE]

Inlines the drawing kit, map engine, data format, map themes, title, Glitchies, every text file
under roms/clankers/data, and the game, into index.src.html. Writes web/dist/clankers.html,
and the editors into web/dist/ as well.
"""
import json, pathlib, sys

WEB = pathlib.Path(__file__).resolve().parent
DATA = WEB.parent / 'data'
DIST = WEB / 'dist'

def read(p): return (WEB / p).read_text()

def data_js():
    files = {str(p.relative_to(DATA)).replace('\\', '/'): p.read_text() for p in sorted(DATA.rglob('*.txt'))}
    return 'const DATA = ' + json.dumps(files, ensure_ascii=False, indent=0) + ';\n'

def engine():
    """Everything the game and the editors share, in load order."""
    themes = [read(str(p.relative_to(WEB))) for p in sorted((WEB / 'themes').glob('*.js'))]
    return '\n'.join([read('lib/draw.js'), read('lib/ow.js'), read('lib/data.js'), read('lib/levelart.js'), *themes,
                      read('lib/bossart.js'), read('lib/cutart.js'), read('lib/title.js'), read('lib/glitchies.js')])

def game_html():
    return read('index.src.html').replace('/*@GAME*/', '\n'.join([engine(), data_js(), read('game.js')]))

def main():
    out = pathlib.Path(sys.argv[sys.argv.index('--out') + 1]) if '--out' in sys.argv else DIST / 'clankers.html'
    DIST.mkdir(exist_ok=True)
    html = game_html()
    out.write_text(html)
    print('built', out, len(html))
    for ed in ('level-editor', 'world-editor'):
        src = WEB / 'editor' / (ed + '.src.html')
        if src.exists():
            page = src.read_text().replace('/*@ENGINE*/', engine() + '\n' + data_js()).replace('/*@GAMEHTML*/', json.dumps(html).replace('</', '<\\/'))
            (out.parent / (ed + '.html')).write_text(page)
            print('built', out.parent / (ed + '.html'), len(page))

if __name__ == '__main__':
    main()
