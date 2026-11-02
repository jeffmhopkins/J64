#!/usr/bin/env python3
"""One-off generator for the World 2 to 4 levels (kept for reference; the .txt files are the source).

    python3 roms/clankers/tools/w234_levels.py

Writes data/w2, data/w3 and data/w4, and checks every level has a start, a charging pad and
three golden gears.
"""
import pathlib

DATA = pathlib.Path(__file__).resolve().parent.parent / 'data'


class Lv:
    def __init__(s, w):
        s.w = w
        s.g = [['.'] * w for _ in range(15)]

    def ground(s, x0, x1, top=12):
        for x in range(x0, x1 + 1):
            for y in range(top, 15):
                s.g[y][x] = '#'

    def gap(s, x0, x1):
        for x in range(x0, x1 + 1):
            for y in range(15):
                s.g[y][x] = '.'

    def fill(s, x0, x1, y0, y1, c):
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                s.g[y][x] = c

    def row(s, x0, x1, y, c):
        for x in range(x0, x1 + 1):
            s.g[y][x] = c

    def put(s, x, y, c):
        s.g[y][x] = c

    def belt(s, x0, x1, y, c):
        """A conveyor belt, > carries you right and < left, with ground under it when y is the floor."""
        s.row(x0, x1, y, c)

    def lift(s, x, bottom, top):
        """A raft that rides straight up and down between rows bottom and top."""
        s.put(x, bottom, 'r')
        for y in range(top, bottom):
            s.put(x, y, '|')

    def cart(s, x, y, x1):
        """A raft that rides sideways from x to x1 (a mine cart in World 3)."""
        s.put(x, y, 'r')
        s.row(x + 1, x1, y, '~')

    def save(s, path, head):
        text = '\n'.join(''.join(r) for r in s.g)
        kind = 'level' if 'kind: level' in head else 'other'
        n = lambda c: text.count(c)
        assert n('@') == 1, path
        if kind == 'level':
            assert n('P') == 1, path
            assert n('G') + n('?') == 3, (path, n('G') + n('?'))
            assert n('H') == n('h'), path
        (DATA / path).parent.mkdir(exist_ok=True)
        (DATA / path).write_text(head + '---\n' + text + '\n')


def arena(kind, name, baby, song, extra=''):
    """A one-screen boss or bonus room: floor, two side planks, a few bolts."""
    L = Lv(20)
    L.ground(0, 19)
    L.put(2 if kind == 'boss' else 9, 11, '@')
    return L, f'name: {name}\nkind: {kind}\nsong: {song}\nbaby: {baby}\n{extra}'


# =========================================================================== World 2: Gear Factory
# 2-1: conveyor belts. Ones that help you along, ones you run against, and belts in the air.
L = Lv(200)
L.ground(0, 20); L.put(2, 11, '@'); L.row(5, 8, 10, 'o')
L.ground(21, 40); L.belt(21, 40, 12, '>'); L.row(24, 38, 10, 'o'); L.put(30, 11, 'b')
L.ground(43, 60); L.row(47, 50, 9, '-'); L.row(51, 53, 7, '-'); L.row(51, 53, 6, 'o'); L.put(52, 4, 'G'); L.put(56, 11, 'b')
L.ground(61, 78); L.belt(61, 78, 12, '<'); L.row(63, 76, 10, 'o'); L.put(70, 11, 's')
L.ground(79, 95); L.put(84, 11, 'F'); L.fill(88, 88, 10, 11, 'X'); L.fill(89, 89, 9, 11, 'X'); L.put(92, 8, '$')
L.belt(98, 103, 10, '>'); L.row(98, 103, 9, 'o')
L.belt(107, 112, 9, '>'); L.put(110, 6, 'G')
L.belt(116, 121, 10, '>'); L.row(116, 121, 9, 'o')
L.ground(124, 150); L.put(130, 11, 'b'); L.put(134, 8, '2'); L.put(138, 11, 'b'); L.row(141, 146, 10, 'o')
L.ground(151, 170); L.belt(151, 170, 12, '>'); L.put(158, 11, 'X'); L.put(165, 11, 'X'); L.row(155, 168, 8, 'o')
L.ground(173, 199); L.put(180, 8, '?'); L.put(186, 11, 'b'); L.row(188, 191, 10, 'o'); L.put(194, 11, 'P')
L.save('w2/2-1.txt', 'name: 2-1 CONVEYOR CORRIDOR\nkind: level\nsong: factory\nbaby: FFB347\npar: 80\n')

