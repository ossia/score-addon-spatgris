#pragma once

#include "BaseProtocol.hpp"

#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_protocol_common.hpp>

namespace Spatialization
{

struct spat_model
{
  struct source
  {
    // General
    bool selected{false};
    std::string name;
    bool enabled{true};
    bool muted{false};
    bool solo{false};
    bool inverted_phase{false};
    float gain{0.f};
    float lfe{-144.5f};
    float lfe2{-144.5f};
    float lfe3{-144.5f};
    float lfe4{-144.5f};
    std::string color;
    
    // Perceptual factors
    float presence{100.f};
    float warmth{30.f};
    float brillance{30.f};
    float room_presence{48.f};
    float running_reverberance{34.f};
    float envelopment{25.f};
    
    // Reverb
    bool reverb_enabled{true};
    bool early{true};
    bool cluster{true};
    bool tail{true};
    
    // Position
    float x{0.f}, y{2.f}, z{0.f};
    
    // Radiation
    bool relative_direction{true};
    float azim{0.f}, elev{0.f}, dist{2.f};
    float yaw{0.f}, pitch{0.f}, roll{0.f};
    float aperture{80.f};
    
    // Spreading
    float spread{0.f};
    float nneig{100.f};
    
    // Options
    float early_width{10.f};
    float radius{2.f};
    bool doppler{false};
    bool air_absorption{true};
    float drop_value{6.f};
    bool drop_mode{1}; // 0: linear, 1: logarithmic
    bool coordinate_mode{0}; // 0: polar, 1: cartesian
    bool z_coordinate_mode{0}; // 0: polar, 1: cartesian
    float pan_rev{0.f};
    
    // Axis EQ
    float axis_g0{0.f}, axis_gl{0.f}, axis_gm{0.f}, axis_gh{0.f};
    float axis_fl{177.f}, axis_fh{5657.f};
    
    // Omni EQ  
    float omni_g0{0.f}, omni_gl{1.7f}, omni_gm{0.f}, omni_gh{-3.8f};
    float omni_fl{177.f}, omni_fh{5657.f};
    
    // Barycentric (for multi-channel)
    float rotx{0.f}, roty{0.f}, rotz{0.f};
    float scale{1.f};
  };
  
  struct room
  {
    bool enabled{true};
    bool muted{false};
    bool armed{false};
    float gain{0.f};
    
    // Reverb
    bool reverb_density{0}; // 0: standard, 1: high
    float size{2500.f};
    bool reverb_enabled{true};
    bool tail_enabled{true};
    float reverb_start{81.f};
    float reverb_gain{0.f};
    float reverb_factor{1.f};
    
    // Perceptual factors
    float reverberance{65.f};
    float heaviness{25.f};
    float liveness{35.f};
    
    // Room response
    float early_min{24.2f}, early_max{40.f};
    float early_dist{0.5f}, early_shape{0.5f};
    float cluster_min{24.2f}, cluster_max{40.f};
    float cluster_dist{0.5f};
    
    // Options
    bool infinite{false};
    bool air_enabled{true};
    float modal_density{1.f};
    float air_freq{10000.f};
    
    // Crossover
    float reverb_fl{177.f}, reverb_fh{5657.f};
    
    // Listener position
    float listener_x{0.f}, listener_y{0.f}, listener_z{0.f};
    float listener_yaw{0.f}, listener_pitch{0.f}, listener_roll{0.f};
  };
  
  source sources[256];
  room rooms[16];
};

class SPATProtocol final : public BaseProtocol
{
public:
  explicit SPATProtocol(
      const ossia::net::network_context_ptr& ctx,
      const ossia::net::outbound_socket_configuration& socket,
      int source_count,
      int room_count = 1)
      : BaseProtocol{ctx, socket}
      , m_sources{std::clamp(source_count, 1, 256)}
      , m_rooms{std::clamp(room_count, 1, 16)}
      , m_socket{socket, ctx->context}
  {
    m_socket.connect();
  }

