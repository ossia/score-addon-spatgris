#pragma once

#include "BaseProtocol.hpp"

#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_fwd.hpp>
#include <ossia/network/osc/detail/osc_packet_processor.hpp>
#include <ossia/network/osc/detail/osc_protocol_common.hpp>
#include <ossia/network/sockets/udp_socket.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

#include <boost/asio/steady_timer.hpp>

namespace Spatialization
{

class ADMOSCProtocol final : public BaseProtocol
{
public:
  explicit ADMOSCProtocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::outbound_socket_configuration& socket,
      int object_count,
      int input_port = 0)
      : BaseProtocol{ctx, socket}
      , m_objects(object_count)
      , m_input_port{input_port}
      , m_send_socket{socket, ctx->context}
      , m_timer{ctx->context}
  {
    m_send_socket.connect();
  }

  ~ADMOSCProtocol() { stop_receive(); }

  void set_device(ossia::net::device_base& dev) override
  {
    using namespace ossia::net;
    
    m_device = &dev;
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
    
    // Listener messages for head tracking and 6DOF
    auto& lis_node = find_or_create_node(adm_node, "lis");
    
    // Orientation (yaw, pitch, roll)
    auto* ypr_node = lis_node.create_child("ypr");
    if (ypr_node)
    {
      auto p = ypr_node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 0.f, 0.f});
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    // Individual orientation parameters
    auto* yaw_node = lis_node.create_child("yaw");
    if (yaw_node)
    {
      auto p = yaw_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    auto* pitch_node = lis_node.create_child("pitch");
    if (pitch_node)
    {
      auto p = pitch_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    auto* roll_node = lis_node.create_child("roll");
    if (roll_node)
    {
      auto p = roll_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    // Position (x, y, z)
    auto* xyz_node = lis_node.create_child("xyz");
    if (xyz_node)
    {
      auto p = xyz_node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 0.f, 0.f});
      p->set_domain(ossia::make_domain(-1.f, 1.f));
      p->set_unit(ossia::cartesian_3d_u{});
    }
    
    // Individual position parameters
    auto* x_node = lis_node.create_child("x");
    if (x_node)
    {
      auto p = x_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }
    
    auto* y_node = lis_node.create_child("y");
    if (y_node)
    {
      auto p = y_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }
    
    auto* z_node = lis_node.create_child("z");
    if (z_node)
    {
      auto p = z_node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-1.f, 1.f));
    }

    // Set up input socket if port is specified
    if (m_input_port > 0)
    {
      setup_receive_socket();
    }
  }

  bool push(const ossia::net::parameter_base& param, const ossia::value& v) override
  {
    using namespace ossia::net;
    using writer_type = ossia::net::socket_writer<ossia::net::udp_send_socket>;
    using send_visitor = ossia::net::osc_value_send_visitor<
        ossia::net::parameter_base,
        osc_extended_policy,
        writer_type>;

    v.apply(
        send_visitor{
            param,
            param.get_node().osc_address(),
            writer_type{m_send_socket}});

    return false;
  }
  
  bool pull(ossia::net::parameter_base& param) override 
  { 
    if (m_input_port <= 0)
      return false;
    
    // Send query message (OSC message without arguments)
    using namespace ossia::net;
    using writer_type = ossia::net::socket_writer<ossia::net::udp_send_socket>;
    using send_visitor = ossia::net::osc_value_send_visitor<
        ossia::net::full_parameter_data,
        osc_extended_policy,
        writer_type>;
    
    const ossia::net::full_parameter_data pd;
    std::string osc_addr = param.get_node().osc_address();

#if 0
    // Send empty message as query
    ossia::net::osc_message_applier::osc_message msg;
    msg.address = osc_addr;
    // No arguments = query
    
    writer_type writer{m_send_socket};
    ossia::net::osc_packet_write_visitor<writer_type> packet_writer{writer};
    packet_writer(msg);

#endif
    return true;
  }
  
  bool push_raw(const ossia::net::full_parameter_data&) override
  {
    return false;
  }

  bool observe(ossia::net::parameter_base& address, bool enable) override
  {
    if (enable)
      m_listening.insert(
          std::make_pair(address.get_node().osc_address(), &address));
    else
      m_listening.erase(address.get_node().osc_address());
    return true;
  }
  bool update(ossia::net::node_base& node_base) override { return false; }

private:
  void setup_receive_socket()
  {
    try 
    {
      m_receive_socket = std::make_unique<ossia::net::udp_receive_socket>(
          ossia::net::inbound_socket_configuration{
              .port = static_cast<uint16_t>(m_input_port)
          },
          m_ctx->context);
      
      m_receive_socket->open();
      m_receive_socket->receive(
          [this](const char* data, std::size_t sz)
          {
            if (!m_device)
              return;
            auto on_message
                = [this](auto&& msg) { this->on_received_message(msg); };
            using processor
                = ossia::net::osc_packet_processor<decltype(on_message)>;
            processor{on_message}(data, sz);
          });
    }
    catch (const std::exception& e)
    {
    }
  }
  
  void stop_receive()
  {
    if (m_receive_socket)
    {
      m_receive_socket->close();
      m_receive_socket.reset();
    }
  }

  void on_received_message(const oscpack::ReceivedMessage& msg)
  {
    ossia::net::on_input_message<false>(
        msg.AddressPattern(),
        ossia::net::osc_message_applier{
            ossia::net::message_origin_identifier{*this, {}}, msg},
        m_listening,
        *m_device,
        m_logger);
  }

  int m_objects{1};
  int m_input_port{0};
  ossia::net::udp_send_socket m_send_socket;
  std::unique_ptr<ossia::net::udp_receive_socket> m_receive_socket;
  boost::asio::steady_timer m_timer;

  ossia::net::device_base* m_device{nullptr};

  ossia::net::listened_parameters m_listening;
};

}
