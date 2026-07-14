#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mixdeck {

/** Story 4.0 — technical spike, not real plugin infrastructure (revisited in
    Story 4.3 once actual plugin editors exist). Proves that a JUCE Component
    can be attached as a native child view of Electron's own window
    (juce::Component::addToDesktop with a foreign nativeWindowToAttachTo),
    living inside the same process as Electron's own NSApplication run loop
    (see ScopedJuceInitialiser_GUI in NodeBinding.cpp). A trivial slider+label
    is used to prove real interactivity, not just a blank rectangle. */
class PluginWindowSpike {
public:
    PluginWindowSpike() = default; // needed: the deleted copy ctor below suppresses the implicit one

    // Attaches (or re-shows) the test panel as a native child view of
    // nativeParentHandle (an NSView* on macOS, as returned by Electron's
    // BrowserWindow.getNativeWindowHandle()). Fixed position/size — no
    // tracking of the host window's own resize (out of scope for this spike).
    void attachToNativeParent(void* nativeParentHandle);

    void hide();

private:
    struct Content : public juce::Component {
        Content();
        void paint(juce::Graphics& g) override;
        void resized() override;

        juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxBelow };
        juce::Label label;
    };

    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindowSpike)
};

} // namespace mixdeck
