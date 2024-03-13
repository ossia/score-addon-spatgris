#pragma once

#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_protocol_common.hpp>
#include <ossia/network/sockets/udp_socket.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

namespace ossia
{

struct spatgris_model
{
  struct source
  {
    int algo{};
    float x{}, y{}, z{};
    float hspan{}, vspan{};
  };

  source sources[256];
};

class spatgris_protocol final : public ossia::net::protocol_base
{
public:
  explicit spatgris_protocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::socket_configuration& socket,
      int source_count)
      : ossia::net::protocol_base{flags{}}
      , m_sources{std::clamp(source_count, 1, 256)}
      , m_socket{socket, ctx->context}
  {
    m_socket.connect();
  }

  void set_device(ossia::net::device_base& dev) override
  {
    using namespace ossia::net;
    std::string root
        = m_sources == 1 ? "/1" : fmt::format("/{{1..{}}}", m_sources);
    for (auto* node : create_nodes(dev.get_root_node(), root + "/clear"))
    {
      node->create_parameter(ossia::val_type::IMPULSE);
    }

    for (auto* node : create_nodes(dev.get_root_node(), root + "/algorithm"))
    {
      auto p = node->create_parameter(ossia::val_type::STRING);
      p->set_value("dome");
      p->set_domain(
          ossia::make_domain(std::vector<std::string>{"dome", "cube"}));
    }

    for (auto* node : create_nodes(dev.get_root_node(), root + "/position"))
    {
      auto p = node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{});
      p->set_unit(ossia::cartesian_3d_u{});
      p->set_domain(ossia::make_domain(-1.66f, 1.66f));
    }

    for (auto* node : create_nodes(dev.get_root_node(), root + "/hspan"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(0.f, 1.f));
    }

    for (auto* node : create_nodes(dev.get_root_node(), root + "/vspan"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(0.f, 1.f));
    }

    for (auto& cld : dev.get_root_node().children())
    {
      int i = std::stoi(cld->get_name());
      this->m_cache[cld.get()] = i - 1;
    }
  }

  bool
  push(const ossia::net::parameter_base& param, const ossia::value& v) override
  {
    using namespace ossia::net;
    using writer_type = ossia::net::socket_writer<ossia::net::udp_send_socket>;
    using send_visitor = ossia::net::osc_value_send_visitor<
        ossia::net::full_parameter_data,
        osc_extended_policy,
        writer_type>;

    const ossia::net::full_parameter_data pd;

    // 1. Update our internal data model
    // Locate the index of the source from the name of the parent
    // e.g. /12/foo -> source at index 11
    auto source_node = param.get_node().get_parent();
    if (!source_node)
      return false;
    auto it = m_cache.find(source_node);
    if (it == m_cache.end())
      return false;

    auto& source = this->m_model.sources[it->second];
    auto send_car = [&]
    {
      send_visitor{pd, "/spat/serv", writer_type{m_socket}}(
          std::vector<ossia::value>{
              "car",
              it->second + 1,
              source.x,
              source.y,
              source.z,
              source.hspan,
              source.vspan});
    };

    auto send_clear = [&]
    {
      send_visitor{pd, "/spat/serv", writer_type{m_socket}}(
          std::vector<ossia::value>{"clr", it->second + 1});
    };

    auto send_alg = [&]
    {
      send_visitor{pd, "/spat/serv", writer_type{m_socket}}(
          std::vector<ossia::value>{
              "alg", source.algo == 0 ? "dome" : "cube"});
    };

    std::string_view node = param.get_node().get_name();
    if (node == "clear")
    {
      send_clear();
    }
    else if (node == "algorithm")
    {
      auto val = ossia::convert<std::string>(v);
      if (val == "dome")
        source.algo = 0;
      else
        source.algo = 1;
      send_alg();
    }
    else if (node == "position")
    {
      auto val = ossia::convert<ossia::vec3f>(v);
      source.x = val[0];
      source.y = val[1];
      source.z = val[2];
      send_car();
    }
    else if (node == "hspan")
    {
      auto val = ossia::convert<float>(v);
      source.hspan = val;
      send_car();
    }
    else if (node == "vspan")
    {
      auto val = ossia::convert<float>(v);
      source.vspan = val;
      send_car();
    }

    return false;
  }
  bool pull(ossia::net::parameter_base&) override { return false; }

  bool push_raw(const net::full_parameter_data&) override { return false; }
  bool observe(net::parameter_base&, bool) override { return false; }
  bool update(net::node_base& node_base) override { return false; }

private:
  int m_sources{0};
  ossia::net::udp_send_socket m_socket;

  spatgris_model m_model;
  ossia::hash_map<ossia::net::node_base*, int> m_cache;
};
}
