#include "Device.hpp"

#include "Protocol.hpp"
#include "SpecificSettings.hpp"

#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>

#include <score/document/DocumentContext.hpp>

#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>

#include <QDebug>

#include <wobjectimpl.h>

W_OBJECT_IMPL(SpatGRIS::DeviceImplementation)

namespace SpatGRIS
{
DeviceImplementation::DeviceImplementation(
    const Device::DeviceSettings& settings,
    const Explorer::DeviceDocumentPlugin& plugin,
    const score::DocumentContext&)
    : OwningDeviceInterface{settings}
    , m_ctx{plugin}
{
  m_capas.canRefreshTree = true;
  m_capas.canAddNode = false;
  m_capas.canRemoveNode = false;
  m_capas.canRenameNode = false;
  m_capas.canSetProperties = false;
  m_capas.canSerialize = false;
}

DeviceImplementation::~DeviceImplementation() { }

bool DeviceImplementation::reconnect()
{
  disconnect();

  const auto& set
      = m_settings.deviceSpecificSettings.value<SpecificSettings>();
  try
  {
    auto socket = ossia::net::socket_configuration{
        .host = set.host.toStdString(),
        .port = (uint16_t)set.port,
        .broadcast = false};

    auto protocol = std::make_unique<ossia::spatgris_protocol>(
        this->m_ctx.networkContext(), socket, set.sources);
    auto dev = std::make_unique<ossia::net::generic_device>(
        std::move(protocol), settings().name.toStdString());

    m_dev = std::move(dev);
    deviceChanged(nullptr, m_dev.get());
  }
  catch (const std::runtime_error& e)
  {
    qDebug() << "SpatGRIS error: " << e.what();
  }
  catch (...)
  {
    qDebug() << "SpatGRIS error";
  }

  return connected();
}

void DeviceImplementation::disconnect()
{
  OwningDeviceInterface::disconnect();
}
}
