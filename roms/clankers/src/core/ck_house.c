// The heroes' house and the little games in it. Mirrors houseScene, paintScene, aquaScene,
// carsScene and pianoScene in web/game.js (the plan is web/lib/housetop.js, in ck_house_map.c).
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ck_internal.h"

const ck_room ck_rooms[CK_HOUSE_ROOMS] = {
    { 0, 400, "The yard", 0x8FD3FF, 0xBDE8FF, 0x5DBB63 },
    { 400, 580, "Bedroom", 0xFFD3E2, 0xFFC1D6, 0x8C6A55 },
    { 580, 760, "Kitchen", 0xBDE7D2, 0xA9DCC3, 0xD9CDB8 },
    { 760, 940, "Paint studio", 0xFFF1D6, 0xF6E1BA, 0x9C7552 },
    { 940, 1120, "Music room", 0xC9B8F0, 0xB8A3E9, 0x7A4E3A },
    { 1120, 1300, "Aquarium", 0x9FD8F0, 0x88CBE6, 0x5D7C99 },
    { 1300, 1520, "Garage", 0xB5BAC4, 0xA4AAB6, 0x6E7280 },
    { 1520, 1900, "Arcade", 0x3A2E5C, 0x46386E, 0x2B2240 },
};
// indexed by SPOT_* (the things in ck_things that have a spot point here)
const ck_spot ck_spots[CK_HOUSE_SPOTS] = {
    { SPOT_SNACK, "Open the fridge" },
    { SPOT_PAINT, "Paint the mural" },
    { SPOT_PIANO, "Play the piano" },
    { SPOT_AQUA, "Swim with the fish" },
    { SPOT_BED, "Sleep and save" },
    { SPOT_CARS, "Race the cars" },
    { SPOT_DRUMS, "Play the drums" },
    { SPOT_MAIL, "Check the mail" },
    { SPOT_TREE, "Pick an apple" },
    { SPOT_BACK, "Back a world" },
    { SPOT_STOVE, "Cook something" },
    { SPOT_PINBALL, "Play pinball" },
    { SPOT_BLAST, "Play Bug Blaster" },
    { SPOT_POND, "Go fishing" },
    { SPOT_BOLT, "Play Bolt Ball" },
    { SPOT_JETRUN, "Play Jet Run" },
    { SPOT_SAND, "Play in the sandbox" },
};
#define C(x, y, r, c) { 0, x, y, r, 0, c }
#define R(x, y, w, h, c) { 1, x, y, w, h, c }
const ck_foodart ck_foods[FOOD_COUNT] = {
    { "apple", 4, { C(0, 0, 4, 0xE04F4F), C(-1.5f, -1.5f, 1.2f, 0xFF8A8A), R(-0.5f, -6, 1, 3, 0x6F4630), R(0.5f, -6, 3, 1.5f, 0x49D17A) } },
    { "banana", 3, { R(-5, -1, 10, 3, 0xFFE066), R(-6, -2, 2, 2, 0x6F4630), R(4, -3, 2, 3, 0xFFE066) } },
    { "cheese", 3, { R(-5, -3, 10, 6, 0xFFD23F), C(-2, -1, 1, 0xE0A92B), C(2, 1, 1.2f, 0xE0A92B) } },
    { "carrot", 3, { R(-1.5f, -3, 3, 9, 0xFF8A3D), R(-2.5f, -6, 2, 3, 0x49D17A), R(0.5f, -6, 2, 3, 0x49D17A) } },
    { "cake", 3, { R(-5, -2, 10, 6, 0xFF8FC7), R(-5, -3, 10, 2, 0xFFF6E6), C(0, -5, 1.5f, 0xE04F4F) } },
    { "ice cream", 3, { R(-2, 0, 4, 6, 0xC98A4B), C(0, -1, 3.5f, 0xFF8FC7), C(-1, -2, 1, 0xFFFFFF) } },
    { "milk", 3, { R(-3, -5, 6, 10, 0xFFFFFF), R(-3, -1, 6, 3, 0x5AD1FF), R(-2, -7, 4, 2, 0xFFFFFF) } },
    { "pizza", 4, { R(-5, -3, 10, 6, 0xE8B46A), R(-5, -3, 10, 1.5f, 0xC98A4B), C(-2, 0, 1.2f, 0xE04F4F), C(2, 1, 1.2f, 0xE04F4F) } },
    { "cookie", 3, { C(0, 0, 4, 0xC98A4B), C(-1, -1, 0.8f, 0x6F4630), C(1.5f, 1, 0.8f, 0x6F4630) } },
    { "egg", 2, { C(0, 0, 3.5f, 0xFFF6E6), C(-1, -1, 1, 0xFFFFFF) } },
    { "tomato", 3, { C(0, 0, 4, 0xE04F4F), R(-1.5f, -5, 3, 1.5f, 0x49D17A), C(-1.5f, -1.5f, 1, 0xFF8A8A) } },
    { "mushroom", 3, { R(-1.5f, -1, 3, 5, 0xFFF6E6), R(-5, -4, 10, 3.5f, 0xC98A4B), C(-2, -3, 0.8f, 0xFFF6E6) } },
    { "bread", 3, { R(-4, -4, 8, 8, 0xE8B46A), R(-4, -5, 8, 2, 0xC98A4B), R(-3, -3, 6, 6, 0xFFE2A8) } },
    { "potato", 3, { C(0, 0, 4.5f, 0xC9A35A), C(-1.5f, -1, 0.8f, 0x8A6A3C), C(1.5f, 1.5f, 0.7f, 0x8A6A3C) } },
    { "rice", 3, { R(-4, -4, 8, 9, 0xE8D9B8), R(-4, -4, 8, 2, 0xC9B27A), C(0, 1, 1.5f, 0xFFFFFF) } },
    { "sausage", 4, { R(-5, -1.5f, 10, 3.5f, 0xC0504A), C(-5, 0.25f, 1.75f, 0xC0504A), C(5, 0.25f, 1.75f, 0xC0504A), R(-3, -1, 6, 1, 0xE07A6A) } },
    { "pasta", 5, { R(-3, -6, 1, 12, 0xFFE2A8), R(-1, -6, 1, 12, 0xFFE2A8), R(1, -6, 1, 12, 0xFFE2A8), R(3, -6, 1, 12, 0xFFE2A8), R(-4, -1, 9, 2, 0xE04F4F) } },
    { "strawberry", 4, { C(0, 1, 4, 0xE0303F), R(-3, -4, 6, 2, 0x49D17A), C(-1.5f, 0, 0.6f, 0xFFE066), C(1.5f, 2, 0.6f, 0xFFE066) } },
    { "orange", 3, { C(0, 0, 4.5f, 0xFF9A2A), C(-1.5f, -1.5f, 1.2f, 0xFFC27A), R(-0.5f, -6, 1, 2, 0x6F4630) } },
    { "lemon", 4, { C(0, 0, 4, 0xFFE94A), C(-4, 0, 1.5f, 0xFFE94A), C(4, 0, 1.5f, 0xFFE94A), C(-1, -1.5f, 1, 0xFFF6A0) } },
    { "grapes", 5, { C(-2, -2, 2, 0x9A4AD1), C(2, -2, 2, 0x9A4AD1), C(0, 1, 2, 0x9A4AD1), C(0, -4.5f, 1.6f, 0xB06AE6), R(-0.5f, -7, 1, 2.5f, 0x6F4630) } },
    { "cherry", 4, { C(-2.5f, 2, 2.5f, 0xC0203A), C(2.5f, 2, 2.5f, 0xC0203A), R(-1.5f, -5, 1, 5, 0x49A04A), R(1, -5, 1, 5, 0x49A04A) } },
    { "pear", 3, { C(0, 2, 4, 0xB6D84A), C(0, -2, 2.5f, 0xB6D84A), R(-0.5f, -6, 1, 2, 0x6F4630) } },
    { "pineapple", 5, { R(-3.5f, -2, 7, 8, 0xE8B43A), R(-3.5f, 0, 7, 1, 0xA0782A), R(-3.5f, 3, 7, 1, 0xA0782A), R(-2.5f, -6, 1.5f, 4, 0x49D17A), R(1, -6, 1.5f, 4, 0x49D17A) } },
    { "blueberry", 4, { C(-2.5f, 1, 2.2f, 0x3A5AD1), C(2.5f, 1, 2.2f, 0x3A5AD1), C(0, -2.5f, 2.2f, 0x4A6AE6), C(0, -2.5f, 0.6f, 0x1B2A6A) } },
    { "watermelon", 4, { C(0, 0, 5, 0x3FA04A), C(0, 0, 4, 0xFF5A6A), C(-1.5f, 0, 0.5f, 0x1B1426), C(1.5f, 1, 0.5f, 0x1B1426) } },
    { "onion", 3, { C(0, 1, 4, 0xE0B8D0), C(0, 1, 2.5f, 0xF2D8E6), R(-0.5f, -5, 1, 2.5f, 0x8A6A3C) } },
    { "pepper", 3, { R(-3.5f, -3, 7, 8, 0xE0303F), C(0, -3, 3.5f, 0xE0303F), R(-0.5f, -6, 1.5f, 2.5f, 0x49A04A) } },
    { "broccoli", 4, { R(-1, 0, 2, 5, 0x8FD06A), C(-2.5f, -1.5f, 2.2f, 0x2F8A3C), C(2.5f, -1.5f, 2.2f, 0x2F8A3C), C(0, -3.5f, 2.2f, 0x3FA04A) } },
    { "peas", 4, { R(-5, -1.5f, 10, 3.5f, 0x5FB04A), C(-2.5f, 0, 1.3f, 0x9AE06A), C(0, 0, 1.3f, 0x9AE06A), C(2.5f, 0, 1.3f, 0x9AE06A) } },
    { "corn", 5, { R(-2, -5, 4, 10, 0xFFD23F), R(-1, -5, 0.6f, 10, 0xE0A92B), R(0.6f, -5, 0.6f, 10, 0xE0A92B), R(-3.5f, -1, 1.5f, 6, 0x6FBF4A), R(2, -1, 1.5f, 6, 0x6FBF4A) } },
    { "lettuce", 3, { C(0, 0, 4.5f, 0x6FCF5A), C(-1.5f, -1, 2.5f, 0x9AE67A), C(1.5f, 1.5f, 1.5f, 0x4FAF4A) } },
    { "cucumber", 4, { R(-5, -1.5f, 10, 3.5f, 0x3F8A3C), C(-5, 0.25f, 1.75f, 0x3F8A3C), C(5, 0.25f, 1.75f, 0x3F8A3C), R(-3, -1, 6, 0.8f, 0x6FBF6A) } },
    { "pumpkin", 3, { C(0, 1, 4.5f, 0xFF8A2A), R(-0.5f, -3, 1, 8, 0xE0701A), R(-0.5f, -5, 1.5f, 2.5f, 0x6F4630) } },
    { "beans", 4, { C(-2, 0, 1.8f, 0xB8402A), C(1, -1.5f, 1.8f, 0xB8402A), C(1.5f, 1.5f, 1.8f, 0xB8402A), C(-0.5f, 2.5f, 1.5f, 0xA0301A) } },
    { "chicken", 5, { C(1, 0, 3.5f, 0xC9783A), C(1, -0.5f, 2.5f, 0xE09A5A), R(-5, -0.5f, 3.5f, 1.5f, 0xFFF6E6), C(-5, -0.8f, 1, 0xFFF6E6), C(-5, 0.8f, 1, 0xFFF6E6) } },
    { "fish", 3, { C(0, 0, 3.5f, 0x8FB8D8), C(-4, 0, 2.2f, 0x6F98B8), C(2, -1, 0.7f, 0x1B1426) } },
    { "shrimp", 4, { C(1, 0, 3, 0xFF8A6A), C(-1.5f, 2, 2, 0xFF8A6A), C(-3, 3, 1.2f, 0xFFAA8A), C(2, -1, 0.6f, 0x1B1426) } },
    { "bacon", 3, { R(-5, -2.5f, 10, 2, 0xD0505A), R(-5, -0.5f, 10, 1, 0xFFD8D0), R(-5, 0.5f, 10, 2, 0xD0505A) } },
    { "ham", 3, { C(0, 0, 4.5f, 0xF09AA8), C(0, 0, 3, 0xFFB8C0), C(-1, -1, 0.8f, 0xFFFFFF) } },
    { "tofu", 3, { R(-4, -3, 8, 6, 0xF8F0DA), R(-4, -3, 8, 1.2f, 0xE0D6B8), R(-1, -0.5f, 2, 2, 0xE8E0C8) } },
    { "chocolate", 4, { R(-4.5f, -3.5f, 9, 7, 0x6A3A20), R(-4, -3, 4, 3, 0x8A5030), R(0.5f, -3, 4, 3, 0x8A5030), R(-4, 0.5f, 4, 3, 0x8A5030) } },
    { "honey", 3, { R(-3.5f, -3, 7, 8, 0xFFB020), R(-3.5f, -5, 7, 2, 0xC98A4B), R(-2, -1, 4, 2, 0xFFF6A0) } },
    { "yogurt", 3, { R(-3.5f, -3, 7, 7, 0xFFFFFF), R(-3.5f, -4, 7, 1.5f, 0xC28BFF), C(0, 0.5f, 1.5f, 0xFF8FC7) } },
    { "cream", 4, { C(0, 2, 4, 0xFFFFFF), C(0, -1, 3, 0xFFF6F0), C(0, -3.5f, 1.8f, 0xFFFFFF), C(0.5f, -5, 0.8f, 0xFFFFFF) } },
    { "butter", 3, { R(-5, -2, 10, 5, 0xFFE88A), R(-5, -2, 10, 1.2f, 0xFFF6C0), R(-5, 2, 10, 1, 0xD8C060) } },
    { "jam", 3, { R(-3.5f, -3, 7, 8, 0xC0203A), R(-4, -5, 8, 2.5f, 0xFFFFFF), R(-2, 0, 4, 2.5f, 0xFFF6E6) } },
    { "flour", 3, { R(-4, -4, 8, 9, 0xF2EAD8), R(-4, -4, 8, 1.5f, 0xC9B27A), R(-2, 0, 4, 2.5f, 0x5AD1FF) } },
    { "noodles", 5, { C(0, 1, 4.5f, 0xFFFFFF), R(-3, -2, 6, 1, 0xF2D27A), R(-3.5f, 0, 7, 1, 0xF2D27A), R(-3, 2, 6, 1, 0xF2D27A), R(1, -6, 0.7f, 6, 0x8A5A3C) } },
    { "tortilla", 4, { C(0, 0, 5, 0xF2D49B), C(-2, -1.5f, 0.8f, 0xC98A4B), C(1.5f, 1.5f, 0.8f, 0xC98A4B), C(2, -2, 0.6f, 0xC98A4B) } },
    { "oats", 3, { R(-3.5f, -4, 7, 9, 0xE0C080), R(-3.5f, -4, 7, 2, 0x5AD1FF), C(0, 1, 1.5f, 0xC9A35A) } },
    { "nuts", 3, { C(-2, 0, 2.2f, 0xA0704A), C(2, -1, 2.2f, 0xB8844A), C(0, 2, 2, 0x8A5A3C) } },
    { "sprinkles", 5, { R(-3, -4, 6, 9, 0xFFFFFF), R(-3, -4, 6, 2, 0xFF8FC7), R(-2, 0, 1, 2, 0xE04F4F), R(0, -1, 1, 2, 0x5AD1FF), R(1.5f, 1, 1, 2, 0xFFD23F) } },
    [FOOD_RASPBERRY] = { "raspberry", 6, { C(0, -2, 1.8f, 0xE0407A), C(-2, 0, 1.8f, 0xE0407A), C(2, 0, 1.8f, 0xD0306A), C(0, 2.5f, 1.8f, 0xC02060), C(-0.6f, -0.6f, 0.7f, 0xFF8FB0), R(-1.5f, -5.5f, 3, 1.5f, 0x49D17A) } },
    [FOOD_HOTDOG] = { "hot dog", 4, { R(-5, -1.4f, 10, 2.8f, 0xE0604A), C(-5, 0, 1.4f, 0xE0604A), C(5, 0, 1.4f, 0xE0604A), R(-4, -0.9f, 8, 0.8f, 0xF09A80) } },
    [FOOD_STEAK] = { "steak", 5, { C(0, 0, 4.5f, 0x8A2A22), C(0.8f, 0.4f, 3.3f, 0xB03A30), C(-3.2f, -1.6f, 1.4f, 0xFFF0E0), R(-2, -1.2f, 4.5f, 0.7f, 0x5A1610), R(-2, 1, 4.5f, 0.7f, 0x5A1610) } },
    [FOOD_TURKEY] = { "ground turkey", 5, { R(-5, -3, 10, 6, 0xFFFFFF), C(-1.8f, -0.5f, 2.2f, 0xD89A88), C(1.8f, 0, 2.2f, 0xC8887A), C(0, -1.5f, 1.8f, 0xE8AAA0), R(-5, 2.5f, 10, 1.2f, 0x5AD1FF) } },
    [FOOD_MARSHMALLOW] = { "marshmallow", 4, { R(-5, -1.5f, 4.5f, 5, 0xFFF6F0), R(-5, -1.5f, 4.5f, 1.2f, 0xFFFFFF), R(0.5f, -3.5f, 4.5f, 5, 0xFFE4EE), R(0.5f, -3.5f, 4.5f, 1.2f, 0xFFFFFF) } },
    [FOOD_SUGAR] = { "sugar", 6, { R(-5, 1, 10, 4, 0x5AD1FF), R(-4.5f, -2, 3, 3, 0xFFFFFF), R(-1.2f, -2, 3, 3, 0xF0F0F6), R(2, -2, 3, 3, 0xFFFFFF), R(-2.8f, -5, 3, 3, 0xF4F4FA), R(0.5f, -5, 3, 3, 0xFFFFFF) } },
    [FOOD_CHOCCHIPS] = { "chocolate chips", 6, { C(-2.5f, 1, 1.5f, 0x5A3018), C(1.5f, 1.5f, 1.5f, 0x6A3A20), C(0, -1.5f, 1.5f, 0x5A3018), C(-1, 3.5f, 1.2f, 0x6A3A20), C(3, -1, 1.2f, 0x5A3018), C(-3, -2, 1.2f, 0x6A3A20) } },
    [FOOD_NACHO] = { "nacho cheese", 4, { R(-5, 0, 10, 4, 0x3A5AD1), C(0, 0, 4, 0xFFA020), C(-1.5f, -1, 1.2f, 0xFFC860), R(3, 1, 1.2f, 3.5f, 0xFFA020) } },
    [FOOD_PEANUTBUTTER] = { "peanut butter", 4, { R(-3.5f, -3, 7, 8, 0xC08040), R(-4, -5, 8, 2.5f, 0xE04F4F), R(-2.5f, -1, 5, 3, 0xFFF6E6), C(0, 0.5f, 1, 0xC08040) } },
    [FOOD_PORKCHOP] = { "pork chop", 5, { R(-6, -1, 4, 1.6f, 0xFFF6E6), C(-6, -1.6f, 1, 0xFFF6E6), C(0.5f, 0, 4, 0xD08A6A), C(1, 0.5f, 2.6f, 0xE8A890), R(-1.5f, -1, 4, 0.6f, 0x8A4A30) } },
    [FOOD_LAMB] = { "lamb", 5, { C(0, 0.5f, 4.2f, 0x9A4A30), C(0.5f, 0, 3, 0xB86040), R(-1.5f, -7, 1, 4, 0xFFF6E6), R(1.5f, -7, 1, 4, 0xFFF6E6), C(-2.2f, 2, 1, 0x49D17A) } },
    [FOOD_CELERY] = { "celery", 6, { R(-3, -4, 2, 10, 0x9AD06A), R(-0.5f, -4, 2, 10, 0x8AC05A), R(2, -4, 2, 10, 0x9AD06A), C(-2, -5, 1.6f, 0x49A04A), C(0.5f, -5.8f, 1.6f, 0x49A04A), C(3, -5, 1.6f, 0x49A04A) } },
    [FOOD_GRAVY] = { "gravy", 4, { R(-4, -2, 8, 5, 0xF2F2F2), R(4, -2.5f, 2.5f, 1.5f, 0xF2F2F2), R(-6, -1.5f, 2, 2.5f, 0xF2F2F2), R(-3.5f, -2, 7, 1.5f, 0x8A5030) } },
    [FOOD_SALMON] = { "salmon", 5, { R(-5, -2, 10, 5, 0xFF8A6A), R(-5, -2, 10, 1, 0xFFB8A0), R(-3, -1.5f, 0.8f, 4, 0xFFD0C0), R(0, -1.5f, 0.8f, 4, 0xFFD0C0), R(3, -1.5f, 0.8f, 4, 0xFFD0C0) } },
    [FOOD_WHITEFISH] = { "white fish", 5, { R(-5, -2, 9, 5, 0xF4F0E8), C(4, 0.5f, 2.5f, 0xF4F0E8), R(-5, -2, 9, 1, 0xFFFFFF), C(-2, 0.5f, 0.8f, 0xD8D0C0), C(1.5f, 1, 0.8f, 0xD8D0C0) } },
    [FOOD_TCHIPS] = { "tortilla chips", 5, { R(-5, 0, 10, 4, 0xE04F4F), C(-2, -1, 2, 0xF2C46B), C(1.5f, -1.5f, 2, 0xE8B450), C(0, -3, 1.8f, 0xF2C46B), C(3, 0, 1.5f, 0xE8B450) } },
    [FOOD_CHILI] = { "chili", 6, { R(-5, 0, 10, 4, 0xFFFFFF), C(0, 0, 4, 0x9A2A18), C(-1.5f, -1, 1, 0xC04A2A), C(1.5f, 0.5f, 1, 0xC04A2A), C(0, -2, 1, 0x6A1A10), C(1, -3, 0.8f, 0x49D17A) } },
    [FOOD_SOURCREAM] = { "sour cream", 4, { R(-4, -2, 8, 6, 0xFFFFFF), R(-4.5f, -3.5f, 9, 2, 0x49A04A), C(0, 1, 1.2f, 0x49A04A), C(0, -4.5f, 1.5f, 0xFFFFFF) } },
    [FOOD_DISH0 + DISH_OMELETTE] = { "Omelette", 3, { C(0, 0, 5, 0xFFE066), C(-2, -1, 1.5f, 0xFFF6A0), C(2, 1, 1, 0xE0A92B) } },
    [FOOD_DISH0 + DISH_CHEESY_OMELETTE] = { "Cheesy omelette", 4, { C(0, 0, 5, 0xFFE066), C(-2, -1, 1.5f, 0xFFF6A0), R(-3, 0, 3, 2, 0xFFB020), R(1, -2, 3, 2, 0xFFB020) } },
    [FOOD_DISH0 + DISH_MUSHROOM_OMELETTE] = { "Mushroom omelette", 5, { C(0, 0, 5, 0xFFE066), C(-2, -1, 1.5f, 0xFFF6A0), C(-2, 1.5f, 1.3f, 0xC98A4B), C(2, -1, 1.3f, 0xC98A4B), R(1.6f, -0.5f, 0.8f, 1.5f, 0xFFF6E6) } },
    [FOOD_DISH0 + DISH_TOMATO_OMELETTE] = { "Tomato omelette", 5, { C(0, 0, 5, 0xFFE066), C(-2, -1, 1.5f, 0xFFF6A0), C(-2, 1.5f, 1.3f, 0xE04F4F), C(2, -1, 1.3f, 0xE04F4F), C(1, 2, 1, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_POTATO_OMELETTE] = { "Potato omelette", 5, { C(0, 0, 5, 0xF0C850), C(-2, -1, 1.5f, 0xFFE066), C(-2, 1.5f, 1.3f, 0xC9A35A), C(2, -1, 1.3f, 0xC9A35A), C(1.5f, 2, 1, 0xC9A35A) } },
    [FOOD_DISH0 + DISH_BIG_BREAKFAST] = { "Big breakfast", 6, { C(0, 0, 6, 0xFFFFFF), C(-2, -1, 2.8f, 0xFFF6E6), C(-2, -1, 1.3f, 0xFFB020), R(1, -3, 4, 1.8f, 0xC0504A), R(1, 0, 4, 1.8f, 0xC0504A), C(-1, 3, 1.2f, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_TOAST] = { "Toast", 3, { R(-5, -3, 10, 6, 0xC98A4B), R(-4, -2.5f, 8, 5, 0xE8B46A), R(-2, -1.5f, 4, 2.5f, 0xFFF6A0) } },
    [FOOD_DISH0 + DISH_CHEESE_TOASTIE] = { "Cheese toastie", 4, { R(-5, -3, 10, 6, 0xC98A4B), R(-4, -2.5f, 8, 5, 0xE8B46A), R(-5, -1, 10, 2, 0xFFD23F), R(-4, 1, 2, 1.5f, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_PIZZA_TOASTIE] = { "Pizza toastie", 6, { R(-5, -3, 10, 6, 0xC98A4B), R(-4, -2.5f, 8, 5, 0xE8B46A), R(-4, -2, 8, 4, 0xE04F4F), C(-1.5f, -0.5f, 1.2f, 0xFFD23F), C(1.5f, 0.5f, 1.2f, 0xFFD23F), C(0, -1.5f, 0.8f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_MUSHROOM_TOAST] = { "Mushroom toast", 5, { R(-5, -3, 10, 6, 0xC98A4B), R(-4, -2.5f, 8, 5, 0xE8B46A), C(-2, -1, 1.5f, 0xC98A4B), C(2, 0, 1.5f, 0xC98A4B), C(0, -2, 1, 0xFFF6E6) } },
    [FOOD_DISH0 + DISH_TOMATO_SOUP] = { "Tomato soup", 3, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xE04F4F), C(-1.5f, -2, 1, 0xFFFFFF) } },
    [FOOD_DISH0 + DISH_MUSHROOM_SOUP] = { "Mushroom soup", 4, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xC9A37A), C(1, -2, 1.3f, 0x8A6A4A), C(-1.5f, -1, 1, 0x8A6A4A) } },
    [FOOD_DISH0 + DISH_CARROT_SOUP] = { "Carrot soup", 3, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xFF8A3D), C(-1, -2, 1, 0x49D17A) } },
    [FOOD_DISH0 + DISH_CHEESE_SOUP] = { "Cheese soup", 4, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xFFD23F), C(1, -2, 1, 0xFFF6A0), C(-1.5f, -1, 0.8f, 0xFFF6A0) } },
    [FOOD_DISH0 + DISH_VEGGIE_STEW] = { "Veggie stew", 5, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xA0603A), C(-2, -2, 1, 0xFF8A3D), C(1, -1, 1, 0x49D17A), C(2, -2.5f, 1, 0xC9A35A) } },
    [FOOD_DISH0 + DISH_SAUSAGE_STEW] = { "Sausage stew", 4, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xA0603A), R(-3, -2.5f, 3, 1.2f, 0xC0504A), R(1, -1.5f, 3, 1.2f, 0xC0504A) } },
    [FOOD_DISH0 + DISH_HOTDOG] = { "Hot dog", 3, { R(-6, -1, 12, 4, 0xE8B46A), R(-5, -2.5f, 10, 2.5f, 0xC0504A), R(-4, -2, 8, 0.8f, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_CHEESY_HOTDOG] = { "Cheesy hot dog", 4, { R(-6, -1, 12, 4, 0xE8B46A), R(-5, -2.5f, 10, 2.5f, 0xC0504A), R(-5, -3, 10, 1.4f, 0xFFB020), R(-3, -1.8f, 1.5f, 1.5f, 0xFFB020) } },
    [FOOD_DISH0 + DISH_KETCHUP_HOTDOG] = { "Hot dog and ketchup", 5, { R(-6, -1, 12, 4, 0xE8B46A), R(-5, -2.5f, 10, 2.5f, 0xC0504A), R(-4, -2, 2, 0.8f, 0xE04F4F), R(-1, -2.6f, 2, 0.8f, 0xE04F4F), R(2, -2, 2, 0.8f, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_SPAGHETTI] = { "Spaghetti and sauce", 5, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFE2A8), R(-3, -2, 6, 0.8f, 0xF0CC88), C(0, -2, 2.2f, 0xE04F4F), C(1, -2.5f, 0.8f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_MEATBALLS] = { "Spaghetti and meatballs", 6, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFE2A8), R(-3, -2, 6, 0.8f, 0xF0CC88), C(0, -2, 2.4f, 0xE04F4F), C(-1.5f, -1.5f, 1.3f, 0x8A4A3A), C(1.5f, -2, 1.3f, 0x8A4A3A) } },
    [FOOD_DISH0 + DISH_MAC_CHEESE] = { "Mac and cheese", 5, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFB020), C(-1.5f, -1.5f, 0.9f, 0xFFE066), C(1.5f, -0.5f, 0.9f, 0xFFE066), C(0, -3, 0.9f, 0xFFE066) } },
    [FOOD_DISH0 + DISH_MUSHROOM_PASTA] = { "Mushroom pasta", 5, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFE2A8), R(-3, -2, 6, 0.8f, 0xF0CC88), C(-1.5f, -1.5f, 1.3f, 0xC98A4B), C(1.5f, -1, 1.3f, 0xC98A4B) } },
    [FOOD_DISH0 + DISH_BUTTER_PASTA] = { "Buttered pasta", 4, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFE2A8), R(-3, -2, 6, 0.8f, 0xF0CC88), R(-1, -2.5f, 2, 1.5f, 0xFFF6A0) } },
    [FOOD_DISH0 + DISH_FRIED_RICE] = { "Egg fried rice", 5, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xF5E08A), C(-2, -2, 1, 0xFFD23F), C(2, -1, 1, 0x49D17A), C(0, -3, 0.8f, 0xFFF6E6) } },
    [FOOD_DISH0 + DISH_SAUSAGE_RICE] = { "Sausage rice", 4, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xFFFFFF), R(-3, -2.5f, 3, 1.3f, 0xC0504A), R(0.5f, -1.5f, 3, 1.3f, 0xC0504A) } },
    [FOOD_DISH0 + DISH_VEGGIE_RICE] = { "Veggie rice", 5, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xFFFFFF), C(-2, -2, 1, 0xFF8A3D), C(2, -1, 1, 0x49D17A), C(0, -3, 0.9f, 0xC98A4B) } },
    [FOOD_DISH0 + DISH_RICE_PUDDING] = { "Rice pudding", 4, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xFFF0D8), C(0, -2, 1.3f, 0xFFE066), C(1.5f, -1, 0.7f, 0xC98A4B) } },
    [FOOD_DISH0 + DISH_RICE_BOWL] = { "Rice bowl", 4, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xFFFFFF), C(0, -3, 1.2f, 0xFFFFFF), R(1, -5, 0.6f, 4, 0x8A5A3C) } },
    [FOOD_DISH0 + DISH_APPLE_PIE] = { "Apple pie", 5, { C(0, 0, 5.5f, 0xC98A4B), C(0, -0.5f, 4, 0xE8B46A), R(-3, -1, 6, 1, 0xC98A4B), R(-0.5f, -4, 1, 7, 0xC98A4B), C(0, -0.5f, 1, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_FRUIT_PIE] = { "Fruit pie", 6, { C(0, 0, 5.5f, 0xC98A4B), C(0, -0.5f, 4, 0xE8B46A), C(-1.5f, -1.5f, 1.2f, 0xE04F4F), C(1.5f, 0, 1.2f, 0xFFE066), C(-1, 1.5f, 1.2f, 0xC28BFF), C(1.5f, -2, 1, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_PANCAKES] = { "Banana pancakes", 5, { R(-5, 1, 10, 2.5f, 0xE8B46A), R(-5, -1.5f, 10, 2.5f, 0xC98A4B), R(-5, -4, 10, 2.5f, 0xE8B46A), C(-2, -4.5f, 1.2f, 0xFFE066), C(1.5f, -4.5f, 1.2f, 0xFFE066) } },
    [FOOD_DISH0 + DISH_CHIPS] = { "Chips", 5, { R(-4, -1, 8, 6, 0xE04F4F), R(-3, -5, 1.5f, 5, 0xFFD23F), R(-1, -6, 1.5f, 6, 0xFFD23F), R(1, -5, 1.5f, 5, 0xFFD23F), R(3, -4.5f, 1.2f, 4, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_CHEESY_CHIPS] = { "Cheesy chips", 6, { R(-4, -1, 8, 6, 0xE04F4F), R(-3, -5, 1.5f, 5, 0xFFD23F), R(-1, -6, 1.5f, 6, 0xFFD23F), R(1, -5, 1.5f, 5, 0xFFD23F), R(3, -4.5f, 1.2f, 4, 0xFFD23F), R(-4, -3.5f, 8, 1.5f, 0xFFB020) } },
    [FOOD_DISH0 + DISH_SAUSAGE_CHIPS] = { "Sausage and chips", 6, { C(0, 0, 6, 0xFFFFFF), R(-4, -3, 1.2f, 5, 0xFFD23F), R(-2.3f, -3.5f, 1.2f, 5, 0xFFD23F), R(-0.6f, -3, 1.2f, 5, 0xFFD23F), R(1, -1, 4.5f, 2, 0xC0504A), R(1, 1.5f, 4.5f, 2, 0xC0504A) } },
    [FOOD_DISH0 + DISH_BANANA_BREAD] = { "Banana bread", 4, { R(-5, -3, 10, 6, 0x8A5A3C), R(-4, -2, 8, 4, 0xC98A4B), C(-1.5f, -2.5f, 1, 0xFFE066), C(1.5f, -2.5f, 1, 0xFFE066) } },
    [FOOD_DISH0 + DISH_CARROT_CAKE] = { "Carrot cake", 5, { R(-5, -2, 10, 6, 0xC98A4B), R(-5, -3, 10, 2, 0xFFF6E6), R(-0.5f, -6, 1, 3, 0xFF8A3D), R(-1.5f, -7, 1, 1.5f, 0x49D17A), R(0.5f, -7, 1, 1.5f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_CHEESE_PLATE] = { "Melted cheese", 3, { C(0, 1, 5.5f, 0xFFFFFF), C(0, 0, 4, 0xFFB020), C(-1, -1, 1.2f, 0xFFE066) } },
    [FOOD_DISH0 + DISH_CURRY] = { "Chicken curry", 5, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xE0A030), C(-2, -2, 1.3f, 0xE09A5A), C(2, -1, 1.3f, 0xE09A5A), C(0, -3, 0.8f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_ROAST] = { "Roast chicken", 5, { C(0, 1, 6, 0xFFFFFF), C(0, 0, 4.5f, 0xC9783A), C(-1.5f, -1.5f, 1.8f, 0xE09A5A), R(3.5f, -3, 3, 1.5f, 0xFFF6E6), R(-6.5f, -3, 3, 1.5f, 0xFFF6E6) } },
    [FOOD_DISH0 + DISH_FISH_SUPPER] = { "Fish supper", 5, { C(0, 1, 6, 0xFFFFFF), R(-4.5f, -2, 7, 4, 0xE0A040), C(3, 0, 1.8f, 0xE0A040), R(-4, 2, 3, 1.5f, 0x9AE06A), C(4, 2.5f, 1, 0xFFE94A) } },
    [FOOD_DISH0 + DISH_SKEWERS] = { "Shrimp skewers", 4, { R(-6, -0.5f, 12, 1, 0xC98A4B), C(-3, 0, 2, 0xFF8A6A), C(0, 0, 2, 0xFF8A6A), C(3, 0, 2, 0xFF8A6A) } },
    [FOOD_DISH0 + DISH_TACOS] = { "Tacos", 5, { C(0, 1, 5, 0xF2D49B), R(-4, -2, 8, 3, 0xF2D49B), C(-1.5f, -2, 1.3f, 0x6FCF5A), C(1, -2.5f, 1.2f, 0xE0303F), C(2.5f, -1.5f, 1, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_PORRIDGE] = { "Porridge", 4, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -1, 4.5f, 0xF2E0B0), C(-1, -2, 1, 0xC98A4B), C(1.5f, -1, 0.8f, 0xFFB020) } },
    [FOOD_DISH0 + DISH_CHOC_CAKE] = { "Chocolate cake", 4, { R(-5, -2, 10, 6, 0x6A3A20), R(-5, -3, 10, 2, 0x8A5030), R(-5, 0.5f, 10, 1, 0xFFF6E6), C(0, -4.5f, 1.5f, 0xC0203A) } },
    [FOOD_DISH0 + DISH_SUNDAE] = { "Sundae", 5, { R(-2.5f, 1, 5, 5, 0xBFE8FF), C(0, -1, 4, 0xFFF0F6), C(-1.5f, -2, 2, 0xFF8FC7), C(1.5f, -3, 1.6f, 0x8A5030), C(0, -5, 1.2f, 0xC0203A) } },
    [FOOD_DISH0 + DISH_SMOOTHIE] = { "Smoothie", 4, { R(-3, -4, 6, 10, 0xBFE8FF), R(-2.5f, -2, 5, 7.5f, 0xFF8FC7), R(1, -8, 0.8f, 5, 0xFFFFFF), C(-1, -3, 0.8f, 0xFFFFFF) } },
    [FOOD_DISH0 + DISH_PUMPKIN_PIE] = { "Pumpkin pie", 3, { C(0, 0, 5.5f, 0xC98A4B), C(0, -0.5f, 4, 0xFF8A2A), C(0, -1.5f, 1.2f, 0xFFFFFF) } },
    [FOOD_DISH0 + DISH_FRUIT_SALAD] = { "Fruit salad", 5, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0xFFF0D8), C(-2, -2, 1.3f, 0xE0303F), C(1.5f, -1.5f, 1.3f, 0xFF9A2A), C(0, -3, 1.2f, 0x9A4AD1) } },
    [FOOD_DISH0 + DISH_SALAD] = { "Garden salad", 4, { R(-6, 0, 12, 4, 0xFFFFFF), C(0, -1, 4.5f, 0x6FCF5A), C(-2, -2, 1.2f, 0x9AE67A), C(2, -1.5f, 1.1f, 0xE0303F) } },
    [FOOD_DISH0 + DISH_STIR_FRY] = { "Stir-fry", 5, { C(0, 1, 6, 0x3A3F4A), C(0, 0, 4.5f, 0xD8A05A), C(-2, -1, 1.2f, 0x2F8A3C), C(1.5f, 0.5f, 1.2f, 0xE0303F), C(0.5f, -2, 1, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_COOKIES] = { "Cookies", 4, { C(-2, 1, 3, 0xC98A4B), C(2, -1, 3, 0xD8A05A), C(-2.5f, 0.5f, 0.7f, 0x6A3A20), C(2, -1.5f, 0.7f, 0x6A3A20) } },
    [FOOD_DISH0 + DISH_CHILI_DOG] = { "Chili dog", 6, { R(-6, -1, 12, 4, 0xE8B46A), R(-5, -2.5f, 10, 2.5f, 0xC0504A), C(-2.5f, -2.8f, 1.6f, 0x9A2A18), C(0.5f, -3, 1.6f, 0x9A2A18), C(3, -2.6f, 1.4f, 0x9A2A18), C(-1, -3.6f, 0.6f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_BLT] = { "BLT sandwich", 6, { R(-5, -4, 10, 2.5f, 0xE8B46A), R(-5.5f, -1.6f, 11, 1.2f, 0x6FCF5A), R(-5, -0.6f, 10, 1.3f, 0xD0505A), R(-5, 0.6f, 10, 1.2f, 0xE04F4F), R(-5, 1.8f, 10, 2.5f, 0xE8B46A), R(-5, -4, 10, 0.8f, 0xC98A4B) } },
    [FOOD_DISH0 + DISH_STEAK_EGGS] = { "Steak and eggs", 6, { C(0, 1, 6, 0xFFFFFF), C(-2, 0.5f, 3.3f, 0x8A2A22), R(-4, 0, 4, 0.6f, 0x5A1610), C(2.8f, -1.5f, 2.2f, 0xFFFFFF), C(2.8f, -1.5f, 1, 0xFFB020), C(-1.5f, -1.2f, 0.9f, 0xB03A30) } },
    [FOOD_DISH0 + DISH_STEAK_POTATOES] = { "Steak and potatoes", 6, { C(0, 1, 6, 0xFFFFFF), C(-1.5f, 0.5f, 3.5f, 0x8A2A22), R(-3.5f, -0.5f, 4, 0.6f, 0x5A1610), R(-3.5f, 1.2f, 4, 0.6f, 0x5A1610), C(3, -1.5f, 1.6f, 0xC9A35A), C(3, 2, 1.5f, 0xD8B46A) } },
    [FOOD_DISH0 + DISH_STEAK_DINNER] = { "Steak dinner", 6, { C(0, 1, 6, 0xFFFFFF), C(-1, 0.5f, 3.8f, 0x8A2A22), R(-3, -0.5f, 4.5f, 0.7f, 0x5A1610), R(-3, 1.5f, 4.5f, 0.7f, 0x5A1610), C(3.5f, -1.5f, 1.5f, 0x2F8A3C), C(3.3f, 2.2f, 1.3f, 0xFFD23F) } },
    [FOOD_DISH0 + DISH_PORK_APPLE] = { "Pork chop and apples", 6, { C(0, 1, 6, 0xFFFFFF), C(-1, 0, 3.5f, 0xD08A6A), C(-0.6f, 0.4f, 2.2f, 0xE8A890), R(1.5f, -3.5f, 4, 1.3f, 0xFFF6E6), C(3, 2.5f, 1.5f, 0xE8D070), C(3.4f, 2.1f, 0.5f, 0xFFF6A0) } },
    [FOOD_DISH0 + DISH_PORK_DINNER] = { "Pork chop dinner", 6, { C(0, 1, 6, 0xFFFFFF), C(-1, 0, 3.5f, 0xD08A6A), C(-0.6f, 0.4f, 2.2f, 0xE8A890), R(1.5f, -3.5f, 4, 1.3f, 0xFFF6E6), C(3.2f, 2.2f, 1.4f, 0x5FB04A), C(2, 3, 1, 0x5FB04A) } },
    [FOOD_DISH0 + DISH_SHEPHERDS_PIE] = { "Shepherd's pie", 6, { R(-6, -1, 12, 5, 0xE0701A), R(-5.5f, -2.5f, 11, 3, 0xFFF0C8), C(-3, -2.8f, 1.4f, 0xFFF6E0), C(0, -3.2f, 1.4f, 0xFFF6E0), C(3, -2.8f, 1.4f, 0xFFF6E0), R(-5.5f, 0.5f, 11, 1, 0x9A4A30) } },
    [FOOD_DISH0 + DISH_LAMB_STEW] = { "Lamb stew", 6, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0x8A5030), C(-2, -2, 1.2f, 0x9A4A30), C(1.5f, -1, 1.2f, 0x9A4A30), C(0, -3, 0.9f, 0xFF8A3D), C(2.5f, -2.5f, 0.8f, 0x9AD06A) } },
    [FOOD_DISH0 + DISH_ROAST_LAMB] = { "Roast lamb", 6, { C(0, 1, 6, 0xFFFFFF), C(0, 0, 4, 0x9A4A30), C(0.5f, -0.5f, 2.8f, 0xB86040), R(-1, -6, 1, 3.5f, 0xFFF6E6), R(1.5f, -6, 1, 3.5f, 0xFFF6E6), C(-3, 2, 1.2f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_TURKEY_DINNER] = { "Turkey dinner", 6, { C(0, 1, 6, 0xFFFFFF), C(-1.5f, 0, 3.5f, 0xE8C8A0), R(-4, -0.5f, 5, 1.5f, 0x8A5030), C(3, -1.5f, 1.8f, 0xFFF0C8), C(3, 2, 1.3f, 0xC0203A), C(-1, 2.8f, 1, 0x5FB04A) } },
    [FOOD_DISH0 + DISH_TURKEY_BURGER] = { "Turkey burger", 6, { R(-5, 1, 10, 2.5f, 0xE8B46A), R(-5, -1, 10, 2, 0xA06A50), R(-5.5f, -1.6f, 11, 0.9f, 0x6FCF5A), R(-5, -4.5f, 10, 3, 0xE8B46A), C(-2, -4, 0.5f, 0xFFF6E6), C(1.5f, -3.6f, 0.5f, 0xFFF6E6) } },
    [FOOD_DISH0 + DISH_BURRITO] = { "Burrito", 6, { R(-6, -2.5f, 12, 5, 0xF2D49B), C(-6, 0, 2.5f, 0xF2D49B), C(6, 0, 2.5f, 0xE8C48B), R(-3, -2.5f, 0.8f, 5, 0xE0C080), R(1, -2.5f, 0.8f, 5, 0xE0C080), C(6, 0, 1.2f, 0x6FCF5A) } },
    [FOOD_DISH0 + DISH_SUSHI] = { "Sushi", 6, { C(-3, 0, 2.6f, 0x1B2A1A), C(-3, 0, 1.8f, 0xFFFFFF), C(-3, 0, 0.9f, 0xFF8A6A), C(3, 0, 2.6f, 0x1B2A1A), C(3, 0, 1.8f, 0xFFFFFF), C(3, 0, 0.9f, 0xFF8A6A) } },
    [FOOD_DISH0 + DISH_BACON_EGGS] = { "Bacon and eggs", 6, { C(0, 1, 6, 0xFFFFFF), C(-2, -1, 2.6f, 0xFFFFFF), C(-2, -1, 1.2f, 0xFFB020), R(0.5f, -3, 5, 1.4f, 0xD0505A), R(0.5f, -0.5f, 5, 1.4f, 0xD0505A), R(0.5f, 2, 5, 1.4f, 0xD0505A) } },
    [FOOD_DISH0 + DISH_PLAIN_PANCAKES] = { "Pancakes", 6, { R(-5, 1, 10, 2.5f, 0xE8B46A), R(-5, -1.5f, 10, 2.5f, 0xC98A4B), R(-5, -4, 10, 2.5f, 0xE8B46A), R(-1.5f, -5, 3, 1.5f, 0xFFE88A), R(2, -4, 1, 4, 0xC0782A), R(-4, -4, 1, 3, 0xC0782A) } },
    [FOOD_DISH0 + DISH_ANTS_LOG] = { "Ants on a log", 6, { R(-6, -1.5f, 12, 4, 0x9AD06A), R(-6, -2, 12, 1.6f, 0xC08040), C(-3.5f, -2.4f, 0.7f, 0x3A1A30), C(-1, -2.4f, 0.7f, 0x3A1A30), C(1.5f, -2.4f, 0.7f, 0x3A1A30), C(4, -2.4f, 0.7f, 0x3A1A30) } },
    [FOOD_DISH0 + DISH_PB_TOAST] = { "Peanut butter toast", 5, { R(-5, -3, 10, 6, 0xC98A4B), R(-4, -2.5f, 8, 5, 0xE8B46A), R(-3.5f, -2, 7, 4, 0xC08040), C(-1, -0.5f, 0.6f, 0xA06A30), C(1.5f, 0.5f, 0.6f, 0xA06A30) } },
    [FOOD_DISH0 + DISH_PBJ] = { "PB and J sandwich", 6, { R(-5, -3.5f, 10, 3, 0xF2E0B0), R(-5, -0.5f, 10, 1.2f, 0xC08040), R(-5, 0.7f, 10, 1.2f, 0xC0203A), R(-5, 1.9f, 10, 2.5f, 0xF2E0B0), R(-5, -3.5f, 10, 0.8f, 0xC98A4B), R(-5, 3.6f, 10, 0.8f, 0xC98A4B) } },
    [FOOD_DISH0 + DISH_MASH_GRAVY] = { "Mash and gravy", 6, { C(0, 1, 6, 0xFFFFFF), C(0, 0, 4, 0xFFF0C8), C(0, -0.5f, 2.5f, 0x8A5030), C(-0.6f, -1, 0.8f, 0xA06A40), C(-3.5f, 2, 1, 0x5FB04A), C(-2.2f, 3, 1, 0x5FB04A) } },
    [FOOD_DISH0 + DISH_BAKED_POTATO] = { "Baked potato", 6, { C(0, 0.5f, 5, 0x9A6A3C), C(0, 0, 3.5f, 0xFFF0C8), C(0, -1, 2, 0xFFFFFF), C(-0.8f, -2.2f, 0.6f, 0x49D17A), C(0.8f, -1.5f, 0.6f, 0x49D17A), R(-1, 1, 2, 1, 0xFFE88A) } },
    [FOOD_DISH0 + DISH_FISH_CHIPS] = { "Fish and chips", 6, { C(0, 1, 6, 0xFFFFFF), R(-5, -2, 6, 3.5f, 0xE0A040), C(1, -0.3f, 1.8f, 0xE0A040), R(2, -1, 1.2f, 4, 0xFFD23F), R(3.5f, -1.5f, 1.2f, 4, 0xFFD23F), C(-3, 2.5f, 1, 0xFFE94A) } },
    [FOOD_DISH0 + DISH_MASHED] = { "Mashed potatoes", 5, { R(-6, 0, 12, 4, 0x5AD1FF), C(0, -0.5f, 4.5f, 0xFFF0C8), C(-1.5f, -2, 1.8f, 0xFFF6E0), R(-1, -2.5f, 2, 1.5f, 0xFFE88A), C(2, -0.5f, 0.6f, 0xE8D8A8) } },
    [FOOD_DISH0 + DISH_BISCUITS_GRAVY] = { "Biscuits and gravy", 6, { C(0, 1, 6, 0xFFFFFF), C(-2, 0, 2.8f, 0xE8B46A), C(2, 0, 2.8f, 0xE8B46A), R(-5, -1, 10, 2, 0xE8D8C0), C(-1, 0, 0.6f, 0x6A5040), C(2, -0.5f, 0.6f, 0x6A5040) } },
    [FOOD_DISH0 + DISH_NUGGETS] = { "Chicken nuggets", 6, { C(0, 1, 6, 0xFFFFFF), C(-2.5f, -1, 1.8f, 0xE0A040), C(0.5f, -2, 1.8f, 0xD89030), C(-1, 1.5f, 1.8f, 0xE0A040), C(2, 1, 1.8f, 0xD89030), C(3.5f, -2, 1.2f, 0xE04F4F) } },
    [FOOD_DISH0 + DISH_CHICKEN_SOUP] = { "Chicken noodle soup", 6, { R(-5, -1, 10, 5, 0x5AD1FF), C(0, -1, 4.5f, 0xF2D27A), R(-3, -2, 6, 0.7f, 0xFFF6D0), R(-3, -0.5f, 6, 0.7f, 0xFFF6D0), C(-1.5f, -2.5f, 0.9f, 0xE09A5A), C(2, -1.5f, 0.8f, 0x9AD06A) } },
    [FOOD_DISH0 + DISH_GRILLED_SALMON] = { "Grilled salmon", 6, { C(0, 1, 6, 0xFFFFFF), R(-4, -2, 7, 4, 0xFF8A6A), R(-3, -2, 0.7f, 4, 0xC0503A), R(-0.5f, -2, 0.7f, 4, 0xC0503A), R(2, -2, 0.7f, 4, 0xC0503A), C(4, 2, 1.3f, 0xFFE94A) } },
    [FOOD_DISH0 + DISH_CHILI_BOWL] = { "Bowl of chili", 6, { R(-5, -1, 10, 5, 0xFFD23F), C(0, -1, 4.5f, 0x9A2A18), C(-1.8f, -1.5f, 1, 0xC04A2A), C(1.8f, -0.5f, 1, 0xC04A2A), C(0, -3, 1.3f, 0xFFFFFF), R(-1, -4.6f, 2, 0.8f, 0xFFA020) } },
    [FOOD_DISH0 + DISH_QUESADILLA] = { "Quesadilla", 6, { C(0, 0, 5, 0xF2D49B), R(-5, -0.5f, 10, 1, 0xFFB020), C(-2, -2, 0.7f, 0xC98A4B), C(2, 2, 0.7f, 0xC98A4B), C(2, -2.5f, 0.6f, 0xC98A4B), R(-0.5f, -0.5f, 1, 1, 0x49D17A) } },
    [FOOD_DISH0 + DISH_NACHOS] = { "Nachos", 6, { C(0, 1, 6, 0xFFFFFF), C(-2, 0, 2.2f, 0xF2C46B), C(2, -0.5f, 2.2f, 0xE8B450), C(0, -2, 2, 0xF2C46B), R(-3, -1, 6, 1.2f, 0xFFA020), C(1, 1.5f, 0.8f, 0x49A04A) } },
    [FOOD_DISH0 + DISH_CHIPS_DIP] = { "Chips and dip", 6, { C(0, 1, 6, 0xFFFFFF), C(2, 0, 2.5f, 0x5AD1FF), C(2, 0, 1.8f, 0xFFFFFF), C(-2.5f, -1.5f, 1.8f, 0xF2C46B), C(-3, 1.5f, 1.8f, 0xE8B450), C(2.3f, -0.4f, 0.5f, 0x49A04A) } },
    [FOOD_DISH0 + DISH_HOMEMADE_PIZZA] = { "Homemade pizza", 6, { C(0, 0, 5.5f, 0xE8B46A), C(0, 0, 4.5f, 0xE04F4F), C(0, 0, 3.8f, 0xFFD23F), C(-1.8f, -1, 0.9f, 0xC0303A), C(1.5f, 1, 0.9f, 0xC0303A), C(1, -1.8f, 0.6f, 0x49D17A) } },
    [FOOD_DISH0 + DISH_BERRY_CRUMBLE] = { "Berry crumble", 6, { R(-6, -1, 12, 5, 0xFFFFFF), R(-5.5f, -1.5f, 11, 3, 0xC0306A), C(-3, -2, 1.3f, 0xE0C080), C(0, -2.3f, 1.3f, 0xD8B070), C(3, -2, 1.3f, 0xE0C080), C(-1.5f, -0.6f, 0.7f, 0x3A5AD1) } },
    [FOOD_DISH0 + DISH_SMORES] = { "S'mores", 6, { R(-5, 1, 10, 3, 0xD8A060), R(-4.5f, -0.8f, 9, 1.8f, 0x6A3A20), R(-4, -3.3f, 8, 2.5f, 0xFFF6F0), R(-5, -6, 10, 2.8f, 0xD8A060), C(-2, -4.6f, 0.5f, 0xB88040), C(2, -4.6f, 0.5f, 0xB88040) } },
    [FOOD_DISH0 + DISH_CHOCCHIP_COOKIES] = { "Choc chip cookies", 6, { C(-2, 1, 3, 0xD8A05A), C(2, -1, 3, 0xE8B46A), C(-2.5f, 0.5f, 0.7f, 0x3A2010), C(-1, 2, 0.7f, 0x3A2010), C(2, -1.5f, 0.7f, 0x3A2010), C(3, 0, 0.6f, 0x3A2010) } },
    [FOOD_DISH0 + DISH_CUPCAKES] = { "Cupcakes", 6, { R(-3.5f, 0, 7, 5, 0xFF8FC7), R(-2.5f, 0, 0.8f, 5, 0xE070A8), R(0.5f, 0, 0.8f, 5, 0xE070A8), C(0, -1.5f, 3.8f, 0xFFF0F6), C(0, -3.5f, 2.2f, 0xFFF6FA), C(0, -5.5f, 1.1f, 0xC0203A) } },
    [FOOD_DISH0 + DISH_LEMONADE] = { "Lemonade", 6, { R(-3, -4, 6, 10, 0xBFE8FF), R(-2.5f, -2, 5, 7.5f, 0xFFF080), R(1, -8, 0.8f, 5, 0xFF8FC7), C(-3, -4, 2, 0xFFE94A), C(-3, -4, 1.2f, 0xFFF6A0), R(-1.5f, -1, 1.5f, 1.5f, 0xFFFFFF) } },
    [FOOD_DISH0 + DISH_POPCORN] = { "Popcorn", 6, { R(-4, -1, 8, 6, 0xFFFFFF), R(-2.5f, -1, 1.2f, 6, 0xE04F4F), R(1.3f, -1, 1.2f, 6, 0xE04F4F), C(-2, -2, 1.8f, 0xFFF6D0), C(1.5f, -2.5f, 1.8f, 0xFFF0B0), C(0, -4, 1.8f, 0xFFF6D0) } },
};
const int ck_cook_ingr[CK_COOK_INGR] = {
    FOOD_APPLE, FOOD_BANANA, FOOD_STRAWBERRY, FOOD_RASPBERRY, FOOD_ORANGE, FOOD_LEMON, FOOD_GRAPES, FOOD_CHERRY, FOOD_PEAR, FOOD_PINEAPPLE, FOOD_BLUEBERRY, FOOD_WATERMELON,   // Fruit
    FOOD_TOMATO, FOOD_MUSHROOM, FOOD_CARROT, FOOD_POTATO, FOOD_ONION, FOOD_PEPPER, FOOD_BROCCOLI, FOOD_PEAS, FOOD_CORN, FOOD_LETTUCE, FOOD_CUCUMBER, FOOD_CELERY, FOOD_PUMPKIN, FOOD_BEANS,   // Veggies
    FOOD_CHICKEN, FOOD_STEAK, FOOD_PORKCHOP, FOOD_LAMB, FOOD_TURKEY, FOOD_SAUSAGE, FOOD_HOTDOG, FOOD_BACON, FOOD_HAM, FOOD_TOFU,   // Meat
    FOOD_FISH, FOOD_SALMON, FOOD_WHITEFISH, FOOD_SHRIMP,   // Fish
    FOOD_EGG, FOOD_CHEESE, FOOD_NACHO, FOOD_BUTTER, FOOD_YOGURT, FOOD_CREAM, FOOD_SOURCREAM,   // Eggs and dairy
    FOOD_BREAD, FOOD_RICE, FOOD_PASTA, FOOD_NOODLES, FOOD_TORTILLA, FOOD_TCHIPS, FOOD_FLOUR, FOOD_OATS,   // Bread and grains
    FOOD_GRAVY, FOOD_CHILI, FOOD_PEANUTBUTTER, FOOD_JAM, FOOD_HONEY,   // Sauces and spreads
    FOOD_CHOCOLATE, FOOD_CHOCCHIPS, FOOD_MARSHMALLOW, FOOD_SUGAR, FOOD_NUTS, FOOD_SPRINKLES,   // Sweets
};
const int ck_cook_group[CK_COOK_GROUPS + 1] = { 0, 12, 26, 36, 40, 47, 55, 60, 66 };
const char *const ck_cook_group_name[CK_COOK_GROUPS] = { "Fruit", "Veggies", "Meat", "Fish", "Eggs and dairy", "Bread and grains", "Sauces and spreads", "Sweets" };
int ck_cook_group_of(int i) { int g = 0; while (g < CK_COOK_GROUPS - 1 && i >= ck_cook_group[g + 1]) g++; return g; }
#undef C
#undef R
const ck_letter ck_letters[CK_LETTERS] = {
    { "the babies", { "We love our new house!", "Thank you for saving us.", "Hugs and beeps!" } },
    { "Clanky Crane", { "Sorry I swung at you.", "My magnet was all glitchy.", "Friends now?" } },
    { "the garage", { "The cars are fuelled up.", "Race a friend and", "jump the cones!" } },
    { "the fish", { "Blub blub blub.", "(That means: please", "feed us more flakes.)" } },
    { "the paint studio", { "The wall is waiting.", "Fly up high and", "paint us a picture!" } },
    { "the parts shop", { "New hats in stock!", "Bolts buy cool things.", "See you soon!" } },
};
const float ck_aqua_chest_x[CK_AQUA_CHESTS] = { 119, 439, 1050, 1500 };
const char *const ck_aqua_sections[CK_AQUA_SECTIONS] = { "Kelp Forest", "Coral Reef", "Shipwreck", "Deep Blue", "Rocky Caves" };
const int ck_aqua_pop[CK_AQUA_SECTIONS][CK_AQUA_PER_SEC] = {
    { AFISH_ORANGE, AFISH_YELLOW, AFISH_PINK, AFISH_ORANGE, AFISH_TANG, AFISH_YELLOW },
    { AFISH_CLOWN, AFISH_TANG, AFISH_ANGEL, AFISH_CLOWN, AFISH_PUFFER, AFISH_ANGEL },
    { AFISH_EEL, AFISH_ORANGE, AFISH_PUFFER, AFISH_EEL, AFISH_PINK, AFISH_YELLOW },
    { AFISH_JELLY, AFISH_JELLY, AFISH_RAY, AFISH_JELLY, AFISH_RAY, AFISH_TANG },
    { AFISH_EEL, AFISH_PUFFER, AFISH_YELLOW, AFISH_PINK, AFISH_TANG, AFISH_ANGEL },
};
const uint32_t ck_mural_pal[CK_MURAL_PAL] = { 0xE04F4F, 0xFF8A3D, 0xFFD23F, 0x49D17A, 0x5AD1FF, 0x3A6FC0, 0xC28BFF, 0xFF8FC7, 0xFFFFFF, 0x2B2230 };

#define HOUSE_SPEED 1.7f
#define HOUSE_HOP 3.0f
#define HOUSE_GRAV 0.26f

static int dir_x(const ck_game *G, int s) { return (HELD(G, s, CK_RIGHT) ? 1 : 0) - (HELD(G, s, CK_LEFT) ? 1 : 0); }
static int dir_y(const ck_game *G, int s) { return (HELD(G, s, CK_DOWN) ? 1 : 0) - (HELD(G, s, CK_UP) ? 1 : 0); }
// a little rng the browser copies, so the babies start in the same places
static uint32_t hseed;
static float hrng(void) { hseed = (hseed * 1103515245u + 12345u) & 0x7fffffffu; return (float)hseed / 2147483648.0f; }

// Every rescued baby: per world, one for each level beaten (up to its four), in that world's colours.
static void find_babies(ck_game *G, uint32_t *out, int *n) {
    static ck_world w;
    *n = 0;
    for (int k = 1; k <= CK_WORLDS && *n < CK_HOUSE_BABIES; k++) {
        const ck_world *wd = NULL;
        const ck_prog *p = k == G->worldNum ? &G->prog : &G->saves.w[k - 1];
        if (p->magic != CK_SAVE_MAGIC && k != G->worldNum) continue;
        if (k == G->worldNum) wd = &G->world;
        else if (G->load) {
            char path[32];
            snprintf(path, sizeof path, "w%d/world.txt", k);
            char *text = G->load(path, G->loadUser);
            if (text && ck_parse_world(text, &w)) wd = &w;
            free(text);
        }
        if (!wd) continue;
        int nb = 0;
        for (int i = 0; i < wd->nnodes; i++) if (wd->nodes[i].kind == CK_NODE_LEVEL && ((p->beaten >> i) & 1)) nb++;
        if (nb > 4) nb = 4;
        for (int j = 0; j < nb && *n < CK_HOUSE_BABIES; j++) out[(*n)++] = wd->babies[j] ? wd->babies[j] : 0xFF8FC7;
    }
}

// ---- the plan: where you can stand, which room you are in, what is near, where the camera goes
static bool in_rect(float x, float y, const ck_trect *r) { return x >= r->x0 && x <= r->x1 && y >= r->y0 && y <= r->y1; }
bool ck_house_solid(float x, float y) {
    bool ok = false;
    for (int i = 0; i < ck_ntwalk && !ok; i++) ok = in_rect(x, y, &ck_twalk[i]);
    if (!ok) return true;
    for (int i = 0; i < ck_nthings; i++) {
        const ck_thing *t = &ck_things[i];
        if (t->fw && fabsf(x - t->x) < t->fw + 4 && y > t->y - t->fd - 2 && y < t->y + 5) return true;
    }
    return false;
}
int ck_house_room_at(float x, float y) {
    for (int i = 0; i < ck_ntrooms; i++) {
        const ck_troom *r = &ck_trooms[i];
        if (x > r->x && x < r->x + r->w && y > r->y && y < r->y + r->h) return i;
    }
    return -1;
}
bool ck_spot_here(const ck_game *G, int k) { return ck_spots[k].kind != SPOT_BACK || G->worldNum > 1; }
// the closest thing with a spot whose footprint is within 12 px across and 16 px in front or behind
int ck_house_thing_at(const ck_game *G, float x, float y) {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < ck_nthings; i++) {
        const ck_thing *t = &ck_things[i];
        if (t->spot < 0 || !ck_spot_here(G, t->spot)) continue;
        float dx = ck_maxf(0, fabsf(x - t->x) - t->fw), dy = y < t->y - t->fd ? t->y - t->fd - y : ck_maxf(0, y - t->y);
        if (dx > 12 || dy > 16) continue;
        if (dx + dy < bd) { bd = dx + dy; best = i; }
    }
    return best;
}
int ck_house_near(const ck_game *G, const ck_hhero *p) {
    if (p->jz > 0) return -1;
    int i = ck_house_thing_at(G, p->x, p->y);
    return i < 0 ? -1 : ck_things[i].spot;
}
// it follows the middle of the party (a little above the feet); indoors it stays inside the room
static float fit(float c, float lo, float hi) { return hi < lo ? roundf((lo + hi) / 2) : ck_clampf(c, lo, hi); }
void ck_house_camera(float mx, float my, float *cx, float *cy) {
    float W = ck_td_world[0], H = ck_td_world[1];
    *cx = ck_clampf(roundf(mx - CK_W / 2), 0, W - CK_W);
    *cy = ck_clampf(roundf(my - 44 - CK_H / 2), 0, H - CK_H);
    int k = ck_house_room_at(mx, my);
    if (k < 0) return;
    const ck_troom *r = &ck_trooms[k];
    *cx = ck_clampf(fit(*cx, r->x - 8, r->x + r->w + 8 - CK_W), 0, W - CK_W);
    if (r->h > 160) *cy = ck_clampf(fit(*cy, r->y - 8, r->y + r->h + 8 - CK_H), 0, H - CK_H);
}
static void house_view(ck_house *H) {
    float mx = 0, my = 0;
    for (int i = 0; i < H->np; i++) { mx += H->ps[i].x; my += H->ps[i].y; }
    if (H->np > 0) { mx /= H->np; my /= H->np; }
    ck_house_camera(mx, my, &H->cam, &H->camy);
}

void ck_house_enter(ck_game *G, bool fromShop) {
    ck_house *H = &G->house;
    (void)fromShop;
    H->np = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        ck_hhero *p = &H->ps[H->np];
        memset(p, 0, sizeof *p);
        p->slot = s; p->x = ck_td_start[H->np][0]; p->y = ck_td_start[H->np][1]; p->face = 1; p->ly = 1; p->ground = true;
        H->np++;
    }
    uint32_t cols[CK_HOUSE_BABIES];
    find_babies(G, cols, &H->nbabies);
    hseed = 7;
    for (int i = 0; i < H->nbabies; i++) {   // each in an area of its own (the garden, then a room each)
        ck_hbaby *b = &H->babies[i];
        const ck_trect *r = &ck_tareas[i % ck_ntareas];
        b->hex = cols[i];
        b->area = i % ck_ntareas;
        b->x = r->x0 + hrng() * (r->x1 - r->x0);
        b->y = r->y0 + hrng() * (r->y1 - r->y0);
        b->tx = b->x; b->ty = b->y; b->z = b->vz = 0;
        b->wait = (int)(hrng() * 90);
    }
    H->mode = HM_WALK;
    H->msgT = 0;
    H->nthrown = 0;
    H->fpick[0] = 0; H->fpick[1] = 2;
    H->cpick[0] = 0; H->cpick[1] = 2;
    house_view(H);
    ck_go(G, SC_HOUSE);
}

static void say(ck_house *H, const char *s) { snprintf(H->msg, sizeof H->msg, "%s", s); H->msgT = 100; }

// the heroes as they come into one of the house's games
void ck_house_game_heroes(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) { memset(&H->gs[i], 0, sizeof H->gs[i]); H->gs[i].slot = H->ps[i].slot; H->gs[i].face = 1; }
}
static void paint_start(ck_game *G);
static void aqua_start(ck_game *G);
static void cars_start(ck_game *G);
static void piano_start(ck_game *G, bool drums);

static void use_spot(ck_game *G, int k, ck_hhero *p) {
    ck_house *H = &G->house;
    switch (ck_spots[k].kind) {
    case SPOT_BACK: ck_sfx(G, SFX_CONFIRM); ck_save(G); ck_game_enter_world(G, G->worldNum - 1, CK_NODE_EXIT); break;
    case SPOT_SNACK:   // the fridge opens: pick something to hold
        ck_sfx(G, SFX_OPEN);
        H->mode = HM_FRIDGE;
        break;
    case SPOT_STOVE:   // the stove: cook something
        ck_sfx(G, SFX_OPEN);
        H->mode = HM_COOK;
        H->npan = 0; H->panNext = 0; H->heat = 0; H->dish = 0;
        break;
    case SPOT_MAIL:
        ck_sfx(G, SFX_OPEN);
        H->letter = H->letter % CK_LETTERS + 1;
        H->mode = HM_LETTER;
        break;
    case SPOT_TREE:
        p->hold = FOOD_APPLE + 1; p->garnish = (ck_garnish){ 0 };
        ck_sfx(G, SFX_SPROUT);
        ck_sparks(G, p->x - H->cam, p->y - H->camy - 50, 6, 0x49D17A);
        say(H, "You picked an apple!");
        break;
    case SPOT_PAINT: ck_sfx(G, SFX_OPEN); paint_start(G); break;
    case SPOT_PIANO: ck_sfx(G, SFX_OPEN); piano_start(G, false); break;
    case SPOT_DRUMS: ck_sfx(G, SFX_OPEN); piano_start(G, true); break;
    case SPOT_AQUA: ck_sfx(G, SFX_OPEN); aqua_start(G); break;
    case SPOT_CARS: ck_sfx(G, SFX_OPEN); cars_start(G); break;
    case SPOT_PINBALL: ck_sfx(G, SFX_OPEN); ck_pin_start(G); break;
    case SPOT_BLAST: ck_sfx(G, SFX_OPEN); ck_blast_start(G); break;
    case SPOT_POND: ck_sfx(G, SFX_OPEN); ck_fish_start(G); break;
    case SPOT_BOLT: ck_sfx(G, SFX_OPEN); ck_bolt_start(G); break;
    case SPOT_JETRUN: ck_sfx(G, SFX_OPEN); ck_run_start(G); break;
    case SPOT_SAND: ck_sfx(G, SFX_OPEN); ck_sand_start(G); break;
    case SPOT_BED: ck_sfx(G, SFX_SELECT); H->mode = HM_SLOTS; H->pick = G->slot; break;
    }
}

void ck_house_back(ck_game *G) { ck_sfx(G, SFX_SELECT); G->scene = SC_HOUSE; G->t = 0; ck_play(G, SONG_SELECT); }
#define back_to_house ck_house_back

// The stove: the dish is named by what went in the pan (the first rule that fits wins).
// is food f in a dish's core list (FOOD_* + 1 each, 0 ends it)?
static bool in_core(const uint8_t *c, int n, int f) { for (int j = 0; j < n && c[j]; j++) if (c[j] == f + 1) return true; return false; }
void ck_cook_dish(const int *pan, int n, int *dish, ck_garnish *extra, char *name, int size) {
    bool has[FOOD_DISH0] = { 0 };
    for (int i = 0; i < n; i++) has[pan[i]] = true;
    int k;
#define H(f) has[FOOD_##f]
    if (H(HOTDOG) || (H(SAUSAGE) && H(BREAD))) k = H(CHILI) ? DISH_CHILI_DOG : H(CHEESE) ? DISH_CHEESY_HOTDOG : H(TOMATO) ? DISH_KETCHUP_HOTDOG : DISH_HOTDOG;
    else if (H(BACON) && H(LETTUCE) && H(TOMATO)) k = DISH_BLT;
    else if (H(STEAK)) k = H(EGG) ? DISH_STEAK_EGGS : H(POTATO) ? DISH_STEAK_POTATOES : DISH_STEAK_DINNER;
    else if (H(PORKCHOP)) k = H(APPLE) ? DISH_PORK_APPLE : DISH_PORK_DINNER;
    else if (H(LAMB)) k = H(POTATO) ? DISH_SHEPHERDS_PIE : H(CARROT) || H(CELERY) || H(ONION) ? DISH_LAMB_STEW : DISH_ROAST_LAMB;
    else if (H(TURKEY)) k = H(GRAVY) || H(POTATO) ? DISH_TURKEY_DINNER : DISH_TURKEY_BURGER;
    else if (H(PASTA)) k = H(TOMATO) && H(SAUSAGE) ? DISH_MEATBALLS : H(TOMATO) ? DISH_SPAGHETTI : H(CHEESE) ? DISH_MAC_CHEESE : H(MUSHROOM) ? DISH_MUSHROOM_PASTA : DISH_BUTTER_PASTA;
    else if (H(RICE)) k = H(TORTILLA) ? DISH_BURRITO : H(SALMON) || H(FISH) || H(WHITEFISH) ? DISH_SUSHI : H(EGG) ? DISH_FRIED_RICE : H(SAUSAGE) ? DISH_SAUSAGE_RICE : H(CARROT) || H(MUSHROOM) ? DISH_VEGGIE_RICE : H(BANANA) || H(APPLE) ? DISH_RICE_PUDDING : DISH_RICE_BOWL;
    else if (H(BANANA) && H(BREAD)) k = DISH_BANANA_BREAD;
    else if (H(APPLE)) k = H(BANANA) ? DISH_FRUIT_PIE : DISH_APPLE_PIE;
    else if (H(BANANA)) k = DISH_PANCAKES;
    else if (H(CARROT) && H(EGG)) k = DISH_CARROT_CAKE;
    else if (H(EGG)) k = H(CHEESE) ? DISH_CHEESY_OMELETTE : H(BACON) ? DISH_BACON_EGGS : H(FLOUR) ? DISH_PLAIN_PANCAKES : H(MUSHROOM) ? DISH_MUSHROOM_OMELETTE : H(TOMATO) ? DISH_TOMATO_OMELETTE : H(SAUSAGE) ? DISH_BIG_BREAKFAST : H(POTATO) ? DISH_POTATO_OMELETTE : DISH_OMELETTE;
    else if (H(CELERY) && H(PEANUTBUTTER)) k = DISH_ANTS_LOG;
    else if (H(PEANUTBUTTER) && H(BREAD)) k = H(JAM) ? DISH_PBJ : DISH_PB_TOAST;
    else if (H(BREAD)) k = H(CHEESE) && H(TOMATO) ? DISH_PIZZA_TOASTIE : H(CHEESE) ? DISH_CHEESE_TOASTIE : H(MUSHROOM) ? DISH_MUSHROOM_TOAST : DISH_TOAST;
    else if (H(POTATO) && H(GRAVY)) k = DISH_MASH_GRAVY;
    else if (H(POTATO) && H(SOURCREAM)) k = DISH_BAKED_POTATO;
    else if (H(POTATO) && (H(FISH) || H(WHITEFISH))) k = DISH_FISH_CHIPS;
    else if (H(POTATO) && H(BUTTER)) k = DISH_MASHED;
    else if (H(POTATO) && !H(TOMATO) && !H(CARROT) && !H(MUSHROOM)) k = H(CHEESE) ? DISH_CHEESY_CHIPS : H(SAUSAGE) ? DISH_SAUSAGE_CHIPS : DISH_CHIPS;
    else if (H(GRAVY)) k = DISH_BISCUITS_GRAVY;
    else if (H(CHICKEN)) k = H(FLOUR) ? DISH_NUGGETS : H(NOODLES) || H(CELERY) ? DISH_CHICKEN_SOUP : H(ONION) || H(PEPPER) || H(TOMATO) || H(PEAS) ? DISH_CURRY : DISH_ROAST;
    else if (H(FISH) || H(SALMON) || H(WHITEFISH) || H(SHRIMP)) k = H(SALMON) ? DISH_GRILLED_SALMON : H(FISH) || H(WHITEFISH) ? DISH_FISH_SUPPER : DISH_SKEWERS;
    else if (H(CHILI)) k = DISH_CHILI_BOWL;
    else if (H(TORTILLA)) k = H(CHEESE) || H(NACHO) ? DISH_QUESADILLA : DISH_TACOS;
    else if (H(NACHO) || H(TCHIPS) || H(SOURCREAM)) k = H(NACHO) || H(CHEESE) ? DISH_NACHOS : DISH_CHIPS_DIP;
    else if (H(FLOUR) && H(TOMATO)) k = DISH_HOMEMADE_PIZZA;
    else if ((H(RASPBERRY) || H(BLUEBERRY) || H(STRAWBERRY)) && (H(OATS) || H(FLOUR))) k = DISH_BERRY_CRUMBLE;
    else if (H(OATS)) k = DISH_PORRIDGE;
    else if (H(MARSHMALLOW)) k = DISH_SMORES;
    else if (H(CHOCCHIPS)) k = DISH_CHOCCHIP_COOKIES;
    else if (H(FLOUR) && H(SUGAR)) k = DISH_CUPCAKES;
    else if (H(CHOCOLATE) || H(FLOUR)) k = DISH_CHOC_CAKE;
    else if (H(LEMON) && H(SUGAR)) k = DISH_LEMONADE;
    else if (H(CORN) && (H(BUTTER) || H(SUGAR))) k = DISH_POPCORN;
    else if (H(CREAM) || H(SPRINKLES)) k = DISH_SUNDAE;
    else if (H(YOGURT)) k = DISH_SMOOTHIE;
    else if (H(PUMPKIN)) k = DISH_PUMPKIN_PIE;
    else if (H(STRAWBERRY) || H(RASPBERRY) || H(ORANGE) || H(LEMON) || H(GRAPES) || H(CHERRY) || H(PEAR) || H(PINEAPPLE) || H(BLUEBERRY) || H(WATERMELON)) k = DISH_FRUIT_SALAD;
    else if (H(LETTUCE) || H(CUCUMBER) || H(CELERY)) k = DISH_SALAD;
    else if (H(ONION) || H(PEPPER) || H(BROCCOLI) || H(PEAS) || H(CORN) || H(BEANS) || H(BACON) || H(HAM) || H(TOFU) || H(NOODLES)) k = DISH_STIR_FRY;
    else if (H(HONEY) || H(JAM) || H(BUTTER) || H(NUTS) || H(SUGAR) || H(PEANUTBUTTER)) k = DISH_COOKIES;
    else k = H(POTATO) ? DISH_VEGGIE_STEW : H(SAUSAGE) ? DISH_SAUSAGE_STEW : H(TOMATO) ? DISH_TOMATO_SOUP : H(MUSHROOM) ? DISH_MUSHROOM_SOUP
         : H(CARROT) ? DISH_CARROT_SOUP : n == 1 ? DISH_CHEESE_PLATE : DISH_CHEESE_SOUP;
#undef H
    *dish = FOOD_DISH0 + k + 1;
    // which ingredients the dish's own name already covers
#define I(f) ((f) + 1)
    static const uint8_t core[CK_DISHES][5] = {   // FOOD_* + 1, 0 ends the list
        [DISH_OMELETTE] = { I(FOOD_EGG) },
        [DISH_CHEESY_OMELETTE] = { I(FOOD_EGG), I(FOOD_CHEESE) },
        [DISH_MUSHROOM_OMELETTE] = { I(FOOD_EGG), I(FOOD_MUSHROOM) },
        [DISH_TOMATO_OMELETTE] = { I(FOOD_EGG), I(FOOD_TOMATO) },
        [DISH_POTATO_OMELETTE] = { I(FOOD_EGG), I(FOOD_POTATO) },
        [DISH_BIG_BREAKFAST] = { I(FOOD_EGG), I(FOOD_SAUSAGE) },
        [DISH_TOAST] = { I(FOOD_BREAD) },
        [DISH_CHEESE_TOASTIE] = { I(FOOD_BREAD), I(FOOD_CHEESE) },
        [DISH_PIZZA_TOASTIE] = { I(FOOD_BREAD), I(FOOD_CHEESE), I(FOOD_TOMATO) },
        [DISH_MUSHROOM_TOAST] = { I(FOOD_BREAD), I(FOOD_MUSHROOM) },
        [DISH_TOMATO_SOUP] = { I(FOOD_TOMATO) },
        [DISH_MUSHROOM_SOUP] = { I(FOOD_MUSHROOM) },
        [DISH_CARROT_SOUP] = { I(FOOD_CARROT) },
        [DISH_CHEESE_SOUP] = { I(FOOD_CHEESE) },
        [DISH_VEGGIE_STEW] = { I(FOOD_POTATO), I(FOOD_CARROT), I(FOOD_TOMATO), I(FOOD_MUSHROOM) },
        [DISH_SAUSAGE_STEW] = { I(FOOD_SAUSAGE) },
        [DISH_HOTDOG] = { I(FOOD_SAUSAGE), I(FOOD_BREAD), I(FOOD_HOTDOG) },
        [DISH_CHEESY_HOTDOG] = { I(FOOD_SAUSAGE), I(FOOD_BREAD), I(FOOD_CHEESE), I(FOOD_HOTDOG) },
        [DISH_KETCHUP_HOTDOG] = { I(FOOD_SAUSAGE), I(FOOD_BREAD), I(FOOD_TOMATO), I(FOOD_HOTDOG) },
        [DISH_SPAGHETTI] = { I(FOOD_PASTA), I(FOOD_TOMATO) },
        [DISH_MEATBALLS] = { I(FOOD_PASTA), I(FOOD_TOMATO), I(FOOD_SAUSAGE) },
        [DISH_MAC_CHEESE] = { I(FOOD_PASTA), I(FOOD_CHEESE) },
        [DISH_MUSHROOM_PASTA] = { I(FOOD_PASTA), I(FOOD_MUSHROOM) },
        [DISH_BUTTER_PASTA] = { I(FOOD_PASTA) },
        [DISH_FRIED_RICE] = { I(FOOD_RICE), I(FOOD_EGG) },
        [DISH_SAUSAGE_RICE] = { I(FOOD_RICE), I(FOOD_SAUSAGE) },
        [DISH_VEGGIE_RICE] = { I(FOOD_RICE), I(FOOD_CARROT), I(FOOD_MUSHROOM) },
        [DISH_RICE_PUDDING] = { I(FOOD_RICE), I(FOOD_BANANA), I(FOOD_APPLE) },
        [DISH_RICE_BOWL] = { I(FOOD_RICE) },
        [DISH_APPLE_PIE] = { I(FOOD_APPLE) },
        [DISH_FRUIT_PIE] = { I(FOOD_APPLE), I(FOOD_BANANA) },
        [DISH_PANCAKES] = { I(FOOD_BANANA), I(FOOD_EGG) },
        [DISH_CHIPS] = { I(FOOD_POTATO) },
        [DISH_CHEESY_CHIPS] = { I(FOOD_POTATO), I(FOOD_CHEESE) },
        [DISH_SAUSAGE_CHIPS] = { I(FOOD_POTATO), I(FOOD_SAUSAGE) },
        [DISH_BANANA_BREAD] = { I(FOOD_BANANA), I(FOOD_BREAD) },
        [DISH_CARROT_CAKE] = { I(FOOD_CARROT), I(FOOD_EGG) },
        [DISH_CHEESE_PLATE] = { I(FOOD_CHEESE) },
        [DISH_CURRY] = { I(FOOD_CHICKEN) },
        [DISH_ROAST] = { I(FOOD_CHICKEN) },
        [DISH_FISH_SUPPER] = { I(FOOD_FISH), I(FOOD_WHITEFISH) },
        [DISH_SKEWERS] = { I(FOOD_SHRIMP) },
        [DISH_TACOS] = { I(FOOD_TORTILLA), I(FOOD_SOURCREAM) },
        [DISH_PORRIDGE] = { I(FOOD_OATS) },
        [DISH_CHOC_CAKE] = { I(FOOD_CHOCOLATE), I(FOOD_FLOUR) },
        [DISH_SUNDAE] = { I(FOOD_CREAM) },
        [DISH_SMOOTHIE] = { I(FOOD_YOGURT) },
        [DISH_PUMPKIN_PIE] = { I(FOOD_PUMPKIN) },
        [DISH_FRUIT_SALAD] = { 0 },
        [DISH_SALAD] = { I(FOOD_LETTUCE) },
        [DISH_STIR_FRY] = { 0 },
        [DISH_COOKIES] = { 0 },
        [DISH_CHILI_DOG] = { I(FOOD_HOTDOG), I(FOOD_SAUSAGE), I(FOOD_BREAD), I(FOOD_CHILI) },
        [DISH_BLT] = { I(FOOD_BACON), I(FOOD_LETTUCE), I(FOOD_TOMATO), I(FOOD_BREAD) },
        [DISH_STEAK_EGGS] = { I(FOOD_STEAK), I(FOOD_EGG) },
        [DISH_STEAK_POTATOES] = { I(FOOD_STEAK), I(FOOD_POTATO) },
        [DISH_STEAK_DINNER] = { I(FOOD_STEAK) },
        [DISH_PORK_APPLE] = { I(FOOD_PORKCHOP), I(FOOD_APPLE) },
        [DISH_PORK_DINNER] = { I(FOOD_PORKCHOP) },
        [DISH_SHEPHERDS_PIE] = { I(FOOD_LAMB), I(FOOD_POTATO) },
        [DISH_LAMB_STEW] = { I(FOOD_LAMB), I(FOOD_CARROT), I(FOOD_CELERY), I(FOOD_ONION) },
        [DISH_ROAST_LAMB] = { I(FOOD_LAMB) },
        [DISH_TURKEY_DINNER] = { I(FOOD_TURKEY), I(FOOD_GRAVY), I(FOOD_POTATO) },
        [DISH_TURKEY_BURGER] = { I(FOOD_TURKEY), I(FOOD_BREAD) },
        [DISH_BURRITO] = { I(FOOD_RICE), I(FOOD_TORTILLA) },
        [DISH_SUSHI] = { I(FOOD_RICE), I(FOOD_SALMON), I(FOOD_FISH), I(FOOD_WHITEFISH) },
        [DISH_BACON_EGGS] = { I(FOOD_EGG), I(FOOD_BACON) },
        [DISH_PLAIN_PANCAKES] = { I(FOOD_EGG), I(FOOD_FLOUR) },
        [DISH_ANTS_LOG] = { I(FOOD_CELERY), I(FOOD_PEANUTBUTTER) },
        [DISH_PB_TOAST] = { I(FOOD_PEANUTBUTTER), I(FOOD_BREAD) },
        [DISH_PBJ] = { I(FOOD_PEANUTBUTTER), I(FOOD_BREAD), I(FOOD_JAM) },
        [DISH_MASH_GRAVY] = { I(FOOD_POTATO), I(FOOD_GRAVY) },
        [DISH_BAKED_POTATO] = { I(FOOD_POTATO), I(FOOD_SOURCREAM) },
        [DISH_FISH_CHIPS] = { I(FOOD_POTATO), I(FOOD_FISH), I(FOOD_WHITEFISH) },
        [DISH_MASHED] = { I(FOOD_POTATO), I(FOOD_BUTTER) },
        [DISH_BISCUITS_GRAVY] = { I(FOOD_GRAVY), I(FOOD_FLOUR) },
        [DISH_NUGGETS] = { I(FOOD_CHICKEN), I(FOOD_FLOUR) },
        [DISH_CHICKEN_SOUP] = { I(FOOD_CHICKEN), I(FOOD_NOODLES), I(FOOD_CELERY) },
        [DISH_GRILLED_SALMON] = { I(FOOD_SALMON) },
        [DISH_CHILI_BOWL] = { I(FOOD_CHILI) },
        [DISH_QUESADILLA] = { I(FOOD_TORTILLA), I(FOOD_CHEESE), I(FOOD_NACHO) },
        [DISH_NACHOS] = { I(FOOD_TCHIPS), I(FOOD_NACHO), I(FOOD_CHEESE) },
        [DISH_CHIPS_DIP] = { I(FOOD_TCHIPS), I(FOOD_SOURCREAM) },
        [DISH_HOMEMADE_PIZZA] = { I(FOOD_FLOUR), I(FOOD_TOMATO), I(FOOD_CHEESE) },
        [DISH_BERRY_CRUMBLE] = { I(FOOD_RASPBERRY), I(FOOD_BLUEBERRY), I(FOOD_STRAWBERRY), I(FOOD_OATS), I(FOOD_FLOUR) },
        [DISH_SMORES] = { I(FOOD_MARSHMALLOW), I(FOOD_CHOCOLATE) },
        [DISH_CHOCCHIP_COOKIES] = { I(FOOD_CHOCCHIPS), I(FOOD_FLOUR) },
        [DISH_CUPCAKES] = { I(FOOD_FLOUR), I(FOOD_SUGAR) },
        [DISH_LEMONADE] = { I(FOOD_LEMON), I(FOOD_SUGAR) },
        [DISH_POPCORN] = { I(FOOD_CORN), I(FOOD_BUTTER) },
    };
#undef I
    // the rest of the pan becomes words: the first extra a word in front, the next two "with" and "and"
    static const char *const adj[CK_COOK_INGR] = { "appley", "tropical", "berry", "raspberry", "zesty", "lemony", "grapey", "cherry", "pear", "pineapple", "blue", "melony", "saucy", "foresty", "crunchy", "spuddy", "oniony", "peppery", "green", "pea", "corny", "leafy", "cool", "celery", "pumpkin", "beany", "chicken", "steak", "porky", "lamby", "turkey", "meaty", "hot dog", "bacon", "hammy", "tofu", "fishy", "salmon", "flaky", "shrimpy", "eggy", "cheesy", "nacho", "buttery", "creamy", "whipped", "sour", "crusty", "fluffy", "noodly", "slurpy", "wrapped", "crispy", "floury", "oaty", "gravy", "chili", "peanutty", "jammy", "honey", "chocolatey", "choc-chip", "gooey", "sugary", "nutty", "sparkly" };
    static const char *const noun[CK_COOK_INGR] = { "apple", "banana", "strawberries", "raspberries", "orange", "lemon", "grapes", "cherries", "pear", "pineapple", "blueberries", "melon", "tomato", "mushrooms", "carrots", "mash", "onions", "peppers", "broccoli", "peas", "corn", "lettuce", "cucumber", "celery", "pumpkin", "beans", "chicken", "steak", "pork chops", "lamb", "turkey", "sausage", "hot dogs", "bacon", "ham", "tofu", "fish", "salmon", "white fish", "shrimp", "egg", "cheese", "nacho cheese", "butter", "yogurt", "cream", "sour cream", "toast", "rice", "pasta", "noodles", "tortilla", "tortilla chips", "flour", "oats", "gravy", "chili", "peanut butter", "jam", "honey", "chocolate", "choc chips", "marshmallows", "sugar", "nuts", "sprinkles" };
    // a doubled main ingredient makes it a "double" dish; a doubled extra makes that word "extra"
    int ex[CK_COOK_INGR], nex = 0, most = 0, mostI = -1;
    const int ncore = (int)(sizeof core[0] / sizeof core[0][0]);
#define IN_CORE(f) in_core(core[k], ncore, f)
    extra->n = 0;
    for (int i = 0; i < CK_COOK_INGR; i++) {
        int f = ck_cook_ingr[i], cnt = 0;
        for (int j = 0; j < n; j++) cnt += pan[j] == f;
        if (cnt > most) { most = cnt; mostI = i; }
        if (cnt && !IN_CORE(f)) { ex[nex++] = i; if (extra->n < CK_COOK_PAN) extra->i[extra->n++] = (uint8_t)i; }
    }
    bool coreMost = most >= 2 && IN_CORE(ck_cook_ingr[mostI]);
#undef IN_CORE
    for (int e = 1; e < nex; e++)   // the doubled extra goes first
        if (most >= 2 && ex[e] == mostI) { ex[e] = ex[0]; ex[0] = mostI; }
    const char *lead = !nex || most < 2 ? "" : coreMost ? "" : ex[0] != mostI ? "" : most >= 3 ? "super " : "extra ";
    const char *base = ck_foods[FOOD_DISH0 + k].name;
    char buf[64];
    int len = snprintf(buf, sizeof buf, "%s%s%s%s", coreMost ? (most >= 4 ? "mega " : most == 3 ? "triple " : "double ") : "", lead, nex ? adj[ex[0]] : "", nex ? " " : "");
    bool lower = len > 0;   // the dish's own name goes lower case after a word in front
    len += snprintf(buf + len, sizeof buf - len, "%c%s", lower ? tolower((unsigned char)base[0]) : base[0], base + 1);
    for (int e = 1; e < nex; e++) {   // more extras, while the name still fits a banner
        int add = (int)strlen(noun[ex[e]]) + (e == 1 ? 6 : 5);
        if (len + add >= size || len + add > 38) break;
        len += snprintf(buf + len, sizeof buf - len, "%s%s", e == 1 ? " with " : " and ", noun[ex[e]]);
    }
    buf[0] = (char)toupper((unsigned char)buf[0]);
    snprintf(name, size, "%.*s", size - 1, buf);
}
static void cook_name(ck_house *H) { ck_cook_dish(H->pan, H->npan, &H->dish, &H->dishExtra, H->dishName, sizeof H->dishName); }
static void cook_update(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->ps[i];
        int s = p->slot, *c = &H->cpick[i];
        // each player has their own row of the shelf: up and down change its group (starting at the
        // group's first), left and right go round the group
        int dx = (HIT(G, s, CK_RIGHT) ? 1 : 0) - (HIT(G, s, CK_LEFT) ? 1 : 0), dy = (HIT(G, s, CK_DOWN) ? 1 : 0) - (HIT(G, s, CK_UP) ? 1 : 0);
        if ((dx || dy) && !H->dish) {
            int g = ck_cook_group_of(*c);
            if (dy) { g = ((g + dy) % CK_COOK_GROUPS + CK_COOK_GROUPS) % CK_COOK_GROUPS; *c = ck_cook_group[g]; }
            else { int a = ck_cook_group[g], len = ck_cook_group[g + 1] - a; *c = a + ((*c - a + dx) % len + len) % len; }
            ck_sfx(G, SFX_SELECT);
        }
        if (H->dish) {   // done: A (or START) takes the dish, and everyone playing gets one
            if (HIT(G, s, CK_A) || HIT(G, s, CK_START)) {
                for (int k = 0; k < H->np; k++) { H->ps[k].hold = H->dish; H->ps[k].garnish = H->dishExtra; }
                char m[56];
                snprintf(m, sizeof m, H->np > 1 ? "%s for two!" : "%s!", H->dishName);
                say(H, m);
                ck_sfx(G, SFX_BUY);
                H->mode = HM_WALK;
                return;
            }
            continue;
        }
        if (HIT(G, s, CK_A)) {   // into the pan; a full pan swaps out its oldest, going round
            if (H->npan < CK_COOK_PAN) { H->pan[H->npan++] = ck_cook_ingr[*c]; H->panNext = 0; }
            else { H->pan[H->panNext] = ck_cook_ingr[*c]; H->panNext = (H->panNext + 1) % CK_COOK_PAN; }
            ck_sfx(G, SFX_HOP);
        }
        if (HIT(G, s, CK_B) && H->npan > 0) {   // stir: it cooks a bit more each time
            H->heat += 9;
            ck_sfx(G, SFX_HOVER);
            for (int k = 0; k < 2; k++) ck_puff(G, 150 + ck_rand(G) * 20, 120, 1, 0xFFFFFF, 160);
            if (H->heat >= 100) { H->heat = 100; cook_name(H); ck_sfx(G, SFX_GEAR); ck_sparks(G, 160, 140, 12, 0xFFD23F); }
        }
    }
    if (H->heat > 0 && !H->dish && (int)(G->t * 60) % 20 == 0) ck_puff(G, 140 + ck_rand(G) * 40, 125, 1, 0xFFFFFF, 120);   // a gentle sizzle
    if (ANYHIT(G, CK_START) || (!H->dish && H->npan == 0 && ANYHIT(G, CK_B))) { H->mode = HM_WALK; ck_sfx(G, SFX_SELECT); }
}

void ck_house_update(ck_game *G) {
    ck_house *H = &G->house;
    if (H->msgT > 0) H->msgT--;
    ck_step_parts(G);
    if (H->mode == HM_SLOTS) {
        if (ANYHIT(G, CK_UP)) { H->pick = (H->pick + CK_SLOTS - 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
        if (ANYHIT(G, CK_DOWN)) { H->pick = (H->pick + 1) % CK_SLOTS; ck_sfx(G, SFX_SELECT); }
        if (ANYHIT(G, CK_B) || ANYHIT(G, CK_START)) { H->mode = HM_WALK; ck_sfx(G, SFX_SELECT); }
        else if (ANYHIT(G, CK_A)) { ck_save_to_slot(G, H->pick); H->mode = HM_SLEEP; H->sleepT = 0; ck_sfx(G, SFX_RECORD); }
        return;
    }
    if (H->mode == HM_LETTER) {
        if (ANYHIT(G, CK_A) || ANYHIT(G, CK_B) || ANYHIT(G, CK_START)) { H->mode = HM_WALK; ck_sfx(G, SFX_SELECT); }
        return;
    }
    if (H->mode == HM_FRIDGE) {   // a 3 x 3 shelf of food, a cursor each
        for (int i = 0; i < H->np; i++) {
            ck_hhero *p = &H->ps[i];
            int s = p->slot, *c = &H->fpick[i];
            if (HIT(G, s, CK_LEFT) && *c % 3 > 0) { (*c)--; ck_sfx(G, SFX_SELECT); }
            if (HIT(G, s, CK_RIGHT) && *c % 3 < 2) { (*c)++; ck_sfx(G, SFX_SELECT); }
            if (HIT(G, s, CK_UP) && *c >= 3) { *c -= 3; ck_sfx(G, SFX_SELECT); }
            if (HIT(G, s, CK_DOWN) && *c < 6) { *c += 3; ck_sfx(G, SFX_SELECT); }
            if (HIT(G, s, CK_A)) {
                p->hold = *c + 1; p->garnish = (ck_garnish){ 0 };
                ck_sfx(G, SFX_BUY);
                char m[32];
                snprintf(m, sizeof m, "Got some %s!", ck_foods[*c].name);
                say(H, m);
                H->mode = HM_WALK;
            }
        }
        if (H->mode == HM_FRIDGE && (ANYHIT(G, CK_B) || ANYHIT(G, CK_START))) { H->mode = HM_WALK; ck_sfx(G, SFX_SELECT); }
        return;
    }
    if (H->mode == HM_COOK) { cook_update(G); return; }
    if (H->mode == HM_SLEEP) {
        H->sleepT += 1.0f / 60;
        if (H->sleepT > 2.6f) { H->mode = HM_WALK; snprintf(H->msg, sizeof H->msg, "Saved in slot %d!", G->slot + 1); H->msgT = 120; }
        return;
    }
    if (ANYHIT(G, CK_START)) { ck_sfx(G, SFX_SELECT); ck_save(G); ck_go_map(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->ps[i];
        int s = p->slot, dx = dir_x(G, s), dy = (HELD(G, s, CK_DOWN) ? 1 : 0) - (HELD(G, s, CK_UP) ? 1 : 0);
        p->moving = false;
        if (p->eat > 0) p->eat--;
        else {
            // walk every way, sliding along walls and furniture; two heroes stay near each other
            if (dx || dy) {
                float n = sqrtf((float)(dx * dx + dy * dy)), vx = dx / n * HOUSE_SPEED, vy = dy / n * HOUSE_SPEED;
                const ck_hhero *o = H->np > 1 ? &H->ps[1 - i] : NULL;
                #define OK(X, Y) (!ck_house_solid(X, Y) && (!o || (fabsf((X) - o->x) < ck_td_leash[0] && fabsf((Y) - o->y) < ck_td_leash[1])))
                if (OK(p->x + vx, p->y)) { p->x += vx; p->moving = true; }
                if (OK(p->x, p->y + vy)) { p->y += vy; p->moving = true; }
                #undef OK
                if (dx) p->face = dx;
                p->lx = dx / n; p->ly = dy / n;
                if (p->moving) p->anim += HOUSE_SPEED;
            }
            // holding food: A eats it, B throws it the way you last walked
            int k = !p->hold ? ck_house_near(G, p) : -1;
            if (p->hold && HIT(G, s, CK_A)) {
                p->col = p->hold - 1; p->hold = 0; p->eat = 50;
                ck_sfx(G, SFX_SPROUT);
                ck_sparks(G, p->x - H->cam, p->y - H->camy - 22, 8, 0xFFD23F);
                bool full = G->prog.lives >= 3;
                G->prog.lives = 3;
                ck_save(G);
                char m[32];
                snprintf(m, sizeof m, full ? "Yum, %s!" : "Yum! Batteries full!", ck_foods[p->col].name);
                say(H, m);
            } else if (p->hold && HIT(G, s, CK_B)) {
                if (H->nthrown < CK_THROWN) H->thrown[H->nthrown++] = (ck_thrown){ p->x + p->lx * 8, p->y + p->ly * 4, 22, p->lx * 3.2f, p->ly * 3.2f, 2.6f, p->hold - 1, 150 };
                p->hold = 0;
                ck_sfx(G, SFX_BLOW);
            }
            // A at a thing uses it; A anywhere else hops
            else if (k >= 0 && HIT(G, s, CK_A)) { use_spot(G, k, p); if (G->scene != SC_HOUSE || H->mode != HM_WALK) return; }
            else if (HIT(G, s, CK_A) && p->jz == 0) { p->jvz = HOUSE_HOP; ck_sfx(G, SFX_JUMP); }
        }
        if (p->jz > 0 || p->jvz > 0) { p->jvz -= HOUSE_GRAV; p->jz += p->jvz; if (p->jz <= 0) { p->jz = 0; p->jvz = 0; } }
        p->ground = p->jz == 0;
        if (p->y > ck_td_gate[2] && fabsf(p->x - ck_td_gate[0]) < ck_td_gate[1]) { ck_sfx(G, SFX_SELECT); ck_save(G); ck_go_map(G); return; }   // out of the front gate
    }
    house_view(H);
    int nt = 0;
    for (int i = 0; i < H->nthrown; i++) {   // thrown food bounces off the floor and the walls
        ck_thrown *f = &H->thrown[i];
        f->vz -= 0.2f; f->z += f->vz;
        if (f->z <= 0) { f->z = 0; f->vz *= -0.45f; f->vx *= 0.75f; f->vy *= 0.75f; if (fabsf(f->vz) < 0.6f) f->vz = 0; }
        if (ck_house_solid(f->x + f->vx, f->y)) f->vx = -f->vx; else f->x += f->vx;
        if (ck_house_solid(f->x, f->y + f->vy)) f->vy = -f->vy; else f->y += f->vy;
        bool gone = --f->life <= 0;
        for (int k = 0; k < H->nbabies && !gone; k++) {   // a baby catches it and munches it
            ck_hbaby *b = &H->babies[k];
            if (fabsf(b->x - f->x) < 10 && fabsf(b->y - f->y) < 8 && f->z < 20) { b->vz = 2.2f; b->z = 0.1f; gone = true; ck_sfx(G, SFX_HOP); ck_sparks(G, f->x - H->cam, f->y - H->camy - f->z, 5, 0xFF8FC7); }
        }
        if (gone) { if (f->life <= 0) ck_puff(G, f->x - H->cam, f->y - H->camy - f->z, 4, 0xFFFFFF, 200); }
        else H->thrown[nt++] = *f;
    }
    H->nthrown = nt;
    for (int i = 0; i < H->nbabies; i++) {   // each baby toddles to a spot in its area, waits, picks another
        ck_hbaby *b = &H->babies[i];
        if (b->z > 0 || b->vz > 0) { b->vz -= 0.12f; b->z += b->vz; if (b->z <= 0) { b->z = 0; b->vz = 0; } }
        if (b->wait > 0) { b->wait--; continue; }
        float dx = b->tx - b->x, dy = b->ty - b->y, d = sqrtf(dx * dx + dy * dy);
        if (d < 1) {
            const ck_trect *r = &ck_tareas[b->area];
            b->wait = 40 + (int)(ck_rand(G) * 120);
            b->tx = r->x0 + ck_rand(G) * (r->x1 - r->x0); b->ty = r->y0 + ck_rand(G) * (r->y1 - r->y0);
            continue;
        }
        float nx = b->x + dx / d * 0.5f, ny = b->y + dy / d * 0.5f;
        if (ck_house_solid(nx, ny)) { b->tx = b->x; b->ty = b->y; continue; }
        b->x = nx; b->y = ny;
        if (b->z == 0 && ck_rand(G) < 0.004f) b->vz = 1.6f;
    }
}

// ---------------------------------------------------------------------------- the paint studio
// Fly with the jetpack (hold A), hold B to paint with your front hand, L and R (or C) change the
// colour. The wall stays painted until the console is switched off.
static void paint_start(ck_game *G) {
    ck_house *H = &G->house;
    ck_house_game_heroes(G);
    for (int i = 0; i < H->np; i++) { ck_hhero *p = &H->gs[i]; p->x = 120 + i * 80; p->y = CK_MURAL_FLOOR; p->col = i ? 4 : 0; }
    ck_go(G, SC_PAINT);
}
uint32_t ck_mural_hex(int v) {
    if (v < 1 || v > 3 * CK_MURAL_PAL) return 0xFFF6E6;
    uint32_t h = ck_mural_pal[(v - 1) % CK_MURAL_PAL];
    int r = h >> 16 & 255, g = h >> 8 & 255, b = h & 255, tier = (v - 1) / CK_MURAL_PAL;
    if (tier == 1) { r = r * 62 / 100; g = g * 62 / 100; b = b * 62 / 100; }
    if (tier == 2) { r = (r + 255) / 2; g = (g + 255) / 2; b = (b + 255) / 2; }
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}
#define C(x, y, r, c) { 0, x, y, r, 0, c }
#define R(x, y, w, h, c) { 1, x, y, w, h, c }
const ck_stickerart ck_stickers[CK_STICKERS] = {
    { "heart", 10, { R(-3, -3, 2, 0, ROLE_MAIN), R(1, -3, 2, 0, ROLE_MAIN), R(-4, -2, 8, 1, ROLE_MAIN), R(-3, 0, 6, 0, ROLE_MAIN), R(-2, 1, 4, 0, ROLE_MAIN),
                     R(-1, 2, 2, 0, ROLE_MAIN), R(0, 3, 0, 0, ROLE_MAIN), R(-3, -2, 1, 0, ROLE_LIGHT), R(2, 0, 1, 0, ROLE_DARK), R(1, 1, 1, 0, ROLE_DARK) } },
    { "star", 11, { R(0, -4, 0, 0, ROLE_MAIN), R(-1, -3, 2, 1, ROLE_MAIN), R(-4, -1, 8, 0, ROLE_MAIN), R(-3, 0, 6, 0, ROLE_MAIN), R(-2, 1, 4, 1, ROLE_MAIN),
                    R(-3, 3, 2, 0, ROLE_MAIN), R(1, 3, 2, 0, ROLE_MAIN), R(-3, 4, 1, 0, ROLE_MAIN), R(2, 4, 1, 0, ROLE_MAIN), R(0, -3, 0, 1, ROLE_LIGHT), R(1, 1, 1, 1, ROLE_DARK) } },
    { "flower", 7, { C(0, -3, 2, ROLE_MAIN), C(-3, 0, 2, ROLE_MAIN), C(3, 0, 2, ROLE_MAIN), C(0, 3, 2, ROLE_DARK), C(0, 0, 1.6f, ROLE_LIGHT),
                     R(0, 0, 0, 0, ROLE_WHITE), R(0, -4, 0, 0, ROLE_LIGHT) } },
    { "smile", 7, { C(0, 0, 4.6f, ROLE_MAIN), R(-2, -2, 0, 1, ROLE_INK), R(2, -2, 0, 1, ROLE_INK), R(-2, 1, 0, 0, ROLE_INK), R(2, 1, 0, 0, ROLE_INK),
                    R(-1, 2, 2, 0, ROLE_INK), R(-2, -3, 0, 0, ROLE_LIGHT) } },
    { "robot", 10, { R(-3, -2, 6, 5, ROLE_MAIN), R(0, -4, 0, 1, ROLE_DARK), R(0, -5, 0, 0, ROLE_LIGHT), R(-2, -1, 1, 1, ROLE_WHITE), R(1, -1, 1, 1, ROLE_WHITE),
                     R(-1, 0, 0, 0, ROLE_INK), R(2, 0, 0, 0, ROLE_INK), R(-2, 2, 4, 0, ROLE_DARK), R(-4, -1, 0, 2, ROLE_DARK), R(4, -1, 0, 2, ROLE_DARK) } },
    { "fish", 8, { C(-1, 0, 2.9f, ROLE_MAIN), R(2, -1, 0, 2, ROLE_DARK), R(3, -2, 0, 4, ROLE_DARK), R(4, -3, 0, 6, ROLE_DARK), R(-1, -3, 1, 0, ROLE_DARK),
                   R(-3, 1, 2, 0, ROLE_LIGHT), R(-2, -1, 0, 0, ROLE_WHITE), R(-3, -1, 0, 0, ROLE_INK) } },
    { "bolt", 10, { R(1, -4, 1, 0, ROLE_MAIN), R(0, -3, 1, 0, ROLE_MAIN), R(-1, -2, 1, 0, ROLE_MAIN), R(-2, -1, 4, 0, ROLE_MAIN), R(0, 0, 1, 0, ROLE_MAIN),
                    R(-1, 1, 1, 0, ROLE_MAIN), R(-2, 2, 1, 0, ROLE_MAIN), R(-3, 3, 1, 0, ROLE_MAIN), R(2, -4, 0, 0, ROLE_LIGHT), R(-3, 3, 0, 0, ROLE_DARK) } },
    { "ghost", 10, { C(0, -1, 3.2f, ROLE_MAIN), R(-3, -1, 6, 3, ROLE_MAIN), R(-3, 3, 0, 0, ROLE_MAIN), R(0, 3, 0, 0, ROLE_MAIN), R(3, 3, 0, 0, ROLE_MAIN),
                     R(-2, -2, 1, 1, ROLE_WHITE), R(1, -2, 1, 1, ROLE_WHITE), R(-1, -1, 0, 0, ROLE_INK), R(2, -1, 0, 0, ROLE_INK), R(-3, -2, 0, 1, ROLE_LIGHT) } },
};
#undef C
#undef R
void ck_sticker_cells(int kind, int c, int out[CK_STICKER_N][CK_STICKER_N]) {
    const ck_stickerart *S = &ck_stickers[kind];
    int role[CK_STICKER_N][CK_STICKER_N], h = CK_STICKER_N / 2;
    for (int y = 0; y < CK_STICKER_N; y++)
        for (int x = 0; x < CK_STICKER_N; x++) {
            float px = x - h, py = y - h;
            role[y][x] = -1;
            for (int k = 0; k < S->n; k++) {
                const ck_fshape *f = &S->s[k];
                bool in = f->kind == 0 ? (px - f->x) * (px - f->x) + (py - f->y) * (py - f->y) <= f->w * f->w
                                       : px >= f->x && px <= f->x + f->w && py >= f->y && py <= f->y + f->h;
                if (in) role[y][x] = (int)f->col;
            }
        }
    for (int y = 0; y < CK_STICKER_N; y++)
        for (int x = 0; x < CK_STICKER_N; x++) {
            int r = role[y][x];
            if (r < 0) {   // the outline: bare cells next to the sticker
                bool edge = (x > 0 && role[y][x - 1] >= 0) || (x < CK_STICKER_N - 1 && role[y][x + 1] >= 0) || (y > 0 && role[y - 1][x] >= 0) || (y < CK_STICKER_N - 1 && role[y + 1][x] >= 0);
                r = edge ? ROLE_INK : -1;
            }
            int v = -1;
            if (r >= 0 && c == CK_MURAL_ERASER) v = 0;
            else if (r == ROLE_MAIN) v = c + 1;
            else if (r == ROLE_DARK) v = c + 1 + CK_MURAL_PAL;
            else if (r == ROLE_LIGHT) v = c + 1 + 2 * CK_MURAL_PAL;
            else if (r == ROLE_WHITE) v = 9;
            else if (r == ROLE_INK) v = 10;
            out[y][x] = v;
        }
}
static void mural_sticker(ck_house *H, float px, float py, int kind, int c) {
    int g[CK_STICKER_N][CK_STICKER_N], h = CK_STICKER_N / 2;
    int cx = (int)floorf(px / CK_MURAL_CELL), cy = (int)floorf((py - CK_MURAL_Y) / CK_MURAL_CELL);
    ck_sticker_cells(kind, c, g);
    for (int y = 0; y < CK_STICKER_N; y++)
        for (int x = 0; x < CK_STICKER_N; x++) {
            int mx = cx + x - h, my = cy + y - h;
            if (g[y][x] < 0 || mx < 0 || my < 0 || mx >= CK_MURAL_W || my >= CK_MURAL_H) continue;
            H->mural[my * CK_MURAL_W + mx] = (uint8_t)g[y][x];
        }
}
void ck_mural_dab(ck_house *H, float px, float py, int c, int size) {
    int cx = (int)floorf(px / CK_MURAL_CELL), cy = (int)floorf((py - CK_MURAL_Y) / CK_MURAL_CELL);
    for (int dy = -(size / 2); dy <= size - size / 2; dy++)
        for (int dx = -(size / 2); dx <= size - size / 2; dx++) {
            int x = cx + dx, y = cy + dy;
            if (x < 0 || y < 0 || x >= CK_MURAL_W || y >= CK_MURAL_H) continue;
            H->mural[y * CK_MURAL_W + x] = (uint8_t)(c == CK_MURAL_ERASER ? 0 : c + 1);
        }
}
static void paint_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, want = dir_x(G, s);
        p->vx += (want * 1.8f - p->vx) * 0.25f;
        // turning round flops the hero to the other side of the brush, so the brush barely moves
        if (want && want != p->face) { p->x += p->face * 16; p->face = want; p->bx += p->face * 2; }
        // no gravity: up flies up, down flies down, and letting go hovers in place
        int wy = HELD(G, s, CK_UP) ? -1 : HELD(G, s, CK_DOWN) ? 1 : 0;
        p->jet = wy < 0;
        p->vy += (wy * 2.2f - p->vy) * 0.25f;
        if (p->jet && (int)(G->t * 60) % 6 == 0) ck_sfx(G, SFX_HOVER);
        p->x = ck_clampf(p->x + p->vx, 10, CK_W - 10);
        p->y += p->vy;
        if (p->y > CK_MURAL_FLOOR) { p->y = CK_MURAL_FLOOR; p->vy = 0; }
        if (p->y < CK_MURAL_Y + 28) { p->y = CK_MURAL_Y + 28; p->vy = 0; }
        if (HIT(G, s, CK_B)) { p->stick = !p->stick; p->brush = false; ck_sfx(G, SFX_GRAB); }   // B: swap the brush for a sticker and back
        if (HIT(G, s, CK_C)) {   // C: a bigger brush, then back to the smallest; or the next sticker
            if (p->stick) p->kind = (p->kind + 1) % CK_STICKERS;
            else p->size = (p->size + 1) % 3;
            ck_sfx(G, SFX_SELECT);
        }
        if (HIT(G, s, CK_R)) { p->col = (p->col + 1) % CK_MURAL_CHOICES; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_L)) { p->col = (p->col + CK_MURAL_CHOICES - 1) % CK_MURAL_CHOICES; ck_sfx(G, SFX_SELECT); }
        float bx = p->x + p->face * 9, by = p->y - 16;
        if (p->col == CK_MURAL_CLEAR) {   // the wipe-all choice: A clears the wall
            if (HIT(G, s, CK_A)) {
                memset(H->mural, 0, sizeof H->mural);
                ck_sfx(G, SFX_WHOOSH);
                for (int k = 0; k < 12; k++) ck_puff(G, 20 + k * 25, CK_MURAL_Y + 40 + (k % 3) * 50, 2, 0xFFFFFF, 220);
            }
            p->brush = false;
        } else if (p->stick) {   // A stamps the sticker
            if (HIT(G, s, CK_A)) { mural_sticker(H, bx + p->face * 14, by, p->kind, p->col); ck_sfx(G, SFX_POP); ck_sparks(G, bx + p->face * 14, by, 5, ck_mural_hex(p->col + 1)); }
        } else if (HELD(G, s, CK_A)) {
            if (!p->brush) { p->bx = bx; p->by = by; p->brush = true; }
            int n = (int)ceilf(hypotf(bx - p->bx, by - p->by) / 2);
            if (n < 1) n = 1;
            for (int k = 1; k <= n; k++) ck_mural_dab(H, p->bx + (bx - p->bx) * k / n, p->by + (by - p->by) * k / n, p->col, p->size);
            p->bx = bx; p->by = by;
        } else p->brush = false;
    }
}

// ---------------------------------------------------------------------------- the aquarium
static float aqua_floor(float x) {
    if (x >= CK_AQUA_BEACH_X) return CK_AQUA_BEACH_Y;
    if (x > CK_AQUA_BEACH_X - 30) return CK_AQUA_SEABED - (x - (CK_AQUA_BEACH_X - 30)) / 30 * (CK_AQUA_SEABED - CK_AQUA_BEACH_Y);
    return CK_AQUA_SEABED;
}
static void aqua_start(ck_game *G) {
    ck_house *H = &G->house;
    ck_house_game_heroes(G);
    for (int i = 0; i < H->np; i++) { H->gs[i].x = 60 + i * 40; H->gs[i].y = 120; }
    hseed = 11;
    for (int i = 0; i < CK_AQUA_FISH; i++) {
        ck_fish *f = &H->fish[i];
        f->home = i / CK_AQUA_PER_SEC; f->kind = ck_aqua_pop[f->home][i % CK_AQUA_PER_SEC];
        f->x = f->home * CK_AQUA_SEC_W + 20 + hrng() * (CK_AQUA_SEC_W - 40); f->y = 70 + hrng() * 130;
        float sp = f->kind == AFISH_JELLY ? 0.15f : f->kind == AFISH_RAY ? 0.5f : 0.4f + hrng() * 0.5f;
        f->vx = (hrng() < 0.5f ? -1 : 1) * sp; f->scare = 0; f->happy = 0; f->follow = 0; f->who = 0; f->ph = hrng() * 6;
    }
    for (int i = 0; i < CK_AQUA_SHRIMP; i++) {
        ck_shrimp *s = &H->shrimp[i];
        s->x = 20 + hrng() * (CK_AQUA_BEACH_X - 70); s->y = CK_AQUA_SEABED - 2; s->vx = s->vy = 0; s->face = hrng() < 0.5f ? -1 : 1; s->rest = (int)(hrng() * 200); s->held = 0; s->spin = 0;
    }
    for (int i = 0; i < CK_AQUA_CRABS; i++) { H->crabs[i] = (ck_crab){ .x = CK_AQUA_BEACH_X + 14 + i * 22, .vx = (i % 2 ? -1 : 1) * 0.5f }; }
    H->nfood = 0;
    H->gcam = 0;
    memset(H->chestT, 0, sizeof H->chestT);
    H->chestGot = 0;
    ck_go(G, SC_AQUA);
}
static void bubble(ck_game *G, float x, float y, float vx, float vy, float r, int life) {
    if (G->nparts >= CK_MAXPARTS) return;
    ck_part *q = &G->parts[G->nparts++];
    memset(q, 0, sizeof *q);
    q->x = x; q->y = y; q->vx = vx; q->vy = vy; q->r = r; q->life = life; q->max = life; q->col = 0xDCFAFF; q->alpha = 220; q->kind = PART_PUFF;
}
static void aqua_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, dx = dir_x(G, s), dy = dir_y(G, s);
        if (dx) p->face = dx;
        p->land = p->x >= CK_AQUA_BEACH_X && p->y <= CK_AQUA_BEACH_Y + 2;
        if (p->land) {
            p->vx += (dx * 1.6f - p->vx) * 0.3f;
            if (HIT(G, s, CK_A) && p->ground) { p->vy = -4; p->ground = false; ck_sfx(G, SFX_JUMP); }
            p->vy += 0.26f;
        } else {
            p->vx = ck_clampf((p->vx + dx * 0.14f) * 0.95f, -2, 2);
            p->vy = ck_clampf((p->vy + dy * 0.14f) * 0.95f + 0.01f, -2.4f, 2);
            if (HIT(G, s, CK_A)) { p->vy -= 1.6f; ck_sfx(G, SFX_FLAP); for (int k = 0; k < 3; k++) bubble(G, p->x + (ck_rand(G) - 0.5f) * 6, p->y - 26, (ck_rand(G) - 0.5f), -0.8f, 2, 30); }
        }
        // B: throw the shrimp in your hand; by a chest, open it; by a crab on the beach, poke it; by a
        // shrimp, pick it up; anywhere else in the water, sprinkle fish food
        int chest = -1, crab = -1, shrimp = -1, mine = -1;
        for (int k = 0; k < CK_AQUA_SHRIMP; k++) {
            const ck_shrimp *sh = &H->shrimp[k];
            if (sh->held == i + 1) mine = k;
            else if (!sh->held && !p->land && fabsf(sh->x - p->x) < 18 && fabsf(sh->y - p->y) < 24) shrimp = k;
        }
        for (int k = 0; k < CK_AQUA_CHESTS; k++) if (fabsf(p->x - ck_aqua_chest_x[k]) < 26 && p->y > CK_AQUA_SEABED - 40) chest = k;
        for (int k = 0; k < CK_AQUA_CRABS; k++) if (p->land && fabsf(p->x - H->crabs[k].x) < 24) crab = k;
        if (HIT(G, s, CK_B) && mine >= 0) {   // throw it the way you face; it tumbles through the water
            ck_shrimp *sh = &H->shrimp[mine];
            sh->held = 0; sh->vx = p->face * 3.4f + p->vx; sh->vy = -1.2f + p->vy * 0.5f; sh->spin = 60; sh->face = p->face;
            ck_sfx(G, SFX_WHOOSH);
        } else if (HIT(G, s, CK_B) && chest >= 0) {
            if (H->chestT[chest] <= 0) {
                H->chestT[chest] = 240;
                ck_sfx(G, SFX_OPEN);
                for (int k = 0; k < 8; k++) bubble(G, ck_aqua_chest_x[chest] + (ck_rand(G) - 0.5f) * 14, CK_AQUA_SEABED - 14, (ck_rand(G) - 0.5f) * 0.6f, -0.6f - ck_rand(G), 2 + ck_rand(G) * 2, 90);
                if (!(H->chestGot & (1 << chest))) {   // the first time each visit: treasure!
                    H->chestGot |= 1 << chest;
                    G->prog.bolts += 5;
                    ck_save(G);
                    ck_sfx(G, SFX_GEAR);
                    ck_sparks(G, ck_aqua_chest_x[chest], CK_AQUA_SEABED - 24, 10, 0xFFD23F);
                }
            }
        } else if (HIT(G, s, CK_B) && crab >= 0) {
            ck_crab *c = &H->crabs[crab];
            if (c->y == 0) { c->vy = -3.2f; c->snap = 40; c->run = 90; c->wait = 0; c->vx = (c->x > p->x ? 1 : -1) * 1.8f; ck_sfx(G, SFX_BOING); }
        } else if (HIT(G, s, CK_B) && shrimp >= 0) {
            H->shrimp[shrimp].held = i + 1;
            ck_sfx(G, SFX_GRAB);
        } else if (HIT(G, s, CK_B) && !p->land) {
            ck_sfx(G, SFX_POP);
            for (int k = 0; k < 3 && H->nfood < CK_AQUA_FOOD; k++)
                H->food[H->nfood++] = (ck_flake){ p->x + p->face * (8 + k * 4), p->y - 20 + k * 2, 600 };
        }
        p->x = ck_clampf(p->x + p->vx, 10, CK_AQUA_W - 10);
        p->y += p->vy;
        float fl = aqua_floor(p->x);
        p->ground = false;
        if (p->y >= fl) { p->y = fl; p->vy = 0; p->ground = true; }
        if (!p->land && p->y < CK_AQUA_TOP + 22) { p->y = CK_AQUA_TOP + 22; p->vy = ck_maxf(0, p->vy); }
        if (!p->land && p->x > CK_AQUA_BEACH_X - 2 && p->y > CK_AQUA_BEACH_Y + 4) p->x = CK_AQUA_BEACH_X - 2;
        if (!p->land && --p->bubT <= 0) { p->bubT = 40 + (int)(ck_rand(G) * 30); bubble(G, p->x, p->y - 30, 0, -0.7f, 1.8f, 70); }
    }
    // the camera follows the heroes; with two, it keeps them both on screen
    float mid = 0;
    for (int i = 0; i < H->np; i++) mid += H->gs[i].x;
    if (H->np) mid /= H->np;
    H->gcam = ck_clampf(mid - CK_W / 2, 0, CK_AQUA_W - CK_W);
    if (H->np > 1) for (int i = 0; i < H->np; i++) H->gs[i].x = ck_clampf(H->gs[i].x, H->gcam + 8, H->gcam + CK_W - 8);
    // food sinks slowly to the seabed and goes after a while
    int nf = 0;
    for (int i = 0; i < H->nfood; i++) {
        ck_flake *f = &H->food[i];
        if (f->y < CK_AQUA_SEABED - 3) f->y += 0.25f;
        if (--f->life > 0) H->food[nf++] = *f;
    }
    H->nfood = nf;
    for (int i = 0; i < CK_AQUA_FISH; i++) {
        ck_fish *f = &H->fish[i];
        int best = -1;
        float bd = 120;
        for (int k = 0; k < H->nfood; k++) { float d = hypotf(H->food[k].x - f->x, H->food[k].y - f->y); if (d < bd) { bd = d; best = k; } }
        if (best >= 0) {   // swim to the nearest flake and gobble it
            ck_flake *fd = &H->food[best];
            f->vx = ck_clampf((fd->x - f->x) * 0.08f, -1.4f, 1.4f);
            if (fabsf(f->vx) < 0.2f) f->vx = f->vx < 0 ? -0.2f : 0.2f;
            f->y += ck_clampf((fd->y - f->y) * 0.08f, -1, 1);
            if (bd < 6) {
                H->food[best] = H->food[--H->nfood];
                f->happy = 40;
                // a fed fish follows the nearest diver for a while
                f->follow = 600; f->who = 0;
                for (int k = 1; k < H->np; k++) if (hypotf(H->gs[k].x - f->x, H->gs[k].y - f->y) < hypotf(H->gs[f->who].x - f->x, H->gs[f->who].y - f->y)) f->who = k;
                ck_sfx(G, SFX_BOLT);
                ck_sparks(G, f->x, f->y, 4, 0xFF8FC7);
            }
        } else if (f->follow > 0 && f->who < H->np && !H->gs[f->who].land) {   // swim along behind your friend
            const ck_hhero *p = &H->gs[f->who];
            float tx = p->x - p->face * (20 + (i % 3) * 8), ty = p->y - 20 + (i % 3) * 6 - 6;
            f->vx = ck_clampf((tx - f->x) * 0.05f, -1.8f, 1.8f);
            if (fabsf(f->vx) < 0.15f) f->vx = p->face * 0.15f;
            f->y += ck_clampf((ty - f->y) * 0.05f, -1.2f, 1.2f);
            f->follow--;
        } else if (f->scare > 0 && --f->scare == 0) f->vx = (f->vx > 0 ? 1 : -1) * (f->kind == AFISH_JELLY ? 0.15f : 0.6f);
        else if (f->scare <= 0) {   // home again: each fish keeps to its own section
            float lo = f->home * CK_AQUA_SEC_W + 12, hi = lo + CK_AQUA_SEC_W - 24;
            if (f->x < lo) f->vx = fabsf(f->vx);
            if (f->x > hi) f->vx = -fabsf(f->vx);
        }
        // swimming into a fish boops it: a wiggle and a bubble
        for (int k = 0; k < H->np; k++) {
            const ck_hhero *p = &H->gs[k];
            if (p->land || f->happy > 0 || f->follow > 0 || hypotf(p->x - f->x, p->y - 18 - f->y) > 14) continue;
            f->happy = 40; f->vx = (f->x > p->x ? 1 : -1) * 1.2f; f->scare = 30;
            ck_sfx(G, SFX_BOING);
            bubble(G, f->x, f->y - 6, 0, -0.8f, 3, 50);
        }
        if (f->happy > 0) f->happy--;
        f->x += f->vx;
        f->y += sinf(G->t * (f->kind == AFISH_JELLY ? 1.2f : 1.5f) + f->ph) * (f->kind == AFISH_JELLY ? 0.6f : 0.25f);   // jellies bob
        if (f->x < 12) { f->x = 12; f->vx = fabsf(f->vx); }
        if (f->x > CK_AQUA_BEACH_X - 16) { f->x = CK_AQUA_BEACH_X - 16; f->vx = -fabsf(f->vx); }
        f->y = ck_clampf(f->y, CK_AQUA_TOP + 14, CK_AQUA_SEABED - 14);
    }
    for (int i = 0; i < CK_AQUA_SHRIMP; i++) {
        ck_shrimp *s = &H->shrimp[i];
        if (s->held) {   // in a hero's hand, wriggling
            const ck_hhero *p = &H->gs[s->held - 1];
            if (s->held > H->np || p->land) { s->held = 0; continue; }   // climbing out lets it go
            s->x = p->x + p->face * 10; s->y = p->y - 14; s->face = p->face; s->vx = s->vy = 0;
            continue;
        }
        if (s->spin > 0) {   // thrown: tumbling along, booping any fish it meets
            s->spin--;
            if (s->spin % 6 == 0) s->face = -s->face;
            for (int k = 0; k < CK_AQUA_FISH; k++) {
                ck_fish *f = &H->fish[k];
                if (f->happy > 0 || hypotf(f->x - s->x, f->y - s->y) > 12) continue;
                f->happy = 40; f->scare = 40; f->vx = (s->vx > 0 ? 1 : -1) * 1.6f;
                ck_sfx(G, SFX_BOING);
                bubble(G, f->x, f->y - 6, 0, -0.8f, 3, 50);
            }
        }
        bool scared = false;
        for (int k = 0; k < H->np; k++) if (!H->gs[k].land && fabsf(H->gs[k].x - s->x) < 24 && H->gs[k].y > CK_AQUA_SEABED - 40) scared = true;
        if (s->y >= CK_AQUA_SEABED - 2 && s->spin <= 0 && (scared || --s->rest <= 0)) {
            s->face = scared ? (H->gs[0].x > s->x ? -1 : 1) : (ck_rand(G) < 0.5f ? -1 : 1);
            s->vx = s->face * 1.2f; s->vy = -1.6f; s->rest = 120 + (int)(ck_rand(G) * 160);
        }
        s->vy += 0.05f;
        s->x += s->vx;
        if (s->x < 12 || s->x > CK_AQUA_BEACH_X - 40) { s->x = ck_clampf(s->x, 12, CK_AQUA_BEACH_X - 40); s->vx = -s->vx * 0.5f; }   // bounce off the glass
        s->y += s->vy;
        s->vx *= 0.98f;
        if (s->y < CK_AQUA_TOP + 8) { s->y = CK_AQUA_TOP + 8; s->vy = 0; }
        if (s->y >= CK_AQUA_SEABED - 2) { s->y = CK_AQUA_SEABED - 2; s->vy = 0; s->vx = 0; s->spin = 0; }
    }
    for (int i = 0; i < CK_AQUA_CRABS; i++) {
        ck_crab *c = &H->crabs[i];
        bool close = false;
        for (int k = 0; k < H->np; k++) if (H->gs[k].land && fabsf(H->gs[k].x - c->x) < 22) close = true;
        if (close && c->snap <= 0) { c->snap = 30; ck_sfx(G, SFX_TICK); }
        if (c->snap > 0) c->snap--;
        if (c->y < 0 || c->vy < 0) { c->vy += 0.25f; c->y += c->vy; if (c->y >= 0) { c->y = 0; c->vy = 0; } }   // a poked crab hops
        if (c->run > 0 && --c->run == 0) c->vx = (c->vx > 0 ? 1 : -1) * 0.5f;   // then calms down
        if (c->wait > 0) { c->wait--; continue; }
        c->x += c->vx;
        if (c->x < CK_AQUA_BEACH_X + 10 || c->x > CK_AQUA_W - 12) { c->vx = -c->vx; c->x = ck_clampf(c->x, CK_AQUA_BEACH_X + 10, CK_AQUA_W - 12); }
        if (c->run <= 0 && ck_rand(G) < 0.01f) c->wait = 30 + (int)(ck_rand(G) * 60);
    }
    for (int k = 0; k < CK_AQUA_CHESTS; k++) {
        if (H->chestT[k] > 0) H->chestT[k]--;
        if (H->chestT[k] > 60 && H->chestT[k] % 12 == 0) bubble(G, ck_aqua_chest_x[k] + (ck_rand(G) - 0.5f) * 10, CK_AQUA_SEABED - 16, 0, -0.9f, 1.6f, 80);
    }
}

// ---------------------------------------------------------------------------- the garage race
const ck_racetrack ck_tracks[CK_RACE_TRACKS] = {
    { "Green Hills", 0x5DBB63, 0x3F9D4E, 0x5A5E6A, 0xE04F4F, 0xFFFFFF, 0xFFF6D8, 0x8FD3FF, 0x3F9D4E },
    { "Desert Dash", 0xE8C77A, 0x6FAF4F, 0xA0805A, 0xFF8A3D, 0xFFF6D8, 0xFFE8B0, 0xFFC27A, 0xC9875A },
    { "Snowy Pass", 0xEEF6FF, 0x3F7D5E, 0x7D8CA3, 0x3A6FC0, 0xFFFFFF, 0xFFFFFF, 0xBFD9F2, 0x8FA8C8 },
    { "Moon Run", 0x2B2840, 0x8F8AA8, 0x5A566E, 0xFFD23F, 0x3A3650, 0xC9C4E0, 0x0F0A19, 0x4A4560 },
};
// both clamp what they are given: the N64 stops dead on powf of a negative number (the horizon row
// asked for the row above it, row -1, and that crashed a race)
float ck_race_row(float zr) { return CK_RACE_ZC / CK_RACE_CAR_Z * powf(CK_RACE_CAR_Z / ck_maxf(zr, 1), CK_RACE_SQUASH); }
float ck_race_depth(float row) { return CK_RACE_CAR_Z * powf(CK_RACE_ZC / CK_RACE_CAR_Z / ck_maxf(row, 0.5f), 1 / CK_RACE_SQUASH); }
float ck_race_curve(int track, float d) {
    if (d < 500) return 0;   // a straight start
    static const float amp[CK_RACE_TRACKS] = { 0.8f, 0.7f, 1.0f, 0.6f };
    float ramp = d < 1200 ? (d - 500) / 700 : 1;
    float bend = ck_clampf(1.8f * sinf(d * 0.0011f + track * 1.9f), -1, 1);   // bends come and go...
    float when = ck_clampf(2.2f * sinf(d * 0.00031f + 0.7f + track), -1, 1);   // ...with straights between runs of them
    return amp[track] * ramp * bend * fabsf(when);
}
void ck_race_pad(int track, int i, float *d, float *x, int *kind) {
    *d = 800 + i * 620 + (track * 37) % 100;
    *x = ((i * 2 + track) % 3 - 1) * CK_RACE_LANE;
    *kind = i % 3 == 2 ? PAD_RAMP : PAD_BOOST;
}
void ck_race_obst(int track, int i, float clock, float *d, float *x, int *kind, float *w, float *h) {
    static const int kinds[5] = { OBST_CONE, OBST_CRATE, OBST_OIL, OBST_CONE, OBST_CAR };
    *kind = kinds[i % 5];
    if (i < 5 && *kind == OBST_OIL) *kind = OBST_CRATE;   // the start is easy: no oil or slow cars yet
    if (i < 10 && *kind == OBST_CAR) *kind = OBST_CONE;
    // the gaps shrink from about 390 to 150 over the first 30, then stay at 150
    float t = i < 30 ? i / 30.0f : 1;
    bool across = track == TRACK_DESERT || track == TRACK_SNOW;   // tumbleweeds and penguins wander across
    *d = 400 + i * 150 + 3600 * (1 - (1 - t) * (1 - t)) + (*kind == OBST_CAR && !across ? clock * 60 : 0);   // cars drive at 1 px a frame
    *x = ((i * 7 + i / 4) % 3 - 1) * CK_RACE_LANE;
    if (*kind == OBST_CAR && across) *x = sinf(clock * (track == TRACK_SNOW ? 0.9f : 1.4f) + i) * 44;
    *w = *kind == OBST_OIL ? 20 : *kind == OBST_CAR ? (across ? 14 : 18) : *kind == OBST_CRATE ? 18 : 12;
    *h = *kind == OBST_OIL ? 14 : *kind == OBST_CAR ? (across ? 14 : 28) : *kind == OBST_CRATE ? 18 : 12;
}
const uint32_t ck_race_bot_col[CK_RACE_CARS - 1] = { 0xFF5A5A, 0x49D17A, 0xC28BFF };
static const float bot_top[CK_RACE_CARS - 1] = { 3.4f, 3.3f, 3.2f };   // the computer cars' top speeds
static void cars_reset(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int slot = c->slot;
        memset(c, 0, sizeof *c);
        c->slot = slot; c->face = 1;
        c->x = (i - 0.5f) * CK_RACE_LANE * (H->np > 1);   // two players start side by side
    }
    H->nbots = CK_RACE_CARS - H->np;
    for (int b = 0; b < H->nbots; b++) {   // the computer cars line up just ahead, one per lane
        ck_hhero *c = &H->bot[b];
        memset(c, 0, sizeof *c);
        c->slot = -1; c->face = 1;
        c->x = (b - 1) * CK_RACE_LANE; c->a = 40 + b * 12; c->key = b;
    }
    H->count = 3; H->win = -1; H->endT = 0; H->clock = 0; H->pickTrack = true; H->nfin = 0;
    ck_play(G, ck_race_song(H->track));
}
// each track has its own tune
int ck_race_song(int track) { static const int s[CK_RACE_TRACKS] = { SONG_RACE, SONG_RACE2, SONG_RACE3, SONG_RACE4 }; return s[track % CK_RACE_TRACKS]; }
// where a car is in the race right now: 1 is in front
int ck_race_rank(const ck_house *H, const ck_hhero *me) {
    int r = 1;
    for (int i = 0; i < H->np + H->nbots; i++) {
        const ck_hhero *o = i < H->np ? &H->gs[i] : &H->bot[i - H->np];
        if (o == me) continue;
        if (o->place && (!me->place || o->place < me->place)) r++;
        else if (!o->place && !me->place && o->a > me->a) r++;
    }
    return r;
}
// one car's frame: gas, brakes, bends, pads, jumps and bumps (sounds only for the players' cars)
static void car_step(ck_game *G, ck_hhero *c, int steer, bool gas, bool brake, float top, int player) {
    ck_house *H = &G->house;
    bool air = c->jz > 0, me = player >= 0;
    float want = gas && !brake ? top : 0, grip = brake ? 0.08f : gas ? 0.04f : 0.015f;
    if (fabsf(c->x) > CK_RACE_ROAD + 2 && !air) want = ck_minf(want, 1.8f);   // on the grass
    if (c->boost > 0) { c->boost--; want = 5.0f; grip = 0.04f; }
    if (c->rep > 0) { c->rep--; want = ck_minf(want, 1); steer = 0; }
    c->s += (want - c->s) * grip;
    c->vx += (steer * 2.2f - c->vx) * 0.25f;
    float bend = ck_race_curve(H->track, c->a);
    c->x = ck_clampf(c->x + c->vx - bend * c->s * 0.3f, -CK_RACE_EDGE, CK_RACE_EDGE);   // a bend pushes you to its outside
    c->lean += (ck_clampf(c->vx / 2.2f + bend * 0.5f, -1, 1) - c->lean) * 0.2f;
    c->sky += bend * c->s * 0.25f;
    if (c->laps) c->s *= 0.97f;   // past the line: roll to a stop
    c->a += c->s;
    if (air || c->jvz > 0) { c->jvz -= 0.14f; c->jz += c->jvz; if (c->jz <= 0) { c->jz = c->jvz = 0; if (me) ck_sfx(G, SFX_HOP); } }
    for (int k = 0; k < CK_RACE_PADS; k++) {
        float pd, px; int kind;
        ck_race_pad(H->track, k, &pd, &px, &kind);
        if (c->pads & (1u << k) || c->jz > 0 || fabsf(c->x - px) > 14 + 9 || fabsf(c->a - pd) > 15 + 12) continue;
        c->pads |= 1u << k;
        if (kind == PAD_BOOST) { c->boost = 75; c->s = ck_maxf(c->s, 4.4f); if (me) ck_sfx(G, SFX_BOOST); }
        else { c->jvz = 3.0f; c->jz = 0.01f; if (me) ck_sfx(G, SFX_BOING); }
    }
    for (int k = 0; k < CK_RACE_OBST; k++) {
        if (c->jz > 2) break;   // flying over it
        float od, ox, w, h; int kind;
        ck_race_obst(H->track, k, H->clock, &od, &ox, &kind, &w, &h);
        if (c->hit & (1ull << k) || fabsf(c->x - ox) > w / 2 + 9 || fabsf(c->a - od) > h / 2 + 12) continue;
        c->hit |= 1ull << k;
        if (kind == OBST_OIL) { c->rep = 40; c->s *= 0.6f; if (me) ck_sfx(G, SFX_SLIDE); }
        else { c->rep = 50; c->s = 0.6f; if (me) ck_sfx(G, SFX_CLANK); }
    }
    if (c->laps == 0 && c->a >= CK_RACE_LEN) {
        c->laps = 1; c->best = H->clock; c->place = ++H->nfin;
        if (me) ck_sfx(G, SFX_GEAR);
        if (me && H->win < 0) { H->win = player; ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 40, 40); ck_play(G, SONG_VICTORY); }   // the race music stops for a fanfare
    }
}
// A computer driver: every so often it picks the lane that is clear furthest ahead and steers for it.
static void bot_drive(ck_game *G, ck_hhero *c, int b) {
    ck_house *H = &G->house;
    if (--c->bounce <= 0) {
        c->bounce = 8 + b * 3;
        float look = 120 - b * 15, clear[3] = { look, look, look };
        for (int k = 0; k < CK_RACE_OBST; k++) {
            float od, ox, w, h; int kind;
            ck_race_obst(H->track, k, H->clock, &od, &ox, &kind, &w, &h);
            float d = od - c->a;
            if (d < -20 || d > look) continue;
            for (int l = 0; l < 3; l++) if (fabsf(ox - (l - 1) * CK_RACE_LANE) < w / 2 + 14) clear[l] = ck_minf(clear[l], d);
        }
        int lane = (int)lroundf(c->x / CK_RACE_LANE) + 1, best = c->key;
        if (best < 0 || best > 2) best = 1;
        if (clear[best] < look) {   // its lane is blocked: the clearest one, the nearest if they tie
            best = lane < 0 ? 0 : lane > 2 ? 2 : lane;
            for (int l = 0; l < 3; l++) if (clear[l] > clear[best] + 1 || (clear[l] >= clear[best] - 1 && abs(l - lane) < abs(best - lane) && clear[l] >= clear[best])) best = l;
        }
        c->key = best;
    }
    float tx = (c->key - 1) * CK_RACE_LANE;
    car_step(G, c, tx > c->x + 3 ? 1 : tx < c->x - 3 ? -1 : 0, !c->laps, false, bot_top[b] * (G->prog.little ? 0.93f : 1), -1);   // gentler for little kids
}
static void cars_start(ck_game *G) { ck_house_game_heroes(G); cars_reset(G); ck_go(G, SC_CARS); }
// Before each race: left and right pick the track, A starts the countdown.
static void cars_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    if (H->pickTrack) {
        int d = (ANYHIT(G, CK_RIGHT) ? 1 : 0) - (ANYHIT(G, CK_LEFT) ? 1 : 0);
        if (d) { H->track = (H->track + d + CK_RACE_TRACKS) % CK_RACE_TRACKS; ck_sfx(G, SFX_SELECT); ck_play(G, ck_race_song(H->track)); }
        if (ANYHIT(G, CK_A)) { H->pickTrack = false; ck_sfx(G, SFX_CONFIRM); }
        return;
    }
    if (H->count > 0) {
        int was = (int)ceilf(H->count);
        H->count -= 1.0f / 60;
        if ((int)ceilf(H->count) != was) ck_sfx(G, H->count <= 0 ? SFX_CONFIRM : SFX_TICK);
        return;
    }
    bool all = true;
    for (int i = 0; i < H->np; i++) if (H->gs[i].laps == 0) all = false;   // the race ends with the players
    if (H->win >= 0) {   // the others get 4 seconds to finish too
        if ((H->endT += 1.0f / 60) > 4) { H->track = (H->track + 1) % CK_RACE_TRACKS; cars_reset(G); return; }   // on to the next track
        if (all) return;
    }
    H->clock += 1.0f / 60;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int s = c->slot;
        // the car only goes while you hold the gas (A or up); let go and it rolls to a stop, B or down brakes
        car_step(G, c, dir_x(G, s), HELD(G, s, CK_UP) || HELD(G, s, CK_A), HELD(G, s, CK_DOWN) || HELD(G, s, CK_B), 3.6f, i);
    }
    for (int b = 0; b < H->nbots; b++) bot_drive(G, &H->bot[b], b);
}

