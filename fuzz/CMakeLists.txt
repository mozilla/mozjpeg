if(NOT WITH_TURBOJPEG)
  message(FATAL_ERROR "Fuzz targets require the TurboJPEG API library.")
endif()

set(FUZZ_BINDIR "" CACHE PATH
  "Directory into which fuzz targets should be installed")
if(NOT FUZZ_BINDIR)
  message(FATAL_ERROR "FUZZ_BINDIR must be specified.")
endif()
message(STATUS "FUZZ_BINDIR = ${FUZZ_BINDIR}")

set(FUZZ_LIBRARY "" CACHE STRING
  "Path to fuzzer library or flags necessary to link with it")
if(NOT FUZZ_LIBRARY)
  message(FATAL_ERROR "FUZZ_LIBRARY must be specified.")
endif()
message(STATUS "FUZZ_LIBRARY = ${FUZZ_LIBRARY}")

enable_language(CXX)

set(EFFECTIVE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UC}}")
message(STATUS "C++ Compiler flags = ${EFFECTIVE_CXX_FLAGS}")

macro(add_fuzz_target target source_file)
  add_executable(${target}_fuzzer ${source_file})
  if(NOT ENABLE_SHARED)
    target_link_libraries(${target}_fuzzer ${FUZZ_LIBRARY} turbojpeg-static)
  else()
    target_link_libraries(${target}_fuzzer ${FUZZ_LIBRARY} turbojpeg)
  endif()
  install(TARGETS ${target}_fuzzer RUNTIME DESTINATION ${FUZZ_BINDIR})
endmacro()

# NOTE: This target is named libjpeg_turbo_fuzzer instead of decompress_fuzzer
# in order to preserve the corpora from Google's OSS-Fuzz target for
# libjpeg-turbo, which this target replaces.
add_fuzz_target(libjpeg_turbo decompress.cc)

add_fuzz_target(decompress_yuv decompress_yuv.cc)

add_fuzz_target(transform transform.cc)
