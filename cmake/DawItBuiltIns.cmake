# The registry of DawIt's built-in effects and instruments.
#
# Each built-in is added by one call of dawit_add_builtin (see the bottom of CMakeLists.txt). The call puts its
# sources into the app (DawIt) and, for the engine ones, the tests (DawItTests), and records it for the list
# dawit_finish_builtins writes as DawItBuiltInList.h, which the tests read. The plugin targets (VST3 and AU) are
# made from the same recorded fields (see PROCESSOR, EDITOR, VST3_CATEGORY below).

set(DAWIT_PLUGIN_MANUFACTURER_CODE "Dwit")   # permanent once anyone saves a session with a DawIt plugin
set(DAWIT_PLUGIN_MANUFACTURER_NAME "DawIt")

# The plugins (VST3, and AU on macOS) are slow to build: each one compiles JUCE again. They are made only on request:
# ./build.sh --plugins.
option(DAWIT_BUILD_PLUGINS "Also build every built-in as a VST3 and AU plugin" OFF)

# Sources every plugin shares (each plugin compiles them again, as JUCE's module rule asks). The fxui frame and what the
# built-ins' editors and guides use, and the engine files of built-ins that others call into (the EQ's filters, the
# compressor's detector, the saturation's harmonics, the subtractive synth's oscillators and its envelope picture); nothing here may include engine/Engine.h, generate/TheoryGuides.h or other app UI.
set(DAWIT_PLUGIN_SHARED_SOURCES
    src/engine/BuiltInSynth.cpp
    src/engine/BuiltInEq.cpp
    src/engine/BuiltInCompressor.cpp
    src/engine/BuiltInSaturation.cpp
    src/engine/BuiltInSubtractive.cpp
    src/engine/Tune.cpp
    src/engine/ReverbKinds.cpp
    src/engine/StereoMeasure.cpp
    src/ui/EffectWindow.cpp
    src/ui/SubtractiveGuide.cpp)

define_property(GLOBAL PROPERTY DAWIT_BUILTINS BRIEF_DOCS "Registered built-ins" FULL_DOCS "Registered built-ins")

