cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

if(NOT TARGET score_lib_base)
  include("${SCORE_SOURCE_DIR}/cmake/ScoreExternalAddon.cmake")
endif()

project(score_addon_spatgris LANGUAGES CXX)

# General initialization
score_common_setup()

# Creation of the library
add_library(score_addon_spatgris
  SpatGRIS/Device.cpp
  SpatGRIS/Device.hpp
  SpatGRIS/ProtocolFactory.cpp
  SpatGRIS/ProtocolFactory.hpp
  SpatGRIS/ProtocolSettingsWidget.cpp
  SpatGRIS/ProtocolSettingsWidget.hpp
  SpatGRIS/SpecificSettings.hpp
  SpatGRIS/SpecificSettingsSerialization.cpp

  score_addon_spatgris.hpp
  score_addon_spatgris.cpp
)

# Link
target_link_libraries(score_addon_spatgris PUBLIC score_plugin_engine)

# Target-specific options
setup_score_plugin(score_addon_spatgris)