// ---------------------------------------------------------------------------- the piano
static const int SEMI_WHITE[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
void ck_piano_key(int i, float *x, bool *black) {
    int o = i / 12, s = i % 12, w = SEMI_WHITE[s];
    if (w >= 0) { *x = CK_KEYS_X + (o * 7 + w) * CK_KEYS_W + CK_KEYS_W / 2.0f; *black = false; }
    else { *x = CK_KEYS_X + (o * 7 + SEMI_WHITE[s - 1] + 1) * CK_KEYS_W; *black = true; }
}
// x, y, r: where each piece sits on the screen
const ck_drum ck_drums[CK_DRUMS] = {
    { "Kick", "A", CK_A, DRUM_DRUM, 160, 172, 34, 0xE04F4F },
    { "Snare", "B", CK_B, DRUM_DRUM, 104, 160, 20, 0xFFF6D8 },
    { "Hi-hat", "C", CK_C, DRUM_CYMBAL, 62, 120, 22, 0xFFD23F },
    { "Crash", "L", CK_L, DRUM_CYMBAL, 92, 72, 26, 0xFFD23F },
    { "Cowbell", "R", CK_R, DRUM_CYMBAL, 232, 72, 18, 0xC9B8C7 },
    { "Tom", "^", CK_UP, DRUM_DRUM, 136, 112, 18, 0x5AD1FF },
    { "Tom", "<", CK_LEFT, DRUM_DRUM, 184, 112, 18, 0x49D17A },
    { "Floor tom", "v", CK_DOWN, DRUM_DRUM, 226, 158, 24, 0xC28BFF },
    { "Clap", ">", CK_RIGHT, DRUM_HANDS, 276, 120, 18, 0xFF8FC7 },
};

static void piano_start(ck_game *G, bool drums) {
    ck_house *H = &G->house;
    ck_house_game_heroes(G);
    H->drums = drums;
    for (int i = 0; i < H->np; i++) H->gs[i].key = i ? 16 : 7;
    memset(H->lit, 0, sizeof H->lit);
    H->nnotes = 0;
    ck_go(G, SC_PIANO);
}
static void play_key(ck_game *G, ck_hhero *p, int i) {
    ck_house *H = &G->house;
    static const uint32_t cols[6] = { 0xFF5A5A, 0xFFD23F, 0x49D17A, 0x5AD1FF, 0xC28BFF, 0xFF8FC7 };
    if (G->nnotes < 4) G->notes[G->nnotes++] = 261.63f * powf(2, i / 12.0f);
    H->lit[i] = 14;
    p->bounce = 10;
    float x;
    bool black;
    ck_piano_key(i, &x, &black);
    if (H->nnotes < 16) H->notes[H->nnotes++] = (ck_note_fx){ x, 140, 60, cols[i % 6] };
}
static void drums_update(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        for (int k = 0; k < CK_DRUMS; k++) {
            if (!HIT(G, p->slot, ck_drums[k].btn)) continue;
            ck_sfx(G, SFX_DRUM0 + k);
            H->lit[k] = 10;
            p->bounce = 10;
            if (H->nnotes < 16) H->notes[H->nnotes++] = (ck_note_fx){ ck_drums[k].x, ck_drums[k].y - ck_drums[k].r, 40, ck_drums[k].col };
        }
        if (p->bounce > 0) p->bounce--;
    }
}
static void piano_update(ck_game *G) {
    ck_house *H = &G->house;
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    if (H->drums) drums_update(G);
    else
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->gs[i];
        int s = p->slot, d = dir_x(G, s);
        if (!d) p->rep = 0;
        else if (p->rep == 0 || (p->rep > 14 && p->rep % 5 == 0)) {
            int was = p->key;
            p->key = p->key + d < 0 ? 0 : p->key + d > CK_KEYS_N - 1 ? CK_KEYS_N - 1 : p->key + d;
            if (p->key != was && HELD(G, s, CK_A) && p->rep > 0) play_key(G, p, p->key);
        }
        if (d) p->rep++;
        if (HIT(G, s, CK_A)) play_key(G, p, p->key);
        if (HIT(G, s, CK_B)) play_key(G, p, p->key + 12 < CK_KEYS_N ? p->key + 12 : p->key - 12 >= 0 ? p->key - 12 : p->key);
        if (p->bounce > 0) p->bounce--;
    }
    for (int i = 0; i < CK_KEYS_N; i++) if (H->lit[i]) H->lit[i]--;
    int n = 0;
    for (int i = 0; i < H->nnotes; i++) { H->notes[i].y -= 0.8f; if (--H->notes[i].life > 0) H->notes[n++] = H->notes[i]; }
    H->nnotes = n;
}

void ck_house_step(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        int s = G->scene == SC_HOUSE ? H->ps[i].slot : H->gs[i].slot;
        if (s >= 0 && s < 2 && (G->in.held[s] & ~G->in.prev[s])) H->last = i;
    }
    if (H->last >= H->np) H->last = 0;
    switch (G->scene) {
    case SC_HOUSE: ck_house_update(G); break;
    case SC_PAINT: paint_update(G); break;
    case SC_AQUA: aqua_update(G); break;
    case SC_CARS: cars_update(G); break;
    case SC_PIANO: piano_update(G); break;
    case SC_PINBALL: ck_pin_update(G); break;
    case SC_BLAST: ck_blast_update(G); break;
    case SC_FISH: ck_fish_update(G); break;
    case SC_BOLT: ck_bolt_update(G); break;
    case SC_JETRUN: ck_run_update(G); break;
    case SC_SAND: ck_sand_update(G); break;
    }
}
