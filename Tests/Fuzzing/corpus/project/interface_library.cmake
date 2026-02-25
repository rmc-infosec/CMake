add_library(myheaderonly INTERFACE)
target_include_directories(myheaderonly INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(myheaderonly INTERFACE cxx_std_17)
target_compile_definitions(myheaderonly INTERFACE HEADER_ONLY=1)
