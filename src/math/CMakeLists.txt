add_library(math
  averaging.cpp
  broadeningfunction.cpp
  cuboid.cpp
  data1d.cpp
  data2d.cpp
  data3d.cpp
  doubleexp.cpp
  error.cpp
  extrema.cpp
  filters.cpp
  ft.cpp
  gaussfit.cpp
  gj.cpp
  histogram1d.cpp
  histogram2d.cpp
  histogram3d.cpp
  integrator.cpp
  interpolator.cpp
  mathfunc.cpp
  matrix3.cpp
  matrix4.cpp
  pairbroadeningfunction.cpp
  plottable.cpp
  poissonfit.cpp
  range.cpp
  regression.cpp
  sampleddouble.cpp
  svd.cpp
  transformer.cpp
  windowfunction.cpp

  averaging.h
  broadeningfunction.h
  constants.h
  cuboid.h
  data1d.h
  data2d.h
  data3d.h
  doubleexp.h
  error.h
  extrema.h
  filters.h
  ft.h
  functionspace.h
  gaussfit.h
  gj.h
  histogram1d.h
  histogram2d.h
  histogram3d.h
  integrator.h
  interpolator.h
  mathfunc.h
  matrix3.h
  matrix4.h
  mc.h
  minimiser.h
  pairbroadeningfunction.h
  plottable.h
  poissonfit.h
  praxis.h
  range.h
  regression.h
  sampleddouble.h
  sd.h
  svd.h
  transformer.h
  windowfunction.h
)

include_directories(
../
${CMAKE_SOURCE_DIR}
${CMAKE_BINARY_DIR}
${CMAKE_BINARY_DIR}/src
)
