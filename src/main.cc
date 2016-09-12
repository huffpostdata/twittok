#include "tokenizer.h"

#include <array>
#include <iostream>
#include <string>

#include "bio.h"
#include "ngram.h"
#include "sparsepp.h"
#include "porter2_stemmer.h"

#define MAX_LINE_SIZE 1024

namespace twittok {

struct NgramInfo {
  size_t nClinton;
  size_t nTrump;
  size_t nBoth; // for the number of clinton and NOT trump, use nClinton - nBoth
  spp::sparse_hash_map<std::string, int32_t> originalTexts;
};

template<int N>
class NgramInfoTable {
public:
  void add(const Ngram<N>& ngram, bool followsClinton, bool followsTrump) {
    gramToInfo[ngram.grams[0]].add(ngram.cdr(), followsClinton, followsTrump);
  }

  spp::sparse_hash_map<std::string, NgramInfoTable<N - 1> > gramToInfo;
};

template<>
class NgramInfoTable<1> {
public:
  void add(const Unigram& unigram, bool followsClinton, bool followsTrump) {
    NgramInfo& info = gramToInfo[unigram.grams[0]];
    if (followsClinton) info.nClinton++;
    if (followsTrump) info.nTrump++;
    if (followsClinton && followsTrump) info.nBoth++;
    ++info.originalTexts[std::string(unigram.originalUtf8, unigram.originalLength)];
  }

  spp::sparse_hash_map<std::string, NgramInfo> gramToInfo;
};

typedef NgramInfoTable<1> UnigramInfoTable;
typedef NgramInfoTable<2> BigramInfoTable;
typedef NgramInfoTable<3> TrigramInfoTable;

}; // namespace twittok

namespace {

template<int N>
std::ostream& operator<<(std::ostream& os, const twittok::NgramInfoTable<N>& table)
{
  for (const auto& pair : table.gramToInfo) {
    os << pair.first << "(" << pair.second << ")";
  }

  return os;
}

template<>
std::ostream& operator<<(std::ostream& os, const twittok::UnigramInfoTable& table)
{
  for (const auto& pair : table.gramToInfo) {
    const auto& info = pair.second;
    os << pair.first << "(c:" << info.nClinton << ",t:" << info.nTrump << ",b:" << info.nBoth << ",";

    for (const auto& innerPair : info.originalTexts) {
      os << "[" << innerPair.first << ":" << innerPair.second << "]";
    }

    os << ")";
  }

  return os;
}

}; // namespace ""

int
main(int argc, char** argv) {
  twittok::Tokenizer tokenizer;

  twittok::UnigramInfoTable unigramInfoTable;
  twittok::BigramInfoTable bigramInfoTable;
  twittok::TrigramInfoTable trigramInfoTable;

  for (std::string line; std::getline(std::cin, line);) {
    re2::StringPiece bioLine(line);

    std::vector<re2::StringPiece> tokens = tokenizer.tokenize(bioLine);
    twittok::Bio bio(tokens, 1, 1);

    auto unigrams = bio.unigrams();
    std::for_each(unigrams.cbegin(), unigrams.cend(), [bio, &unigramInfoTable](auto const& unigram) { unigramInfoTable.add(unigram, bio.followsClinton, bio.followsTrump); });

    auto bigrams = bio.bigrams();
    std::for_each(bigrams.cbegin(), bigrams.cend(), [bio, &bigramInfoTable](auto const& bigram) { bigramInfoTable.add(bigram, bio.followsClinton, bio.followsTrump); });

    auto trigrams = bio.trigrams();
    std::for_each(trigrams.cbegin(), trigrams.cend(), [bio, &trigramInfoTable](auto const& trigram) { trigramInfoTable.add(trigram, bio.followsClinton, bio.followsTrump); });
  }

  std::cout << unigramInfoTable << std::endl;
  std::cout << bigramInfoTable << std::endl;
  std::cout << trigramInfoTable << std::endl;
}
