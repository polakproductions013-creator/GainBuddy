#include "PluginProcessor.h"
#include "PluginEditor.h"

GainBuddyAudioProcessor::GainBuddyAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameters())
{}

GainBuddyAudioProcessor::~GainBuddyAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
GainBuddyAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        "trim", "Trim",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    return { p.begin(), p.end() };
}

static constexpr float LUFS_CORRECTION = 3.32f;

void GainBuddyAudioProcessor::setFilterCoeffs (KFilter& kL, KFilter& kR)
{
    auto preCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate_, 1681.0f, 0.7071f,
        juce::Decibels::decibelsToGain (4.0f));
    *kL.pre.coefficients = *preCoeffs;
    *kR.pre.coefficients = *preCoeffs;

    auto rlbCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (
        sampleRate_, 38.135f, 0.5003f);
    *kL.rlb.coefficients = *rlbCoeffs;
    *kR.rlb.coefficients = *rlbCoeffs;
}

void GainBuddyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_      = sampleRate;
    samplesPerBlock_ = samplesPerBlock;

    juce::dsp::ProcessSpec mono { sampleRate, (juce::uint32)samplesPerBlock, 1 };
    kInL.prepare(mono);  kInR.prepare(mono);
    kOutL.prepare(mono); kOutR.prepare(mono);
    kInL.reset(); kInR.reset(); kOutL.reset(); kOutR.reset();
    setFilterCoeffs (kInL, kInR);
    setFilterCoeffs (kOutL, kOutR);

    double blocksPerSec = sampleRate / (double)samplesPerBlock;
    int ringSize = (int)std::ceil (blocksPerSec * 3.0) + 2;
    inputRing.init  (ringSize);
    outputRing.init (ringSize);

    smoothedGainLin.reset (sampleRate, 0.15);
    smoothedGainLin.setCurrentAndTargetValue (1.0f);
}

void GainBuddyAudioProcessor::releaseResources()
{
    kInL.reset(); kInR.reset(); kOutL.reset(); kOutR.reset();
}

static float msToLUFS (double ms)
{
    if (ms < 1e-10) return -70.0f;
    return -0.691f + 10.0f * std::log10f ((float)ms);
}

void GainBuddyAudioProcessor::snapToTarget (float targetLUFS)
{
    currentTarget.store (targetLUFS);
    float measured = inputLUFS.load();
    if (measured <= -69.0f) return;

    float correctionDB = targetLUFS - measured;
    correctionDB = juce::jlimit (-40.0f, 40.0f, correctionDB);
    autoGainDB.store (correctionDB);

    float trimDB = apvts.getRawParameterValue ("trim")->load();
    smoothedGainLin.setTargetValue (
        juce::Decibels::decibelsToGain (correctionDB + trimDB));
}

void GainBuddyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* L = buffer.getWritePointer (0);
    auto* R = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    double inSumSq = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float kl = kInL.process (L[i]);
        float kr = R ? kInR.process (R[i]) : kl;
        double mono = (double)(kl*kl + kr*kr) * 0.5;
        if (std::isfinite (mono)) inSumSq += mono;
    }
    double inMS = inSumSq / (double)numSamples;
    if (std::isfinite (inMS)) {
        inputRing.push (inMS);
        float raw = msToLUFS (inputRing.mean());
        if (std::isfinite (raw))
            inputLUFS.store (raw + LUFS_CORRECTION);
    }

    float trimDB  = apvts.getRawParameterValue ("trim")->load();
    float totalDB = autoGainDB.load() + trimDB;
    smoothedGainLin.setTargetValue (juce::Decibels::decibelsToGain (totalDB));

    for (int i = 0; i < numSamples; ++i)
    {
        float g = smoothedGainLin.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer(ch)[i] *= g;
    }

    double outSumSq = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float kl = kOutL.process (L[i]);
        float kr = R ? kOutR.process (R[i]) : kl;
        double mono = (double)(kl*kl + kr*kr) * 0.5;
        if (std::isfinite (mono)) outSumSq += mono;
    }
    double outMS = outSumSq / (double)numSamples;
    if (std::isfinite (outMS)) {
        outputRing.push (outMS);
        float raw = msToLUFS (outputRing.mean());
        if (std::isfinite (raw))
            displayLUFS.store (raw + LUFS_CORRECTION);
    }
}

juce::AudioProcessorEditor* GainBuddyAudioProcessor::createEditor()
{
    return new GainBuddyAudioProcessorEditor (*this);
}

void GainBuddyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GainBuddyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainBuddyAudioProcessor();
}
