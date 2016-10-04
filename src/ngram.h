#ifndef NGRAM_H
#define NGRAM_H

#include <algorithm>
#include <array>
#include <string>

#include "string_ref.h"

namespace twittok {

template<int N>
class Ngram {
public:
  std::array<std::string, N> grams;
  StringRef original;

  std::string gramsString() const {
    if (N == 1) return grams[0];

    size_t len = N - 1; // spaces
    for (size_t i = 0; i < N; i++) {
      len += grams[i].size();
    }

    std::string ret(len, ' '); // spaces so we don't need to write them in between
    size_t pos = 0;
    for (size_t i = 0; i < N; i++) {
      ret.replace(pos, grams[i].size(), grams[i]);
      pos += grams[i].size() + 1;
    }

    return ret;
  }

  /**
   * Returns whether LHS grams == RHS grams.
   *
   * Even if this returns true, the original strings may be different.
   */
  bool operator==(const Ngram<N>& rhs) const {
    for (size_t i = 0; i < N; i++) {
      if (grams[i] != rhs.grams[i]) return false;
    }
    return true;
  }

  /**
   * Returns whether LHS grams < RHS grams.
   */
  bool operator<(const Ngram<N>& rhs) const {
    for (size_t i = 0; i < N; i++) {
      int c = grams[i].compare(rhs.grams[i]);
      if (c < 0) return true;
      if (c > 0) return false;
    }

    return false;
  }
};

typedef Ngram<1> Unigram;
typedef Ngram<2> Bigram;
typedef Ngram<3> Trigram;

}; // namespace twittok

#endif /* NGRAM_H */
