#ifndef NGRAM_PASS_H
#define NGRAM_PASS_H

#include <forward_list>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "bio.h"
#include "ngram_info.h"

namespace twittok {

/**
 * Given ngrams of length N, tallies ngrams of length N+1.
 */
template<size_t N>
class NgramPass {
public:
  NgramPass(const std::unordered_set<std::string>& prefixes)
    : prefixes(prefixes)
  {
  }

  void scanBios(const std::forward_list<Bio>& bios);
  void dump(std::ostream& os, size_t minCount) const;
  std::unordered_set<std::string> ngramStrings(size_t minCount) const;

  std::unordered_set<std::string> prefixes; // calculated in previous pass
  std::unordered_map<std::string, NgramInfo> gramToInfo; // calculated this pass
};

} // namespace twittok

#endif /* NGRAM_PASS_H */
