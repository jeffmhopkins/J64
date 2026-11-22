// The heroes' house and the little games in it. Mirrors houseScene, paintScene, aquaScene,
// carsScene and pianoScene in web/game.js (the layout is HOUSEART.LAYOUT in web/lib/houseart.js).
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
    { 1520, 1720, "Arcade", 0x3A2E5C, 0x46386E, 0x2B2240 },
};
// indexed by SPOT_*; the bedroom is first so saving is quick
const ck_spot ck_spots[CK_HOUSE_SPOTS] = {
    { 615, SPOT_SNACK, FURN_FRIDGE, "Open the fridge" },
    { 850, SPOT_PAINT, FURN_EASEL, "Paint the mural" },
    { 1005, SPOT_PIANO, FURN_PIANO, "Play the piano" },
    { 1210, SPOT_AQUA, FURN_TANK, "Swim with the fish" },
    { 490, SPOT_BED, FURN_BED, "Sleep and save" },
    { 1400, SPOT_CARS, FURN_CAR, "Race the cars" },
    { 1085, SPOT_DRUMS, FURN_DRUM, "Play the drums" },
    { 262, SPOT_MAIL, FURN_MAILBOX, "Check the mail" },
    { 120, SPOT_TREE, FURN_TREE, "Pick an apple" },
    { 44, SPOT_BACK, FURN_SIGN, "Back a world" },
    { 685, SPOT_STOVE, FURN_STOVE, "Cook something" },
    { 1580, SPOT_PINBALL, FURN_PINBALL, "Play pinball" },
    { 1665, SPOT_BLAST, FURN_ARCADE, "Play Bug Blaster" },
    { 345, SPOT_POND, FURN_POND, "Go fishing" },
};
const ck_decor ck_decors[CK_HOUSE_DECOR] = {
    { 728, FURN_TABLE }, { 920, FURN_PLANT }, { 1285, FURN_PLANT }, { 555, FURN_LAMP }, { 1490, FURN_TOOLS },
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
};
const int ck_cook_ingr[CK_COOK_INGR] = { FOOD_EGG, FOOD_TOMATO, FOOD_CHEESE, FOOD_MUSHROOM, FOOD_CARROT, FOOD_BREAD,
                                         FOOD_POTATO, FOOD_RICE, FOOD_SAUSAGE, FOOD_PASTA, FOOD_APPLE, FOOD_BANANA };
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
#define HOUSE_JUMP (-4.4f)
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

void ck_house_enter(ck_game *G, bool fromShop) {
    ck_house *H = &G->house;
    (void)fromShop;
    float x = 200;
    H->np = 0;
    for (int s = 0; s < 2; s++) {
        if (!G->prog.joined[s]) continue;
        ck_hhero *p = &H->ps[H->np];
        memset(p, 0, sizeof *p);
        p->slot = s; p->x = x - 10 + H->np * 20; p->y = CK_HOUSE_FLOOR; p->face = 1; p->ground = true;
        H->np++;
    }
    uint32_t cols[CK_HOUSE_BABIES];
    find_babies(G, cols, &H->nbabies);
    hseed = 7;
    for (int i = 0; i < H->nbabies; i++) {
        ck_hbaby *b = &H->babies[i];
        const ck_room *r = &ck_rooms[i % CK_HOUSE_ROOMS];
        b->hex = cols[i];
        b->x = r->x0 + 30 + hrng() * (r->x1 - r->x0 - 60);
        b->x0 = r->x0 + 14; b->x1 = r->x1 - 14;
        b->vx = (hrng() < 0.5f ? -1 : 1) * (0.3f + hrng() * 0.3f);
        b->y = 0; b->vy = 0;
        b->wait = (int)(hrng() * 120);
    }
    H->mode = HM_WALK;
    H->msgT = 0;
    H->nthrown = 0;
    H->fpick[0] = 0; H->fpick[1] = 2;
    H->cpick[0] = 0; H->cpick[1] = 2;
    H->cam = ck_clampf(x - CK_W / 2, 0, CK_HOUSE_W - CK_W);
    ck_go(G, SC_HOUSE);
}

