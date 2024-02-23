#pragma once
#include <score/tools/std/StringHash.hpp>

#include <verdigris>

namespace SpatGRIS
{
struct SpecificSettings
{
  int control{1234};
};
}

Q_DECLARE_METATYPE(SpatGRIS::SpecificSettings)
W_REGISTER_ARGTYPE(SpatGRIS::SpecificSettings)