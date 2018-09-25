set(GOESPROC_SRCS
  area.cc
  config.cc
  filename.cc
  file_writer.cc
  goesproc.cc
  gradient.cc
  handler_emwin.cc
  handler_goesn.cc
  handler_goesr.cc
  handler_himawari8.cc
  handler_nws_image.cc
  handler_nws_text.cc
  handler_text.cc
  image.cc
  lrit_processor.cc
  options.cc
  packet_processor.cc
  string.cc
  )

find_package(PkgConfig)
pkg_check_modules(OPENCV REQUIRED opencv)
pkg_check_modules(PROJ proj)

if(PROJ_FOUND)
  list(APPEND GOESPROC_SRCS proj.cc map_drawer.cc)
endif()

add_executable(goesproc ${GOESPROC_SRCS})
add_sanitizers(goesproc)
install(TARGETS goesproc COMPONENT goestools RUNTIME DESTINATION bin)
target_link_libraries(goesproc lrit util assembler packet_reader dir)
target_link_libraries(goesproc zip)
target_link_libraries(goesproc nlohmann_json)
target_link_libraries(goesproc timer)
target_link_libraries(goesproc version)

if(OPENCV_FOUND)
  target_link_libraries(goesproc opencv_core opencv_highgui opencv_imgproc)
  if(${OPENCV_VERSION} VERSION_GREATER 3.0)
    target_link_libraries(goesproc opencv_imgcodecs)
  endif()
endif()

if(PROJ_FOUND)
  string(REPLACE "." ";" PROJ_VERSION_LIST ${PROJ_VERSION})
  list(GET PROJ_VERSION_LIST 0 PROJ_VERSION_MAJOR)
  list(GET PROJ_VERSION_LIST 1 PROJ_VERSION_MINOR)
  list(GET PROJ_VERSION_LIST 2 PROJ_VERSION_PATCH)
  target_compile_definitions(goesproc PRIVATE
    PROJ_VERSION_MAJOR=${PROJ_VERSION_MAJOR}
    PROJ_VERSION_MINOR=${PROJ_VERSION_MINOR}
    PROJ_VERSION_PATCH=${PROJ_VERSION_PATCH})
  target_compile_definitions(goesproc PRIVATE HAS_PROJ)
  target_link_libraries(goesproc proj)
endif()