  void set_device(ossia::net::device_base& dev) override
  {
    using namespace ossia::net;
    
    // Create source nodes
    std::string src_range = m_sources == 1 ? "/1" : fmt::format("/{{1..{}}}", m_sources);
    
    // General parameters
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/select"))
    {
      auto p = node->create_parameter(ossia::val_type::BOOL);
      p->set_value(false);
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/name"))
    {
      node->create_parameter(ossia::val_type::STRING);
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/enable"))
    {
      auto p = node->create_parameter(ossia::val_type::BOOL);
      p->set_value(true);
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/mute"))
    {
      auto p = node->create_parameter(ossia::val_type::BOOL);
      p->set_value(false);
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/solo"))
    {
      auto p = node->create_parameter(ossia::val_type::BOOL);
      p->set_value(false);
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/gain"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-144.5f, 24.f));
      p->set_unit(ossia::decibel_u{});
    }
    
    // Position parameters (Cartesian)
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/x"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-100.f, 100.f));
      p->set_unit(ossia::meter_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/y"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(2.f);
      p->set_domain(ossia::make_domain(-100.f, 100.f));
      p->set_unit(ossia::meter_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/z"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-100.f, 100.f));
      p->set_unit(ossia::meter_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/xyz"))
    {
      auto p = node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 2.f, 0.f});
      p->set_unit(ossia::cartesian_3d_u{});
    }
    
    // Radiation parameters (Polar)
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/azim"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/elev"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-90.f, 90.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/dist"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(2.f);
      p->set_domain(ossia::make_domain(0.01f, 100.f));
      p->set_unit(ossia::meter_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/aed"))
    {
      auto p = node->create_parameter(ossia::val_type::VEC3F);
      p->set_value(ossia::vec3f{0.f, 0.f, 2.f});
    }
    
    // Orientation
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/yaw"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-180.f, 180.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/pitch"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-90.f, 90.f));
      p->set_unit(ossia::degree_u{});
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/roll"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(-90.f, 90.f));
      p->set_unit(ossia::degree_u{});
    }
    
    // Perceptual factors
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/pres"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(100.f);
      p->set_domain(ossia::make_domain(0.f, 120.f));
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/warmth"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(30.f);
      p->set_domain(ossia::make_domain(0.f, 60.f));
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/bril"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(30.f);
      p->set_domain(ossia::make_domain(0.f, 60.f));
    }
    
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/prer"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(48.f);
      p->set_domain(ossia::make_domain(0.f, 120.f));
    }
    
    // Spreading
    for (auto* node : create_nodes(dev.get_root_node(), "/source" + src_range + "/spread"))
    {
      auto p = node->create_parameter(ossia::val_type::FLOAT);
      p->set_value(0.f);
      p->set_domain(ossia::make_domain(0.f, 100.f));
    }
    
    // Room nodes if needed
    if (m_rooms > 0)
    {
      std::string room_range = m_rooms == 1 ? "/1" : fmt::format("/{{1..{}}}", m_rooms);
      
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/enable"))
      {
        auto p = node->create_parameter(ossia::val_type::BOOL);
        p->set_value(true);
      }
      
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/gain"))
      {
        auto p = node->create_parameter(ossia::val_type::FLOAT);
        p->set_value(0.f);
        p->set_domain(ossia::make_domain(-144.5f, 24.f));
        p->set_unit(ossia::decibel_u{});
      }
      
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/size"))
      {
        auto p = node->create_parameter(ossia::val_type::FLOAT);
        p->set_value(2500.f);
        p->set_domain(ossia::make_domain(10.f, 15000.f));
      }
      
      // Listener position
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/x"))
      {
        auto p = node->create_parameter(ossia::val_type::FLOAT);
        p->set_value(0.f);
        p->set_domain(ossia::make_domain(-100.f, 100.f));
        p->set_unit(ossia::meter_u{});
      }
      
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/y"))
      {
        auto p = node->create_parameter(ossia::val_type::FLOAT);
        p->set_value(0.f);
        p->set_domain(ossia::make_domain(-100.f, 100.f));
        p->set_unit(ossia::meter_u{});
      }
      
      for (auto* node : create_nodes(dev.get_root_node(), "/room" + room_range + "/z"))
      {
        auto p = node->create_parameter(ossia::val_type::FLOAT);
        p->set_value(0.f);
        p->set_domain(ossia::make_domain(-100.f, 100.f));
        p->set_unit(ossia::meter_u{});
      }
    }
    
    // Cache indices
    build_cache(dev.get_root_node());
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

    // Build OSC address from parameter path

    // Send the OSC message
    v.apply(
        send_visitor{
            pd, param.get_node().osc_address(), writer_type{m_socket}});

    // Update internal model
    update_model(param, v);
    
    return false;
  }
  
  bool pull(ossia::net::parameter_base&) override { return false; }
  bool push_raw(const ossia::net::full_parameter_data&) override { return false; }
  bool observe(ossia::net::parameter_base&, bool) override { return false; }
  bool update(ossia::net::node_base& node_base) override { return false; }

private:
  void build_cache(ossia::net::node_base& root)
  {
    using namespace std::literals;
    // Cache source nodes
    if (auto* src_parent = root.find_child("source"sv))
    {
      for (auto& src : src_parent->children())
      {
        int idx = std::stoi(src->get_name()) - 1;
        m_source_cache[src.get()] = idx;
      }
    }
    
    // Cache room nodes
    if (auto* room_parent = root.find_child("room"sv))
    {
      for (auto& room : room_parent->children())
      {
        int idx = std::stoi(room->get_name()) - 1;
        m_room_cache[room.get()] = idx;
      }
    }
  }

  void update_model(const ossia::net::parameter_base& param, const ossia::value& v)
  {
    // Parse parameter path and update internal model
    auto* node = &param.get_node();
    auto* parent = node->get_parent();
    if (!parent)
      return;
    
    // Check if it's a source parameter
    auto src_it = m_source_cache.find(parent);
    if (src_it != m_source_cache.end())
    {
      auto& source = m_model.sources[src_it->second];
      auto name = node->get_name();
      
      // Update source parameters based on name
      if (name == "x")
        source.x = ossia::convert<float>(v);
      else if (name == "y")
        source.y = ossia::convert<float>(v);
      else if (name == "z")
        source.z = ossia::convert<float>(v);
      else if (name == "xyz")
      {
        auto vec = ossia::convert<ossia::vec3f>(v);
        source.x = vec[0];
        source.y = vec[1];
        source.z = vec[2];
      }
      else if (name == "azim")
        source.azim = ossia::convert<float>(v);
      else if (name == "elev")
        source.elev = ossia::convert<float>(v);
      else if (name == "dist")
        source.dist = ossia::convert<float>(v);
      else if (name == "aed")
      {
        auto vec = ossia::convert<ossia::vec3f>(v);
        source.azim = vec[0];
        source.elev = vec[1];
        source.dist = vec[2];
      }
      else if (name == "gain")
        source.gain = ossia::convert<float>(v);
      else if (name == "spread")
        source.spread = ossia::convert<float>(v);
      // Add more parameter mappings as needed
    }
    
    // Check if it's a room parameter
    auto room_it = m_room_cache.find(parent);
    if (room_it != m_room_cache.end())
    {
      auto& room = m_model.rooms[room_it->second];
      auto name = node->get_name();
      
      if (name == "gain")
        room.gain = ossia::convert<float>(v);
      else if (name == "size")
        room.size = ossia::convert<float>(v);
      // Add more room parameter mappings as needed
    }
  }
  
  int m_sources{1};
  int m_rooms{1};
  ossia::net::udp_send_socket m_socket;
  
  spat_model m_model;
  ossia::hash_map<ossia::net::node_base*, int> m_source_cache;
  ossia::hash_map<ossia::net::node_base*, int> m_room_cache;
};

}
