cmake_minimum_required(VERSION 3.22)

project(DawIt VERSION 0.1.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "14.0")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version" FORCE)
    endif()
endif()

include(cmake/DawItBuiltIns.cmake)

# --- JUCE -------------------------------------------------------------------
# Uses a local checkout in external/JUCE when present, otherwise downloads it.
set(DAWIT_JUCE_VERSION "8.0.9")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${DAWIT_JUCE_VERSION}
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    # JUCE checks the SDK version using this legacy macro. Apple's headers cap
    # it at the deployment target, so use the actual SDK version for this module.
    set_property(SOURCE "${JUCE_MODULES_DIR}/juce_gui_basics/juce_gui_basics.mm" APPEND PROPERTY
        COMPILE_DEFINITIONS "MAC_OS_X_VERSION_MAX_ALLOWED=__MAC_OS_X_VERSION_MAX_ALLOWED")
    set_property(SOURCE "${JUCE_MODULES_DIR}/juce_gui_basics/juce_gui_basics.mm" APPEND PROPERTY
        COMPILE_OPTIONS "-include" "Availability.h")
endif()

# --- Stem separation: demucs.cpp (Demucs v4 inference, MIT) on Eigen (MPL 2) ----
# Both are downloaded at pinned versions. Only the inference library is built; the model
# weights are downloaded by the app on first use (see src/engine/Stems.cpp).
include(FetchContent)
FetchContent_Declare(demucscpp
    URL      https://github.com/sevagh/demucs.cpp/archive/f1206e9adeea103aef4a636b9e62297cf1f8e34e.tar.gz
    URL_HASH SHA256=f08bcacb49f99fdedac8affe13d671ec82cf3639c096b92c9c3da08c4b952934
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR none)   # not its own CMakeLists: that also builds the CLI apps and tests
FetchContent_Declare(eigen
    URL      https://gitlab.com/libeigen/eigen/-/archive/dd8c71e62852b2fe429edb6682ac91fd1c578a26/eigen-dd8c71e62852b2fe429edb6682ac91fd1c578a26.tar.gz
    URL_HASH SHA256=e8cbc003b1fa5e0d0d142c58155aaf341f87783d213d31a605a19b2f13d27acc
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR none)   # header-only
FetchContent_MakeAvailable(demucscpp eigen)

file(GLOB DEMUCS_SOURCES "${demucscpp_SOURCE_DIR}/src/*.cpp")
add_library(demucs STATIC ${DEMUCS_SOURCES})
target_include_directories(demucs SYSTEM PUBLIC "${demucscpp_SOURCE_DIR}/src" "${eigen_SOURCE_DIR}")
set_target_properties(demucs PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE ON)
# Always optimised: unoptimised Eigen makes a separation take hours.
if(MSVC)
    target_compile_options(demucs PRIVATE /O2 /w /bigobj)
else()
    target_compile_options(demucs PRIVATE -O3 -w)
endif()
target_compile_definitions(demucs PUBLIC EIGEN_MPL2_ONLY PRIVATE NDEBUG)

if(APPLE)
    # Matrix products through Accelerate, Apple's BLAS.
    target_compile_definitions(demucs PUBLIC EIGEN_USE_BLAS)
    target_link_libraries(demucs PUBLIC "-framework Accelerate")
endif()

# --- Audio to MIDI: Basic Pitch (Spotify, Apache 2.0) on ONNX Runtime (MIT) --------
# Development builds use Microsoft's package; release builds provide a source-built runtime.
set(DAWIT_ORT_VERSION "1.30.0")
set(DAWIT_ORT_SOURCE_BUILD "" CACHE PATH "Directory containing the ONNX source and build directories")

if(APPLE)
    set(ORT_PACKAGE "onnxruntime-osx-arm64-${DAWIT_ORT_VERSION}.tgz")
    set(ORT_SHA256 6ebb5062a934537c352937821f9fe9718e7de1a2db1122a93dd363ffd53a7012)
elseif(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
    set(ORT_PACKAGE "onnxruntime-win-arm64-${DAWIT_ORT_VERSION}.zip")
    set(ORT_SHA256 e53db8a50b23ae35be901cc93428baf997dc8d420333b097b2eae53d3ea9f2d3)
elseif(WIN32)
    set(ORT_PACKAGE "onnxruntime-win-x64-${DAWIT_ORT_VERSION}.zip")
    set(ORT_SHA256 c6ba983baf5681af108599675d2a89c2d145512d02de28aed0bff177cd0ba949)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ORT_PACKAGE "onnxruntime-linux-aarch64-${DAWIT_ORT_VERSION}.tgz")
    set(ORT_SHA256 e16a27a8ed330bbc698df7330b0cf56e722f354e3bcc92118682c74ef3c3e3da)
else()
    set(ORT_PACKAGE "onnxruntime-linux-x64-${DAWIT_ORT_VERSION}.tgz")
    set(ORT_SHA256 a5ed5a3cac51fbb2e90da632ae43d19212faaa20e76484e62bcb7c23ddb3b3fd)
endif()

