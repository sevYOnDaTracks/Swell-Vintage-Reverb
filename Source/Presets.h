#pragma once
#include <array>
#include <cstring>

// Space character: picks how the engine builds the room (not exposed as a knob, like a hardware program).
enum SpaceType { Hall, Room, Chamber, Plate, Ambience, Space, Spring, Gated };

// One factory program. The first block is the hidden character of the space,
// the second block is what the program writes into the visible controls when it loads.
struct Program {
    const char* category; const char* name; SpaceType type;
    float size;       // 0.25 (booth) .. 2 (cathedral): scales every delay length
    float diffusion;  // input diffuser feedback, 0.5 .. 0.8
    float modulation; // tail chorus depth, 0 .. 1
    float decay;      // seconds
    float predelay;   // ms
    int pattern;      // early reflection set 1..40
    float earlyLate;  // 0 = early only, 1 = max late
    float vlf;        // dB
    float rollRef, rollRev;              // Hz
    float lowFreq, lowMul, highFreq, highMul;
    float delayTime, delayLevel;         // ms, 0..1
};

inline constexpr std::array<const char*, 7> categories = { "Halls", "Rooms", "Chambers", "Plates", "Ambience", "Spaces", "Vintage" };

inline constexpr int programCount = 50;
inline constexpr std::array<Program, programCount> programs = {{
    // category    name                 type      size  diff  mod   decay pre  pat  e/l   vlf   rRef   rRev   lowF  lowM  highF  highM dly  dlyLvl
    { "Halls",    "Big Hall Vocal",     Hall,     1.30f, .72f, .35f, 2.6f, 24,  15, .75f, -6.f, 12000, 9000,  450, 1.2f, 4200, .70f, 0,   0 },
    { "Halls",    "Concert Hall",       Hall,     1.45f, .74f, .30f, 2.2f, 18,  3,  .65f, -4.f, 10000, 8000,  400, 1.3f, 3600, .65f, 0,   0 },
    { "Halls",    "Warm Hall",          Hall,     1.35f, .72f, .30f, 2.8f, 20,  7,  .70f, -3.f, 8000,  5500,  500, 1.4f, 2800, .55f, 0,   0 },
    { "Halls",    "Bright Hall",        Hall,     1.25f, .70f, .40f, 2.0f, 16,  11, .70f, -8.f, 16000, 13000, 350, 1.0f, 6000, .85f, 0,   0 },
    { "Halls",    "Dark Hall",          Hall,     1.40f, .74f, .25f, 3.2f, 30,  19, .75f, -2.f, 6000,  4000,  600, 1.5f, 2200, .45f, 0,   0 },
    { "Halls",    "Opera House",        Hall,     1.60f, .75f, .30f, 2.4f, 28,  23, .60f, -5.f, 11000, 8500,  420, 1.2f, 4000, .70f, 0,   0 },
    { "Halls",    "Small Hall",         Hall,     0.95f, .70f, .30f, 1.5f, 12,  27, .65f, -6.f, 12000, 9500,  400, 1.1f, 4500, .75f, 0,   0 },
    { "Halls",    "Gold Hall",          Hall,     1.30f, .73f, .45f, 2.5f, 22,  31, .80f, -5.f, 14000, 11000, 380, 1.1f, 5200, .80f, 0,   0 },

    { "Rooms",    "Tight Drum Room",    Room,     0.45f, .62f, .10f, 0.6f, 2,   2,  .35f, -8.f, 14000, 10000, 300, 1.0f, 5000, .70f, 0,   0 },
    { "Rooms",    "Studio Live",        Room,     0.60f, .65f, .15f, 0.9f, 4,   6,  .45f, -7.f, 13000, 9000,  320, 1.1f, 4500, .70f, 0,   0 },
    { "Rooms",    "Wood Room",          Room,     0.55f, .64f, .15f, 0.8f, 3,   10, .45f, -5.f, 9000,  6500,  380, 1.2f, 3000, .55f, 0,   0 },
    { "Rooms",    "Vocal Booth",        Room,     0.28f, .55f, .05f, 0.35f,0,   14, .30f, -12.f,11000, 8000,  300, 0.9f, 4000, .60f, 0,   0 },
    { "Rooms",    "Large Room",         Room,     0.85f, .68f, .20f, 1.3f, 8,   18, .55f, -6.f, 12000, 9000,  350, 1.1f, 4200, .70f, 0,   0 },
    { "Rooms",    "Garage",             Room,     0.70f, .60f, .10f, 1.1f, 5,   22, .40f, -4.f, 15000, 12000, 300, 1.0f, 6000, .90f, 0,   0 },
    { "Rooms",    "Bedroom",            Room,     0.40f, .58f, .10f, 0.5f, 1,   26, .40f, -10.f,8000,  6000,  350, 1.0f, 2500, .50f, 0,   0 },
    { "Rooms",    "Stone Room",         Room,     0.65f, .66f, .15f, 1.2f, 6,   30, .50f, -5.f, 16000, 13000, 300, 1.0f, 7000, .95f, 0,   0 },

    { "Chambers", "Echo Chamber",       Chamber,  0.80f, .74f, .20f, 1.8f, 10,  4,  .70f, -5.f, 11000, 8000,  400, 1.2f, 3800, .70f, 0,   0 },
    { "Chambers", "Stone Chamber",      Chamber,  0.85f, .75f, .20f, 2.1f, 12,  8,  .70f, -4.f, 14000, 11000, 350, 1.1f, 5500, .85f, 0,   0 },
    { "Chambers", "Tiled Chamber",      Chamber,  0.75f, .73f, .25f, 1.6f, 8,   12, .65f, -6.f, 16000, 13000, 320, 1.0f, 7000, .95f, 0,   0 },
    { "Chambers", "Vocal Chamber",      Chamber,  0.70f, .74f, .30f, 1.4f, 20,  16, .75f, -8.f, 12000, 9000,  450, 1.0f, 4500, .75f, 0,   0 },
    { "Chambers", "Dark Chamber",       Chamber,  0.85f, .75f, .20f, 2.3f, 14,  20, .70f, -3.f, 7000,  4500,  500, 1.4f, 2200, .50f, 0,   0 },
    { "Chambers", "Bright Chamber",     Chamber,  0.75f, .72f, .30f, 1.5f, 10,  24, .70f, -7.f, 17000, 15000, 300, 1.0f, 8000, 1.0f, 0,   0 },

    { "Plates",   "Silver Plate",       Plate,    0.70f, .80f, .30f, 1.8f, 10,  1,  .95f, -9.f, 15000, 12000, 300, 0.8f, 6000, .90f, 0,   0 },
    { "Plates",   "Vocal Plate",        Plate,    0.75f, .80f, .35f, 2.2f, 40,  1,  .95f, -10.f,13000, 10000, 350, 0.8f, 5000, .85f, 0,   0 },
    { "Plates",   "Snare Plate",        Plate,    0.60f, .78f, .25f, 1.3f, 0,   1,  .90f, -8.f, 16000, 14000, 280, 0.9f, 7000, .95f, 0,   0 },
    { "Plates",   "Gold Plate",         Plate,    0.75f, .80f, .40f, 2.6f, 20,  1,  .95f, -8.f, 12000, 9000,  320, 0.9f, 4500, .80f, 0,   0 },
    { "Plates",   "Dark Plate",         Plate,    0.75f, .80f, .30f, 2.4f, 15,  1,  .95f, -6.f, 7000,  5000,  400, 1.0f, 2500, .55f, 0,   0 },
    { "Plates",   "Short Plate",        Plate,    0.55f, .78f, .25f, 0.8f, 5,   1,  .90f, -10.f,15000, 12000, 300, 0.8f, 6000, .90f, 0,   0 },
    { "Plates",   "Long Plate",         Plate,    0.85f, .80f, .40f, 4.5f, 30,  1,  .95f, -8.f, 13000, 10000, 320, 0.9f, 5000, .80f, 0,   0 },
    { "Plates",   "Steel Plate",        Plate,    0.65f, .79f, .20f, 1.6f, 8,   1,  .95f, -11.f,18000, 16000, 250, 0.7f, 9000, 1.0f, 0,   0 },

    { "Ambience", "Subtle Air",         Ambience, 0.50f, .70f, .20f, 0.45f,0,   5,  .30f, -12.f,16000, 12000, 300, 0.8f, 6000, .80f, 0,   0 },
    { "Ambience", "Tight Ambience",     Ambience, 0.40f, .68f, .15f, 0.3f, 0,   9,  .25f, -12.f,14000, 11000, 300, 0.8f, 5000, .75f, 0,   0 },
    { "Ambience", "Wide Ambience",      Ambience, 0.70f, .70f, .30f, 0.6f, 4,   13, .35f, -10.f,15000, 12000, 300, 0.9f, 5500, .80f, 0,   0 },
    { "Ambience", "Small Space",        Ambience, 0.55f, .68f, .20f, 0.5f, 2,   17, .35f, -10.f,12000, 9000,  320, 0.9f, 4500, .70f, 0,   0 },
    { "Ambience", "Glue Ambience",      Ambience, 0.60f, .72f, .25f, 0.7f, 0,   21, .45f, -9.f, 11000, 8500,  350, 1.0f, 4000, .70f, 0,   0 },
    { "Ambience", "Front Wall",         Ambience, 0.45f, .62f, .10f, 0.4f, 6,   25, .20f, -12.f,13000, 10000, 300, 0.8f, 5000, .75f, 0,   0 },

    { "Spaces",   "Cathedral",          Space,    2.00f, .76f, .35f, 7.0f, 45,  29, .85f, -2.f, 9000,  6500,  500, 1.4f, 3000, .55f, 0,   0 },
    { "Spaces",   "Infinite Space",     Space,    1.90f, .78f, .60f, 18.f, 60,  33, .95f, -4.f, 12000, 9000,  400, 1.2f, 4000, .75f, 0,   0 },
    { "Spaces",   "Church",             Space,    1.70f, .75f, .30f, 4.5f, 35,  37, .80f, -3.f, 10000, 7500,  450, 1.3f, 3500, .60f, 0,   0 },
    { "Spaces",   "Cave",               Space,    1.50f, .70f, .20f, 5.0f, 25,  39, .75f, 0.f,  8000,  5000,  600, 1.5f, 2500, .50f, 0,   0 },
    { "Spaces",   "Warehouse",          Space,    1.40f, .66f, .25f, 3.0f, 20,  35, .70f, -4.f, 14000, 11000, 350, 1.1f, 5000, .80f, 0,   0 },
    { "Spaces",   "Tunnel",             Space,    1.20f, .62f, .15f, 3.5f, 15,  40, .70f, -2.f, 9000,  6000,  500, 1.3f, 3000, .60f, 0,   0 },
    { "Spaces",   "Stadium",            Space,    2.00f, .70f, .30f, 4.0f, 80,  32, .80f, -4.f, 10000, 8000,  400, 1.2f, 3500, .65f, 180, .25f },
    { "Spaces",   "Atrium",             Space,    1.60f, .74f, .40f, 3.2f, 30,  28, .80f, -6.f, 15000, 12000, 350, 1.0f, 6000, .85f, 0,   0 },

    { "Vintage",  "Dub Spring",         Spring,   0.50f, .60f, .20f, 2.4f, 0,   1,  .80f, -6.f, 6000,  4500,  300, 1.0f, 3000, .70f, 0,   0 },
    { "Vintage",  "Surf Spring",        Spring,   0.45f, .58f, .15f, 1.8f, 0,   1,  .75f, -8.f, 7000,  5500,  300, 1.0f, 3500, .75f, 0,   0 },
    { "Vintage",  "Tank Spring",        Spring,   0.55f, .62f, .20f, 3.0f, 0,   1,  .85f, -5.f, 5000,  4000,  350, 1.1f, 2600, .65f, 0,   0 },
    { "Vintage",  "80s Digital Hall",   Hall,     1.30f, .66f, .50f, 3.0f, 30,  36, .80f, -6.f, 10000, 8000,  400, 1.0f, 5000, 1.0f, 0,   0 },
    { "Vintage",  "Lo-Fi Plate",        Plate,    0.65f, .78f, .55f, 1.8f, 10,  1,  .95f, -6.f, 5000,  3500,  400, 1.0f, 2000, .60f, 0,   0 },
    { "Vintage",  "Gated Drums",        Gated,    0.90f, .75f, .10f, 0.45f,0,   38, .85f, -4.f, 14000, 11000, 300, 1.0f, 5000, 1.0f, 0,   0 },
}};

// Index of the n-th program of a category, or -1.
inline int programIndex(int category, int n) {
    int seen = 0;
    for (int i = 0; i < programCount; ++i)
        if (std::strcmp(programs[(size_t) i].category, categories[(size_t) category]) == 0 && seen++ == n) return i;
    return -1;
}
inline int categoryOf(int program) {
    for (int c = 0; c < (int) categories.size(); ++c)
        if (std::strcmp(programs[(size_t) program].category, categories[(size_t) c]) == 0) return c;
    return 0;
}
inline int programsIn(int category) { int n = 0; while (programIndex(category, n) >= 0) ++n; return n; }
inline int positionInCategory(int program) {
    int c = categoryOf(program), n = 0;
    for (int i = 0; i < program; ++i) if (categoryOf(i) == c) ++n;
    return n;
}
