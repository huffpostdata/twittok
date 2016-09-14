#include "bio.h"

#include <algorithm>
#include <iostream>

#include "ngram.h"
#include "tokenizer.h"
#include "stemmer.h"

namespace {

static const size_t MaxStemmedStringBytes = twittok::stemmer::MaxBytesToStem * 4; // hand-wavy...

twittok::Unigram
tokenToUnigram(const twittok::Tokenizer::Token& token)
{
  std::string stemmed = twittok::stemmer::stem(token.data(), token.size());

  return {
    { stemmed },
    twittok::StringRef(token.data(), token.size())
  };
};

template<int N>
std::vector<twittok::Ngram<N> >
unigramsToNgrams(const std::vector<twittok::Unigram>& unigrams)
{
  if (unigrams.size() < N) return std::vector<twittok::Ngram<N> >();

  size_t size = unigrams.size() - N + 1;
  auto ngrams = std::vector<twittok::Ngram<N> >(size);

  for (size_t i = 0; i < size; i++) {
    twittok::Ngram<N>& ngram(ngrams[i]); // initialized to zero

    for (size_t j = 0; j < N; j++) {
      ngram.grams[j] = unigrams[i + j].grams[0];
    }

    const twittok::Unigram& beginUnigram(unigrams[i]);
    const twittok::Unigram& endUnigram(unigrams[i + N - 1]);

    ngram.original = twittok::StringRef(
      beginUnigram.original.data(),
      endUnigram.original.data() - beginUnigram.original.data() + endUnigram.original.size()
    );
  }

  return ngrams;
}

}; // namespace ""

namespace twittok {

Bio::Bio(const std::vector<Tokenizer::Token>& tokens, bool followsClinton_, bool followsTrump_)
  : followsClinton(followsClinton_)
  , followsTrump(followsTrump_)
{
  unigrams_.reserve(tokens.size());
  for (const auto& token : tokens) {
    Unigram unigram = tokenToUnigram(token);
    if (!unigram.grams[0].empty()) {
      unigrams_.push_back(unigram);
    }
  }
}

Bio
Bio::buildByTokenizing(const UntokenizedBio& untokenizedBio, const Tokenizer& tokenizer)
{
  const re2::StringPiece str(untokenizedBio.utf8);
  const auto tokens = tokenizer.tokenize(str);
  return Bio(tokens, untokenizedBio.followsClinton, untokenizedBio.followsTrump);
}

std::vector<Unigram>
Bio::unigrams() const
{
  std::vector<Unigram> ret(unigrams_);
  auto new_end = std::unique(ret.begin(), ret.end());
  ret.resize(std::distance(ret.begin(), new_end));
  return ret;
}

std::vector<Bigram>
Bio::bigrams() const
{
  std::vector<Bigram> ret(unigramsToNgrams<2>(unigrams_));
  auto new_end = std::unique(ret.begin(), ret.end());
  ret.resize(std::distance(ret.begin(), new_end));
  return ret;
}

std::vector<Trigram>
Bio::trigrams() const
{
  std::vector<Trigram> ret(unigramsToNgrams<3>(unigrams_));
  auto new_end = std::unique(ret.begin(), ret.end());
  ret.resize(std::distance(ret.begin(), new_end));
  return ret;
}

}; // namespace twittok
