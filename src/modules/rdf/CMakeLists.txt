# Main Module Library
add_library(rdfmodule
  core.cpp
  definition.cpp
  functions.cpp
  init.cpp
  process.cpp

  rdf.h
)

# 'nogui' library
add_library(rdfmodulenogui
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
