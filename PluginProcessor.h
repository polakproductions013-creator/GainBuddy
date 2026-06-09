#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace GB {
    const juce::Colour bg        { 0xffd4d0c8 };
    const juce::Colour panel     { 0xffbfbbb3 };
    const juce::Colour topBar    { 0xffc8c4bc };
    const juce::Colour orange    { 0xffe8570a };
    const juce::Colour black     { 0xff1a1a1a };
    const juce::Colour btnBlack  { 0xff1a1a1a };  // inactive button
    const juce::Colour btnBlackText { 0xff555555 };
    const juce::Colour knobBody  { 0xff222222 };
    const juce::Colour lcdBg     { 0xff111111 };
    const juce::Colour lcdOrange { 0xffe8570a };
    const juce::Colour greenGlow { 0xff2ecc40 };
    const juce::Colour red       { 0xffcc2020 };
}

class NineONineLAF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override
    {
        float cx=x+w*0.5f, cy=y+h*0.5f, radius=std::min(w,h)*0.42f;
        float angle=startAngle+pos*(endAngle-startAngle);

        g.setColour(GB::orange.withAlpha(0.2f));
        juce::Path t; t.addArc(cx-radius-5,cy-radius-5,(radius+5)*2,(radius+5)*2,startAngle,endAngle,true);
        g.strokePath(t,juce::PathStrokeType(3.0f));

        g.setColour(GB::orange);
        juce::Path a; a.addArc(cx-radius-5,cy-radius-5,(radius+5)*2,(radius+5)*2,startAngle,angle,true);
        g.strokePath(a,juce::PathStrokeType(3.0f));

        g.setColour(GB::knobBody);
        g.fillEllipse(cx-radius,cy-radius,radius*2,radius*2);
        g.setColour(juce::Colour(0xff555555));
        g.drawEllipse(cx-radius,cy-radius,radius*2,radius*2,1.5f);

        float da=angle-juce::MathConstants<float>::halfPi;
        g.setColour(GB::orange);
        g.fillEllipse(cx+radius*0.62f*std::cos(da)-3.5f,
                      cy+radius*0.62f*std::sin(da)-3.5f,7.0f,7.0f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                               const juce::Colour& bgCol, bool, bool isDown) override
    {
        auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(bgCol);
        g.fillRoundedRectangle(b, 4.0f);

        // top highlight
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.fillRoundedRectangle(b.withHeight(b.getHeight()*0.35f), 4.0f);

        // bottom shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(b.getX(), b.getBottom()-4, b.getWidth(), 4.0f);

        // orange glow when active (orange bg)
        if (bgCol == GB::orange)
        {
            g.setColour(GB::orange.withAlpha(0.6f));
            g.drawRoundedRectangle(b.expanded(2.0f), 5.0f, 3.0f);
            g.drawRoundedRectangle(b.expanded(4.0f), 6.0f, 2.0f);
            g.drawRoundedRectangle(b.expanded(6.0f), 7.0f, 1.0f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& btn, bool, bool) override
    {
        g.setColour(btn.findColour(juce::TextButton::textColourOffId));
        g.setFont(juce::Font("Arial Black", 12.0f, juce::Font::bold));
        g.drawText(btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred);
    }
};

class LUFSDisplay : public juce::Component, public juce::Timer
{
public:
    LUFSDisplay(GainBuddyAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(GB::lcdBg);
        g.fillRoundedRectangle(b, 6.0f);

        float lufs   = proc.getDisplayLUFS();
        float target = proc.getCurrentTarget();
        float diff   = lufs - target;
        float absDiff = std::abs(diff);

        juce::Colour col, borderCol{0xff333333};
        if (absDiff < 2.0f) {
            float t = juce::jmap(absDiff, 2.0f, 0.0f, 0.0f, 1.0f);
            col       = GB::lcdOrange.interpolatedWith(GB::greenGlow, t);
            borderCol = juce::Colour(0xff1a5a1a).interpolatedWith(juce::Colour(0xff2ecc40), t*0.6f);
        } else if (diff > 2.0f) {
            col = GB::red; borderCol = juce::Colour(0xff5a1a1a);
        } else {
            col = juce::Colour(0xff777777);
        }

        g.setColour(borderCol);
        g.drawRoundedRectangle(b.reduced(0.5f), 6.0f, 2.0f);

        // Big number — whole number only
        juce::String numStr = (lufs < -69.0f) ? "---"
                            : juce::String((int)std::round(lufs));
        g.setColour(col);
        g.setFont(juce::Font("Courier New", 52.0f, juce::Font::bold));
        g.drawText(numStr,
                   b.reduced(8,4).withBottom(b.getBottom()-46).toNearestInt(),
                   juce::Justification::centred);

        // Status text — below number, larger than before
        juce::String status;
        if (lufs < -69.0f)       status = "NO SIGNAL";
        else if (absDiff < 0.5f) status = "ON TARGET";
        else if (absDiff < 2.0f) status = "SWEET SPOT";
        else if (diff > 2.0f)    status = "TOO LOUD";
        else                     status = "TOO QUIET";

        g.setColour(col);
        g.setFont(juce::Font("Courier New", 13.0f, juce::Font::bold));
        g.drawText(status,
                   b.reduced(8,0).withTop(b.getBottom()-46).withBottom(b.getBottom()-24).toNearestInt(),
                   juce::Justification::centred);

        // Unit label
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Courier New", 9.0f, juce::Font::plain));
        g.drawText("SHORT-TERM OUTPUT",
                   b.reduced(8,4).withTop(b.getBottom()-22).toNearestInt(),
                   juce::Justification::centred);
    }
private:
    GainBuddyAudioProcessor& proc;
};

class GainBuddyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    GainBuddyAudioProcessorEditor(GainBuddyAudioProcessor&);
    ~GainBuddyAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    GainBuddyAudioProcessor& proc;
    NineONineLAF laf;
    LUFSDisplay  lufsDisplay;

    juce::TextButton btn18{"-18 LUFS"}, btn24{"-24 LUFS"};

    juce::Slider trimKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trimAttach;
    juce::Label  trimValLabel, autoGainLabel;

    void updateTargetButtons();
    void drawScrew(juce::Graphics& g, float x, float y);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainBuddyAudioProcessorEditor)
};
