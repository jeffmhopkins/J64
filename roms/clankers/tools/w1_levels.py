#!/usr/bin/env python3
"""One-off generator for the rebuilt World 1 levels (kept for reference; the .txt files are the source)."""
import pathlib
OUT = pathlib.Path(__file__).resolve().parent.parent / 'data' / 'w1'

class Lv:
    def __init__(s, w): s.w = w; s.g = [['.'] * w for _ in range(15)]
    def ground(s, x0, x1, top=12):
        for x in range(x0, x1 + 1):
            for y in range(top, 15): s.g[y][x] = '#'
    def fill(s, x0, x1, y0, y1, c):
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1): s.g[y][x] = c
    def row(s, x0, x1, y, c):
        for x in range(x0, x1 + 1): s.g[y][x] = c
    def put(s, x, y, c): s.g[y][x] = c
    def save(s, name, head):
        (OUT / name).write_text(head + '---\n' + '\n'.join(''.join(r) for r in s.g) + '\n')

# 1-1: run, jump, bounce on Glitchies. Nothing else.
L = Lv(200)
L.ground(0, 13); L.ground(14, 17, 11); L.ground(18, 30)
L.put(2, 11, '@'); L.row(6, 10, 10, 'o'); L.put(24, 11, 'b'); L.row(20, 22, 10, 'o')
for x, y in [(30, 9), (31, 8), (32, 8), (33, 9)]: L.put(x, y, 'o')
L.ground(33, 50); L.row(37, 39, 9, '-'); L.row(37, 39, 8, 'o'); L.row(41, 43, 7, '-'); L.put(42, 5, 'G'); L.put(47, 11, 'b')
L.put(52, 8, 'G'); L.ground(54, 70, 11); L.put(60, 10, 'b'); L.row(62, 64, 9, 'o')
for x, y in [(66, 10), (67, 10), (67, 9), (68, 10), (68, 9), (68, 8)]: L.put(x, y, 'X')
L.put(69, 5, 'G'); L.ground(71, 74, 11); L.ground(75, 88); L.put(80, 11, 'b'); L.put(85, 11, 'b'); L.row(81, 84, 9, 'o')
L.ground(91, 110); L.row(93, 96, 10, 'o'); L.put(98, 8, '$'); L.put(101, 11, 'F')
L.row(104, 106, 9, '-'); L.row(104, 106, 8, 'o'); L.row(108, 110, 7, '-'); L.row(108, 110, 6, 'o')
for x, y in [(111, 9), (112, 8), (113, 9)]: L.put(x, y, 'o')
L.ground(114, 129); L.put(120, 11, 'b'); L.put(126, 11, 'b'); L.ground(130, 134, 11); L.ground(135, 140, 10); L.row(136, 139, 8, 'o')
L.ground(143, 160, 10); L.put(150, 9, 'b'); L.row(146, 148, 6, '-'); L.row(146, 148, 5, 'o'); L.put(156, 9, 'X'); L.put(157, 9, 'X'); L.put(157, 8, 'X')
L.ground(163, 199); L.put(170, 11, 'b'); L.row(172, 176, 10, 'o'); L.put(178, 11, 'b'); L.put(182, 8, '$'); L.put(190, 11, 'P')
L.save('1-1.txt', 'name: 1-1 SCRAP PATH\nkind: level\nsong: level\nbaby: FF8FC7\npar: 75\n')

