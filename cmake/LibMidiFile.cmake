# Provide the MidiFile static library target, sources are in libs/midifile
# This is not a Find module.

set(LIB_MIDIFILE_DIR "libs/midifile")
set(LIB_MIDIFILE_SRC_DIR "${LIB_MIDIFILE_DIR}/src-library/")
set(LIB_MIDIFILE_INCLUDES "${LIB_MIDIFILE_DIR}/include/")

add_library(MidiFile STATIC EXCLUDE_FROM_ALL
  ${LIB_MIDIFILE_SRC_DIR}/Binasc.cpp
  ${LIB_MIDIFILE_SRC_DIR}/MidiEvent.cpp
  ${LIB_MIDIFILE_SRC_DIR}/MidiEventList.cpp
  ${LIB_MIDIFILE_SRC_DIR}/MidiFile.cpp
  ${LIB_MIDIFILE_SRC_DIR}/MidiMessage.cpp)

target_include_directories(MidiFile
  PUBLIC ${LIB_MIDIFILE_INCLUDES})
