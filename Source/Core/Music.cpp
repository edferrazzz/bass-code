#include "Music.h"

#include <algorithm>
#include <random>

namespace bass {

const std::vector<ScaleDef>& scales() {
    static const std::vector<ScaleDef> s {
        { "MENOR",            { 0, 2, 3, 5, 7, 8, 10 } },
        { "MAIOR",            { 0, 2, 4, 5, 7, 9, 11 } },
        { "FRYGIO",           { 0, 1, 3, 5, 7, 8, 10 } },
        { "DORICO",           { 0, 2, 3, 5, 7, 9, 10 } },
        { "MENOR HARMONICA",  { 0, 2, 3, 5, 7, 8, 11 } },
    };
    return s;
}

// Sound indices refer to the preset table in Dsp/Synth.cpp (presets()).
const StyleDef& style(int s) {
    static const std::array<StyleDef, numStyles> defs {{
        { "PSY",            145, { "ROLLING 1/16", "GALLOP", "OCTAVE ROLL" }, { 0, 1, 2, 3, 4, 5 },      0, 0.62f, 0.10f, 0.35f, false, false },
        { "HOUSE",          124, { "OFFBEAT", "CLASSIC", "WALKING" },         { 6, 7, 8, 9, 10, 11 },    0, 0.70f, 0.18f, 0.55f, false, false },
        { "AFRO HOUSE",     122, { "SYNCOPATED", "LOG DRUM", "DEEP" },        { 12, 13, 14, 15, 16, 17 }, 0, 0.66f, 0.22f, 0.45f, false, true },
        { "BRAZILIAN BASS", 125, { "BOUNCE", "WOBBLE", "DROP" },              { 18, 19, 20, 21, 22, 23 }, 0, 0.78f, 0.12f, 0.70f, false, false },
        { "ELETRONICA",     128, { "DRIVE", "STAB", "SLIDE" },                { 24, 25, 26, 27, 28, 29 }, 0, 0.58f, 0.15f, 0.50f, true,  false },
    }};
    return defs[(size_t) std::clamp(s, 0, numStyles - 1)];
}

const char* noteName(int pc) {
    static const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[((pc % 12) + 12) % 12];
}

std::string noteLabel(int pitch) {
    return std::string(noteName(pitch)) + std::to_string(pitch / 12 - 1);
}

namespace {

enum Role { root, octave, fifth, third, approach, seventh };

struct Hit { int step; int len; Role role; float vel; bool slide = false; };

using Rng = std::mt19937;

int pick(Rng& rng, int n) { return (int) (rng() % (uint32_t) n); }
float chance(Rng& rng) { return (float) (rng() % 10000) / 10000.0f; }

// Chord-root progressions in scale degrees (0 = tonic), one entry per bar of a 4-bar phrase.
const std::vector<std::array<int, 4>>& progressions(int s) {
    static const std::vector<std::array<int, 4>> table[numStyles] {
        { { 0, 0, 0, 0 }, { 0, 0, 0, 5 }, { 0, 0, 5, 6 }, { 0, 0, 0, 6 } },            // psy: stays home
        { { 0, 5, 2, 6 }, { 0, 3, 4, 0 }, { 0, 6, 5, 6 }, { 0, 0, 3, 4 } },            // house
        { { 0, 6, 5, 6 }, { 0, 3, 0, 4 }, { 0, 0, 5, 6 }, { 0, 2, 3, 4 } },            // afro house
        { { 0, 5, 6, 0 }, { 0, 5, 2, 6 }, { 0, 0, 5, 6 }, { 0, 3, 5, 4 } },            // brazilian bass
        { { 0, 6, 5, 4 }, { 0, 5, 6, 0 }, { 0, 0, 5, 5 }, { 0, 3, 5, 6 } },            // electronic
    };
    return table[std::clamp(s, 0, numStyles - 1)];
}

// One bar of rhythm for a style + groove.
std::vector<Hit> grooveBar(int s, int groove, int bar, Rng& rng) {
    std::vector<Hit> h;
    switch (s) {
    case psy:
        for (int b = 0; b < 4; ++b) {
            const int o = b * 4;
            if (groove == 0) {          // K B B B
                h.push_back({ o + 1, 1, root, 0.78f });
                h.push_back({ o + 2, 1, root, 0.95f });
                h.push_back({ o + 3, 1, root, 0.84f });
            } else if (groove == 1) {   // K - B B
                h.push_back({ o + 2, 1, root, 0.95f });
                h.push_back({ o + 3, 1, root, 0.82f });
            } else {                    // K B B O
                h.push_back({ o + 1, 1, root, 0.8f });
                h.push_back({ o + 2, 1, root, 0.95f });
                h.push_back({ o + 3, 1, octave, 0.78f });
            }
        }
        break;
    case house:
        if (groove == 0) {
            for (int b = 0; b < 4; ++b) h.push_back({ b * 4 + 2, 2, root, b % 2 ? 0.95f : 0.85f });
        } else if (groove == 1) {
            h = { { 2, 1, root, 0.9f }, { 3, 1, octave, 0.7f }, { 6, 2, root, 0.95f },
                  { 10, 1, root, 0.85f }, { 11, 1, fifth, 0.75f }, { 14, 2, root, 0.95f } };
        } else {
            h = { { 2, 2, root, 0.9f }, { 6, 2, octave, 0.85f }, { 10, 2, fifth, 0.9f }, { 14, 2, octave, 0.85f } };
            if (bar % 2) h[3].role = approach;
        }
        break;
    case afroHouse:
        if (groove == 0) {
            h = { { 2, 2, root, 0.9f }, { 6, 1, fifth, 0.75f }, { 7, 2, root, 0.92f },
                  { 10, 2, root, 0.88f }, { 13, 1, octave, 0.72f }, { 14, 2, root, 0.9f } };
        } else if (groove == 1) {   // log drum: short, talking
            h = { { 3, 1, root, 0.95f }, { 6, 1, root, 0.8f }, { 8, 1, fifth, 0.85f },
                  { 11, 1, root, 0.95f }, { 14, 1, third, 0.8f } };
            if (pick(rng, 2)) h.push_back({ 15, 1, root, 0.7f });
        } else {
            h = { { 2, 5, root, 0.9f }, { 10, 3, root, 0.85f }, { 14, 2, fifth, 0.8f } };
        }
        break;
    case brazilianBass:
        if (groove == 0) {
            h = { { 0, 3, root, 0.95f }, { 3, 1, root, 0.8f }, { 6, 2, octave, 0.88f },
                  { 8, 3, root, 0.95f }, { 11, 1, root, 0.8f }, { 14, 2, fifth, 0.85f } };
        } else if (groove == 1) {
            h = { { 0, 6, root, 0.95f }, { 6, 2, octave, 0.85f, true }, { 8, 8, root, 0.92f, true } };
        } else {
            h = { { 0, 1, root, 1.0f }, { 3, 1, root, 0.85f }, { 6, 1, octave, 0.9f },
                  { 10, 1, root, 0.9f }, { 12, 1, root, 0.95f }, { 14, 2, fifth, 0.85f } };
        }
        break;
    default: // electronic
        if (groove == 0) {
            for (int i = 0; i < 8; ++i) h.push_back({ i * 2, 1, root, i % 2 ? 0.95f : 0.7f });
        } else if (groove == 1) {
            h = { { 2, 1, root, 0.95f }, { 7, 1, octave, 0.85f }, { 10, 1, root, 0.95f }, { 15, 1, fifth, 0.8f } };
        } else {    // 303-ish slide line
            for (int st = 0; st < 16; ++st) {
                if (st % 4 == 0 && chance(rng) < 0.5f) continue;
                if (chance(rng) < 0.22f) continue;
                const float r = chance(rng);
                Role role = r < 0.55f ? root : r < 0.75f ? octave : r < 0.9f ? fifth : seventh;
                h.push_back({ st, 1, role, st % 4 == 2 ? 0.95f : 0.78f, chance(rng) < 0.25f });
            }
        }
        break;
    }
    return h;
}

} // namespace

Pattern generate(const GenSettings& gs, const Pattern* keep) {
    Rng rng(gs.seed * 2654435761u + 12345u);
    Pattern p;
    p.bars = std::clamp(gs.bars, 1, 16);
    const auto& sd = style(gs.style);

    if (keep != nullptr) {
        p.key = keep->key;
        p.scale = keep->scale;
    } else {
        p.key = gs.key >= 0 ? gs.key % 12 : pick(rng, 12);
        p.scale = gs.scale >= 0 ? gs.scale : sd.defaultScale;
    }
    const auto& sc = scales()[(size_t) std::clamp(p.scale, 0, (int) scales().size() - 1)].steps;

    // tonic between E1 (28) and D#2 (39): deep but still audible on small speakers with the harmonics
    int tonic = 24 + p.key;
    if (tonic < 28) tonic += 12;

    // bar roots (scale degrees)
    std::vector<int> degrees((size_t) p.bars);
    if (keep != nullptr && (int) keep->barRoots.size() == p.bars) {
        for (int b = 0; b < p.bars; ++b) {
            int best = 0;
            for (int d = 0; d < 7; ++d) if ((p.key + sc[(size_t) d]) % 12 == keep->barRoots[(size_t) b]) best = d;
            degrees[(size_t) b] = best;
        }
    } else {
        const auto& progs = progressions(gs.style);
        const auto prog = progs[(size_t) pick(rng, (int) progs.size())];
        for (int b = 0; b < p.bars; ++b) degrees[(size_t) b] = p.bars == 2 ? (b == 0 ? 0 : prog[3]) : prog[(size_t) (b % 4)];
        // 8 bars: second phrase can answer the first differently at the end
        if (p.bars >= 8 && pick(rng, 2)) degrees[7] = prog[3] == 0 ? 6 : 0;
    }

    auto degreePitch = [&](int deg) {
        const int oct = deg / 7;
        int d = ((deg % 7) + 7) % 7;
        int v = tonic + sc[(size_t) d] + 12 * (deg < 0 ? oct - 1 : oct);
        if (v > tonic + 7) v -= 12;   // keep chord roots close to the tonic, below if needed
        return v;
    };

    for (int b = 0; b < p.bars; ++b) {
        const int deg = degrees[(size_t) b];
        const int r = degreePitch(deg);
        const int nextDeg = degrees[(size_t) ((b + 1) % p.bars)];
        const int nextRoot = degreePitch(nextDeg);
        const int thirdInterval = (sc[(size_t) ((deg + 2) % 7)] - sc[(size_t) (deg % 7)] + 12) % 12;
        p.barRoots.push_back(r % 12);
        p.barMinor.push_back(thirdInterval == 3);

        auto hits = grooveBar(gs.style, gs.groove, b, rng);

        // density
        if (gs.density == 0) {
            std::vector<Hit> kept;
            for (size_t i = 0; i < hits.size(); ++i)
                if (i == 0 || chance(rng) > 0.33f) kept.push_back(hits[i]);
            hits = kept;
        } else if (gs.density == 2 && gs.style != psy) {
            for (int k = 0; k < 3; ++k) {
                const int st = pick(rng, 16);
                if (st % 4 == 0) continue;
                bool used = false;
                for (auto& h : hits) if (st >= h.step && st < h.step + h.len) used = true;
                if (!used) hits.push_back({ st, 1, pick(rng, 2) ? octave : fifth, 0.72f });
            }
        }

        // turnaround at the end of each phrase (and sometimes every 2 bars)
        const bool phraseEnd = (b % 4 == 3) || (b == p.bars - 1);
        const bool halfEnd = (b % 2 == 1) && gs.density == 2;
        if ((phraseEnd || halfEnd) && !hits.empty()) {
            std::sort(hits.begin(), hits.end(), [](auto& a, auto& c) { return a.step < c.step; });
            const int n = std::min<int>((int) hits.size(), gs.style == psy ? 3 : 2);
            const int kind = pick(rng, 3);
            for (int i = 0; i < n; ++i) {
                auto& h = hits[hits.size() - 1 - (size_t) i];
                if (kind == 0) h.role = i == 0 ? approach : octave;
                else if (kind == 1) h.role = i == 0 ? approach : fifth;
                else h.role = i % 2 ? octave : approach;
            }
        }

        for (auto& h : hits) {
            int pitch = r;
            switch (h.role) {
                case root: break;
                case octave: pitch = r + 12; break;
                case fifth: pitch = degreePitch(deg + 4); if (pitch < r) pitch += 12; break;
                case third: pitch = degreePitch(deg + 2); if (pitch < r) pitch += 12; break;
                case seventh: pitch = degreePitch(deg + 6); if (pitch < r) pitch += 12; break;
                case approach: {
                    // a scale step next to where the line goes next
                    const int up = degreePitch(nextDeg + 1), down = degreePitch(nextDeg - 1);
                    pitch = pick(rng, 2) ? up : down;
                    while (pitch < nextRoot - 7) pitch += 12;
                    while (pitch > nextRoot + 7) pitch -= 12;
                    break;
                }
            }
            p.notes.push_back({ b * 16 + h.step, h.len, std::clamp(pitch, 24, 60), h.vel, h.slide });
        }
    }
    std::sort(p.notes.begin(), p.notes.end(), [](auto& a, auto& c) { return a.start < c.start; });
    return p;
}

} // namespace bass
