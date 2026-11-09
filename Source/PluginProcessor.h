#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "ReverbEngine.h"

class ReverbProcessor : public juce::AudioProcessor {
public:
    ReverbProcessor();
    juce::AudioProcessorValueTreeState state;
    // Block peaks for the LED meters (linear gain, written by the audio thread).
    std::atomic<float> meterIn { 0 }, meterOut { 0 }, meterEarly { 0 }, meterLate { 0 }, meterVlf { 0 };

    const juce::String getName() const override { return "Swell Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 30; }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    int getNumPrograms() override { return programCount; }
    int getCurrentProgram() override { return (int) value("program"); }
    void setCurrentProgram(int i) override { loadProgram(i); }
    const juce::String getProgramName(int i) override { return programName(i); }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Loads a factory program: picks the space and writes its settings into the controls (Mix and Gain are kept).
    void loadProgram(int index);
    static juce::String programName(int i);
    float value(const char* id) const { return state.getRawParameterValue(id)->load(); }
    void setValue(const char* id, float v);
    swell::ReverbParams currentParams() const;

    // User presets live in Documents/Swell Audio/Swell Reverb Presets as .swellverb XML files.
    static juce::File userPresetFolder();
    bool saveUserPreset(const juce::File&);
    bool loadUserPreset(const juce::File&);
    juce::String userPresetName;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void render(juce::AudioBuffer<float>&, bool bypassed);
    swell::ReverbEngine engine;
    juce::AudioBuffer<float> wet;
    juce::SmoothedValue<float> dryGain, wetGain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbProcessor)
};
