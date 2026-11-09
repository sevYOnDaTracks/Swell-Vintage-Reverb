#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "Presets.h"

namespace swell {
constexpr float pi = 3.14159265358979f;

// Topology-preserving one-pole low-pass (stable at any cutoff).
struct OnePole {
    float s = 0, G = 0;
    void set(float hz, float sr) { float g = std::tan(pi * std::min(hz, sr * .45f) / sr); G = g / (1 + g); }
    float lp(float x) { float v = (x - s) * G, y = v + s; s = y + v; return y; }
    void reset() { s = 0; }
};

// Power-of-two circular buffer. Call push() first, then ago(d) returns the sample pushed d samples earlier.
struct Line {
    std::vector<float> b; int w = 0, mask = 0;
    void init(int maxLen) { int n = 1; while (n < maxLen + 8) n <<= 1; b.assign((size_t) n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill(b.begin(), b.end(), 0.f); w = 0; }
    void push(float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    float ago(int d) const { return b[(size_t) ((w - 1 - d) & mask)]; }
    float agof(float d) const { int i = (int) d; float f = d - (float) i, a = ago(i), c = ago(i + 1); return a + (c - a) * f; }
};

// Schroeder all-pass diffuser.
struct Allpass {
    Line l; int len = 1; float g = .6f;
    float process(float x) { float d = l.ago(len - 1), v = x - g * d; l.push(v); return d + g * v; }
};

// First-order all-pass, chained to get the dispersive "chirp" of a spring tank.
struct AP1 {
    float a = .6f, x1 = 0, y1 = 0;
    float process(float x) { float y = a * x + x1 - a * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

// RBJ biquad, also used by the editor to draw the EQ curve.
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; float z1 = 0, z2 = 0;
    float process(float x) { float y = (float) b0 * x + z1; z1 = (float) b1 * x - (float) a1 * y + z2; z2 = (float) b2 * x - (float) a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set(double n0, double n1, double n2, double d0, double d1, double d2) { b0 = n0 / d0; b1 = n1 / d0; b2 = n2 / d0; a1 = d1 / d0; a2 = d2 / d0; }
    void peak(double f, double q, double db, double sr) {
        double A = std::pow(10., db / 40), w = 2 * 3.14159265358979 * f / sr, cs = std::cos(w), al = std::sin(w) / (2 * q);
        set(1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
    void shelf(bool high, double f, double db, double sr) {
        double A = std::pow(10., db / 40), w = 2 * 3.14159265358979 * f / sr, cs = std::cos(w), al = std::sin(w) / 2 * std::sqrt(2.), r = 2 * std::sqrt(A) * al;
        if (high) set(A * ((A + 1) + (A - 1) * cs + r), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - r), (A + 1) - (A - 1) * cs + r, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - r);
        else      set(A * ((A + 1) - (A - 1) * cs + r), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - r), (A + 1) + (A - 1) * cs + r, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - r);
    }
    double magnitudeDb(double f, double sr) const {
        double w = 2 * 3.14159265358979 * f / sr, c1 = std::cos(w), s1 = std::sin(w), c2 = std::cos(2 * w), s2 = std::sin(2 * w);
        double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2), dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        return 10 * std::log10((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-30);
    }
};

struct ReverbParams {
    float decay = 2.6f, predelayMs = 24, delayMs = 0, delayLevel = 0;
    float rollRef = 12000, rollRev = 9000, lowFreq = 450, lowMul = 1.2f, highFreq = 4200, highMul = .7f;
    float vlfDb = -6, earlyLate = .75f, gainDb = 0;
    int pattern = 15;
    std::array<float, 4> eqFreq { 100, 400, 2500, 9000 }, eqGain { 0, 0, 0, 0 };
};

// The four master EQ bands: low shelf, two bells, high shelf.
inline void designEq(std::array<Biquad, 4>& eq, const ReverbParams& p, double sr) {
    eq[0].shelf(false, p.eqFreq[0], p.eqGain[0], sr);
    eq[1].peak(p.eqFreq[1], .8, p.eqGain[1], sr);
    eq[2].peak(p.eqFreq[2], .8, p.eqGain[2], sr);
    eq[3].shelf(true, p.eqFreq[3], p.eqGain[3], sr);
}

// Stereo algorithmic reverb: pre-delay, early reflection patterns, a diffused 8-line feedback delay
// network with three-band decay (low / mid / high RT), a separately levelled very-low-frequency tail,
// roll-off filters, a delay tap feeding the tail and a master EQ. Returns the wet signal only.
class ReverbEngine {
public:
    static constexpr int lines = 8;
    float peakEarly = 0, peakLate = 0, peakVlf = 0;

    void prepare(double sampleRate, int programIndex) {
        sr = (float) sampleRate;
        program = juce::jlimit(0, programCount - 1, programIndex); pending = -1; fade = 1;
        inL.init((int) (sr * 1.8f)); inR.init((int) (sr * 1.8f));
        for (auto& l : fdn) l.init((int) (sr * .16f));
        for (auto* d : { &difL, &difR }) for (auto& a : *d) a.l.init((int) (sr * .014f));
        springL.init((int) (sr * .08f)); springR.init((int) (sr * .08f));
        applyProgram(program);
    }
    // Switch space with a short fade so the tail never clicks.
    void requestProgram(int index) { if (index != program && index >= 0 && index < programCount) pending = index; }
    int currentProgram() const { return program; }

    void setParams(const ReverbParams& p) {
        bool rebuild = p.pattern != params.pattern;
        params = p;
        if (rebuild) buildTaps();
        updateGains();
    }

    void process(const float* xl, const float* xr, float* outL, float* outR, int n) {
        const auto& pr = programs[(size_t) program];
        const int pre = juce::jlimit(0, (int) (sr * .5f), (int) (params.predelayMs * .001f * sr));
        const int dly = juce::jlimit(1, (int) (sr * 1.f), (int) (params.delayMs * .001f * sr));
        const float dlyLvl = params.delayLevel, eG = std::min(1.f, 2 * (1 - params.earlyLate)), lG = std::min(1.f, 2 * params.earlyLate);
        const float gain = juce::Decibels::decibelsToGain(params.gainDb), vlfG = juce::Decibels::decibelsToGain(params.vlfDb, -60.f);
        const float fadeStep = 1.f / (.02f * sr);
        float pe = 0, pl = 0, pv = 0;
        for (int i = 0; i < n; ++i) {
            inL.push(xl[i]); inR.push(xr[i]);
            // Early reflections read straight from the input history, after the pre-delay.
            float eL = 0, eR = 0;
            for (auto& t : tapsL) eL += t.g * (t.cross ? inR : inL).ago(pre + t.d);
            for (auto& t : tapsR) eR += t.g * (t.cross ? inL : inR).ago(pre + t.d);
            float echoL = 0, echoR = 0;
            if (dlyLvl > 0) { echoL = dlyLvl * inL.ago(dly); echoR = dlyLvl * inR.ago(dly); }
            float dL = inL.ago(pre) + .3f * eL + echoL, dR = inR.ago(pre) + .3f * eR + echoR;
            for (auto& a : difL) dL = a.process(dL);
            for (auto& a : difR) dR = a.process(dR);

            // Feedback delay network with Householder mixing and per-line three-band damping.
            float outs[lines], filt[lines], sum = 0, lateL = 0, lateR = 0;
            for (int k = 0; k < lines; ++k) {
                lfo[k] += lfoInc[k]; if (lfo[k] > 2 * pi) lfo[k] -= 2 * pi;
                outs[k] = fdn[k].agof(len[k] + modDepth * (1 + std::sin(lfo[k])));
                float y = outs[k], lo = lowLP[k].lp(y);
                y = lowRatio[k] * lo + (y - lo);
                float hi = highLP[k].lp(y);
                y = (hi + highRatio[k] * (y - hi)) * gainMid[k];
                filt[k] = y; sum += y;
                lateL += outs[k] * signL[k]; lateR += outs[k] * signR[k];
            }
            for (int k = 0; k < lines; ++k) fdn[k].push(filt[k] - .25f * sum + (k & 1 ? dR : dL) * signIn[k] * .35f);
            lateL *= .3f; lateR *= .3f;

            if (pr.type == Spring) {
                float sL = inL.ago(pre) + springFb * springL.ago(springLenL), sR = inR.ago(pre) + springFb * springR.ago(springLenR);
                for (auto& a : chainL) sL = a.process(sL);
                for (auto& a : chainR) sR = a.process(sR);
                springL.push(sL); springR.push(sR);
                lateL += .55f * sL; lateR += .55f * sR;
            }
            if (pr.type == Gated) {
                float x = std::max(std::abs(xl[i]), std::abs(xr[i]));
                env = x > env ? x : env * envRelease;
                if (env > .03f) hold = params.decay * sr;
                else if (hold > 0) hold -= 1;
                float target = hold > 0 ? 1.f : 0.f;
                gate += (target - gate) * (target > gate ? gateAttack : gateRelease);
                lateL *= gate; lateR *= gate;
            }
            // Very low frequency tail gets its own level, then the reverb roll-off.
            float loL = vlfL.lp(lateL), loR = vlfR.lp(lateR);
            lateL += (vlfG - 1) * loL; lateR += (vlfG - 1) * loR;
            pv = std::max(pv, std::max(std::abs(vlfG * loL), std::abs(vlfG * loR)));
            lateL = revL2.lp(revL1.lp(lateL)); lateR = revR2.lp(revR1.lp(lateR));
            eL = refL2.lp(refL1.lp(eL)); eR = refR2.lp(refR1.lp(eR));
            pe = std::max(pe, std::max(std::abs(eL), std::abs(eR)) * eG);
            pl = std::max(pl, std::max(std::abs(lateL), std::abs(lateR)) * lG);

            float wl = eG * eL + lG * lateL + .5f * echoL, wr = eG * eR + lG * lateR + .5f * echoR;
            for (auto& b : eqL) wl = b.process(wl);
            for (auto& b : eqR) wr = b.process(wr);

            if (pending >= 0) { fade -= fadeStep; if (fade <= 0) { fade = 0; applyProgram(pending); pending = -1; } }
            else if (fade < 1) fade = std::min(1.f, fade + fadeStep);
            outL[i] = wl * gain * fade; outR[i] = wr * gain * fade;
        }
        peakEarly = pe; peakLate = pl; peakVlf = pv;
    }

    void reset() { applyProgram(program); }

private:
    struct Tap { int d = 1; float g = 0; bool cross = false; };
    float sr = 48000;
    int program = 0, pending = -1;
    float fade = 1;
    ReverbParams params;
    Line inL, inR, springL, springR;
    std::array<Line, lines> fdn;
    std::array<Allpass, 4> difL, difR;
    std::array<AP1, 40> chainL, chainR;
    std::array<Tap, 16> tapsL, tapsR;
    std::array<float, lines> len {}, gainMid {}, lowRatio {}, highRatio {}, lfo {}, lfoInc {};
    std::array<OnePole, lines> lowLP, highLP;
    OnePole vlfL, vlfR, revL1, revL2, revR1, revR2, refL1, refL2, refR1, refR2;
    std::array<Biquad, 4> eqL, eqR;
    float modDepth = 0, springFb = 0, env = 0, hold = 0, gate = 0, envRelease = .999f, gateAttack = .02f, gateRelease = .002f;
    int springLenL = 1, springLenR = 1;
    static constexpr float signL[lines] { 1, -1, 1, -1, 1, -1, 1, -1 }, signR[lines] { 1, 1, -1, -1, 1, 1, -1, -1 }, signIn[lines] { 1, 1, -1, 1, 1, -1, -1, -1 };

    void applyProgram(int index) {
        program = index;
        const auto& p = programs[(size_t) index];
        static constexpr float baseMs[lines] { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f };
        for (int k = 0; k < lines; ++k) {
            len[k] = std::max(3.f, baseMs[k] * p.size) * .001f * sr;
            lfoInc[k] = 2 * pi * (.21f + .13f * (float) k) * (.6f + .8f * p.modulation) / sr;
            lfo[k] = (float) k * .77f;
            fdn[k].clear(); lowLP[k].reset(); highLP[k].reset();
        }
        modDepth = p.modulation * .0011f * sr;
        static constexpr float difMs[4] { 1.31f, 2.17f, 3.83f, 5.71f };
        for (int k = 0; k < 4; ++k) {
            difL[(size_t) k].len = std::max(2, (int) (difMs[k] * (.6f + .4f * p.size) * .001f * sr));
            difR[(size_t) k].len = std::max(2, (int) (difMs[k] * 1.13f * (.6f + .4f * p.size) * .001f * sr));
            difL[(size_t) k].g = difR[(size_t) k].g = p.diffusion;
            difL[(size_t) k].l.clear(); difR[(size_t) k].l.clear();
        }
        springLenL = (int) (.037f * sr); springLenR = (int) (.043f * sr);
        for (auto& a : chainL) { a.a = .62f; a.reset(); }
        for (auto& a : chainR) { a.a = .60f; a.reset(); }
        inL.clear(); inR.clear(); springL.clear(); springR.clear();
        for (auto* f : { &vlfL, &vlfR, &revL1, &revL2, &revR1, &revR2, &refL1, &refL2, &refR1, &refR2 }) f->reset();
        for (auto& b : eqL) b.reset();
        for (auto& b : eqR) b.reset();
        env = hold = gate = 0;
        envRelease = std::exp(-1.f / (.05f * sr)); gateAttack = 1 - std::exp(-1.f / (.001f * sr)); gateRelease = 1 - std::exp(-1.f / (.025f * sr));
        buildTaps();
        updateGains();
    }

    // Deterministic early reflection set: four families (natural, clustered, slap, lateral) across 40 patterns.
    void buildTaps() {
        const auto& p = programs[(size_t) program];
        juce::Random r(params.pattern * 7919 + 13);
        const float span = (.008f + .055f * p.size) * sr;
        const int style = (params.pattern - 1) % 4;
        for (int side = 0; side < 2; ++side) {
            auto& taps = side ? tapsR : tapsL;
            float energy = 0;
            for (int k = 0; k < (int) taps.size(); ++k) {
                float u = ((float) k + r.nextFloat()) / (float) taps.size();
                float t = style == 0 ? std::pow(u, 1.4f) : style == 1 ? .7f * std::pow(u, 2.2f) : style == 2 ? u : std::pow(u, 1.1f);
                auto& tap = taps[(size_t) k];
                tap.d = 1 + (int) (t * span) + (side ? r.nextInt((int) (.002f * sr) + 1) : 0);
                tap.g = std::exp(-2.2f * t) * (.55f + .45f * r.nextFloat()) * (r.nextBool() ? 1.f : -1.f);
                if (style == 2 && k % 3) tap.g *= .25f;
                tap.cross = style == 3 ? k % 2 == 0 : r.nextFloat() < .3f;
                energy += tap.g * tap.g;
            }
            float scale = .8f / std::sqrt(energy + 1e-9f);
            for (auto& t : taps) t.g *= scale;
        }
    }

    void updateGains() {
        const auto& p = programs[(size_t) program];
        float rt = juce::jlimit(.05f, 60.f, params.decay);
        if (p.type == Gated) rt = 1.2f + .6f * p.size; // dense tail, the gate sets the length
        const float rtLow = juce::jlimit(.05f, 60.f, rt * params.lowMul), rtHigh = juce::jlimit(.05f, 60.f, rt * params.highMul);
        for (int k = 0; k < lines; ++k) {
            float s = len[k] / sr;
            float gm = std::pow(10.f, -3 * s / rt), gl = std::pow(10.f, -3 * s / rtLow), gh = std::pow(10.f, -3 * s / rtHigh);
            gainMid[k] = std::min(gm, .9995f);
            lowRatio[k] = std::min(gl, .9995f) / gainMid[k];
            highRatio[k] = std::min(gh, .9995f) / gainMid[k];
            // Both shelves boosting at once could push a band above unity: keep the loop strictly lossy.
            float bound = gainMid[k] * std::max(1.f, lowRatio[k]) * std::max(1.f, highRatio[k]);
            if (bound > .9995f) gainMid[k] *= .9995f / bound;
            lowLP[k].set(params.lowFreq, sr); highLP[k].set(params.highFreq, sr);
        }
        springFb = std::pow(10.f, -3 * (.04f) / juce::jlimit(.2f, 20.f, params.decay));
        vlfL.set(120, sr); vlfR.set(120, sr);
        for (auto* f : { &revL1, &revL2, &revR1, &revR2 }) f->set(params.rollRev, sr);
        for (auto* f : { &refL1, &refL2, &refR1, &refR2 }) f->set(params.rollRef, sr);
        designEq(eqL, params, sr); designEq(eqR, params, sr);
    }
};
} // namespace swell