# Makes the plugin of one built-in: a juce_add_plugin target (VST3, plus AU on macOS) whose entry file, made from
# src/plugin/PluginEntry.cpp.in, creates the processor and sets the function that makes its editor.
function(dawit_add_plugin_target name displayName kind code vst3Category processor header editor editorHeader sources)
    set(target DawItPlugin${name})
    set(formats VST3)
    if(APPLE)
        list(APPEND formats AU)
    endif()
    if(kind STREQUAL "instrument")
        set(isSynth TRUE)
    else()
        set(isSynth FALSE)
    endif()
    string(TOLOWER "${name}" lowerName)
    string(REPLACE "|" " " categories "${vst3Category}")

    juce_add_plugin(${target}
        PRODUCT_NAME "${displayName}"
        COMPANY_NAME "${DAWIT_PLUGIN_MANUFACTURER_NAME}"
        BUNDLE_ID "com.dawit.plugin.${lowerName}"
        VERSION ${PROJECT_VERSION}
        FORMATS ${formats}
        PLUGIN_MANUFACTURER_CODE ${DAWIT_PLUGIN_MANUFACTURER_CODE}
        PLUGIN_CODE ${code}
        IS_SYNTH ${isSynth}
        NEEDS_MIDI_INPUT ${isSynth}
        NEEDS_MIDI_OUTPUT FALSE
        VST3_CATEGORIES ${categories}
        DESCRIPTION "${displayName}: a built-in effect or instrument of DawIt, with its guide")

    set(PLUGIN_PROCESSOR "${processor}")
    set(PLUGIN_HEADER "${header}")
    set(PLUGIN_EDITOR "${editor}")
    set(PLUGIN_EDITOR_HEADER "${editorHeader}")
    configure_file("${CMAKE_SOURCE_DIR}/src/plugin/PluginEntry.cpp.in" "${CMAKE_BINARY_DIR}/plugin/${name}Entry.cpp" @ONLY)

    # A built-in's own file may also be in the shared list (its EQ is used by others), so each file is listed once.
    set(allSources ${sources} ${DAWIT_PLUGIN_SHARED_SOURCES})
    list(REMOVE_DUPLICATES allSources)
    target_sources(${target} PRIVATE "${CMAKE_BINARY_DIR}/plugin/${name}Entry.cpp" ${allSources})
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_compile_definitions(${target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)
    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    set_property(GLOBAL APPEND PROPERTY DAWIT_PLUGIN_TARGETS ${target})
endfunction()

# dawit_add_builtin (<Name>
#     DISPLAY_NAME <"DawIt Reverb">    the identifier and the name in menus and hosts
#     KIND <effect|instrument>
#     CODE <Drvb>                      the plugin's four-character code: one capital, first, then lower case
#     VST3_CATEGORY <"Fx|Reverb">      the VST3 sub-categories
#     PROCESSOR <BuiltInReverb> HEADER <engine/BuiltInReverb.h>
#     EDITOR <BuiltInReverbEditor> EDITOR_HEADER <ui/ReverbEditor.h>
#     ENGINE_SOURCES <...>             in the app, the tests and the plugin
#     UI_SOURCES <...>                 in the app and the plugin
#     PLUGIN_EXTRA_SOURCES <...>       only in the plugin: another built-in's engine file it calls into
#     [APP_ONLY])                      no plugin is made
function(dawit_add_builtin name)
    cmake_parse_arguments(B "APP_ONLY" "DISPLAY_NAME;KIND;CODE;VST3_CATEGORY;PROCESSOR;HEADER;EDITOR;EDITOR_HEADER"
                          "ENGINE_SOURCES;UI_SOURCES;PLUGIN_EXTRA_SOURCES" ${ARGN})

    if(NOT B_KIND MATCHES "^(effect|instrument)$")
        message(FATAL_ERROR "dawit_add_builtin(${name}): KIND must be effect or instrument")
    endif()
    if(NOT B_CODE MATCHES "^[A-Z][a-z][a-z][a-z]$")
        message(FATAL_ERROR "dawit_add_builtin(${name}): CODE must be one capital then three lower case letters")
    endif()

    get_property(existing GLOBAL PROPERTY DAWIT_BUILTINS)
    foreach(other IN LISTS existing)
        get_property(otherCode GLOBAL PROPERTY DAWIT_BUILTIN_${other}_CODE)
        if(otherCode STREQUAL B_CODE)
            message(FATAL_ERROR "dawit_add_builtin(${name}): code ${B_CODE} is already ${other}'s")
        endif()
    endforeach()

    target_sources(DawIt PRIVATE ${B_ENGINE_SOURCES} ${B_UI_SOURCES})
    if(TARGET DawItTests)
        target_sources(DawItTests PRIVATE ${B_ENGINE_SOURCES})
    endif()

    if(DAWIT_BUILD_PLUGINS AND NOT B_APP_ONLY)
        dawit_add_plugin_target(${name} "${B_DISPLAY_NAME}" ${B_KIND} ${B_CODE} "${B_VST3_CATEGORY}"
                                "${B_PROCESSOR}" "${B_HEADER}" "${B_EDITOR}" "${B_EDITOR_HEADER}"
                                "${B_ENGINE_SOURCES};${B_UI_SOURCES};${B_PLUGIN_EXTRA_SOURCES}")
    endif()

    set_property(GLOBAL APPEND PROPERTY DAWIT_BUILTINS ${name})
    foreach(field DISPLAY_NAME KIND CODE VST3_CATEGORY PROCESSOR HEADER EDITOR EDITOR_HEADER ENGINE_SOURCES UI_SOURCES APP_ONLY)
        set_property(GLOBAL PROPERTY DAWIT_BUILTIN_${name}_${field} "${B_${field}}")
    endforeach()
endfunction()

# Writes DawItBuiltInList.h (name, identifier, code, kind, app-only of every built-in) into the build folder and
# adds its folder to the include path of the app and the tests.
function(dawit_finish_builtins)
    # One target that builds every plugin: cmake --build <dir> --target DawItPlugins
    get_property(pluginTargets GLOBAL PROPERTY DAWIT_PLUGIN_TARGETS)
    if(pluginTargets)
        set(formatTargets "")
        foreach(target IN LISTS pluginTargets)
            list(APPEND formatTargets ${target}_VST3)
            if(APPLE)
                list(APPEND formatTargets ${target}_AU)
            endif()
        endforeach()
        add_custom_target(DawItPlugins DEPENDS ${formatTargets})
    endif()

    get_property(all GLOBAL PROPERTY DAWIT_BUILTINS)
    set(content "// Generated by cmake/DawItBuiltIns.cmake from the dawit_add_builtin calls. Do not edit.\n#pragma once\n\n")
    string(APPEND content "namespace daw::builtinlist\n{\nstruct Entry\n{\n    const char* identifier;   // \"DawIt Reverb\"\n")
    string(APPEND content "    const char* code;         // the plugin's four-character code\n    bool isInstrument;\n    bool appOnly;             // no plugin is made\n};\n\n")
    string(APPEND content "inline constexpr const char* manufacturerCode = \"${DAWIT_PLUGIN_MANUFACTURER_CODE}\";\n\ninline constexpr Entry entries[] =\n{\n")
    foreach(name IN LISTS all)
        get_property(display GLOBAL PROPERTY DAWIT_BUILTIN_${name}_DISPLAY_NAME)
        get_property(code GLOBAL PROPERTY DAWIT_BUILTIN_${name}_CODE)
        get_property(kind GLOBAL PROPERTY DAWIT_BUILTIN_${name}_KIND)
        get_property(appOnly GLOBAL PROPERTY DAWIT_BUILTIN_${name}_APP_ONLY)
        if(kind STREQUAL "instrument")
            set(isInstrument "true")
        else()
            set(isInstrument "false")
        endif()
        if(appOnly)
            set(appOnlyText "true")
        else()
            set(appOnlyText "false")
        endif()
        string(APPEND content "    { \"${display}\", \"${code}\", ${isInstrument}, ${appOnlyText} },\n")
    endforeach()
    string(APPEND content "};\n} // namespace daw::builtinlist\n")

    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/generated/DawItBuiltInList.h" CONTENT "${content}")
    target_include_directories(DawIt PRIVATE "${CMAKE_BINARY_DIR}/generated")
    if(TARGET DawItTests)
        target_include_directories(DawItTests PRIVATE "${CMAKE_BINARY_DIR}/generated")
    endif()
endfunction()
