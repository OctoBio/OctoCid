cmake_minimum_required(VERSION 3.22)

project(OctoCid VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Runtime MSVC statique : le .vst3 ne dépend pas des redistribuables VC++
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------------------
# JUCE (même version que OctoSYNTH)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(OctoCid
    COMPANY_NAME                "OctoBio"
    COMPANY_WEBSITE             "https://github.com/OctoBio"
    BUNDLE_ID                   "com.octobio.octocid"
    PLUGIN_MANUFACTURER_CODE    Octb        # à aligner sur celui d'OctoSYNTH
    PLUGIN_CODE                 OCid        # unique par plugin
    PRODUCT_NAME                "OctoCid"
    DESCRIPTION                 "Acid bass synthesizer"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    FORMATS                     VST3 Standalone
)

juce_generate_juce_header(OctoCid)

target_sources(OctoCid PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/SequencerView.cpp
    Source/ui/SequencerView.h
    Source/ui/Palette.h
    Source/ui/OctoLookAndFeel.h
    Source/ui/Widgets.h
    Source/ui/Mascot.h
    Source/ui/Snail.h
    Source/Params.h
    Source/Presets.h
    Source/dsp/DspUtils.h
    Source/dsp/Filters.h
    Source/dsp/Oscillator.h
    Source/dsp/DiodeLadder.h
    Source/dsp/NoteStack.h
    Source/dsp/AcidVoice.h
    Source/dsp/Drive.h
    Source/dsp/Lfo.h
    Source/dsp/Fx.h
    Source/dsp/FxPlus.h
    Source/seq/Pattern.h
    Source/seq/PatternIO.h
    Source/seq/Sequencer.h
)

target_include_directories(OctoCid PRIVATE Source)

target_compile_definitions(OctoCid PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

juce_add_binary_data(OctoCidAssets SOURCES
    Assets/portail_violet.jpg
    Assets/portail_violet_blur.jpg
    Assets/portail_violet_soft.jpg
    Assets/logo_glitch.png
    Assets/acide.jpg
    Assets/acide_soft.jpg
    Assets/acide_blur.jpg
    Assets/orage.jpg
    Assets/orage_soft.jpg
    Assets/orage_blur.jpg
    Assets/acide_orage.jpg
    Assets/acide_orage_soft.jpg
    Assets/acide_orage_blur.jpg
    Assets/portail_vert.jpg
    Assets/portail_vert_soft.jpg
    Assets/portail_vert_blur.jpg
    Assets/portail_feu.jpg
    Assets/portail_feu_soft.jpg
    Assets/portail_feu_blur.jpg
    Assets/portail_bleu.jpg
    Assets/portail_bleu_soft.jpg
    Assets/portail_bleu_blur.jpg
    Assets/portail_vert_eclairs_violets.jpg
    Assets/portail_vert_eclairs_violets_soft.jpg
    Assets/portail_vert_eclairs_violets_blur.jpg)

target_link_libraries(OctoCid
    PRIVATE
        OctoCidAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
