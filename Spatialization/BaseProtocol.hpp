#pragma once

#include <ossia/detail/hash_map.hpp>
#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/sockets/udp_socket.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

#include <charconv>
#include <string_view>

namespace Spatialization
{

//! Rewrites the index in an address of the shape <prefix><n>/<rest> so that
//! source n leaves as source n + offset; anything else goes out untouched.
inline std::string
offset_source_address(const std::string& addr, std::string_view prefix, int offset)
{
  if(offset == 0 || !std::string_view{addr}.starts_with(prefix))
    return addr;

  const std::string_view rest = std::string_view{addr}.substr(prefix.size());
  const auto slash = rest.find('/');
  const std::string_view num = rest.substr(0, slash);

  int index{};
  const auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), index);
  if(ec != std::errc{} || ptr != num.data() + num.size())
    return addr;

  return fmt::format(
      "{}{}{}", prefix, index + offset,
      slash == std::string_view::npos ? std::string_view{} : rest.substr(slash));
}

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
  //! The address every parameter's value goes out on, resolved once when the
  //! tree is built: the push path must not build a string per message.
  void build_address_cache(
      ossia::net::node_base& root, std::string_view prefix, int offset)
  {
    m_addresses.clear();
    ossia::net::iterate_all_children(
        &root, [&](ossia::net::parameter_base& p) {
      auto& node = p.get_node();
      m_addresses[&node] = offset_source_address(node.osc_address(), prefix, offset);
    });
  }

  const std::string*
  cached_address(const ossia::net::node_base& node) const noexcept
  {
    auto it = m_addresses.find(&node);
    return it != m_addresses.end() ? &it->second : nullptr;
  }

  ossia::net::network_context_ptr m_ctx;
  ossia::net::outbound_socket_configuration m_socket_config;
  ossia::hash_map<const ossia::net::node_base*, std::string> m_addresses;
};

}
