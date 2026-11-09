#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
using Range = juce::NormalisableRange<float>;
Range skewed(float lo, float hi, float centre) { Range r(lo, hi); r.setSkewForCentre(centre); return r; }
void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name, Range r, float def, const char* unit = "") {
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel(unit)));
}
const char* eqF[4] { "eqLowFreq", "eqMid1Freq", "eqMid2Freq", "eqHighFreq" };
const char* eqG[4] { "eqLowGain", "eqMid1Gain", "eqMid2Gain", "eqHighGain" };
}

ReverbProcessor::ReverbProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "SwellReverb", layout()) {}

juce::String ReverbProcessor::programName(int i) {
    const auto& p = programs[(size_t) juce::jlimit(0, programCount - 1, i)];
    return juce::String(p.category) + " / " + p.name;
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    juce::StringArray names;
    for (int i = 0; i < programCount; ++i) names.add(programName(i));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "program", 1 }, "Program", names, 0));
    const auto& p = programs[0];
    addFloat(l, "decay", "Decay Time", skewed(.1f, 30, 2.5f), p.decay, "s");
    addFloat(l, "mix", "Mix", Range(0, 100, .1f), 35, "%");
    addFloat(l, "gain", "Gain", Range(-18, 18, .1f), 0, "dB");
    addFloat(l, "vlf", "Very Low Freq Reverb", Range(-24, 6, .1f), p.vlf, "dB");
    addFloat(l, "earlyLate", "Early / Late", Range(0, 1, .001f), p.earlyLate);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "pattern", 1 }, "Reflections Pattern", 1, 40, p.pattern));
    addFloat(l, "predelay", "Pre-Delay", skewed(0, 500, 60), p.predelay, "ms");
    addFloat(l, "delayTime", "Delay Time", skewed(0, 1000, 150), 100, "ms");
    addFloat(l, "delayLevel", "Delay Level", Range(0, 100, .1f), 0, "%");
    addFloat(l, "rollRef", "Roll-off Reflections", skewed(1000, 20000, 6000), p.rollRef, "Hz");
    addFloat(l, "rollRev", "Roll-off Reverb", skewed(1000, 20000, 6000), p.rollRev, "Hz");
    addFloat(l, "lowFreq", "Low Freq", skewed(50, 2000, 400), p.lowFreq, "Hz");
    addFloat(l, "lowMul", "Low Multiplier", skewed(.25f, 4, 1), p.lowMul, "x");
    addFloat(l, "highFreq", "High Freq", skewed(1000, 16000, 4500), p.highFreq, "Hz");
    addFloat(l, "highMul", "High Multiplier", skewed(.1f, 2, .7f), p.highMul, "x");
    const float eqDef[4] { 100, 400, 2500, 9000 };
    const Range eqRange[4] { skewed(20, 1000, 120), skewed(80, 5000, 500), skewed(300, 12000, 2500), skewed(1500, 20000, 8000) };
    for (int b = 0; b < 4; ++b) {
        addFloat(l, eqF[b], b == 0 ? "EQ Low Freq" : b == 1 ? "EQ Mid 1 Freq" : b == 2 ? "EQ Mid 2 Freq" : "EQ High Freq", eqRange[b], eqDef[b], "Hz");
        addFloat(l, eqG[b], b == 0 ? "EQ Low Gain" : b == 1 ? "EQ Mid 1 Gain" : b == 2 ? "EQ Mid 2 Gain" : "EQ High Gain", Range(-15, 15, .1f), 0, "dB");
    }
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

bool ReverbProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void ReverbProcessor::setValue(const char* id, float v) {
    auto* p = state.getParameter(id);
    p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(v)); p->endChangeGesture();
}

void ReverbProcessor::loadProgram(int index) {
    index = juce::jlimit(0, programCount - 1, index);
    const auto& p = programs[(size_t) index];
    setValue("program", (float) index);
    setValue("decay", p.decay); setValue("predelay", p.predelay); setValue("pattern", (float) p.pattern);
    setValue("earlyLate", p.earlyLate); setValue("vlf", p.vlf);
    setValue("rollRef", p.rollRef); setValue("rollRev", p.rollRev);
    setValue("lowFreq", p.lowFreq); setValue("lowMul", p.lowMul); setValue("highFreq", p.highFreq); setValue("highMul", p.highMul);
    setValue("delayTime", p.delayTime > 0 ? p.delayTime : 100); setValue("delayLevel", p.delayLevel * 100);
    userPresetName = {};
}

swell::ReverbParams ReverbProcessor::currentParams() const {
    swell::ReverbParams r;
    r.decay = value("decay"); r.predelayMs = value("predelay"); r.delayMs = value("delayTime"); r.delayLevel = value("delayLevel") / 100;
    r.rollRef = value("rollRef"); r.rollRev = value("rollRev");
    r.lowFreq = value("lowFreq"); r.lowMul = value("lowMul"); r.highFreq = value("highFreq"); r.highMul = value("highMul");
    r.vlfDb = value("vlf"); r.earlyLate = value("earlyLate"); r.gainDb = value("gain"); r.pattern = (int) value("pattern");
    for (int b = 0; b < 4; ++b) { r.eqFreq[(size_t) b] = value(eqF[b]); r.eqGain[(size_t) b] = value(eqG[b]); }
    return r;
}

