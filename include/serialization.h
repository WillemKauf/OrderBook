#ifndef SERIALIZATION_H
#define SERIALIZATION_H

/////////////////
/// std
/////////////////
#include <sstream>

/////////////////
/// cereal
/////////////////
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/array.hpp"
#include "cereal/types/chrono.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/optional.hpp"
#include "cereal/types/utility.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/vector.hpp"

template <typename T>
auto SerializeObject(const T& object) {
  std::ostringstream oss;
  {
    cereal::BinaryOutputArchive archive_o(oss);
    archive_o << object;
  }

  return oss.str();
}

template <typename Buf_T, typename T>
void DeserializeObject(T& object, const Buf_T& buffer) {
  auto stringBuffer = std::string(buffer, sizeof(T));
  std::istringstream iss(stringBuffer);
  cereal::BinaryInputArchive archive_i(iss);
  archive_i >> object;
}

#endif  // #ifndef SERIALIZATION_H
