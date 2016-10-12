#ifndef NGRAM_INFO_H
#define NGRAM_INFO_H

#include <cstdint>
#include <vector>

#include "string_ref.h"

namespace twittok {

struct NgramInfo {
  struct OriginalTexts {
    // For a given token, we store all mappings back to the original in a
    // vector. *gasp*, you say, a _vector_? That means O(N^2) operations? Yes.
    //
    // But here's what's great about a vector: insertions use contiguous RAM,
    // and comparisons almost always use the same RAM because we rarely (never?)
    // have compare the original string's bytes (the hash and length are good
    // enough).
    //
    // Bonus: a vector uses half the RAM of a map.

    struct Item {
      StringRef string;
      uint32_t n;
      bool operator<(const StringRef& rhs) const;
    };

    uint32_t& operator[](const StringRef& string);

    std::vector<Item> values;
  }; // struct OriginalTexts

  size_t nClinton;
  size_t nTrump;
  size_t nBoth; // for the number of clinton and NOT trump, use nClinton - nBoth
  OriginalTexts originalTexts;

  inline size_t nTotal() const { return nClinton + nTrump - nBoth; }
  inline size_t nVariants() const { return originalTexts.values.size(); }
};

} // namespace twittok

#endif /* NGRAM_INFO_H */