void ReverbProcessor::prepareToPlay(double sr, int block) {
    engine.prepare(sr, (int) value("program"));
    engine.setParams(currentParams());
    wet.setSize(2, std::max(1, block));
    dryGain.reset(sr, .03); wetGain.reset(sr, .03);
    float m = value("mix") / 100;
    bool off = value("bypass") > .5f;
    dryGain.setCurrentAndTargetValue(off ? 1.f : std::min(1.f, 2 * (1 - m)));
    wetGain.setCurrentAndTargetValue(off ? 0.f : std::min(1.f, 2 * m));
}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) {
    if (! licence->isUnlocked()) { b.clear(); return; }
    render(b, false);
}
void ReverbProcessor::processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { render(b, true); }

void ReverbProcessor::render(juce::AudioBuffer<float>& b, bool bypassed) {
    juce::ScopedNoDenormals guard;
    const int n = b.getNumSamples(), inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if (n == 0) return;
    if (wet.getNumSamples() < n) wet.setSize(2, n, false, false, true);
    for (int c = inCh; c < outCh; ++c) b.copyFrom(c, 0, b, 0, 0, n); // mono in, stereo out
    const float* l = b.getReadPointer(0);
    const float* r = b.getReadPointer(outCh > 1 ? 1 : 0);
    float inPeak = std::max(b.getMagnitude(0, 0, n), outCh > 1 ? b.getMagnitude(1, 0, n) : 0.f);

    engine.requestProgram((int) value("program"));
    engine.setParams(currentParams());
    engine.process(l, r, wet.getWritePointer(0), wet.getWritePointer(1), n);

    const float m = value("mix") / 100;
    const bool off = bypassed || value("bypass") > .5f;
    dryGain.setTargetValue(off ? 1.f : std::min(1.f, 2 * (1 - m)));
    wetGain.setTargetValue(off ? 0.f : std::min(1.f, 2 * m));
    auto* wl = wet.getReadPointer(0);
    auto* wr = wet.getReadPointer(1);
    for (int i = 0; i < n; ++i) {
        float d = dryGain.getNextValue(), w = wetGain.getNextValue();
        if (outCh > 1) {
            b.setSample(0, i, b.getSample(0, i) * d + wl[i] * w);
            b.setSample(1, i, b.getSample(1, i) * d + wr[i] * w);
        } else
            b.setSample(0, i, b.getSample(0, i) * d + .5f * (wl[i] + wr[i]) * w);
    }
    auto hold = [](std::atomic<float>& a, float v) { float prior = a.load(); while (v > prior && !a.compare_exchange_weak(prior, v)) {} };
    hold(meterIn, inPeak);
    hold(meterOut, std::max(b.getMagnitude(0, 0, n), outCh > 1 ? b.getMagnitude(1, 0, n) : 0.f));
    hold(meterEarly, engine.peakEarly); hold(meterLate, engine.peakLate); hold(meterVlf, engine.peakVlf);
}

void ReverbProcessor::getStateInformation(juce::MemoryBlock& block) {
    auto t = state.copyState();
    t.setProperty("userPreset", userPresetName, nullptr);
    if (auto xml = t.createXml()) copyXmlToBinary(*xml, block);
}

void ReverbProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (xml && xml->hasTagName(state.state.getType())) {
        auto t = juce::ValueTree::fromXml(*xml);
        userPresetName = t.getProperty("userPreset", "").toString();
        state.replaceState(t);
    }
}

juce::File ReverbProcessor::userPresetFolder() {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Swell Audio").getChildFile("Swell Reverb Presets");
}

bool ReverbProcessor::saveUserPreset(const juce::File& f) {
    auto t = state.copyState();
    t.removeProperty("userPreset", nullptr);
    auto xml = t.createXml();
    if (!xml) return false;
    f.getParentDirectory().createDirectory();
    if (!xml->writeTo(f)) return false;
    userPresetName = f.getFileNameWithoutExtension();
    return true;
}

bool ReverbProcessor::loadUserPreset(const juce::File& f) {
    auto xml = juce::XmlDocument::parse(f);
    if (!xml || !xml->hasTagName(state.state.getType())) return false;
    auto t = juce::ValueTree::fromXml(*xml);
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            auto id = ranged->getParameterID();
            if (id == "mix" || id == "gain" || id == "bypass") continue; // a preset never changes the level you set
            for (auto child : t)
                if (child.getProperty("id").toString() == id) setValue(id.toRawUTF8(), (float) child.getProperty("value"));
        }
    userPresetName = f.getFileNameWithoutExtension();
    return true;
}

juce::AudioProcessorEditor* ReverbProcessor::createEditor() { return swell::LicenceGuard::attach(new ReverbEditor(*this)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ReverbProcessor(); }
