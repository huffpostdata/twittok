#ifndef BIO_H
#define BIO_H

#include <vector>
#include <iterator>

#include "ngram.h"
#include "tokenizer.h"

namespace twittok {

/**
 * A Twitter bio.
 *
 * Beware: if the original char* backing the tokens is freed, this Bio and all
 * its return values will be invalid.
 */
class Bio {
public:
  Bio(const std::vector<Tokenizer::Token>& tokens, bool followsClinton_, bool followsTrump_);

  std::vector<Unigram> unigrams() const;
  std::vector<Bigram> bigrams() const;
  std::vector<Trigram> trigrams() const;

  bool followsClinton;
  bool followsTrump;

private:
  std::vector<Unigram> unigrams_;
};

}; // namespace twittok

#endif /* BIO_H */