if(NOT DAWIT_ORT_SOURCE_BUILD)
    FetchContent_Declare(onnxruntime
        URL      https://github.com/microsoft/onnxruntime/releases/download/v${DAWIT_ORT_VERSION}/${ORT_PACKAGE}
        URL_HASH SHA256=${ORT_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR none)
    FetchContent_MakeAvailable(onnxruntime)
endif()

add_library(onnxruntime SHARED IMPORTED)
if(DAWIT_ORT_SOURCE_BUILD)
    set(ORT_INCLUDE "${DAWIT_ORT_SOURCE_BUILD}/source/include/onnxruntime/core/session")
    set(ORT_LIB_DIR "${DAWIT_ORT_SOURCE_BUILD}/build/Release")
else()
    set(ORT_INCLUDE "${onnxruntime_SOURCE_DIR}/include")
    set(ORT_LIB_DIR "${onnxruntime_SOURCE_DIR}/lib")
endif()
target_include_directories(onnxruntime SYSTEM INTERFACE "${ORT_INCLUDE}")

if(APPLE)
    set(ORT_LIBRARY "${ORT_LIB_DIR}/libonnxruntime.${DAWIT_ORT_VERSION}.dylib")
    set(ORT_RUNTIME_NAME "libonnxruntime.1.dylib")   # its install name: @rpath/libonnxruntime.1.dylib
    set_target_properties(onnxruntime PROPERTIES IMPORTED_LOCATION "${ORT_LIBRARY}")
elseif(WIN32)
    set(ORT_LIBRARY "${ORT_LIB_DIR}/onnxruntime.dll")
    set(ORT_RUNTIME_NAME "onnxruntime.dll")
    set_target_properties(onnxruntime PROPERTIES IMPORTED_LOCATION "${ORT_LIBRARY}"
                                                 IMPORTED_IMPLIB "${ORT_LIB_DIR}/onnxruntime.lib")
else()
    set(ORT_LIBRARY "${ORT_LIB_DIR}/libonnxruntime.so.${DAWIT_ORT_VERSION}")
    set(ORT_RUNTIME_NAME "libonnxruntime.so.1")      # its soname
    set_target_properties(onnxruntime PROPERTIES IMPORTED_LOCATION "${ORT_LIBRARY}")
endif()
if(NOT EXISTS "${ORT_INCLUDE}/onnxruntime_cxx_api.h" OR NOT EXISTS "${ORT_LIBRARY}")
    message(FATAL_ERROR "ONNX Runtime headers or library are missing; build the selected runtime first.")
endif()
if(WIN32 AND NOT EXISTS "${ORT_LIB_DIR}/onnxruntime.lib")
    message(FATAL_ERROR "ONNX Runtime import library is missing.")
endif()

# The Basic Pitch model (230 KB) is built into the app.
set(BASIC_PITCH_MODEL "${CMAKE_BINARY_DIR}/basic-pitch/nmp.onnx")
file(DOWNLOAD
    https://raw.githubusercontent.com/spotify/basic-pitch/v0.4.0/basic_pitch/saved_models/icassp_2022/nmp.onnx
    "${BASIC_PITCH_MODEL}"
    EXPECTED_HASH SHA256=2c3c1d144bfa61ad236e92e169c13535c880469a12a047d4e73451f2c059a0ec)
juce_add_binary_data(DawItModels NAMESPACE dawmodels HEADER_NAME DawItModels.h SOURCES "${BASIC_PITCH_MODEL}")
set_target_properties(DawItModels PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The CMU Pronouncing Dictionary (Carnegie Mellon University; use "for any research or commercial purpose is
# completely unrestricted", with its origin acknowledged) is built into the app (3.6 MB): Sing this looks up how
# each word sounds in it. Pinned to a commit of cmusphinx/cmudict.
set(CMUDICT_COMMIT 74790861f652b15e4ac49015a90074ad62a27690)
set(CMUDICT_FILE "${CMAKE_BINARY_DIR}/cmudict/cmudict.dict")
file(DOWNLOAD
    https://raw.githubusercontent.com/cmusphinx/cmudict/${CMUDICT_COMMIT}/cmudict.dict
    "${CMUDICT_FILE}"
    EXPECTED_HASH SHA256=81917843c7f44ce2b094ac63873c2c7a4cf802040792c455ba3ca406891c3d22)
juce_add_binary_data(DawItDictionary NAMESPACE dawdict HEADER_NAME DawItDictionary.h SOURCES "${CMUDICT_FILE}")
set_target_properties(DawItDictionary PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Lyrics from audio: whisper.cpp (MIT) with OpenAI's Whisper models (MIT) ---------------
# Built as static libraries at a pinned release, on Metal and Accelerate on macOS. The English
# models are downloaded by the app on first use (see src/analysis/LyricsFinder.cpp).
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(WHISPER_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(GGML_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(GGML_NATIVE OFF CACHE BOOL "" FORCE)    # runs on any Mac of the architecture, not only this one
set(GGML_OPENMP OFF CACHE BOOL "" FORCE)    # ggml's own threads; no OpenMP runtime to ship
set(GGML_CCACHE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(whispercpp
    URL      https://github.com/ggml-org/whisper.cpp/archive/refs/tags/v1.9.5.tar.gz
    URL_HASH SHA256=ff1a9053feb509ff9d7729703355541ae9690073a6b1c40eb692c962e0dc1720
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.10)    # its CMakeLists asks for CMake 3.5, which CMake 4 warns about
FetchContent_MakeAvailable(whispercpp)
unset(CMAKE_POLICY_VERSION_MINIMUM)

# --- Sampled instruments: TinySoundFont (MIT, SoundFont player) and stb_vorbis (MIT, for .sf3) -----
# Two single-file libraries, pinned by hash and compiled together in src/engine/SoundFontCore.cpp with
# their warnings silenced there. Everything else reaches them through soundfonts:: and BuiltInSampler.
FetchContent_Declare(tinysoundfont
    URL      https://github.com/schellingb/TinySoundFont/archive/853a0a171759f1ddba0de1442133a75912bbeffa.tar.gz   # v0.9
    URL_HASH SHA256=76c356df524f71f2a34f67fa0cb2cb946036fe7a65d2930a849569c5e742fa57
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR none)   # no CMake project: only tsf.h is used
FetchContent_Declare(stbvorbis
    URL      https://github.com/nothings/stb/archive/1ee679ca2ef753a528db5ba6801e1067b40481b8.tar.gz   # stb_vorbis.c v1.22
    URL_HASH SHA256=be9842d207353c89c0b970d618030fe98a944ab3fe1374534cba1b8e934bfdd7
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR none)   # no CMake project: only stb_vorbis.c is used
FetchContent_MakeAvailable(tinysoundfont stbvorbis)

add_library(soundfontcore STATIC src/engine/SoundFontCore.cpp)
target_include_directories(soundfontcore SYSTEM PUBLIC "${tinysoundfont_SOURCE_DIR}")
target_include_directories(soundfontcore SYSTEM PRIVATE "${stbvorbis_SOURCE_DIR}")
target_compile_definitions(soundfontcore PRIVATE STB_VORBIS_NO_STDIO STB_VORBIS_NO_PUSHDATA_API TSF_NO_STDIO)
set_target_properties(soundfontcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(soundfontcore PRIVATE /w)
else()
    target_compile_options(soundfontcore PRIVATE -w)
endif()

# The built-in chord playing styles (see src/generate/ChordStyles.h).
juce_add_binary_data(DawItStyles NAMESPACE dawstyles HEADER_NAME DawItStyles.h SOURCES src/generate/ChordStyles.json)
set_target_properties(DawItStyles PROPERTIES POSITION_INDEPENDENT_CODE ON)

# SOURCES.md, embedded: the last topic of every guide shows its lists (see src/generate/GuideSources.h).
juce_add_binary_data(DawItSources NAMESPACE dawsources HEADER_NAME DawItSources.h SOURCES SOURCES.md)
set_target_properties(DawItSources PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LICENSE and every file under licenses/, embedded so the About window reads them offline (see src/engine/AboutInfo.h).
# A new notice file is picked up by the glob; its readable name and group go in about:: (a test fails without them).
file(GLOB_RECURSE DAWIT_NOTICE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/licenses/*")
list(FILTER DAWIT_NOTICE_FILES EXCLUDE REGEX "/\\.[^/]*$")   # no .DS_Store and the like
list(SORT DAWIT_NOTICE_FILES)
juce_add_binary_data(DawItNotices NAMESPACE dawnotices HEADER_NAME DawItNotices.h
    SOURCES "${CMAKE_SOURCE_DIR}/LICENSE" ${DAWIT_NOTICE_FILES})
set_target_properties(DawItNotices PROPERTIES POSITION_INDEPENDENT_CODE ON)

# BuildInfo.h (commit, local changes, exact tag, DAWIT_SOURCE_URL) is written by cmake/BuildInfo.cmake at every
# build and only rewritten when its content changes, so an unchanged tree recompiles nothing.
set(DAWIT_SOURCE_URL "" CACHE STRING "Overrides the 'Source code' link of the About window (for example a source archive)")
set(DAWIT_BUILD_INFO_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_target(DawItBuildInfo ALL
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DOUT_FILE=${DAWIT_BUILD_INFO_DIR}/BuildInfo.h
        -DDAWIT_VERSION=${PROJECT_VERSION}
        "-DDAWIT_SOURCE_URL=${DAWIT_SOURCE_URL}"
        -P ${CMAKE_SOURCE_DIR}/cmake/BuildInfo.cmake
    BYPRODUCTS ${DAWIT_BUILD_INFO_DIR}/BuildInfo.h
    COMMENT "Recording the version and commit"
    VERBATIM)

# Bravura (Steinberg; SIL Open Font License 1.1), the reference font of SMuFL, draws the clefs, rests, flags and
# accidentals of the score view (889 KB, built into the app). Pinned to a commit of steinbergmedia/bravura; its
# licence text is copied into the app's Resources.
set(BRAVURA_COMMIT 37b194378b710cc40e406ab6c4b07608bb9548ae)
set(BRAVURA_FILE "${CMAKE_BINARY_DIR}/bravura/Bravura.otf")
set(BRAVURA_LICENCE "${CMAKE_BINARY_DIR}/bravura/Bravura-LICENSE.txt")
file(DOWNLOAD
    https://raw.githubusercontent.com/steinbergmedia/bravura/${BRAVURA_COMMIT}/redist/otf/Bravura.otf
    "${BRAVURA_FILE}"
    EXPECTED_HASH SHA256=cdf0f893ee1fdb64b7f6713d71ee0dcfc349c0ac01429a8e451b01a9e79f5f3b)
file(DOWNLOAD
    https://raw.githubusercontent.com/steinbergmedia/bravura/${BRAVURA_COMMIT}/LICENSE.txt
    "${BRAVURA_LICENCE}"
    EXPECTED_HASH SHA256=bdca252186865f3b2c0e76da4a4e335c14ba1d37f80194fca254ddfa88e84fbf)
juce_add_binary_data(DawItFonts NAMESPACE dawfonts HEADER_NAME DawItFonts.h SOURCES "${BRAVURA_FILE}")
set_target_properties(DawItFonts PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Puts a copy of ONNX Runtime where the executable finds it: the app's Frameworks folder on
# macOS, next to the executable elsewhere.
function(dawit_bundle_onnxruntime target)
    if(APPLE AND NOT target STREQUAL "DawItTests")
        set(dir "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks")
        set(rpath "@executable_path/../Frameworks")
    elseif(APPLE)
        set(dir "$<TARGET_FILE_DIR:${target}>")
        set(rpath "@executable_path")
    else()
        set(dir "$<TARGET_FILE_DIR:${target}>")
        set(rpath "$ORIGIN")
    endif()

    # Only the relative path, so the app doesn't depend on (or reveal) the build folder.
    set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH ON INSTALL_RPATH "${rpath}")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ORT_LIBRARY}" "${dir}/${ORT_RUNTIME_NAME}"
        VERBATIM)
endfunction()

# --- App --------------------------------------------------------------------
juce_add_gui_app(DawIt
    PRODUCT_NAME "DawIt"
    COMPANY_NAME "DawIt"
    BUNDLE_ID    "com.dawit.app"
    VERSION      ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED TRUE   # macOS asks the first time an audio track is armed
    MICROPHONE_PERMISSION_TEXT "DawIt records your voice or instrument onto a track. Nothing leaves your computer."
    NEEDS_CURL   TRUE)   # Linux: HTTP for the local AI generator (Ollama)

target_sources(DawIt PRIVATE
    src/Main.cpp
    src/analysis/ChordFinder.cpp
    src/analysis/HitFinder.cpp
    src/analysis/LyricsFinder.cpp
    src/analysis/MidiHarmony.cpp
    src/analysis/MonoAudio.cpp
    src/analysis/OnsetFinder.cpp
    src/analysis/RangeFinder.cpp
    src/analysis/StructureFinder.cpp
    src/analysis/TempoFinder.cpp
    src/engine/AudioToMidi.cpp
    src/engine/HitsToMidi.cpp
    src/engine/DrumTriggerGuide.cpp
    src/engine/DiffSinger.cpp
    src/engine/OnnxEnvironment.cpp
    src/engine/Singer.cpp
    src/engine/Voice.cpp
    src/engine/VoiceInstall.cpp
    src/engine/YamlLite.cpp
    src/engine/StereoMeasure.cpp
    src/engine/LoudnessMeter.cpp
    src/engine/LoudnessSong.cpp
    src/engine/PitchTracker.cpp
    src/engine/Tuner.cpp
    src/engine/Tune.cpp
    src/engine/Harmoniser.cpp
    src/engine/Psola.cpp
    src/engine/BuiltInFormat.cpp
    src/engine/ReverbKinds.cpp
    src/engine/BuiltInSynth.cpp
    src/engine/SoundFonts.cpp
    src/engine/SamplerGuide.cpp
    src/engine/Engine.cpp
    src/engine/Bounce.cpp
    src/engine/GainStaging.cpp
    src/engine/LatencyMeasure.cpp
    src/engine/RecordingGuide.cpp
    src/engine/ExampleSong.cpp
    src/engine/Routing.cpp
    src/engine/UndoHistory.cpp
    src/engine/RoutingGuide.cpp
    src/engine/HandoverGuide.cpp
    src/engine/SignalFlow.cpp
    src/engine/Lyrics.cpp
    src/engine/AboutInfo.cpp
    src/engine/UserPresets.cpp
    src/engine/Notebook.cpp
    src/engine/TempoMap.cpp
    src/engine/TimeSignature.cpp
    src/engine/MidiGuide.cpp
    src/engine/ComputerKeyboard.cpp
    src/engine/PdfWriter.cpp
    src/engine/GuideBooklet.cpp
    src/engine/SoundGuide.cpp
    src/engine/MidiExport.cpp
    src/engine/MidiTransforms.cpp
    src/engine/MidiQuantise.cpp
    src/engine/ModelDownload.cpp
    src/engine/ProjectFile.cpp
    src/engine/Stems.cpp
    src/engine/Track.cpp
    src/generate/ChordGuide.cpp
    src/generate/ChordRoll.cpp
    src/generate/ChordSheet.cpp
    src/generate/Notation.cpp
    src/generate/ReadingGuide.cpp
    src/generate/InstrumentsGuide.cpp
    src/generate/KeyFindingGuide.cpp
    src/generate/LyricsGuide.cpp
    src/generate/LyricsAi.cpp
    src/generate/ChordStyles.cpp
    src/generate/ChordDiagrams.cpp
    src/generate/ChatMusician.cpp
    src/generate/NoteRealizer.cpp
    src/generate/OllamaClient.cpp
    src/generate/PromptBuilder.cpp
    src/generate/RhythmGuide.cpp
    src/generate/StructureGuide.cpp
    src/generate/SongTemplates.cpp
    src/generate/StylesGuide.cpp
    src/generate/PlayAlong.cpp
    src/generate/SingAlong.cpp
    src/generate/Pronunciation.cpp
    src/generate/Rhyme.cpp
    src/generate/SingPairing.cpp
    src/generate/SingMatch.cpp
    src/generate/SingScore.cpp
    src/generate/SingingGuide.cpp
    src/generate/TuneAnalysis.cpp
    src/generate/TuningGuide.cpp
    src/generate/TuneGuideText.cpp
    src/generate/HarmoniserGuideText.cpp
    src/generate/Takes.cpp
    src/generate/TheoryGuides.cpp
    src/generate/TheoryGuideLinks.cpp
    src/generate/GuideSources.cpp
    src/ui/BounceDialog.cpp
    src/ui/BounceActions.cpp
    src/ui/LoudnessActions.cpp
    src/ui/LyricsActions.cpp
    src/ui/StylePictures.cpp
    src/ui/StylesExplainer.cpp
    src/ui/NewFromStyleWindow.cpp
    src/ui/LyricsExplainer.cpp
    src/ui/LyricsEditWindow.cpp
    src/ui/SingingExplainer.cpp
    src/ui/LyricsLane.cpp
    src/ui/MainComponent.cpp
    src/ui/TransportBar.cpp
    src/ui/TimeSignatureUi.cpp
    src/ui/ComputerKeyboardTyping.cpp
    src/ui/ComputerKeyboardStrip.cpp
    src/ui/GuideButton.cpp
    src/ui/GuideSourcesPanel.cpp
    src/ui/GuideNavigator.cpp
    src/ui/GuideFooter.cpp
    src/ui/ArrangementView.cpp
    src/ui/AudioToMidiActions.cpp
    src/ui/HitsToMidiActions.cpp
    src/ui/HitsToMidiWindow.cpp
    src/ui/ChordActions.cpp
    src/ui/ChordStyleActions.cpp
    src/ui/BuiltInSynthEditor.cpp
    src/ui/TunerWindow.cpp
    src/ui/AboutWindow.cpp
    src/ui/EffectWindow.cpp
    src/ui/GainStagingExplainer.cpp
    src/ui/SoundExplainer.cpp
    src/ui/ChordDiagramViews.cpp
    src/ui/ChordPlayAlong.cpp
    src/ui/LyricsSingAlong.cpp
    src/ui/TempoLane.cpp
    src/ui/MidiExplainer.cpp
    src/ui/MidiTransformMenu.cpp
    src/ui/ChordExplainer.cpp
    src/ui/ChordSheetExport.cpp
    src/ui/GuidePrint.cpp
    src/ui/ChordLane.cpp
    src/ui/DeviceBar.cpp
    src/ui/ChainPictures.cpp
    src/ui/SignalFlowView.cpp
    src/ui/DevScreenshots.cpp
    src/ui/GeneratePanel.cpp
    src/ui/MidiExportActions.cpp
    src/ui/PluginActions.cpp
    src/ui/SamplerActions.cpp
    src/ui/TrackActions.cpp
    src/ui/HandoverExplainer.cpp
    src/ui/RecordingExplainer.cpp
    src/ui/DrumTriggerExplainer.cpp
    src/ui/TuningExplainer.cpp
    src/ui/RoutingExplainer.cpp
    src/ui/TrackHeader.cpp
    src/ui/NotebookWindow.cpp
    src/ui/NoteIcon.cpp
    src/ui/RecordingActions.cpp
    src/ui/AudioSettingsPanel.cpp
    src/ui/TrackLanes.cpp
    src/ui/ControllerLane.cpp
    src/ui/PianoRoll.cpp
    src/ui/QuantiseDialog.cpp
    src/ui/ScorePainter.cpp
    src/ui/ScoreView.cpp
    src/ui/SheetMusicExport.cpp
    src/ui/WaveEditor.cpp
    src/ui/MixerView.cpp
    src/ui/SendKnob.cpp
    src/ui/InspectorPanel.cpp
    src/ui/ProjectManager.cpp
    src/ui/KeyFindingExplainer.cpp
    src/ui/InstrumentsExplainer.cpp
    src/ui/ReadingExplainer.cpp
    src/ui/RhythmExplainer.cpp
    src/ui/SingActions.cpp
    src/ui/SingingRangeActions.cpp
    src/ui/SingWindow.cpp
    src/ui/SectionLane.cpp
    src/ui/StemActions.cpp
    src/ui/StructureActions.cpp
    src/ui/TempoActions.cpp
    src/ui/StructureExplainer.cpp
    src/ui/TheoryGuidesWindow.cpp)

target_include_directories(DawIt PRIVATE src)

# Audio Units exist only on macOS; VST3 works everywhere.
if(APPLE)
    set(DAWIT_HOST_AU 1)
else()
    set(DAWIT_HOST_AU 0)
endif()

target_compile_definitions(DawIt PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DawIt,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DawIt,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=${DAWIT_HOST_AU}
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_WEB_BROWSER=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

add_dependencies(DawIt DawItBuildInfo)
target_include_directories(DawIt PRIVATE ${DAWIT_BUILD_INFO_DIR})

target_link_libraries(DawIt
    PRIVATE
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_dsp
        demucs
        whisper
        soundfontcore
        onnxruntime
        DawItModels
        DawItDictionary
        DawItStyles
        DawItSources
        DawItNotices
        DawItFonts
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

dawit_bundle_onnxruntime(DawIt)

# Bravura's licence travels with the app: in Resources on macOS, next to the executable elsewhere.
if(APPLE)
    set(BRAVURA_LICENCE_DIR "$<TARGET_BUNDLE_CONTENT_DIR:DawIt>/Resources")
else()
    set(BRAVURA_LICENCE_DIR "$<TARGET_FILE_DIR:DawIt>")
endif()
add_custom_command(TARGET DawIt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BRAVURA_LICENCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BRAVURA_LICENCE}" "${BRAVURA_LICENCE_DIR}/Bravura-LICENSE.txt"
    VERBATIM)

# The linker signs only the executable, under the name "DawIt". Signing the whole bundle (ad hoc) binds its
# Info.plist and bundle ID, so macOS's microphone permission belongs to com.dawit.app.
if(APPLE)
    add_custom_command(TARGET DawIt POST_BUILD
        COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:DawIt>"
        VERBATIM)
endif()

# --- Engine tests (headless) ------------------------------------------------
option(DAWIT_BUILD_TESTS "Build the headless engine tests" ON)

if(DAWIT_BUILD_TESTS)
    juce_add_console_app(DawItTests PRODUCT_NAME "DawItTests" NEEDS_CURL TRUE)

    # The test suite holds several large engine fixtures in the same function.
    if(MSVC)
        target_link_options(DawItTests PRIVATE /STACK:16777216)
    endif()

    target_sources(DawItTests PRIVATE
        tests/EngineTests.cpp
        src/analysis/ChordFinder.cpp
        src/analysis/HitFinder.cpp
        src/analysis/LyricsFinder.cpp
        src/analysis/MidiHarmony.cpp
        src/analysis/MonoAudio.cpp
        src/analysis/OnsetFinder.cpp
        src/analysis/RangeFinder.cpp
        src/analysis/StructureFinder.cpp
        src/analysis/TempoFinder.cpp
        src/engine/AudioToMidi.cpp
        src/engine/HitsToMidi.cpp
        src/engine/DrumTriggerGuide.cpp
        src/engine/DiffSinger.cpp
        src/engine/OnnxEnvironment.cpp
        src/engine/Singer.cpp
        src/engine/Voice.cpp
        src/engine/VoiceInstall.cpp
        src/engine/YamlLite.cpp
        src/engine/StereoMeasure.cpp
        src/engine/LoudnessMeter.cpp
        src/engine/LoudnessSong.cpp
        src/engine/PitchTracker.cpp
        src/engine/Tuner.cpp
        src/engine/Tune.cpp
        src/engine/Harmoniser.cpp
        src/engine/Psola.cpp
        src/engine/BuiltInFormat.cpp
        src/engine/ReverbKinds.cpp
        src/engine/BuiltInSynth.cpp
        src/engine/SoundFonts.cpp
        src/engine/SamplerGuide.cpp
        src/engine/Engine.cpp
        src/engine/Bounce.cpp
        src/engine/GainStaging.cpp
        src/engine/LatencyMeasure.cpp
        src/engine/RecordingGuide.cpp
        src/engine/ExampleSong.cpp
        src/engine/Routing.cpp
        src/engine/UndoHistory.cpp
        src/engine/RoutingGuide.cpp
        src/engine/HandoverGuide.cpp
        src/engine/SignalFlow.cpp
        src/engine/Lyrics.cpp
        src/engine/AboutInfo.cpp
        src/engine/UserPresets.cpp
        src/engine/Notebook.cpp
        src/engine/TempoMap.cpp
        src/engine/TimeSignature.cpp
        src/engine/MidiGuide.cpp
        src/engine/ComputerKeyboard.cpp
        src/engine/PdfWriter.cpp
        src/engine/GuideBooklet.cpp
        src/engine/SoundGuide.cpp
        src/engine/MidiExport.cpp
        src/engine/MidiTransforms.cpp
        src/engine/MidiQuantise.cpp
        src/engine/ModelDownload.cpp
        src/engine/ProjectFile.cpp
        src/engine/Stems.cpp
        src/engine/Track.cpp
        src/generate/ChordGuide.cpp
        src/generate/ChordRoll.cpp
        src/generate/ChordSheet.cpp
        src/generate/Notation.cpp
        src/generate/ReadingGuide.cpp
        src/generate/InstrumentsGuide.cpp
        src/generate/KeyFindingGuide.cpp
        src/generate/LyricsGuide.cpp
        src/generate/LyricsAi.cpp
        src/generate/ChordStyles.cpp
        src/generate/ChordDiagrams.cpp
        src/generate/ChatMusician.cpp
        src/generate/NoteRealizer.cpp
        src/generate/OllamaClient.cpp
        src/generate/PromptBuilder.cpp
        src/generate/RhythmGuide.cpp
        src/generate/StructureGuide.cpp
        src/generate/SongTemplates.cpp
        src/generate/StylesGuide.cpp
        src/generate/PlayAlong.cpp
        src/generate/SingAlong.cpp
        src/generate/Pronunciation.cpp
        src/generate/Rhyme.cpp
        src/generate/SingPairing.cpp
        src/generate/SingMatch.cpp
        src/generate/SingScore.cpp
        src/generate/SingingGuide.cpp
        src/generate/TuneAnalysis.cpp
        src/generate/TuningGuide.cpp
        src/generate/TuneGuideText.cpp
        src/generate/HarmoniserGuideText.cpp
        src/generate/Takes.cpp
        src/generate/TheoryGuides.cpp
        src/generate/TheoryGuideLinks.cpp
        src/generate/GuideSources.cpp)

    target_include_directories(DawItTests PRIVATE src)

    target_compile_definitions(DawItTests PRIVATE
        DAWIT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=${DAWIT_HOST_AU}
        JUCE_WEB_BROWSER=0)

    target_link_libraries(DawItTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_cryptography
            juce::juce_dsp
            demucs
            whisper
            soundfontcore
            onnxruntime
            DawItModels
            DawItDictionary
            DawItStyles
            DawItSources
            DawItNotices
            DawItFonts
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    dawit_bundle_onnxruntime(DawItTests)
endif()

# --- Built-in effects and instruments ---------------------------------------
# One call each: its sources go into the app (and the engine ones into the tests). See cmake/DawItBuiltIns.cmake.

dawit_add_builtin(Eq
    DISPLAY_NAME "DawIt EQ"
    KIND effect
    CODE Dequ
    VST3_CATEGORY "Fx|EQ"
    PROCESSOR BuiltInEq HEADER engine/BuiltInEq.h
    EDITOR BuiltInEqEditor EDITOR_HEADER ui/EqEditor.h
    ENGINE_SOURCES src/engine/BuiltInEq.cpp
    UI_SOURCES src/ui/EqEditor.cpp
        src/ui/EqExplainer.cpp
        src/ui/EqGuide.cpp)

dawit_add_builtin(Compressor
    DISPLAY_NAME "DawIt Compressor"
    KIND effect
    CODE Dcmp
    VST3_CATEGORY "Fx|Dynamics"
    PROCESSOR BuiltInCompressor HEADER engine/BuiltInCompressor.h
    EDITOR BuiltInCompressorEditor EDITOR_HEADER ui/CompressorEditor.h
    ENGINE_SOURCES src/engine/BuiltInCompressor.cpp
    UI_SOURCES src/ui/CompressorEditor.cpp
        src/ui/CompressorExplainer.cpp
        src/ui/CompressorGuide.cpp)

dawit_add_builtin(Gate
    DISPLAY_NAME "DawIt Gate"
    KIND effect
    CODE Dgat
    VST3_CATEGORY "Fx|Dynamics"
    PROCESSOR BuiltInGate HEADER engine/BuiltInGate.h
    EDITOR BuiltInGateEditor EDITOR_HEADER ui/GateEditor.h
    ENGINE_SOURCES src/engine/BuiltInGate.cpp
    UI_SOURCES src/ui/GateEditor.cpp
        src/ui/GateGuide.cpp)

dawit_add_builtin(Stereo
    DISPLAY_NAME "DawIt Stereo"
    KIND effect
    CODE Dstr
    VST3_CATEGORY "Fx|Spatial"
    PROCESSOR BuiltInStereo HEADER engine/BuiltInStereo.h
    EDITOR BuiltInStereoEditor EDITOR_HEADER ui/StereoEditor.h
    ENGINE_SOURCES src/engine/BuiltInStereo.cpp
    UI_SOURCES src/ui/StereoScope.cpp
        src/ui/StereoEditor.cpp
        src/ui/StereoGuide.cpp)

dawit_add_builtin(Reverb
    DISPLAY_NAME "DawIt Reverb"
    KIND effect
    CODE Drvb
    VST3_CATEGORY "Fx|Reverb"
    PROCESSOR BuiltInReverb HEADER engine/BuiltInReverb.h
    EDITOR BuiltInReverbEditor EDITOR_HEADER ui/ReverbEditor.h
    ENGINE_SOURCES src/engine/BuiltInReverb.cpp
    UI_SOURCES src/ui/ReverbEditor.cpp
        src/ui/ReverbGuide.cpp)

dawit_add_builtin(Delay
    DISPLAY_NAME "DawIt Delay"
    KIND effect
    CODE Ddly
    VST3_CATEGORY "Fx|Delay"
    PROCESSOR BuiltInDelay HEADER engine/BuiltInDelay.h
    EDITOR BuiltInDelayEditor EDITOR_HEADER ui/DelayEditor.h
    ENGINE_SOURCES src/engine/BuiltInDelay.cpp
    UI_SOURCES src/ui/DelayEditor.cpp
        src/ui/DelayGuide.cpp)

dawit_add_builtin(Slicer
    DISPLAY_NAME "DawIt Slicer"
    KIND effect
    CODE Dslc
    VST3_CATEGORY "Fx"
    PROCESSOR BuiltInSlicer HEADER engine/BuiltInSlicer.h
    EDITOR BuiltInSlicerEditor EDITOR_HEADER ui/SlicerEditor.h
    ENGINE_SOURCES src/engine/BuiltInSlicer.cpp
    UI_SOURCES src/ui/SlicerEditor.cpp
        src/ui/SlicerGuide.cpp)

dawit_add_builtin(StepFilter
    DISPLAY_NAME "DawIt Step Filter"
    KIND effect
    CODE Dstf
    VST3_CATEGORY "Fx|Filter"
    PROCESSOR BuiltInStepFilter HEADER engine/BuiltInStepFilter.h
    EDITOR BuiltInStepFilterEditor EDITOR_HEADER ui/StepFilterEditor.h
    ENGINE_SOURCES src/engine/BuiltInStepFilter.cpp
    UI_SOURCES src/ui/StepFilterEditor.cpp
        src/ui/StepFilterGuide.cpp)

dawit_add_builtin(SpectralFreeze
    DISPLAY_NAME "DawIt Spectral Freeze"
    KIND effect
    CODE Dfrz
    VST3_CATEGORY "Fx|Spectral"
    PROCESSOR BuiltInSpectralFreeze HEADER engine/BuiltInSpectralFreeze.h
    EDITOR BuiltInSpectralFreezeEditor EDITOR_HEADER ui/SpectralFreezeEditor.h
    ENGINE_SOURCES src/engine/BuiltInSpectralFreeze.cpp
    UI_SOURCES src/ui/SpectralFreezeEditor.cpp
        src/ui/SpectralFreezeGuide.cpp)

dawit_add_builtin(Saturation
    DISPLAY_NAME "DawIt Saturation"
    KIND effect
    CODE Dsat
    VST3_CATEGORY "Fx|Distortion"
    PROCESSOR BuiltInSaturation HEADER engine/BuiltInSaturation.h
    EDITOR BuiltInSaturationEditor EDITOR_HEADER ui/SaturationEditor.h
    ENGINE_SOURCES src/engine/BuiltInSaturation.cpp
    UI_SOURCES src/ui/SaturationEditor.cpp
        src/ui/SaturationGuide.cpp)

dawit_add_builtin(Distortion
    DISPLAY_NAME "DawIt Distortion"
    KIND effect
    CODE Ddst
    VST3_CATEGORY "Fx|Distortion"
    PROCESSOR BuiltInDistortion HEADER engine/BuiltInDistortion.h
    EDITOR BuiltInDistortionEditor EDITOR_HEADER ui/DistortionEditor.h
    ENGINE_SOURCES src/engine/BuiltInDistortion.cpp
    UI_SOURCES src/ui/DistortionEditor.cpp
        src/ui/DistortionGuide.cpp)

dawit_add_builtin(Subtractive
    DISPLAY_NAME "DawIt Subtractive"
    KIND instrument
    CODE Dsub
    VST3_CATEGORY "Instrument|Synth"
    PROCESSOR BuiltInSubtractive HEADER engine/BuiltInSubtractive.h
    EDITOR BuiltInSubtractiveEditor EDITOR_HEADER ui/SubtractiveEditor.h
    ENGINE_SOURCES src/engine/BuiltInSubtractive.cpp
    UI_SOURCES src/ui/SubtractiveEditor.cpp
        src/ui/SubtractiveGuide.cpp)

dawit_add_builtin(Sampler
    DISPLAY_NAME "DawIt Sampler"
    KIND instrument
    CODE Dsmp
    VST3_CATEGORY "Instrument|Sampler"
    PROCESSOR BuiltInSampler HEADER engine/BuiltInSampler.h
    EDITOR BuiltInSamplerEditor EDITOR_HEADER ui/SamplerEditor.h
    APP_ONLY
    ENGINE_SOURCES src/engine/BuiltInSampler.cpp
    UI_SOURCES src/ui/SamplerEditor.cpp
        src/ui/SamplerPictures.cpp
        src/ui/SamplerGuide.cpp)

dawit_add_builtin(Additive
    DISPLAY_NAME "DawIt Additive"
    KIND instrument
    CODE Dadd
    VST3_CATEGORY "Instrument|Synth"
    PROCESSOR BuiltInAdditive HEADER engine/BuiltInAdditive.h
    EDITOR BuiltInAdditiveEditor EDITOR_HEADER ui/AdditiveEditor.h
    ENGINE_SOURCES src/engine/BuiltInAdditive.cpp
    UI_SOURCES src/ui/AdditiveEditor.cpp
        src/ui/AdditiveGuide.cpp)

dawit_add_builtin(Fm
    DISPLAY_NAME "DawIt FM"
    KIND instrument
    CODE Dfmx
    VST3_CATEGORY "Instrument|Synth"
    PROCESSOR BuiltInFm HEADER engine/BuiltInFm.h
    EDITOR BuiltInFmEditor EDITOR_HEADER ui/FmEditor.h
    ENGINE_SOURCES src/engine/BuiltInFm.cpp
    UI_SOURCES src/ui/FmEditor.cpp
        src/ui/FmGuide.cpp)

dawit_add_builtin(Drums
    DISPLAY_NAME "DawIt Drums"
    KIND instrument
    CODE Ddrm
    VST3_CATEGORY "Instrument|Drum"
    PROCESSOR BuiltInDrums HEADER engine/BuiltInDrums.h
    EDITOR BuiltInDrumsEditor EDITOR_HEADER ui/DrumsEditor.h
    ENGINE_SOURCES src/engine/BuiltInDrums.cpp
    UI_SOURCES src/ui/DrumsEditor.cpp
        src/ui/DrumsGuide.cpp)

dawit_add_builtin(Physical
    DISPLAY_NAME "DawIt Physical"
    KIND instrument
    CODE Dphy
    VST3_CATEGORY "Instrument|Synth"
    PROCESSOR BuiltInPhysical HEADER engine/BuiltInPhysical.h
    EDITOR BuiltInPhysicalEditor EDITOR_HEADER ui/PhysicalEditor.h
    ENGINE_SOURCES src/engine/BuiltInPhysical.cpp
    UI_SOURCES src/ui/PhysicalEditor.cpp
        src/ui/PhysicalGuide.cpp)

dawit_add_builtin(Arpeggiator
    DISPLAY_NAME "DawIt Arpeggiator"
    KIND effect
    CODE Darp
    VST3_CATEGORY "Fx"
    PROCESSOR BuiltInArpeggiator HEADER engine/BuiltInArpeggiator.h
    EDITOR BuiltInArpeggiatorEditor EDITOR_HEADER ui/ArpeggiatorEditor.h
    ENGINE_SOURCES src/engine/BuiltInArpeggiator.cpp
    UI_SOURCES src/ui/ArpeggiatorEditor.cpp
        src/ui/ArpeggiatorGuide.cpp
    APP_ONLY)

dawit_add_builtin(Tune
    DISPLAY_NAME "DawIt Tune"
    KIND effect
    CODE Dtun
    VST3_CATEGORY "Fx|Pitch Shift"
    PROCESSOR BuiltInTune HEADER engine/BuiltInTune.h
    EDITOR BuiltInTuneEditor EDITOR_HEADER ui/TuneEditor.h
    ENGINE_SOURCES src/engine/BuiltInTune.cpp
    UI_SOURCES src/ui/TunePictures.cpp
        src/ui/TuneGuide.cpp
        src/ui/TuneEditor.cpp
    APP_ONLY)

dawit_add_builtin(Harmoniser
    DISPLAY_NAME "DawIt Harmoniser"
    KIND effect
    CODE Dhrm
    VST3_CATEGORY "Fx|Pitch Shift"
    PROCESSOR BuiltInHarmoniser HEADER engine/BuiltInHarmoniser.h
    EDITOR BuiltInHarmoniserEditor EDITOR_HEADER ui/HarmoniserEditor.h
    ENGINE_SOURCES src/engine/BuiltInHarmoniser.cpp
    UI_SOURCES src/ui/HarmoniserGuide.cpp
        src/ui/HarmoniserEditor.cpp
    APP_ONLY)

dawit_add_builtin(Meter
    DISPLAY_NAME "DawIt Meter"
    KIND effect
    CODE Dmtr
    VST3_CATEGORY "Fx|Analyzer"
    PROCESSOR BuiltInMeter HEADER engine/BuiltInMeter.h
    EDITOR BuiltInMeterEditor EDITOR_HEADER ui/MeterEditor.h
    ENGINE_SOURCES src/engine/BuiltInMeter.cpp
    UI_SOURCES src/ui/StereoScope.cpp
        src/ui/MeterEditor.cpp
        src/ui/MeterGuide.cpp)

dawit_finish_builtins()