# 2-2: lifts that ride up and down, a junk press, a swinging hook.
L = Lv(190)
L.ground(0, 24); L.put(2, 11, '@'); L.row(5, 8, 10, 'o')
L.fill(11, 12, 10, 11, '='); L.fill(15, 16, 8, 11, '='); L.row(15, 16, 7, 'o'); L.put(16, 4, 'G')
L.lift(26, 11, 6); L.row(30, 35, 7, '='); L.row(30, 34, 6, 'o')
L.ground(33, 62); L.put(40, 11, 'b'); L.put(46, 6, 'C'); L.row(46, 47, 11, 'o')
L.put(52, 11, 'T'); L.row(54, 58, 5, '='); L.row(54, 58, 4, 'o'); L.put(59, 11, 's')
L.put(61, 11, 'F'); L.row(62, 64, 9, '='); L.ground(63, 64)
L.cart(65, 10, 72); L.row(66, 73, 8, 'o')
L.ground(75, 110); L.row(75, 77, 9, '='); L.put(76, 6, 'G')
L.fill(84, 84, 10, 11, 'X'); L.put(90, 11, 'g'); L.put(96, 11, 'b'); L.put(100, 8, '$'); L.row(102, 106, 10, 'o')
L.lift(113, 11, 5); L.row(117, 124, 5, '='); L.put(122, 2, '?'); L.row(118, 121, 4, 'o')
L.ground(121, 189); L.put(135, 6, 'C'); L.row(135, 136, 11, 'o'); L.put(142, 11, 's'); L.put(150, 11, 's')
L.put(156, 11, 'T'); L.row(155, 159, 4, 'o'); L.put(165, 11, 'b'); L.put(172, 11, 'b'); L.row(174, 178, 10, 'o'); L.put(182, 11, 'P')
L.save('w2/2-2.txt', 'name: 2-2 GEAR CLIMB\nkind: level\nsong: level3\nbaby: 8FD3FF\npar: 110\n')

# 2-3: paint crates to punch through, a lever and its door, the jetpack hatch, more belts.
L = Lv(200)
L.ground(0, 33); L.put(2, 11, '@'); L.put(6, 8, '1'); L.row(4, 7, 10, 'o')
L.fill(14, 15, 8, 11, 'X'); L.put(20, 11, 'L'); L.row(21, 24, 8, '-'); L.put(23, 5, 'G'); L.fill(28, 28, 5, 11, 'D')
L.ground(36, 59); L.belt(40, 50, 12, '<'); L.put(45, 11, 'g'); L.row(41, 49, 9, 'o'); L.put(56, 11, 'H')
L.belt(61, 64, 10, '>'); L.belt(68, 71, 9, '>'); L.row(68, 71, 8, 'o'); L.belt(74, 77, 10, '>')
L.ground(78, 118); L.put(80, 11, 'h'); L.put(83, 11, 'F'); L.put(88, 11, 'X'); L.row(92, 95, 8, '-'); L.put(94, 5, 'G')
L.put(106, 11, 'b'); L.put(110, 8, '$'); L.put(112, 11, 'b'); L.fill(116, 116, 10, 11, 'X'); L.fill(117, 117, 9, 11, 'X')
L.ground(119, 199); L.belt(120, 135, 12, '>'); L.put(128, 11, 's'); L.row(122, 134, 9, 'o')
L.put(140, 8, '?'); L.put(150, 11, 'g'); L.put(158, 11, 'b'); L.put(166, 11, 'b'); L.put(172, 11, 'T'); L.row(171, 175, 4, 'o')
L.row(180, 184, 10, 'o'); L.put(190, 11, 'P')
L.save('w2/2-3.txt', 'name: 2-3 PAINT SHOP\nkind: level\nsong: factory\nbaby: C28BFF\nhatch: jet1\npar: 110\n')

L, head = arena('bonus', 'BONUS: SPIN ROOM', 'FFB347', 'bonus')
L.belt(0, 9, 12, '>'); L.belt(10, 19, 12, '<'); L.row(2, 5, 9, '-'); L.row(14, 17, 9, '-'); L.row(8, 11, 6, '-')
L.save('w2/bonus.txt', head)

