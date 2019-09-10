
if(DEFINED VSTGUI_INCLUDED)
  return()
endif()

set(VSTGUI_INCLUDED 1)
set(VSTGUI_SOURCE_DIR "${VSTGUI_DIR}/vstgui")

if(UNIX AND NOT APPLE)
  include(FindPkgConfig)
  find_package(Freetype REQUIRED)
  pkg_check_modules(FONTCONFIG "fontconfig")
  pkg_check_modules(CAIRO "cairo")
  pkg_check_modules(XLIBS "xkbcommon-x11 xcb-cursor xcb-keysyms xcb-xkb xcb-util x11")
endif()

if(WIN32)
  set(VSTGUI_SOURCES
    "${VSTGUI_SOURCE_DIR}/vstgui_win32.cpp"
    "${VSTGUI_SOURCE_DIR}/vstgui_uidescription_win32.cpp")
endif()

if(APPLE)
  set(VSTGUI_SOURCES
    "${VSTGUI_SOURCE_DIR}/vstgui_mac.mm"
    "${VSTGUI_SOURCE_DIR}/vstgui_uidescription_mac.mm")
endif()

if(UNIX AND NOT APPLE)
  file(GLOB VSTGUI_SOURCES
    "${VSTGUI_SOURCE_DIR}/vstgui.cpp"
    "${VSTGUI_SOURCE_DIR}/lib/platform/linux/*.cpp"
    "${VSTGUI_SOURCE_DIR}/lib/platform/common/genericoptionmenu.cpp"
    "${VSTGUI_SOURCE_DIR}/lib/platform/common/generictextedit.cpp")
endif()

list(APPEND VSTGUI_SOURCES
  "${VSTGUI_SOURCE_DIR}/plugin-bindings/plugguieditor.cpp")

add_library(vstgui STATIC EXCLUDE_FROM_ALL
  ${VSTGUI_SOURCES})
target_include_directories(vstgui PUBLIC
  "${VSTGUI_DIR}")

if(UNIX AND NOT APPLE)
  target_include_directories(vstgui PUBLIC
    ${CAIRO_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS} ${XLIBS_INCLUDE_DIRS})
  target_link_libraries(vstgui PUBLIC
    ${CAIRO_LIBRARIES} ${FREETYPE_LIBRARIES} ${FONTCONFIG_LIBRARIES} ${XLIBS_LIBRARIES})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "^GNU|Clang$")
  target_compile_options(vstgui PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-multichar>)
endif()
