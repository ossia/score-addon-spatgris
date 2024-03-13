#pragma once
#include <score/tools/std/StringHash.hpp>

#include <verdigris>

namespace SpatGRIS
{
struct SpecificSettings
{
  QString host = "127.0.0.1";
  int port{18032};
  int sources{128};
};
}

Q_DECLARE_METATYPE(SpatGRIS::SpecificSettings)
W_REGISTER_ARGTYPE(SpatGRIS::SpecificSettings)
