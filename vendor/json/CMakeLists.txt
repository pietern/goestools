project(nlohmann_json LANGUAGES CXX)
add_library(nlohmann_json INTERFACE)
target_include_directories(
  nlohmann_json
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
