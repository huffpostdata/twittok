#include "string_ref.h"

#include <cstring>

#include <city.h>

namespace twittok {

StringRef::StringRef(const char* str, size_t len)
  : str_(str)
  , len_(len)
  , hash_(CityHash64(str, len))
{
}

bool
StringRef::contains(char c) const
{
  return memchr(str_, c, len_) != NULL;
}

bool
StringRef::operator==(const StringRef& rhs) const
{
  return hash_ == rhs.hash_ && len_ == rhs.len_ && memcmp(str_, rhs.str_, len_) == 0;
}

} // namespace twittok