L, head = arena('boss', 'BIG GLITCH: THE STAMPER', '49F08A', 'boss', 'boss: stamper\n')
L.row(3, 6, 8, '-'); L.row(13, 16, 8, '-'); L.row(3, 6, 7, 'o'); L.row(13, 16, 7, 'o'); L.put(9, 10, 'o'); L.put(10, 10, 'o')
L.save('w2/boss.txt', head)

# =========================================================================== World 3: Magnet Mountain
# 3-1: a cave with low roofs to slide under, and the Magnet Helmet.
L = Lv(200)
L.ground(0, 199); L.put(2, 11, '@'); L.put(6, 8, '4'); L.row(4, 9, 10, 'o')
L.fill(20, 180, 0, 1, '#')
L.put(21, 11, 'v'); L.fill(24, 29, 2, 10, '#'); L.row(24, 29, 11, 'o')
L.put(34, 11, 's'); L.row(33, 36, 7, 'o'); L.gap(38, 40); L.fill(38, 40, 0, 1, '#')
L.put(43, 11, 'v'); L.fill(46, 52, 2, 10, '#'); L.row(46, 48, 11, 'o'); L.put(49, 11, 'G'); L.row(50, 52, 11, 'o')
L.put(58, 11, 'b'); L.put(63, 11, 'b'); L.row(67, 70, 8, '-'); L.put(69, 5, 'G'); L.row(67, 70, 7, 'o')
L.gap(72, 74); L.fill(72, 74, 0, 1, '#'); L.put(78, 11, 'F')
L.put(86, 11, 'T'); L.row(88, 93, 5, '-'); L.row(88, 93, 4, 'o')
L.put(96, 11, 'g'); L.put(98, 11, 'v'); L.fill(100, 106, 2, 10, '#'); L.row(100, 106, 11, 'o')
L.gap(110, 112); L.fill(110, 112, 0, 1, '#'); L.put(116, 11, 's'); L.put(122, 11, 's'); L.put(126, 8, '$')
L.put(129, 11, 'v'); L.fill(132, 140, 2, 10, '#'); L.row(132, 140, 11, 'o')
L.put(147, 8, '?'); L.put(152, 11, 'b'); L.put(158, 11, 'b'); L.gap(162, 163); L.fill(162, 163, 0, 1, '#')
L.put(170, 11, 'b'); L.row(174, 178, 10, 'o'); L.put(190, 11, 'P')
L.save('w3/3-1.txt', 'name: 3-1 MAGNET CAVE\nkind: level\nsong: mountain\nbaby: C28BFF\npar: 90\n')

# 3-2: mine carts across the pits, against the clock.
L = Lv(210)
L.ground(0, 25); L.put(2, 11, '@'); L.row(5, 9, 10, 'o'); L.put(14, 11, 'b')
L.cart(26, 12, 37); L.row(28, 37, 9, 'o')
L.ground(40, 60); L.put(45, 11, 'b'); L.row(48, 51, 9, '-'); L.put(49, 6, 'G'); L.put(55, 11, 's')
L.cart(61, 12, 77); L.row(63, 69, 9, 'o'); L.put(71, 9, 'G'); L.row(73, 77, 9, 'o')
L.ground(80, 110); L.put(84, 11, 'F'); L.put(92, 11, 'b'); L.put(98, 11, 'b'); L.fill(95, 95, 10, 11, 'X'); L.put(102, 8, '$')
L.cart(111, 12, 115); L.ground(118, 120); L.cart(121, 12, 126); L.row(112, 126, 9, 'o')
L.ground(129, 160); L.put(135, 11, 's'); L.put(142, 11, 'T'); L.row(143, 147, 5, '-'); L.row(143, 147, 4, 'o'); L.put(150, 8, '?'); L.put(155, 11, 'b')
L.cart(161, 12, 175); L.row(163, 175, 9, 'o')
L.ground(178, 209); L.put(185, 11, 'b'); L.put(192, 11, 'b'); L.row(194, 198, 10, 'o'); L.put(202, 11, 'P')
L.save('w3/3-2.txt', 'name: 3-2 MINE CART RUN\nkind: level\nsong: level2\nbaby: 3BF0FF\npar: 110\ntime: 200\n')