bool ck_spot_here(const ck_game *G, int k) { return ck_spots[k].kind != SPOT_BACK || G->worldNum > 1; }
int ck_house_near(const ck_game *G, const ck_hhero *p) {
    for (int k = 0; k < CK_HOUSE_SPOTS; k++) if (ck_spot_here(G, k) && fabsf(p->x - ck_spots[k].x) < 22) return k;
    return -1;
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
        H->npan = 0; H->heat = 0; H->dish = 0;
        break;
    case SPOT_MAIL:
        ck_sfx(G, SFX_OPEN);
        H->letter = H->letter % CK_LETTERS + 1;
        H->mode = HM_LETTER;
        break;
    case SPOT_TREE:
        p->hold = FOOD_APPLE + 1;
        ck_sfx(G, SFX_SPROUT);
        ck_sparks(G, p->x - H->cam, p->y - 50, 6, 0x49D17A);
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
    case SPOT_BED: ck_sfx(G, SFX_SELECT); H->mode = HM_SLOTS; H->pick = G->slot; break;
    }
}

void ck_house_back(ck_game *G) { ck_sfx(G, SFX_SELECT); G->scene = SC_HOUSE; G->t = 0; ck_play(G, SONG_SELECT); }
#define back_to_house ck_house_back

// The stove: the dish is named by what went in the pan (the first rule that fits wins).
void ck_cook_dish(const int *pan, int n, int *dish, char *name, int size) {
    bool has[FOOD_COUNT] = { 0 };
    for (int i = 0; i < n; i++) has[pan[i]] = true;
    int k;
    if (has[FOOD_SAUSAGE] && has[FOOD_BREAD]) k = has[FOOD_CHEESE] ? DISH_CHEESY_HOTDOG : has[FOOD_TOMATO] ? DISH_KETCHUP_HOTDOG : DISH_HOTDOG;
    else if (has[FOOD_PASTA]) k = has[FOOD_TOMATO] && has[FOOD_SAUSAGE] ? DISH_MEATBALLS : has[FOOD_TOMATO] ? DISH_SPAGHETTI : has[FOOD_CHEESE] ? DISH_MAC_CHEESE : has[FOOD_MUSHROOM] ? DISH_MUSHROOM_PASTA : DISH_BUTTER_PASTA;
    else if (has[FOOD_RICE]) k = has[FOOD_EGG] ? DISH_FRIED_RICE : has[FOOD_SAUSAGE] ? DISH_SAUSAGE_RICE : has[FOOD_CARROT] || has[FOOD_MUSHROOM] ? DISH_VEGGIE_RICE : has[FOOD_BANANA] || has[FOOD_APPLE] ? DISH_RICE_PUDDING : DISH_RICE_BOWL;
    else if (has[FOOD_BANANA] && has[FOOD_BREAD]) k = DISH_BANANA_BREAD;
    else if (has[FOOD_APPLE]) k = has[FOOD_BANANA] ? DISH_FRUIT_PIE : DISH_APPLE_PIE;
    else if (has[FOOD_BANANA]) k = DISH_PANCAKES;
    else if (has[FOOD_CARROT] && has[FOOD_EGG]) k = DISH_CARROT_CAKE;
    else if (has[FOOD_EGG]) k = has[FOOD_CHEESE] ? DISH_CHEESY_OMELETTE : has[FOOD_MUSHROOM] ? DISH_MUSHROOM_OMELETTE : has[FOOD_TOMATO] ? DISH_TOMATO_OMELETTE : has[FOOD_SAUSAGE] ? DISH_BIG_BREAKFAST : has[FOOD_POTATO] ? DISH_POTATO_OMELETTE : DISH_OMELETTE;
    else if (has[FOOD_BREAD]) k = has[FOOD_CHEESE] && has[FOOD_TOMATO] ? DISH_PIZZA_TOASTIE : has[FOOD_CHEESE] ? DISH_CHEESE_TOASTIE : has[FOOD_MUSHROOM] ? DISH_MUSHROOM_TOAST : DISH_TOAST;
    else if (has[FOOD_POTATO] && !has[FOOD_TOMATO] && !has[FOOD_CARROT] && !has[FOOD_MUSHROOM]) k = has[FOOD_CHEESE] ? DISH_CHEESY_CHIPS : has[FOOD_SAUSAGE] ? DISH_SAUSAGE_CHIPS : DISH_CHIPS;
    else k = has[FOOD_POTATO] ? DISH_VEGGIE_STEW : has[FOOD_SAUSAGE] ? DISH_SAUSAGE_STEW : has[FOOD_TOMATO] ? DISH_TOMATO_SOUP : has[FOOD_MUSHROOM] ? DISH_MUSHROOM_SOUP
         : has[FOOD_CARROT] ? DISH_CARROT_SOUP : n == 1 ? DISH_CHEESE_PLATE : DISH_CHEESE_SOUP;
    *dish = FOOD_DISH0 + k + 1;
    snprintf(name, size, "%s", ck_foods[FOOD_DISH0 + k].name);
}
static void cook_name(ck_house *H) { ck_cook_dish(H->pan, H->npan, &H->dish, H->dishName, sizeof H->dishName); }
static void cook_update(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *p = &H->ps[i];
        int s = p->slot, *c = &H->cpick[i];
        if (HIT(G, s, CK_LEFT) && *c % CK_COOK_ROW > 0) { (*c)--; ck_sfx(G, SFX_SELECT); }
        if (HIT(G, s, CK_RIGHT) && *c % CK_COOK_ROW < CK_COOK_ROW - 1) { (*c)++; ck_sfx(G, SFX_SELECT); }
        if ((HIT(G, s, CK_UP) || HIT(G, s, CK_DOWN)) && !H->dish) { *c = (*c + CK_COOK_ROW) % CK_COOK_INGR; ck_sfx(G, SFX_SELECT); }   // the other shelf
        if (H->dish) {   // done: A takes the dish, and everyone playing gets one
            if (HIT(G, s, CK_A)) {
                for (int k = 0; k < H->np; k++) H->ps[k].hold = H->dish;
                char m[40];
                snprintf(m, sizeof m, H->np > 1 ? "%s for two!" : "%s!", H->dishName);
                say(H, m);
                ck_sfx(G, SFX_BUY);
                H->mode = HM_WALK;
                return;
            }
            continue;
        }
        if (HIT(G, s, CK_A)) {   // into the pan; a full pan swaps its last one for this
            if (H->npan < CK_COOK_PAN) H->pan[H->npan++] = ck_cook_ingr[*c];
            else H->pan[CK_COOK_PAN - 1] = ck_cook_ingr[*c];
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
                p->hold = *c + 1;
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
        int s = p->slot, want = dir_x(G, s);
        if (p->eat > 0) { p->eat--; p->vx = 0; }
        else {
            p->vx += (want * HOUSE_SPEED - p->vx) * 0.3f;
            if (want) p->face = want;
            // holding food: A eats it, B throws it the way you face
            int k = p->ground && !p->hold ? ck_house_near(G, p) : -1;
            if (p->hold && HIT(G, s, CK_A)) {
                p->col = p->hold - 1; p->hold = 0; p->eat = 50;
                ck_sfx(G, SFX_SPROUT);
                ck_sparks(G, p->x - H->cam, p->y - 22, 8, 0xFFD23F);
                bool full = G->prog.lives >= 3;
                G->prog.lives = 3;
                ck_save(G);
                char m[32];
                snprintf(m, sizeof m, full ? "Yum, %s!" : "Yum! Batteries full!", ck_foods[p->col].name);
                say(H, m);
            } else if (p->hold && HIT(G, s, CK_B)) {
                if (H->nthrown < CK_THROWN) H->thrown[H->nthrown++] = (ck_thrown){ p->x + p->face * 8, p->y - 22, p->face * 3.2f + p->vx * 0.5f, -2.6f, p->hold - 1, 150 };
                p->hold = 0;
                ck_sfx(G, SFX_BLOW);
            }
            // A (or down) at a room's thing uses it; A anywhere else jumps
            else if (k >= 0 && HIT(G, s, CK_A)) { use_spot(G, k, p); if (G->scene != SC_HOUSE || H->mode != HM_WALK) return; }
            else if (HIT(G, s, CK_A) && p->ground) { p->vy = HOUSE_JUMP; p->ground = false; ck_sfx(G, SFX_JUMP); }
        }
        p->vy += HOUSE_GRAV; p->x += p->vx; p->y += p->vy;
        if (p->y >= CK_HOUSE_FLOOR) { p->y = CK_HOUSE_FLOOR; p->vy = 0; p->ground = true; }
        p->x = ck_clampf(p->x, -4, CK_HOUSE_W - 10);
        p->anim += fabsf(p->vx);
        if (p->x < 0) { ck_sfx(G, SFX_SELECT); ck_save(G); ck_go_map(G); return; }
    }
    float mid = 0;
    for (int i = 0; i < H->np; i++) mid += H->ps[i].x;
    mid /= H->np > 0 ? H->np : 1;
    H->cam = ck_clampf(mid - CK_W / 2, 0, CK_HOUSE_W - CK_W);
    if (H->np > 1) for (int i = 0; i < H->np; i++) H->ps[i].x = ck_clampf(H->ps[i].x, H->cam > 0 ? H->cam + 8 : -4, H->cam + CK_W - 8);   // at the far left you can still walk out to the map
    int nt = 0;
    for (int i = 0; i < H->nthrown; i++) {
        ck_thrown *f = &H->thrown[i];
        f->vy += 0.2f; f->x += f->vx; f->y += f->vy;
        if (f->y >= CK_HOUSE_FLOOR - 4) { f->y = CK_HOUSE_FLOOR - 4; f->vy *= -0.45f; f->vx *= 0.75f; if (fabsf(f->vy) < 0.6f) f->vy = 0; }
        if (f->x < 4 || f->x > CK_HOUSE_W - 4) f->vx = -f->vx;
        bool gone = --f->life <= 0;
        for (int k = 0; k < H->nbabies && !gone; k++)   // a baby catches it and munches it
            if (fabsf(H->babies[k].x - f->x) < 10 && f->y > CK_HOUSE_FLOOR - 20) { H->babies[k].vy = -2.2f; H->babies[k].y = -0.1f; gone = true; ck_sfx(G, SFX_HOP); ck_sparks(G, f->x - H->cam, f->y, 5, 0xFF8FC7); }
        if (gone) { if (f->life <= 0) ck_puff(G, f->x - H->cam, f->y, 4, 0xFFFFFF, 200); }
        else H->thrown[nt++] = *f;
    }
    H->nthrown = nt;
    for (int i = 0; i < H->nbabies; i++) {
        ck_hbaby *b = &H->babies[i];
        if (b->wait > 0) { b->wait--; continue; }
        b->x += b->vx;
        if (b->x < b->x0 || b->x > b->x1) { b->vx = -b->vx; b->x = ck_clampf(b->x, b->x0, b->x1); }
        if (b->y == 0 && ck_rand(G) < 0.006f) b->vy = -1.8f;
        if (b->y < 0 || b->vy < 0) { b->vy += 0.12f; b->y += b->vy; if (b->y >= 0) { b->y = 0; b->vy = 0; } }
        if (ck_rand(G) < 0.003f) b->wait = 60 + (int)(ck_rand(G) * 90);
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
static void cars_reset(ck_game *G) {
    ck_house *H = &G->house;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int slot = c->slot;
        memset(c, 0, sizeof *c);
        c->slot = slot; c->face = 1;
    }
    H->count = 3; H->win = -1; H->endT = 0; H->clock = 0; H->pickTrack = true;
}
static void cars_start(ck_game *G) { ck_house_game_heroes(G); cars_reset(G); ck_go(G, SC_CARS); }
// Before each race: left and right pick the track, A starts the countdown.
static void cars_update(ck_game *G) {
    ck_house *H = &G->house;
    ck_step_parts(G);
    if (ANYHIT(G, CK_START)) { back_to_house(G); return; }
    if (H->pickTrack) {
        int d = (ANYHIT(G, CK_RIGHT) ? 1 : 0) - (ANYHIT(G, CK_LEFT) ? 1 : 0);
        if (d) { H->track = (H->track + d + CK_RACE_TRACKS) % CK_RACE_TRACKS; ck_sfx(G, SFX_SELECT); }
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
    for (int i = 0; i < H->np; i++) if (H->gs[i].laps == 0) all = false;
    if (H->win >= 0) {   // the others get 4 seconds to finish too
        if ((H->endT += 1.0f / 60) > 4) { H->track = (H->track + 1) % CK_RACE_TRACKS; cars_reset(G); return; }   // on to the next track
        if (all) return;
    }
    H->clock += 1.0f / 60;
    for (int i = 0; i < H->np; i++) {
        ck_hhero *c = &H->gs[i];
        int s = c->slot, steer = dir_x(G, s);
        bool air = c->jz > 0;
        // the car only goes while you hold the gas (A or up); let go and it rolls to a stop, B or down brakes
        bool gas = HELD(G, s, CK_UP) || HELD(G, s, CK_A), brake = HELD(G, s, CK_DOWN) || HELD(G, s, CK_B);
        float want = gas && !brake ? 3.6f : 0, grip = brake ? 0.08f : gas ? 0.04f : 0.015f;
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
        if (air || c->jvz > 0) { c->jvz -= 0.14f; c->jz += c->jvz; if (c->jz <= 0) { c->jz = c->jvz = 0; ck_sfx(G, SFX_HOP); } }
        for (int k = 0; k < CK_RACE_PADS; k++) {
            float pd, px; int kind;
            ck_race_pad(H->track, k, &pd, &px, &kind);
            if (c->pads & (1u << k) || c->jz > 0 || fabsf(c->x - px) > 14 + 9 || fabsf(c->a - pd) > 15 + 12) continue;
            c->pads |= 1u << k;
            if (kind == PAD_BOOST) { c->boost = 75; c->s = ck_maxf(c->s, 4.4f); ck_sfx(G, SFX_BOOST); }
            else { c->jvz = 3.0f; c->jz = 0.01f; ck_sfx(G, SFX_BOING); }
        }
        for (int k = 0; k < CK_RACE_OBST; k++) {
            if (c->jz > 2) break;   // flying over it
            float od, ox, w, h; int kind;
            ck_race_obst(H->track, k, H->clock, &od, &ox, &kind, &w, &h);
            if (c->hit & (1ull << k) || fabsf(c->x - ox) > w / 2 + 9 || fabsf(c->a - od) > h / 2 + 12) continue;
            c->hit |= 1ull << k;
            if (kind == OBST_OIL) { c->rep = 40; c->s *= 0.6f; ck_sfx(G, SFX_SLIDE); }
            else { c->rep = 50; c->s = 0.6f; ck_sfx(G, SFX_CLANK); }
        }
        if (c->laps == 0 && c->a >= CK_RACE_LEN) {
            c->laps = 1; c->best = H->clock;
            ck_sfx(G, SFX_GEAR);
            if (H->win < 0) { H->win = i; ck_sfx(G, SFX_FLAG); ck_confetti(G, 160, 40, 40); }
        }
    }
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
    }
}
