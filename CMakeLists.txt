﻿# CMakeList.txt : CMake project for jpegrip, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("jpegrip")

# Add source to this project's executable.
add_executable (jpegrip "main.c" "jpegrip.c" "jpegrip.h" "log.c" "log.h" "jpeg.c" "jpeg.h")
add_executable (jpeghdr "jpeghdr.c" "jpeg.c" "jpeg.h" "log.c" "log.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  # set_property(TARGET jpegrip PROPERTY CXX_STANDARD 20)
  set_property(TARGET jpegrip PROPERTY C_STANDARD 90)
endif()

# TODO: Add tests and install targets if needed.