# 3-3: up and down the mountain, a tyre or the jetpack hatch over the big wall, Rainbow Overdrive.
L = Lv(200)
L.ground(0, 15); L.put(2, 11, '@'); L.put(7, 8, '2'); L.row(4, 6, 10, 'o')
L.ground(16, 22, 10); L.row(17, 21, 8, 'o'); L.ground(23, 30, 8); L.put(29, 7, 'T')
L.ground(31, 38, 6); L.row(32, 37, 5, 'o'); L.put(35, 2, 'G')
L.ground(39, 46, 8); L.ground(47, 55, 10); L.put(50, 9, 'b')
L.ground(59, 80); L.put(63, 11, 's'); L.put(70, 11, 's'); L.put(74, 11, 'H'); L.put(78, 11, 'T')
L.ground(79, 80, 9); L.ground(81, 90, 6); L.row(82, 89, 5, 'o'); L.put(86, 3, 'G'); L.ground(91, 99, 9); L.put(95, 8, 'b')
L.ground(100, 107); L.put(100, 11, 'h'); L.put(103, 11, 'F')
L.ground(108, 115, 10); L.ground(116, 123, 8); L.row(117, 122, 7, 'o')
L.row(124, 127, 6, '-'); L.row(124, 127, 5, 'o'); L.put(126, 2, '?'); L.ground(124, 131, 9); L.ground(132, 140, 11)
L.ground(144, 199); L.put(150, 11, 'b'); L.put(156, 11, 'b'); L.put(162, 11, 's'); L.put(168, 11, 'g')
L.put(175, 11, 'T'); L.row(174, 178, 4, 'o'); L.put(182, 11, 'b'); L.row(184, 188, 10, 'o'); L.put(192, 11, 'P')
L.save('w3/3-3.txt', 'name: 3-3 WINDY PEAK\nkind: level\nsong: mountain\nbaby: FF8FC7\nhatch: jet1\npar: 120\n')

L, head = arena('bonus', 'BONUS: CRYSTAL SLIDE', 'C28BFF', 'bonus')
L.row(2, 5, 9, '-'); L.row(14, 17, 9, '-'); L.row(7, 12, 6, '-')
L.save('w3/bonus.txt', head)

L, head = arena('boss', 'BIG GLITCH: RUMBLEBOULDER', 'FFB347', 'boss', 'boss: boulder\n')
L.row(3, 6, 8, '-'); L.row(13, 16, 8, '-'); L.row(3, 6, 7, 'o'); L.row(13, 16, 7, 'o')
L.save('w3/boss.txt', head)

# =========================================================================== World 4: Moon Base
# Every Moon level has low gravity: you jump higher and float down slowly.
MOON = 'gravity: low\n'

# 4-1: big floaty jumps over craters.
L = Lv(200)
L.ground(0, 20); L.put(2, 11, '@'); L.row(5, 9, 9, 'o')
L.ground(25, 40); L.put(32, 11, 'b'); L.row(36, 40, 8, '-'); L.put(38, 4, 'G'); L.row(21, 24, 8, 'o')
L.ground(45, 70); L.put(50, 11, 'T'); L.row(52, 57, 3, '-'); L.row(52, 57, 2, 'o'); L.put(60, 11, 's'); L.put(66, 11, 'b'); L.row(71, 75, 7, 'o')
L.ground(75, 110); L.put(79, 11, 'F'); L.row(84, 88, 7, '='); L.put(86, 4, '$'); L.put(95, 11, 'b'); L.put(102, 11, 'b')
L.ground(115, 117, 10); L.ground(121, 123, 9); L.put(122, 5, 'G'); L.ground(127, 129, 10); L.row(115, 129, 6, 'o')
L.ground(133, 199); L.put(140, 8, '?'); L.put(150, 11, 'b'); L.put(158, 11, 'b'); L.put(166, 11, 's'); L.row(172, 178, 9, 'o'); L.put(190, 11, 'P')
L.save('w4/4-1.txt', 'name: 4-1 MOON BOUNCE\nkind: level\nsong: moon\nbaby: 3BF0FF\n' + MOON + 'par: 80\n')

