#pragma once

#include "../Spatialization/SpatGRISProtocol.hpp"
#include "../Spatialization/ADMOSCProtocol.hpp"
#include "../Spatialization/SPATProtocol.hpp"

// For backward compatibility, expose the old names in ossia namespace
namespace ossia
{
using spatgris_protocol = Spatialization::SpatGRISProtocol;
using spatgris_model = Spatialization::spatgris_model;
}