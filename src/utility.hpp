#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace TinyCDN {

export "C" {
  struct ChunkedCursor {
    std::size_t bufferSize;
    std::size_t seekPosition;
    std::string_view buffer;
    std::optional<std::ofstream> handle;

    inline ChunkedCursor(std::size_t bufferSize,
                         std::size_t seekPosition,
                         std::ofstream handle)
      : bufferSize(bufferSize),
        seekPosition(seekPosition),
        //        buffer(buffer),
        handle(std::make_optional<std::ofstream>(handle))
   }
}

constexpr std::size_t operator""_kB(unsigned long long v) {
  return 1024u * v;
}

constexpr std::size_t operator""_mB(unsigned long long v) {
  return 1048576u * v;
}

constexpr std::size_t operator""_gB(unsigned long long v) {
  return 1073741824 * v;
}

struct Size {
  // Immutable
  const uintmax_t size;
  inline Size(uintmax_t size) : size(size) {}

  const Size operator+(Size& S2) const {
    return Size{this->size + S2.size};
  }
  const Size operator-(Size& S2) const {
    return Size{this->size - S2.size};
  }
  const Size operator*(Size& S2) const {
    return Size{this->size * S2.size};
  }
  const Size operator/(Size& S2) const {
    return Size{this->size / S2.size};
  }
  bool operator>(const Size& S2) const {
    return this->size > S2.size;
  }
  bool operator>=(const Size& S2) const {
    return this->size >= S2.size;
  }

  inline Size (const Size& oldSize) : size(oldSize.size) {
  }
};

/*! Takes a container and converts to a comma-separated string
 * Treats a comma'd string value as a container of string-convertible values.
 * Overload for specific types to create k-tuples from stored text
 * This is used to convert structures into a format that can be created at run-time by reading from a file.
 */
template <typename t>
inline std::string asCSV(t container) {
  std::string csv;

  // TODO optimize
  if (container.size() == 1) {
    csv.append(container[0]);
    return csv;
  }

  for (auto elem : container) {
    // TODO don't cast here, use a "statusfield"
    csv.append(static_cast<std::string>(elem));
    csv.append(",");
  }
  return csv;
};

/*! Takes a comma-separated string and outputs a vector of the string's values
 */
std::vector<std::string> fromCSV(std::string csv);
}
