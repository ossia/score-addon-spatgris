#include "SpecificSettings.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

template <>
void DataStreamReader::read(const SpatGRIS::SpecificSettings& n)
{
  m_stream << n.host << n.port << n.inputPort << n.sources << static_cast<int>(n.format)
           << n.programs << n.sourceOffset;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(SpatGRIS::SpecificSettings& n)
{
  int format = 0;
  m_stream >> n.host >> n.port >> n.inputPort >> n.sources >> format >> n.programs;
  n.format = static_cast<SpatGRIS::SpatFormat>(format);

  // Settings saved without a source offset have the delimiter here instead.
  int32_t next = 0;
  m_stream >> next;
  if(next == int32_t(0xDEADBEEF))
    return;

  n.sourceOffset = next;
  checkDelimiter();
}

template <>
void JSONReader::read(const SpatGRIS::SpecificSettings& n)
{
  obj["Host"] = n.host;
  obj["Port"] = n.port;
  obj["InputPort"] = n.inputPort;
  obj["Sources"] = n.sources;
  obj["Format"] = static_cast<int>(n.format);
  obj["Programs"] = n.programs;
  obj["SourceOffset"] = n.sourceOffset;
}

template<>
void JSONWriter::write(SpatGRIS::SpecificSettings &n)
{
    if (!obj.tryGet("Host"))
        return;
    n.host <<= obj["Host"];
    n.port <<= obj["Port"];
    if(obj.tryGet("InputPort"))
        n.inputPort <<= obj["InputPort"];
    n.sources <<= obj["Sources"];
    if(obj.tryGet("Format"))
    {
        int format = 0;
        format <<= obj["Format"];
        n.format = static_cast<SpatGRIS::SpatFormat>(format);
    }
    if(obj.tryGet("Programs"))
        n.programs <<= obj["Programs"];
    if(obj.tryGet("SourceOffset"))
        n.sourceOffset <<= obj["SourceOffset"];
}
