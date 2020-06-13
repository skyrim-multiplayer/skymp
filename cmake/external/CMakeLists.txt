# See _external.cmake

cmake_minimum_required(VERSION 3.12)
project(viet_raknet_external LANGUAGES CXX)

include(ExternalProject)

ExternalProject_Add(CrabNet
  PREFIX ${CMAKE_BINARY_DIR}/CrabNet
  GIT_REPOSITORY https://github.com/TES3MP/CrabNet.git
  GIT_TAG ab1306050fe0f5b0f9c4f56893a79e56a9459567
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
  LOG_INSTALL ON
)
