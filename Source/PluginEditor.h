#pragma once
#include "PluginProcessor.h"

class ReverbLook : public juce::LookAndFeel_V4 {
public:
    ReverbLook();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
};

// Label above, knob, value below (Seventh Heaven style).
class Knob : public juce::Component {
public:
    Knob(ReverbProcessor&, const char* paramId, juce::String label, std::function<juce::String(float)> format, bool small);
    void place(juce::Rectangle<int> dial, juce::Rectangle<int> value);
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label;
    std::function<juce::String(float)> format;
    juce::Rectangle<int> readout;
    bool small;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

class HSlider : public juce::Component {
public:
    HSlider(ReverbProcessor&, const char* paramId, juce::String label, std::function<juce::String(float)> format);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label;
    std::function<juce::String(float)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

// Recessed display with ‹ › arrows; clicking the name opens a menu.
class PresetBox : public juce::Component {
public:
    std::function<void()> onPrev, onNext, onMenu;
    juce::String label;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
};

class Meters : public juce::Component {
public:
    std::array<float, 5> level { -100, -100, -100, -100, -100 }; // dB
    void paint(juce::Graphics&) override;
};

class IconButton : public juce::Component {
public:
    enum Kind { Save, Folder, Settings, Help };
    explicit IconButton(Kind k) : kind(k) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onClick;
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
private:
    Kind kind;
};

class TabLabel : public juce::Component {
public:
    juce::String name; bool active = false;
    std::function<void()> onClick;
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override { if (onClick) onClick(); }
};

// Master EQ curve with four draggable bands.
class EqDisplay : public juce::Component, private juce::AudioProcessorValueTreeState::Listener {
public:
    explicit EqDisplay(ReverbProcessor&);
    ~EqDisplay() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
private:
    ReverbProcessor& proc;
    int dragging = -1;
    float xOf(float hz) const;
    float hzOf(float x) const;
    float yOf(float db) const;
    float dbOf(float y) const;
    int bandAt(juce::Point<float>) const;
    void parameterChanged(const juce::String&, float) override { juce::MessageManager::callAsync([safe = juce::Component::SafePointer<EqDisplay>(this)] { if (safe) safe->repaint(); }); }
};

// Everything at the 1068 x 510 design size; the editor scales it.
class Panel : public juce::Component, private juce::Timer {
public:
    explicit Panel(ReverbProcessor&);
    ~Panel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void(float)> onScale;
    void filmTick() { timerCallback(); }
    static constexpr int W = 1068, H = 534;
private:
    ReverbProcessor& proc;
    ReverbLook look;

    std::vector<std::unique_ptr<Knob>> mainKnobs, advKnobs;
    std::unique_ptr<HSlider> vlf, earlyLate;
    PresetBox categoryBox, programBox;
    Meters meters;
    IconButton save { IconButton::Save }, folder { IconButton::Folder }, settings { IconButton::Settings }, help { IconButton::Help };
    TabLabel advTab, eqTab;
    EqDisplay eq;
    std::unique_ptr<juce::FileChooser> chooser;
    bool showEq = false;
    void timerCallback() override;
    void showTab(bool eqTab);
    void stepCategory(int);
    void stepProgram(int);
    void programMenu();
    void categoryMenu();
    void folderMenu();
    void savePreset();
    void settingsMenu();
    void refreshNames();

};

class ReverbEditor : public juce::AudioProcessorEditor {
public:
    explicit ReverbEditor(ReverbProcessor&);
    void resized() override;
private:
    Panel panel;
    float scale = 1;
    bool ready = false;
    std::unique_ptr<juce::PropertiesFile> props;
};
