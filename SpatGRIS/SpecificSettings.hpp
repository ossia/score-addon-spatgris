#pragma once
#include <score/tools/std/StringHash.hpp>

#include <verdigris>

namespace SpatGRIS
{
enum class SpatFormat
{
  SpatGRIS,
  ADMOSC,
  SPAT
};

struct SpecificSettings
{
  QString host = "127.0.0.1";
  int port{18032};
  int sources{16};
  SpatFormat format{SpatFormat::SpatGRIS};
  int programs{1}; // For ADM-OSC
};
}

Q_DECLARE_METATYPE(SpatGRIS::SpecificSettings)
W_REGISTER_ARGTYPE(SpatGRIS::SpecificSettings)
Q_DECLARE_METATYPE(SpatGRIS::SpatFormat)
W_REGISTER_ARGTYPE(SpatGRIS::SpatFormat)
