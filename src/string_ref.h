#ifndef STRING_REF_H
#define STRING_REF_H

#include <cstdlib>
#include <string>

namespace twittok {

class StringRef {
public:
  StringRef() : str_(0), len_(0), hash_(0) {}
  StringRef(const char* str, size_t len);

  bool operator==(const StringRef& rhs) const;
  bool contains(char c) const;
  std::string to_string() const { return std::string(str_, len_); }
  inline size_t hash() const { return hash_; }
  inline const char* data() const { return str_; }
  inline size_t size() const { return len_; }

private:
  const char* str_;
  size_t len_;
  size_t hash_;
};

} // namespace twittok

#endif /* STRING_REF_H */
