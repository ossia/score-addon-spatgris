#pragma once

#include "BaseProtocol.hpp"

#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_protocol_common.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

namespace Spatialization
{

struct admosc_model
{
  struct object
  {
    bool cartesian{false};
    float azim{0.f}, elev{0.f}, dist{1.f};
    float x{0.f}, y{0.f}, z{0.f};
    float width{0.f};
    float gain{1.f};
    float abs_distance{1.f};
    float ref_distance{1.f};
  };

  object objects[128];
};

class ADMOSCProtocol final : public BaseProtocol
{
public:
  explicit ADMOSCProtocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::outbound_socket_configuration& socket,
      int object_count)
      : BaseProtocol{ctx, socket}
      , m_objects{std::clamp(object_count, 1, 128)}
      , m_socket{socket, ctx->context}
  {
    m_socket.connect();
  }

  void set_device(ossia::net::device_base& dev) override
  {
    using namespace ossia::net;
    
    auto& root = dev.get_root_node();
    auto& adm_node = find_or_create_node(root, "adm");
    
    // Create object nodes for dynamic messages
    std::string obj_range = m_objects == 1 ? "/1" : fmt::format("/{{1..{}}}", m_objects);
    
    // Polar coordinates
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/azim"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/elev"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-90.f, 90.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/dist"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(1.f);
      p->set_domain(ossia::make_domain(0.f, 1.f));
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/aed"))
    {
      auto p = node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 0.f, 1.f});
      p->set_unit(ossia::aed_u{});
    }
    
    // Cartesian coordinates
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/x"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/y"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/z"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/xyz"))
    {
      auto p = node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 0.f, 0.f});
      p->set_domain(ossia::make_domain(-1.f, 1.f));
      p->set_unit(ossia::cartesian_3d_u{});
    }
    
    // Width and gain
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/w"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(0.f, 1.f));
    }
    
    for (auto* node : create_nodes(adm_node, "/obj" + obj_range + "/gain"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(1.f);
      p->set_domain(ossia::make_domain(0.f, 2.f));
    }
    
    // Static configuration
    auto& config_node = find_or_create_node(adm_node, "env");

    for (auto* node : create_nodes(config_node, "/change"))
    {
      node->create_parameter(ossia::val_type::STRING);
    }
  }

  bool push(const ossia::net::parameter_base& param, const ossia::value& v) override
  {
    using namespace ossia::net;
    using writer_type = ossia::net::socket_writer<ossia::net::udp_send_socket>;
    using send_visitor = ossia::net::osc_value_send_visitor<
        ossia::net::full_parameter_data,
        osc_extended_policy,
        writer_type>;

    const ossia::net::full_parameter_data pd;

    v.apply(
        send_visitor{
            pd, param.get_node().osc_address(), writer_type{m_socket}});

    return false;
  }
  
  bool pull(ossia::net::parameter_base&) override { return false; }
  bool push_raw(const ossia::net::full_parameter_data&) override
  {
    return false;
  }
  bool observe(ossia::net::parameter_base&, bool) override { return false; }
  bool update(ossia::net::node_base& node_base) override { return false; }

private:
  int m_objects{1};
  ossia::net::udp_send_socket m_socket;

  admosc_model m_model;
};

}
