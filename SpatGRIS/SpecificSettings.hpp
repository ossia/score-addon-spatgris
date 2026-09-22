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
  int inputPort{0}; // 0 means no input port
  int sources{16};
  SpatFormat format{SpatFormat::SpatGRIS};
  int programs{1}; // For ADM-OSC

  //! Index of the first source on the wire: source n of this device is
  //! transmitted as source n + sourceOffset. The device tree keeps numbering
  //! its sources from 1.
  int sourceOffset{0};
};
}

Q_DECLARE_METATYPE(SpatGRIS::SpecificSettings)
W_REGISTER_ARGTYPE(SpatGRIS::SpecificSettings)
Q_DECLARE_METATYPE(SpatGRIS::SpatFormat)
W_REGISTER_ARGTYPE(SpatGRIS::SpatFormat)
