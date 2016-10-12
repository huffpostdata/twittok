#ifndef BIO_H
#define BIO_H

#include <vector>
#include <iterator>

#include "ngram.h"
#include "tokenizer.h"
#include "untokenized_bio.h"

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
  static Bio buildByTokenizing(const UntokenizedBio& untokenizedBio, const Tokenizer& tokenizer);

  template<size_t N> std::vector<Ngram<N> > ngrams() const;

  bool followsClinton;
  bool followsTrump;

private:
  std::vector<Unigram> unigrams_;
};

}; // namespace twittok

#endif /* BIO_H */
