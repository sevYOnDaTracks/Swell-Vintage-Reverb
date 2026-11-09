#include "../Source/PluginProcessor.h"
#include <iostream>
#include <stdexcept>

void require(bool ok, const std::string& msg) { if (!ok) throw std::runtime_error(msg); }
void set(ReverbProcessor& p, const char* id, float v) { auto* a = p.state.getParameter(id); a->setValueNotifyingHost(a->convertTo0to1(v)); }
constexpr int SR = 48000, BLOCK = 256;

// Renders 'seconds' of stereo audio (same signal on both inputs), returns the left output.
template <class F> std::vector<float> run(ReverbProcessor& p, double seconds, F in, bool prepare = true) {
    if (prepare) p.prepareToPlay(SR, BLOCK);
    juce::MidiBuffer m;
    juce::AudioBuffer<float> b(2, BLOCK);
    std::vector<float> out;
    for (int start = 0; start < (int) (SR * seconds); start += BLOCK) {
        for (int n = 0; n < BLOCK; ++n) { float x = in(start + n); b.setSample(0, n, x); b.setSample(1, n, x); }
        p.processBlock(b, m);
        for (int n = 0; n < BLOCK; ++n) {
            require(std::isfinite(b.getSample(0, n)) && std::isfinite(b.getSample(1, n)), "non-finite output");
            out.push_back(b.getSample(0, n));
        }
    }
    return out;
}
float energy(const std::vector<float>& v, double from, double to) {
    double e = 0;
    for (int i = (int) (from * SR); i < (int) (to * SR) && i < (int) v.size(); ++i) e += v[(size_t) i] * v[(size_t) i];
    return (float) e;
}
// RT60 from the Schroeder backward integral, fitted between -5 and -25 dB (T20).
float rt60(const std::vector<float>& ir) {
    std::vector<double> edc(ir.size());
    double acc = 0;
    for (size_t i = ir.size(); i-- > 0;) { acc += (double) ir[i] * ir[i]; edc[i] = acc; }
    double t5 = -1, t25 = -1;
    for (size_t i = 0; i < edc.size(); ++i) {
        double d = 10 * std::log10(edc[i] / edc[0] + 1e-30);
        if (t5 < 0 && d < -5) t5 = (double) i / SR;
        if (t25 < 0 && d < -25) { t25 = (double) i / SR; break; }
    }
    require(t5 >= 0 && t25 > t5, "decay too long to measure");
    return (float) (3 * (t25 - t5));
}
std::vector<float> impulse(ReverbProcessor& p, double seconds) {
    set(p, "mix", 100); set(p, "predelay", 0);
    return run(p, seconds, [](int n) { return n == 0 ? 1.f : 0.f; });
}
const char* eqGains[4] { "eqLowGain", "eqMid1Gain", "eqMid2Gain", "eqHighGain" };

