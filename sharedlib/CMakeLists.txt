# Anything that must be linked against the shared C library on Windows must
# be built in this subdirectory, because CMake doesn't allow us to override
# the compiler flags for each build type except at directory scope.  Note
# to CMake developers:  Add a COMPILE_FLAGS_<CONFIG> target property, or
# better yet, provide a friendly way of configuring a Windows target to use the
# static C library.

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)

if(MSVC_LIKE)
  # Build all configurations against shared C library
  if(CMAKE_VERSION VERSION_EQUAL "3.15" OR
    CMAKE_VERSION VERSION_GREATER "3.15")
    if(CMAKE_BUILD_TYPE_UC STREQUAL "DEBUG")
      set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDebugDLL)
    elseif(MSVC_IDE)
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
      set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    endif()
    message(STATUS "Visual C++ run-time library for libjpeg API DLL: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
  elseif(MSVC)
    foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      if(${var} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${var} "${${var}}")
      endif()
    endforeach()
  endif()
endif()

foreach(src ${JPEG_SOURCES})
  set(JPEG_SRCS ${JPEG_SRCS} ../${src})
endforeach()

if(WITH_SIMD AND (MSVC_IDE OR XCODE))
  # This tells CMake that the "source" files haven't been generated yet
  set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
endif()

if(WIN32)
  set(DEFFILE ../win/jpeg${SO_MAJOR_VERSION}.def)
endif()
if(MSVC_LIKE)
  configure_file(${CMAKE_SOURCE_DIR}/win/jpeg.rc.in
    ${CMAKE_BINARY_DIR}/win/jpeg.rc)
  set(JPEG_SRCS ${JPEG_SRCS} ${CMAKE_BINARY_DIR}/win/jpeg.rc)
endif()
add_library(jpeg SHARED ${JPEG_SRCS} ${DEFFILE} ${SIMD_TARGET_OBJECTS}
  ${SIMD_OBJS} $<TARGET_OBJECTS:jpeg12> $<TARGET_OBJECTS:jpeg16>)
if(UNIX)
  target_link_libraries(jpeg m)
endif()

set_target_properties(jpeg PROPERTIES SOVERSION ${SO_MAJOR_VERSION}
  VERSION ${SO_MAJOR_VERSION}.${SO_AGE}.${SO_MINOR_VERSION})
if(APPLE AND (NOT CMAKE_OSX_DEPLOYMENT_TARGET OR
              CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER 10.4))
  if(NOT CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG)
    set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
  endif()
  set_target_properties(jpeg PROPERTIES MACOSX_RPATH 1)
endif()
if(MAPFLAG)
  set_target_properties(jpeg PROPERTIES
    LINK_FLAGS "${MAPFLAG}${CMAKE_CURRENT_BINARY_DIR}/../libjpeg.map")
endif()
if(MSVC_LIKE)
  set_target_properties(jpeg PROPERTIES
    RUNTIME_OUTPUT_NAME jpeg${SO_MAJOR_VERSION})
  # The jsimd_*.c file is built to use libcmt or libcmtd, so this prevents a
  # linker warning.
  if(MSVC)
    set_target_properties(jpeg PROPERTIES LINK_FLAGS
      "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:LIBCMTD")
  else()
    set_target_properties(jpeg PROPERTIES LINK_FLAGS
      "-Wl,-nodefaultlib:libcmt -Wl,-nodefaultlib:libcmtd")
  endif()
elseif(MINGW)
  set_target_properties(jpeg PROPERTIES SUFFIX -${SO_MAJOR_VERSION}.dll)
endif()

if(WIN32)
  set(USE_SETMODE "-DUSE_SETMODE")
endif()
set(CDJPEG_COMPILE_FLAGS
  "-DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED -DTARGA_SUPPORTED ${USE_SETMODE}")

# Compile a separate version of these source files with 12-bit and 16-bit data
# precision.
add_library(cjpeg12 OBJECT ../rdgif.c ../rdppm.c)
set_property(TARGET cjpeg12 PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=12 -DGIF_SUPPORTED -DPPM_SUPPORTED")
add_library(cjpeg16 OBJECT ../rdgif.c ../rdppm.c)
set_property(TARGET cjpeg16 PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=16 -DGIF_SUPPORTED -DPPM_SUPPORTED")
add_executable(cjpeg ../cjpeg.c ../cdjpeg.c ../rdbmp.c ../rdgif.c ../rdppm.c
  ../rdjpeg.c
  ../rdswitch.c ../rdtarga.c $<TARGET_OBJECTS:cjpeg12>
  $<TARGET_OBJECTS:cjpeg16>)
set_property(TARGET cjpeg PROPERTY COMPILE_FLAGS ${CDJPEG_COMPILE_FLAGS})
target_link_libraries(cjpeg jpeg)

# Compile a separate version of these source files with 12-bit and 16-bit data
# precision.
add_library(djpeg12 OBJECT ../rdcolmap.c ../wrgif.c ../wrppm.c)
set_property(TARGET djpeg12 PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=12 -DGIF_SUPPORTED -DPPM_SUPPORTED")
add_library(djpeg16 OBJECT ../wrppm.c)
set_property(TARGET djpeg16 PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=16 -DPPM_SUPPORTED")

if(PNG_SUPPORTED)
  report_option(PNG_SUPPORTED "PNG reading support")
  set(COMPILE_FLAGS "${COMPILE_FLAGS} -DPNG_SUPPORTED")
  set(CJPEG_BMP_SOURCES ${CJPEG_BMP_SOURCES} ../rdpng.c)
endif()

if(PNG_SUPPORTED)
  # to avoid finding static library from CMake cache
  unset(PNG_LIBRARY CACHE)
  unset(PNG_LIBRARY_RELEASE CACHE)
  unset(PNG_LIBRARY_DEBUG CACHE)
  unset(ZLIB_LIBRARY CACHE)
  unset(ZLIB_LIBRARY_RELEASE CACHE)
  unset(ZLIB_LIBRARY_DEBUG CACHE)

  find_package(PNG 1.6 REQUIRED)
  find_package(ZLIB REQUIRED)
  target_include_directories(cjpeg PUBLIC ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
  target_link_libraries(cjpeg ${PNG_LIBRARY} ${ZLIB_LIBRARY})
endif()

add_executable(djpeg ../djpeg.c ../cdjpeg.c ../rdcolmap.c ../rdswitch.c
  ../wrbmp.c ../wrgif.c ../wrppm.c ../wrtarga.c $<TARGET_OBJECTS:djpeg12>
  $<TARGET_OBJECTS:djpeg16>)
set_property(TARGET djpeg PROPERTY COMPILE_FLAGS ${CDJPEG_COMPILE_FLAGS})
target_link_libraries(djpeg jpeg)

add_executable(jpegtran ../jpegtran.c ../cdjpeg.c ../rdswitch.c ../transupp.c)
target_link_libraries(jpegtran jpeg)
set_property(TARGET jpegtran PROPERTY COMPILE_FLAGS "${USE_SETMODE}")

add_executable(example ../example.c)
target_link_libraries(example jpeg)

add_executable(jcstest ../jcstest.c)
target_link_libraries(jcstest jpeg)

install(TARGETS jpeg EXPORT ${CMAKE_PROJECT_NAME}Targets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin)
install(TARGETS cjpeg djpeg jpegtran
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin)
if(NOT CMAKE_VERSION VERSION_LESS "3.1" AND MSVC_LIKE AND
  CMAKE_C_LINKER_SUPPORTS_PDB)
  install(FILES "$<TARGET_PDB_FILE:jpeg>"
    DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin OPTIONAL)
endif()
