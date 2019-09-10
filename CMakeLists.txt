cmake_minimum_required(VERSION 3.10)
project(Surge VERSION 1.0.0 LANGUAGES CXX ASM)
#set(CMAKE_BUILD_TYPE Debug)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED FALSE)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_ASM_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

include(cmake/Surge.cmake)
include(cmake/LibMidiFile.cmake)

option(BUILD_HEADLESS "Build headless" ON)
option(BUILD_LV2 "Build LV2" ON)
set(VSTGUI_DIR "vstgui.surge" CACHE STRING "Root directory of VSTGUI sources")

if(BUILD_HEADLESS)
  add_surge_standalone(surge-headless
    src/headless/main.cpp
    src/headless/UserInteractionsHeadless.cpp
    src/headless/LinkFixesHeadless.cpp
    src/headless/HeadlessUtils.cpp
    src/headless/Player.cpp
    src/headless/Stress.cpp)
  target_link_libraries(surge-headless PRIVATE
    MidiFile)
  target_compile_definitions(surge-headless PRIVATE
    TARGET_HEADLESS=1
    LIBMIDIFILE=1)
  target_include_directories(surge-headless PRIVATE
    src/headless)

  find_package(LibSndFile)
  if(NOT LIBSNDFILE_FOUND)
    message(WARNING "LibSndFile not installed; building without wav support")
    message(WARNING "You can 'brew install libsndfile' or 'apt-get install libsndfile1-dev'")
  else()
    target_compile_definitions(surge-headless PRIVATE LIBSNDFILE=1)
    target_link_libraries(surge-headless PRIVATE ${LIBSNDFILE_LIBRARIES})
    target_include_directories(surge-headless PRIVATE ${LIBSNDFILE_INCLUDE_DIRS})
  endif()
endif()

if(BUILD_LV2)
  include(cmake/Vstgui.cmake)

  add_surge_plugin(surge-lv2
    src/lv2/SurgeLv2Export.cpp
    src/lv2/SurgeLv2Ui.cpp
    src/lv2/SurgeLv2Util.cpp
    src/lv2/SurgeLv2Vstgui.cpp
    src/lv2/SurgeLv2Wrapper.cpp)
  add_surge_ui(surge-lv2)
  target_compile_definitions(surge-lv2 PRIVATE
    TARGET_LV2=1)
  target_include_directories(surge-lv2 PRIVATE
    src/lv2
    libs/lv2)
  set_target_properties(surge-lv2 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "lv2/Surge.lv2")

  add_custom_command(TARGET surge-lv2 POST_BUILD
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/linux/generate-lv2-ttl.py
    $<TARGET_FILE:surge-lv2>)
endif()