# 4-2: inside the station: a lever high up opens the airlocks, crates to punch through, presses and belts.
L = Lv(200)
L.fill(0, 199, 0, 1, '=')
L.ground(0, 55); L.put(2, 11, '@'); L.row(5, 8, 10, 'o')
L.fill(14, 16, 9, 11, '='); L.fill(18, 20, 6, 8, '='); L.put(19, 5, 'L'); L.put(17, 3, 'G'); L.fill(26, 26, 2, 11, 'D')
L.belt(30, 45, 12, '>'); L.put(36, 11, 'b'); L.put(40, 8, '1'); L.row(31, 44, 9, 'o')
L.fill(50, 51, 6, 11, 'X')
L.ground(59, 105); L.put(62, 11, 'F'); L.put(68, 4, 'C'); L.row(68, 69, 11, 'o'); L.fill(76, 76, 2, 11, 'D')
L.put(82, 11, 'g'); L.put(88, 11, 'b'); L.belt(92, 104, 12, '<'); L.put(98, 11, 's'); L.row(98, 102, 8, '-'); L.put(100, 5, 'G')
L.ground(109, 149); L.put(114, 4, 'C'); L.row(114, 115, 11, 'o'); L.put(120, 4, 'C'); L.row(120, 121, 11, 'o')
L.put(126, 8, '$'); L.put(130, 11, 'b'); L.put(136, 11, 'b'); L.put(145, 8, '?')
L.ground(153, 199); L.put(160, 11, 'g'); L.put(168, 11, 'b'); L.put(174, 11, 'b'); L.fill(180, 180, 2, 11, 'D'); L.row(182, 186, 10, 'o'); L.put(190, 11, 'P')
L.save('w4/4-2.txt', 'name: 4-2 AIRLOCK DOORS\nkind: level\nsong: level3\nbaby: FF8FC7\n' + MOON + 'par: 100\n')

# 4-3: lifts and hooks up to the rocket, the Bubble Blaster, the jetpack hatch.
L = Lv(210)
L.ground(0, 20); L.put(2, 11, '@'); L.put(8, 8, '3'); L.row(4, 7, 10, 'o')
L.lift(23, 11, 5); L.row(27, 36, 6, '='); L.put(30, 3, 'G'); L.row(28, 34, 5, 'o')
L.ground(33, 60); L.put(40, 11, 'b'); L.put(46, 11, 's'); L.put(52, 11, 'T'); L.row(52, 56, 5, 'o'); L.put(58, 11, 'H')
L.row(59, 61, 8, '='); L.row(64, 66, 9, '='); L.row(69, 71, 8, '='); L.row(74, 76, 9, '='); L.row(64, 76, 6, 'o'); L.row(79, 81, 8, '=')
L.ground(79, 120); L.put(82, 11, 'F'); L.put(86, 11, 'h'); L.put(95, 5, 'C'); L.row(95, 96, 11, 'o'); L.put(101, 5, 'C'); L.row(101, 102, 11, 'o')
L.put(108, 11, 'b'); L.put(112, 8, '$'); L.row(114, 118, 7, '-'); L.put(116, 4, 'G')
L.lift(122, 11, 4); L.row(126, 131, 5, '='); L.put(129, 1, '?')
L.ground(129, 209); L.put(136, 11, 's'); L.put(144, 11, 's'); L.put(150, 11, 'b'); L.put(158, 11, 'g')
L.belt(165, 180, 12, '>'); L.put(172, 11, 'b'); L.row(166, 179, 9, 'o'); L.put(200, 11, 'P')
L.save('w4/4-3.txt', 'name: 4-3 STAR ROCKET\nkind: level\nsong: moon\nbaby: 49F08A\nhatch: jet1\n' + MOON + 'par: 120\n')

L, head = arena('bonus', 'BONUS: ZERO-G ROOM', '3BF0FF', 'bonus', MOON)
L.row(2, 5, 9, '-'); L.row(14, 17, 9, '-'); L.row(7, 12, 5, '-')
L.save('w4/bonus.txt', head)

L, head = arena('boss', 'BIG GLITCH: THE SKY HAULER', 'FFD23F', 'boss', 'boss: hauler\n' + MOON)
L.row(3, 6, 8, '-'); L.row(13, 16, 8, '-'); L.row(3, 6, 7, 'o'); L.row(13, 16, 7, 'o')
L.save('w4/boss.txt', head)
print('ok')
