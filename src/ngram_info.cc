#include "ngram_info.h"

#include <algorithm>
#include <cstring>

namespace twittok {

bool
NgramInfo::OriginalTexts::Item::operator<(const StringRef& rhs) const
{
  // We maintain a sorted set by sorting the hashes. No need to read the
  // original string.
  return string.hash() < rhs.hash()
    || (string.hash() == rhs.hash() && string.size() < rhs.size())
    || (string.hash() == rhs.hash() && string.size() == rhs.size() && memcmp(string.data(), rhs.data(), string.size()) < 0);
}

uint32_t&
NgramInfo::OriginalTexts::operator[](const StringRef& string)
{
  auto it = std::lower_bound(values.begin(), values.end(), string);

  // Keeping the vector sorted is ~10% faster for 6.2M bios.
  // (Inserting is O(n), so our main loop is O(n^2). But if we didn't keep it
  // sorted, we'd be O(n^2) anyway.)

  if (it != values.end() && it->string == string) {
    return it->n;
  } else {
    auto new_it = values.insert(it, { string, 0 });
    return new_it->n;
  }
}

} // namespace twittok
