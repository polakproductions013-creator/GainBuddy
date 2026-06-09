#include "PluginEditor.h"

GainBuddyAudioProcessorEditor::GainBuddyAudioProcessorEditor(GainBuddyAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p), lufsDisplay(p)
{
    setSize(380, 400);
    setResizable(false, false);

    addAndMakeVisible(lufsDisplay);

    for (auto* b : {&btn18, &btn24})
    {
        b->setLookAndFeel(&laf);
        addAndMakeVisible(b);
    }

    btn18.onClick = [this] { proc.snapToTarget(-18.0f); updateTargetButtons(); };
    btn24.onClick = [this] { proc.snapToTarget(-24.0f); updateTargetButtons(); };

    trimKnob.setLookAndFeel(&laf);
    trimKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    trimKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(trimKnob);
    trimAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "trim", trimKnob);

    auto setupLbl = [&](juce::Label& l, juce::Colour c, float sz) {
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, c);
        l.setColour(juce::Label::backgroundColourId, GB::lcdBg);
        l.setFont(juce::Font("Courier New", sz, juce::Font::bold));
        addAndMakeVisible(l);
    };
    setupLbl(trimValLabel,  GB::lcdOrange,             12.0f);
    setupLbl(autoGainLabel, juce::Colour(0xff888888),  11.0f);

    updateTargetButtons();
    startTimerHz(30);
}

GainBuddyAudioProcessorEditor::~GainBuddyAudioProcessorEditor()
{
    stopTimer();
    trimKnob.setLookAndFeel(nullptr);
    btn18.setLookAndFeel(nullptr);
    btn24.setLookAndFeel(nullptr);
}

void GainBuddyAudioProcessorEditor::updateTargetButtons()
{
    float t = proc.getCurrentTarget();
    bool is18 = (t > -19.0f);

    // Active button: orange with glow
    // Inactive button: permanently black
    btn18.setColour(juce::TextButton::buttonColourId,
                    is18 ? GB::orange : GB::btnBlack);
    btn24.setColour(juce::TextButton::buttonColourId,
                    !is18 ? GB::orange : GB::btnBlack);

    // Text: black on orange, dim grey on black
    btn18.setColour(juce::TextButton::textColourOffId,
                    is18 ? GB::black : GB::btnBlackText);
    btn24.setColour(juce::TextButton::textColourOffId,
                    !is18 ? GB::black : GB::btnBlackText);

    btn18.repaint();
    btn24.repaint();
}

void GainBuddyAudioProcessorEditor::timerCallback()
{
    float trim = proc.apvts.getRawParameterValue("trim")->load();
    trimValLabel.setText((trim >= 0 ? "+" : "") + juce::String(trim, 2) + " dB",
                         juce::dontSendNotification);
    float ag = proc.getAutoGainDB();
    autoGainLabel.setText("auto: " + (ag >= 0 ? juce::String("+") : juce::String(""))
                          + juce::String(ag, 1) + " dB",
                          juce::dontSendNotification);
    updateTargetButtons();
}

void GainBuddyAudioProcessorEditor::drawScrew(juce::Graphics& g, float x, float y)
{
    g.setColour(juce::Colour(0xff9a9690)); g.fillEllipse(x-6,y-6,12,12);
    g.setColour(juce::Colour(0xff555250)); g.drawEllipse(x-6,y-6,12,12,1.0f);
    g.setColour(juce::Colour(0xff333030));
    g.fillRect(x-3.0f,y-0.7f,6.0f,1.4f);
    g.fillRect(x-0.7f,y-3.0f,1.4f,6.0f);
}

void GainBuddyAudioProcessorEditor::paint(juce::Graphics& g)
{
    float w=(float)getWidth(), h=(float)getHeight();

    g.setColour(GB::bg); g.fillRoundedRectangle(0,0,w,h,10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.28f)); g.drawLine(1,1,w-1,1,1.5f);
    g.setColour(juce::Colour(0xff888480)); g.drawLine(1,h-1,w-1,h-1,1.5f);

    // Top bar
    g.setColour(GB::topBar); g.fillRoundedRectangle(0,0,w,62,10.0f);
    g.fillRect(0.0f,20.0f,w,42.0f);
    g.setColour(juce::Colour(0xffaaa8a0)); g.drawHorizontalLine(62,0,w);

    // Logo: GAIN BUDDY with one space
    g.setColour(GB::black);
    g.setFont(juce::Font("Arial Black", 26.0f, juce::Font::bold));
    g.drawText("GAIN", 18, 10, 76, 36, juce::Justification::centredLeft);
    g.setColour(GB::orange);
    g.drawText("BUDDY", 80, 10, 120, 36, juce::Justification::centredLeft);

    // Subtitle: Made by Polak
    g.setColour(juce::Colour(0xff666260));
    g.setFont(juce::Font("Arial Black", 10.0f, juce::Font::bold));
    g.drawText("MADE BY POLAK", 18, 42, 200, 16, juce::Justification::centredLeft);

    // Panel backgrounds
    auto drawPanel = [&](int px, int py, int pw, int ph) {
        g.setColour(GB::panel);
        g.fillRoundedRectangle((float)px,(float)py,(float)pw,(float)ph, 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawLine((float)px,(float)py+1,(float)(px+pw),(float)py+1, 0.8f);
        g.setColour(juce::Colour(0xff999590));
        g.drawRoundedRectangle((float)px,(float)py,(float)pw,(float)ph, 5.0f, 0.8f);
    };
    drawPanel(14, 68,  getWidth()-28, 70);
    drawPanel(14, 150, getWidth()-28, 216);

    // Section labels — same font as buttons
    g.setColour(juce::Colour(0xff555250));
    g.setFont(juce::Font("Arial Black", 11.0f, juce::Font::bold));
    g.drawText("TARGET", 20, 70,  200, 14, juce::Justification::centredLeft);
    g.drawText("LUFS",   20, 152, 200, 14, juce::Justification::centredLeft);
    g.drawText("TRIM",   224, 152, 80,  14, juce::Justification::centredLeft);
    g.setFont(juce::Font("Arial Black", 9.0f, juce::Font::bold));

    drawScrew(g,12,12); drawScrew(g,w-12,12);
    drawScrew(g,12,h-12); drawScrew(g,w-12,h-12);
}

void GainBuddyAudioProcessorEditor::resized()
{
    btn18.setBounds(20,  84, 158, 36);
    btn24.setBounds(186, 84, 158, 36);

    lufsDisplay.setBounds(20, 168, 182, 180);

    trimKnob.setBounds(216, 168, 120, 96);
    trimValLabel.setBounds(216, 304, 136, 22);
    autoGainLabel.setBounds(216, 328, 136, 20);
}
