#include "SpecificSettings.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

template <>
void DataStreamReader::read(const SpatGRIS::SpecificSettings& n)
{
  m_stream << n.host << n.port << n.sources << static_cast<int>(n.format) << n.programs;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(SpatGRIS::SpecificSettings& n)
{
  int format = 0;
  m_stream >> n.host >> n.port >> n.sources >> format >> n.programs;
  n.format = static_cast<SpatGRIS::SpatFormat>(format);
  checkDelimiter();
}

template <>
void JSONReader::read(const SpatGRIS::SpecificSettings& n)
{
  obj["Host"] = n.host;
  obj["Port"] = n.port;
  obj["Sources"] = n.sources;
  obj["Format"] = static_cast<int>(n.format);
  obj["Programs"] = n.programs;
}

template<>
void JSONWriter::write(SpatGRIS::SpecificSettings &n)
{
    if (!obj.tryGet("Host"))
        return;
    n.host <<= obj["Host"];
    n.port <<= obj["Port"];
    n.sources <<= obj["Sources"];
    if(obj.tryGet("Format"))
    {
        int format = 0;
        format <<= obj["Format"];
        n.format = static_cast<SpatGRIS::SpatFormat>(format);
    }
    if(obj.tryGet("Programs"))
        n.programs <<= obj["Programs"];
}
