#include "SpecificSettings.hpp"

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

template <>
void DataStreamReader::read(const SpatGRIS::SpecificSettings& n)
{
  m_stream << n.control;
  insertDelimiter();
}

template <>
void DataStreamWriter::write(SpatGRIS::SpecificSettings& n)
{
  m_stream >> n.control;
  checkDelimiter();
}

template <>
void JSONReader::read(const SpatGRIS::SpecificSettings& n)
{
  obj["Control"] = n.control;
}

template <>
void JSONWriter::write(SpatGRIS::SpecificSettings& n)
{
  n.control <<= obj["Control"];
}