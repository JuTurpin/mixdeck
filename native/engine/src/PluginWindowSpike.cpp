#include "PluginWindowSpike.h"

namespace mixdeck {

namespace {
constexpr int panelWidth = 220;
constexpr int panelHeight = 160;
} // namespace

PluginWindowSpike::Content::Content() {
    setSize(panelWidth, panelHeight);

    label.setText("Plugin window spike (4.0)", juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(label);

    slider.setRange(0.0, 1.0);
    slider.setValue(0.5, juce::dontSendNotification);
    addAndMakeVisible(slider);
}

void PluginWindowSpike::Content::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2a2d35));
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawRect(getLocalBounds(), 1);
}

void PluginWindowSpike::Content::resized() {
    auto area = getLocalBounds().reduced(10);
    label.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    slider.setBounds(area.removeFromTop(40));
}

void PluginWindowSpike::attachToNativeParent(void* nativeParentHandle) {
    // Fixed offset from the parent view's origin — no tracking of the host
    // window's own size/resize (out of scope for this spike, see 4.3).
    content.setTopLeftPosition(24, 24);
    content.addToDesktop(0, nativeParentHandle);
    content.setVisible(true);
    content.toFront(false);
}

void PluginWindowSpike::hide() {
    content.setVisible(false);
}

} // namespace mixdeck
