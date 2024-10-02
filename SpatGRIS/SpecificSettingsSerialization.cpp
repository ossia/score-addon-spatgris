#include "SpecificSettings.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

template <>
void DataStreamReader::read(const SpatGRIS::SpecificSettings& n)
{
  m_stream << n.host << n.port << n.sources;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(SpatGRIS::SpecificSettings& n)
{
  m_stream >> n.host >> n.port >> n.sources;
  checkDelimiter();
}

template <>
void JSONReader::read(const SpatGRIS::SpecificSettings& n)
{
  obj["Host"] = n.host;
  obj["Port"] = n.port;
  obj["Sources"] = n.sources;
}

template<>
void JSONWriter::write(SpatGRIS::SpecificSettings &n)
{
    if (!obj.tryGet("Host"))
        return;
    n.host <<= obj["Host"];
    n.port <<= obj["Port"];
    n.sources <<= obj["Sources"];
}