int main() {
    swell::Licence::unlockForTesting(); // rendu hors ligne, sans fichier de licence
    try {
        juce::ScopedJuceInitialiser_GUI gui;
        // Every program: bounded, finite, and its tail dies away.
        for (int i = 0; i < programCount; ++i) {
            ReverbProcessor p; p.loadProgram(i); set(p, "mix", 100);
            float d = p.value("decay");
            auto out = run(p, std::min(14.0, 1.0 + d * 1.6), [](int n) { return n < 2400 ? (float) std::sin(n * .07) * .5f : 0.f; });
            float peak = 0;
            for (float v : out) peak = std::max(peak, std::abs(v));
            require(peak < 3, std::string("program too loud: ") + programs[(size_t) i].name + " " + std::to_string(peak));
            double len = (double) out.size() / SR;
            require(energy(out, len - .3, len) < energy(out, 0, .5) * .05f, std::string("tail does not decay: ") + programs[(size_t) i].name);
        }
        // Worst case settings stay stable for 10 s of noise.
        for (int i : { 0, 22, 36, 44, 49 }) {
            ReverbProcessor p; p.loadProgram(i); set(p, "mix", 100); set(p, "decay", 30); set(p, "lowMul", 4); set(p, "highMul", 2); set(p, "vlf", 6);
            for (auto* id : eqGains) set(p, id, 15);
            juce::Random r(1);
            auto out = run(p, 10, [&](int n) { return n < SR * 2 ? (r.nextFloat() - .5f) * .5f : 0.f; });
            float late = 0;
            for (int n = SR * 9; n < SR * 10; ++n) late = std::max(late, std::abs(out[(size_t) n]));
            require(late < 20, std::string("unstable at maximum decay: ") + programs[(size_t) i].name);
        }
        // The Decay Time knob sets the measured RT60.
        {
            ReverbProcessor p; p.loadProgram(1); set(p, "earlyLate", 1); set(p, "lowMul", 1); set(p, "highMul", 1);
            set(p, "decay", 1); float r1 = rt60(impulse(p, 4));
            set(p, "decay", 3); float r3 = rt60(impulse(p, 10));
            std::cout << "RT60 measured: decay 1 s -> " << r1 << " s, decay 3 s -> " << r3 << " s\n";
            require(r1 > .6f && r1 < 1.6f, "decay 1 s gives wrong RT60");
            require(r3 > 2.f && r3 < 4.2f, "decay 3 s gives wrong RT60");
        }
        // High multiplier darkens the tail.
        {
            auto hfShare = [](float mul) {
                ReverbProcessor p; p.loadProgram(1); set(p, "highMul", mul); set(p, "rollRev", 20000); set(p, "earlyLate", 1);
                auto ir = impulse(p, 3);
                float hp = 0, all = 0, prev = 0;
                for (int n = SR; n < 3 * SR; ++n) { float d = ir[(size_t) n] - prev; prev = ir[(size_t) n]; hp += d * d; all += ir[(size_t) n] * ir[(size_t) n]; }
                return hp / all;
            };
            require(hfShare(.3f) < hfShare(1.5f) * .7f, "high multiplier has no effect");
        }
        // Mix 0 and bypass are bit-transparent.
        for (const char* mode : { "mix", "bypass" }) {
            ReverbProcessor p;
            if (juce::String(mode) == "mix") set(p, "mix", 0); else set(p, "bypass", 1);
            auto out = run(p, .5, [](int n) { return (float) std::sin(n * .01) * .5f; });
            for (int n = 0; n < (int) out.size(); ++n) require(out[(size_t) n] == (float) std::sin(n * .01) * .5f, std::string(mode) + " is not transparent");
        }
        // Pre-delay holds the reverb back.
        {
            ReverbProcessor p; p.loadProgram(1); set(p, "mix", 100); set(p, "predelay", 200);
            auto out = run(p, 1, [](int n) { return n == 0 ? 1.f : 0.f; });
            require(energy(out, 0, .19) < 1e-9f && energy(out, .2, .6) > 1e-4f, "pre-delay not applied");
        }
        // Early only: nothing after the reflections.
        {
            ReverbProcessor p; p.loadProgram(1); set(p, "mix", 100); set(p, "predelay", 0); set(p, "earlyLate", 0);
            auto out = run(p, 1.5, [](int n) { return n == 0 ? 1.f : 0.f; });
            require(energy(out, .3, 1.5) < 1e-8f, "early-only still has a tail");
        }
        // The gated program closes after its time.
        {
            ReverbProcessor p; p.loadProgram(49); set(p, "mix", 100);
            auto out = run(p, 2, [](int n) { return n < 4800 ? (float) std::sin(n * .05) * .8f : 0.f; });
            require(energy(out, .2, .5) > 1e-3f && energy(out, .9, 2) < energy(out, .2, .5) * 1e-3f, "gate does not close");
        }
        // Programs, state and user presets.
        {
            ReverbProcessor p; p.loadProgram(36); set(p, "mix", 42);
            require(std::abs(p.value("decay") - programs[36].decay) < .05f, "program does not load its decay");
            juce::MemoryBlock st; p.getStateInformation(st);
            ReverbProcessor q; q.setStateInformation(st.getData(), (int) st.getSize());
            require((int) q.value("program") == 36 && std::abs(q.value("mix") - 42) < .2f, "state not restored");
            auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("swell-reverb-test.swellverb");
            set(p, "decay", 5.5f);
            require(p.saveUserPreset(f), "user preset not saved");
            ReverbProcessor r; set(r, "mix", 80);
            require(r.loadUserPreset(f), "user preset not loaded");
            require(std::abs(r.value("decay") - 5.5f) < .05f && (int) r.value("program") == 36 && std::abs(r.value("mix") - 80) < .2f, "user preset values wrong");
            f.deleteFile();
        }
        // Switching program while audio plays never jumps.
        {
            ReverbProcessor p; p.loadProgram(0); set(p, "mix", 100);
            juce::Random r(2);
            run(p, 1, [&](int) { return (r.nextFloat() - .5f) * .3f; });
            p.loadProgram(22);
            auto out = run(p, .5, [&](int) { return (r.nextFloat() - .5f) * .3f; }, false);
            float jump = 0;
            for (size_t n = 1; n < out.size(); ++n) jump = std::max(jump, std::abs(out[n] - out[n - 1]));
            require(jump < .5f, "click on program change");
        }
        std::cout << "PASS: 50 programs bounded and decaying, max settings stable, RT60 follows Decay, high multiplier, mix 0 and bypass transparent, pre-delay, early only, gate, state, user presets, program switch\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
