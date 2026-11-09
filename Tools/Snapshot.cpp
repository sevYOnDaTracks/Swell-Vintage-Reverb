#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
// Renders the editor to a PNG: ReverbSnapshot <out.png> [program] [eq]
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    ReverbProcessor p;
    if (argc > 2) p.loadProgram(juce::String(argv[2]).getIntValue());
    p.prepareToPlay(48000, 256);
    juce::MidiBuffer m;
    juce::AudioBuffer<float> b(2, 256);
    for (int i = 0; i < 60; ++i) {
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < 256; ++n) b.setSample(c, n, (i % 8 == 0 && n < 40) ? .8f : 0.f);
        p.processBlock(b, m);
    }
    std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
    if (argc > 3)
        if (auto* panel = e->getChildComponent(0))
            for (auto* c : panel->getChildren())
                if (auto* tab = dynamic_cast<TabLabel*>(c))
                    if (tab->name == "MASTER EQUALISER" && tab->onClick) tab->onClick();
    auto image = e->createComponentSnapshot(e->getLocalBounds(), true, 2.f);
    juce::File out(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshot.png"));
    out.deleteFile();
    juce::FileOutputStream s(out);
    juce::PNGImageFormat().writeImageToStream(image, s);
    return 0;
}