# 1-2: crouch and slide. Crawl-spaces you have to slide through, Glitchies you can slide into.
L = Lv(200)
L.ground(0, 36); L.put(2, 11, '@'); L.row(4, 7, 10, 'o'); L.put(8, 11, 'v')
L.fill(11, 15, 3, 10, '='); L.row(11, 15, 11, 'o')
L.put(20, 11, 'b'); L.put(24, 11, 'v')
L.fill(26, 33, 4, 10, '='); L.row(27, 29, 11, 'o'); L.put(30, 11, 'G'); L.row(31, 32, 11, 'o')
L.ground(39, 58); L.row(41, 44, 9, '-'); L.row(41, 44, 8, 'o')
L.row(48, 56, 10, '='); L.put(51, 11, 'b'); L.put(54, 11, 'b'); L.row(48, 53, 9, 'o'); L.put(55, 8, 'G')
L.ground(61, 92); L.fill(66, 70, 3, 10, '='); L.row(66, 70, 11, 'o'); L.put(75, 11, 'b')
L.row(79, 81, 9, '-'); L.row(83, 85, 7, '-'); L.row(87, 89, 5, '-'); L.put(88, 3, 'G'); L.row(79, 81, 8, 'o')
L.ground(95, 130); L.put(98, 11, 'v'); L.fill(100, 104, 4, 10, '='); L.row(100, 104, 11, 'o'); L.put(108, 11, 'F')
L.row(112, 119, 10, '='); L.put(114, 11, 'b'); L.put(117, 11, 'b'); L.row(112, 116, 9, 'o')
L.ground(133, 161); L.row(135, 137, 9, '-'); L.row(135, 137, 8, 'o'); L.put(140, 11, 'v'); L.fill(142, 147, 3, 10, '='); L.row(142, 147, 11, 'o'); L.put(152, 11, 'b')
L.ground(164, 199); L.row(168, 176, 10, '='); L.put(170, 11, 'b'); L.put(174, 11, 'b'); L.row(168, 176, 9, 'o'); L.fill(182, 185, 4, 10, '='); L.row(182, 185, 11, 'o'); L.put(192, 11, 'P')
L.save('1-2.txt', 'name: 1-2 SQUEEZE ALLEY\nkind: level\nsong: level2\nbaby: 49F08A\npar: 100\ntime: 200\n')

# 1-3: tyre springs, slide-then-bounce combos, a raft, the jetpack hatch, and the first power-up (Mega Battery).
L = Lv(210)
L.ground(0, 35); L.put(2, 11, '@'); L.put(8, 8, '1'); L.row(5, 7, 10, 'o')
L.fill(15, 15, 9, 11, 'X'); L.fill(16, 16, 10, 11, 'X')
L.put(22, 11, 'T'); L.row(24, 28, 5, '-'); L.row(24, 27, 4, 'o'); L.put(28, 3, 'G'); L.put(31, 11, 's')
L.put(36, 12, 'r'); L.row(37, 41, 12, '~'); L.row(37, 41, 9, 'o')
L.ground(43, 60); L.put(46, 11, 'v'); L.fill(48, 52, 3, 10, '='); L.put(54, 11, 'T'); L.put(54, 3, 'G'); L.put(58, 11, 's')
L.put(61, 11, 'H'); L.ground(61, 76); L.put(68, 11, 'T'); L.row(70, 74, 5, '='); L.row(70, 74, 4, 'o')
L.ground(79, 95, 11); L.put(84, 10, 'b'); L.put(90, 8, '?'); L.row(86, 88, 8, 'o')
L.ground(96, 151); L.put(96, 11, 'h'); L.put(99, 11, 'F'); L.fill(101, 101, 9, 11, 'X'); L.fill(102, 102, 9, 11, 'X'); L.put(106, 11, 'g'); L.put(110, 11, 'T'); L.put(114, 11, 'b')
for x, y in [(111, 6), (112, 5), (113, 5), (114, 6)]: L.put(x, y, 'o')
L.put(118, 11, 'b'); L.put(128, 11, 'T'); L.row(130, 134, 6, '-'); L.row(130, 134, 5, 'o'); L.fill(140, 140, 9, 11, 'X'); L.fill(141, 141, 9, 11, 'X'); L.put(146, 11, 's')
L.put(152, 12, 'r'); L.row(153, 158, 12, '~'); L.row(153, 158, 9, 'o')
L.ground(160, 209); L.put(162, 11, 'v'); L.fill(164, 168, 3, 10, '='); L.row(164, 168, 11, 'o'); L.put(172, 11, 'T'); L.row(171, 173, 4, 'o'); L.put(178, 11, 's'); L.put(184, 11, 'b'); L.put(188, 11, 'g'); L.put(193, 11, 'b'); L.row(195, 198, 10, 'o'); L.put(202, 11, 'P')
L.save('1-3.txt', 'name: 1-3 TYRE BOUNCE\nkind: level\nsong: level3\nbaby: FFB347\nhatch: jet1\npar: 120\n')
print('ok')
