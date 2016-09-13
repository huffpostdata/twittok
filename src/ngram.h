#ifndef NGRAM_H
#define NGRAM_H

#include <algorithm>
#include <array>
#include <string>

namespace twittok {

template<int N>
class Ngram {
public:
  std::array<std::string, N> grams;
  const char* originalUtf8;
  size_t originalLength;

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

  std::string originalString() const { return std::string(originalUtf8, originalLength); }

  /**
   * Returns an Ngram containing all but the first token in this ngram.
   *
   * ("cdr" is from LISP: https://en.wikipedia.org/wiki/CAR_and_CDR)
   */
  Ngram<N - 1> cdr() const {
    std::array<std::string, N - 1> cdrGrams;
    std::copy(grams.cbegin() + 1, grams.cend(), cdrGrams.begin());

    return {
      cdrGrams,
      originalUtf8,
      originalLength
    };
  }

  /**
   * Returns whether LHS grams == RHS grams.
   */
  bool operator==(const Ngram<N>& rhs) const {
    for (size_t i = 0; i < N; i++) {
      if (grams[i] != rhs.grams[i]) return false;
    }
    return true;
  }
};

typedef Ngram<1> Unigram;
typedef Ngram<2> Bigram;
typedef Ngram<3> Trigram;

}; // namespace twittok

#endif /* NGRAM_H */
