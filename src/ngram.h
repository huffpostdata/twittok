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
