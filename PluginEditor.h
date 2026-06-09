cmake_minimum_required(VERSION 3.22)
project(GainBuddy VERSION 3.0.0)

add_subdirectory(JUCE)

juce_add_plugin(GainBuddy
    VERSION                     3.0.0
    COMPANY_NAME                "YourName"
    PLUGIN_MANUFACTURER_CODE    Yrnm
    PLUGIN_CODE                 Gnbd
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "GainBuddy"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

juce_generate_juce_header(GainBuddy)

target_sources(GainBuddy PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
)

target_compile_definitions(GainBuddy PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GainBuddy PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_plugin_client
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
