cmake_minimum_required(VERSION 3.16)
project(lang-switch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(X11 REQUIRED x11)

add_library(langswitch_core
  src/layout_names.cpp
  src/popup_controller.cpp
)
target_include_directories(langswitch_core PUBLIC include)
target_compile_options(langswitch_core PRIVATE -Wall -Wextra)

add_executable(lang-switch
  src/main.cpp
  src/xkb_monitor.cpp
)
target_include_directories(lang-switch PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${SDL2_TTF_INCLUDE_DIRS}
  ${X11_INCLUDE_DIRS}
)
target_link_libraries(lang-switch PRIVATE
  langswitch_core
  ${SDL2_LIBRARIES}
  ${SDL2_TTF_LIBRARIES}
  ${X11_LIBRARIES}
)
target_compile_options(lang-switch PRIVATE -Wall -Wextra)

option(LANGSWITCH_BUILD_TESTS "Build unit tests" ON)
if(LANGSWITCH_BUILD_TESTS)
  enable_testing()
  include(FetchContent)
  set(INSTALL_GTEST OFF)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(langswitch_tests
    tests/test_layout_names.cpp
    tests/test_popup_controller.cpp
  )
  target_link_libraries(langswitch_tests PRIVATE
    langswitch_core
    GTest::gtest_main
  )
  target_compile_options(langswitch_tests PRIVATE -Wall -Wextra)

  include(GoogleTest)
  gtest_discover_tests(langswitch_tests)
endif()

# Installation (used by the .deb package).
install(TARGETS lang-switch RUNTIME DESTINATION bin)
install(FILES packaging/lang-switch.desktop
        DESTINATION /etc/xdg/autostart)
install(FILES README.md
        DESTINATION share/doc/lang-switch
        RENAME README)

set(CPACK_PACKAGE_NAME "lang-switch")
set(CPACK_PACKAGE_VERSION "1.1.1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Keyboard layout switch indicator popup for X11")
set(CPACK_PACKAGE_DESCRIPTION
  "Shows a popup with the new keyboard layout for one second in the middle
  of the bottom third of the screen whenever the layout changes. Runs
  silently in the background (no taskbar entry) and autostarts with the
  desktop session.")
set(CPACK_PACKAGE_CONTACT "Alexander <alexander@localhost>")
set(CPACK_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libc6, libstdc++6, libx11-6, libsdl2-2.0-0, libsdl2-ttf-2.0-0, fonts-dejavu-core")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
include(CPack)
