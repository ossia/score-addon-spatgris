#pragma once

#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/sockets/udp_socket.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

namespace Spatialization
{

enum class SpatFormat
{
  SpatGRIS,
  ADMOSC,
  SPAT
};

class BaseProtocol : public ossia::net::protocol_base
{
public:
  BaseProtocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::outbound_socket_configuration& socket)
      : ossia::net::protocol_base{flags{}}
      , m_ctx{ctx}
      , m_socket_config{socket}
  {
  }

  virtual ~BaseProtocol() = default;

protected:
  ossia::net::network_context_ptr m_ctx;
  ossia::net::outbound_socket_configuration m_socket_config;
};

}
