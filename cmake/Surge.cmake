macro(add_surge_standalone TARGET)
  add_executable(${TARGET} ${ARGN} ${SURGE_COMMON_SOURCES})
  target_include_directories(${TARGET} PRIVATE ${SURGE_COMMON_INCLUDES})
  set_surge_compile_options(${TARGET})
endmacro()

macro(add_surge_plugin TARGET)
  add_library(${TARGET} MODULE ${ARGN} ${SURGE_COMMON_SOURCES})
  target_include_directories(${TARGET} PRIVATE ${SURGE_COMMON_INCLUDES})
  set_surge_compile_options(${TARGET})
  set_target_properties(${TARGET} PROPERTIES
    OUTPUT_NAME "Surge"
    PREFIX "")
endmacro()

macro(add_surge_ui TARGET)
  target_sources(${TARGET} PRIVATE ${SURGE_GUI_SOURCES})
  target_link_libraries(${TARGET} PRIVATE vstgui)
endmacro()

################################################################################

find_package(Threads REQUIRED)

if(APPLE)
  find_library(APPLICATION_SERVICES_FRAMEWORK "ApplicationServices")
  find_library(CORE_FOUNDATION_FRAMEWORK "CoreFoundation")
endif()

if(UNIX AND NOT APPLE)
  find_library(STD_FILESYSTEM_LIBRARY "stdc++fs")
endif()

if(WIN32)
  find_library(WINMM_LIBRARY "winmm")
endif()

################################################################################

if(UNIX AND NOT APPLE)
  add_custom_target(
    scalable-piggy ALL
    DEPENDS ${PROJECT_SOURCE_DIR}/src/linux/_ScalablePiggy.S)
  add_custom_command(
    OUTPUT
    ${PROJECT_SOURCE_DIR}/src/linux/_ScalablePiggy.S
    ${PROJECT_SOURCE_DIR}/src/linux/ScalablePiggy.S
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/linux/emit-vector-piggy.py ${PROJECT_SOURCE_DIR})
endif()

################################################################################

macro(set_surge_compile_options TARGET)
  if(CMAKE_C_COMPILER_ID MATCHES "^GNU|Clang$")
    target_compile_options(${TARGET} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:-mfpmath=sse -msse2 -Wno-multichar>)
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "^GNU|Clang$")
    target_compile_options(${TARGET} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-mfpmath=sse -msse2 -Wno-multichar>)
  endif()

  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TARGET} PRIVATE
      $<$<COMPILE_LANGUAGE:C>:/arch:SSE2 "/FI precompiled.h" /bigobj>)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TARGET} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:/arch:SSE2 "/FI precompiled.h" /Zc:alignedNew /bigobj>)
  endif()

  if(NOT WIN32)
    # XXX: not used target_compile_definitions, as cmake will not reject
    # the function-like form, which not all compilers accept
    target_compile_options(${TARGET} PRIVATE
      "-D_aligned_malloc(x,a)=malloc(x)"
      "-D_aligned_free(x)=free(x)")
  endif()

  if(APPLE)
    target_compile_definitions(${TARGET} PRIVATE
      MAC=1
      MAC_COCOA=1
      COCOA=1
      OBJC_OLD_DISPATCH_PROTOTYPES=1)
    target_include_directories(${TARGET} PRIVATE
      src/mac)
    target_link_libraries(${TARGET} PRIVATE
      ${APPLICATION_SERVICES_FRAMEWORK}
      ${CORE_FOUNDATION_FRAMEWORK})
  endif()

  if(UNIX AND NOT APPLE)
    target_compile_definitions(${TARGET} PRIVATE
      LINUX=1)
    target_sources(${TARGET} PRIVATE
      src/linux/ConfigurationXml.S
      src/linux/ScalablePiggy.S)
    target_include_directories(${TARGET} PRIVATE
      src/linux)
    target_link_libraries(${TARGET} PRIVATE
      ${STD_FILESYSTEM_LIBRARY})
  endif()

  if(WIN32)
    target_compile_definitions(${TARGET} PRIVATE
      WINDOWS=1
      NOMINMAX=1)
    target_link_libraries(${TARGET} PRIVATE
      ${WINMM_LIBRARY})
  endif()

  target_link_libraries(${TARGET} PRIVATE
    ${CMAKE_THREAD_LIBS_INIT})
