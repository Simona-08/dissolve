# Main Module Library
add_library(epsrmodule
  core.cpp
  definition.cpp
  functions.cpp
  io.cpp
  init.cpp
  process.cpp

  epsr.h
)

# 'nogui' library
add_library(epsrmodulenogui
  nogui.cpp
)

# gui library
if(GUI)
  add_subdirectory(gui)
endif(GUI)

include_directories(
../../
${CMAKE_SOURCE_DIR}
${CMAKE_BINARY_DIR}
${CMAKE_BINARY_DIR}/src
)
