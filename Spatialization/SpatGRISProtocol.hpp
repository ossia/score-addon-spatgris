#pragma once

#include "BaseProtocol.hpp"

#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_protocol_common.hpp>
#include <ossia/network/osc/detail/osc_packet_processor.hpp>
#include <ossia/network/osc/detail/osc_fwd.hpp>
#include <ossia/protocols/osc/osc_generic_protocol.hpp>

namespace Spatialization
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

class SpatGRISProtocol final : public BaseProtocol
{
public:
  explicit SpatGRISProtocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::outbound_socket_configuration& socket,
      int source_count,
      int input_port = 0)
      : BaseProtocol{ctx, socket}
      , m_sources{std::clamp(source_count, 1, 256)}
      , m_input_port{input_port}
      , m_socket{socket, ctx->context}
  {
    m_socket.connect();
  }

  ~SpatGRISProtocol() { stop_receive(); }

  void set_device(ossia::net::device_base& dev) override
  {
    using namespace ossia::net;
    
    m_device = &dev;
    std::string root = m_sources == 1 ? "/1" : fmt::format("/{{1..{}}}", m_sources);
    
    for (auto* node : create_nodes(dev.get_root_node(), root + "/clear"))
    {
      node->create_parameter(ossia::val_type::IMPULSE);
    }

    for (auto* node : create_nodes(dev.get_root_node(), root + "/algorithm"))
    {
      auto p = node->create_parameter(ossia::val_type::STRING);
      p->set_value("dome");
      p->set_domain(ossia::make_domain(std::vector<std::string>{"dome", "cube"}));
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
      send_visitor{param, "/spat/serv", writer_type{m_socket}}(
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
      send_visitor{param, "/spat/serv", writer_type{m_socket}}(
          std::vector<ossia::value>{"clr", it->second + 1});
    };

    auto send_alg = [&]
    {
      send_visitor{param, "/spat/serv", writer_type{m_socket}}(
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
      // Log error but don't fail - bidirectional communication is optional
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
    // For SpatGRIS, we need to handle the special message format
    // Messages come as /spat/serv with arguments like ["car", index, x, y, z, hspan, vspan]
    
    std::string addr = msg.AddressPattern();
    
    if (addr == "/spat/serv")
    {
      // Parse SpatGRIS-specific messages
      handle_spatgris_message(msg);
    }
    else
    {
      // Handle standard OSC messages (e.g., /1/position, /2/algorithm, etc.)
      ossia::net::on_input_message<false>(
          msg.AddressPattern(),
          ossia::net::osc_message_applier{
              ossia::net::message_origin_identifier{*this, {}}, msg},
          m_listening,
          *m_device,
          m_logger);
    }
  }
  
  void handle_spatgris_message(const oscpack::ReceivedMessage& msg)
  {
    if (!m_device)
      return;
#if 0
    try
    {
      auto it = msg.ArgumentsBegin();
      if (it == msg.ArgumentsEnd())
        return;
        
      // First argument should be the command type
      if (!it->IsString())
        return;
        
      std::string cmd = it->AsString();
      ++it;
      
      if (cmd == "car" && it != msg.ArgumentsEnd())
      {
        // Format: ["car", index, x, y, z, hspan, vspan]
        int index = it->AsInt32();
        if (index < 1 || index > m_sources)
          return;
          
        auto* source_node = m_device->find_child(std::to_string(index));
        if (!source_node)
          return;
          
        ++it;
        if (it != msg.ArgumentsEnd())
        {
          float x = it->AsFloat();
          ++it;
          float y = (it != msg.ArgumentsEnd()) ? it->AsFloat() : 0.f;
          ++it;
          float z = (it != msg.ArgumentsEnd()) ? it->AsFloat() : 0.f;
          
          // Update position parameter
          auto* pos_node = source_node->find_child("position");
          if (pos_node && pos_node->get_parameter())
          {
            pos_node->get_parameter()->set_value(ossia::vec3f{x, y, z});
          }
          
          // Update model
          auto& source = m_model.sources[index - 1];
          source.x = x;
          source.y = y;
          source.z = z;
          
          // Handle hspan and vspan if present
          ++it;
          if (it != msg.ArgumentsEnd())
          {
            float hspan = it->AsFloat();
            auto* hspan_node = source_node->find_child("hspan");
            if (hspan_node && hspan_node->get_parameter())
            {
              hspan_node->get_parameter()->set_value(hspan);
            }
            source.hspan = hspan;
            
            ++it;
            if (it != msg.ArgumentsEnd())
            {
              float vspan = it->AsFloat();
              auto* vspan_node = source_node->find_child("vspan");
              if (vspan_node && vspan_node->get_parameter())
              {
                vspan_node->get_parameter()->set_value(vspan);
              }
              source.vspan = vspan;
            }
          }
        }
      }
      else if (cmd == "alg" && it != msg.ArgumentsEnd())
      {
        // Format: ["alg", "dome" or "cube"]
        std::string algo = it->AsString();
        
        // Update all sources' algorithm
        for (int i = 0; i < m_sources; ++i)
        {
          auto* source_node = m_device->find_child(std::to_string(i + 1));
          if (source_node)
          {
            auto* algo_node = source_node->find_child("algorithm");
            if (algo_node && algo_node->get_parameter())
            {
              algo_node->get_parameter()->set_value(algo);
            }
          }
          m_model.sources[i].algo = (algo == "dome") ? 0 : 1;
        }
      }
      else if (cmd == "clr" && it != msg.ArgumentsEnd())
      {
        // Format: ["clr", index]
        int index = it->AsInt32();
        if (index < 1 || index > m_sources)
          return;
          
        // Clear/reset the source
        auto& source = m_model.sources[index - 1];
        source.x = 0.f;
        source.y = 0.f;
        source.z = 0.f;
        source.hspan = 0.f;
        source.vspan = 0.f;
        
        auto* source_node = m_device->find_child(std::to_string(index));
        if (source_node)
        {
          auto* pos_node = source_node->find_child("position");
          if (pos_node && pos_node->get_parameter())
          {
            pos_node->get_parameter()->set_value(ossia::vec3f{0.f, 0.f, 0.f});
          }
        }
      }
    }
    catch (const oscpack::Exception& e)
    {
      // Log parse error
    }
#endif
  }

  int m_sources{0};
  int m_input_port{0};
  ossia::net::udp_send_socket m_socket;
  std::unique_ptr<ossia::net::udp_receive_socket> m_receive_socket;

  spatgris_model m_model;
  ossia::hash_map<ossia::net::node_base*, int> m_cache;
  
  ossia::net::device_base* m_device{nullptr};
  ossia::net::listened_parameters m_listening;
};

}