endmacro()

################################################################################

set(SURGE_COMMON_SOURCES
  src/common/dsp/effect/ConditionerEffect.cpp
  src/common/dsp/effect/DistortionEffect.cpp
  src/common/dsp/effect/DualDelayEffect.cpp
  src/common/dsp/effect/Effect.cpp
  src/common/dsp/effect/FreqshiftEffect.cpp
  src/common/dsp/effect/PhaserEffect.cpp
  src/common/dsp/effect/Reverb1Effect.cpp
  src/common/dsp/effect/Reverb2Effect.cpp
  src/common/dsp/effect/RotarySpeakerEffect.cpp
  src/common/dsp/effect/VocoderEffect.cpp
  src/common/dsp/AdsrEnvelope.cpp
  src/common/dsp/BiquadFilter.cpp
  src/common/dsp/BiquadFilterSSE2.cpp
  src/common/dsp/DspUtilities.cpp
  src/common/dsp/FilterCoefficientMaker.cpp
  src/common/dsp/FMOscillator.cpp
  src/common/dsp/LfoModulationSource.cpp
  src/common/dsp/Oscillator.cpp
  src/common/dsp/QuadFilterChain.cpp
  src/common/dsp/QuadFilterUnit.cpp
  src/common/dsp/SampleAndHoldOscillator.cpp
  src/common/dsp/SurgeSuperOscillator.cpp
  src/common/dsp/SurgeVoice.cpp
  src/common/dsp/VectorizedSvfFilter.cpp
  src/common/dsp/Wavetable.cpp
  src/common/dsp/WavetableOscillator.cpp
  src/common/dsp/WindowOscillator.cpp
  src/common/thread/CriticalSection.cpp
  src/common/util/FpuState.cpp
  src/common/vt_dsp/basic_dsp.cpp
  src/common/vt_dsp/halfratefilter.cpp
  src/common/vt_dsp/lipol.cpp
  src/common/vt_dsp/macspecific.cpp
  src/common/Parameter.cpp
  src/common/precompiled.cpp
  src/common/SurgeError.cpp
  src/common/SurgePatch.cpp
  src/common/SurgeStorage.cpp
  src/common/SurgeSynthesizer.cpp
  src/common/SurgeSynthesizerIO.cpp
  src/common/Tunings.cpp
  src/common/UserDefaults.cpp
  src/common/WavSupport.cpp
  libs/xml/tinyxml.cpp
  libs/xml/tinyxmlerror.cpp
  libs/xml/tinyxmlparser.cpp
  libs/strnatcmp/strnatcmp.cpp
  libs/filesystem/filesystem.cpp
)

set(SURGE_GUI_SOURCES
  src/common/gui/CAboutBox.cpp
  src/common/gui/CCursorHidingControl.cpp
  src/common/gui/CDIBitmap.cpp
  src/common/gui/CEffectSettings.cpp
  src/common/gui/CHSwitch2.cpp
  src/common/gui/CLFOGui.cpp
  src/common/gui/CModulationSourceButton.cpp
  src/common/gui/CNumberField.cpp
  src/common/gui/COscillatorDisplay.cpp
  src/common/gui/CPatchBrowser.cpp
  src/common/gui/CStatusPanel.cpp
  src/common/gui/CScalableBitmap.cpp
  src/common/gui/CSnapshotMenu.cpp
  src/common/gui/CSurgeSlider.cpp
  src/common/gui/CSurgeVuMeter.cpp
  src/common/gui/CSwitchControl.cpp
  src/common/gui/PopupEditorDialog.cpp
  src/common/gui/SurgeBitmaps.cpp
  src/common/gui/SurgeGUIEditor.cpp)

set(SURGE_COMMON_INCLUDES
  libs/
  libs/filesystem
  libs/xml
  libs/strnatcmp
  libs/nanosvg/src
  src/common
  src/common/dsp
  src/common/gui
  src/common/thread
  src/common/vt_dsp
)
