#include "score_addon_spatgris.hpp"

#include <score/plugins/FactorySetup.hpp>

#include <SpatGRIS/ProtocolFactory.hpp>

score_addon_spatgris::score_addon_spatgris() { }

score_addon_spatgris::~score_addon_spatgris() { }

std::vector<score::InterfaceBase*>
score_addon_spatgris::factories(
    const score::ApplicationContext& ctx,
    const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext,
      FW<Device::ProtocolFactory, SpatGRIS::ProtocolFactory>>(ctx, key);
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_spatgris)
