#include "tokenizer.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "bio.h"
#include "csv_bio_reader.h"
#include "untokenized_bio.h"
#include "ngram.h"
#include "sparsepp.h"

#define MAX_LINE_SIZE 1024

namespace {
  typedef spp::sparse_hash_map<std::string, int32_t> OriginalMapping;
} // namespace

namespace twittok {

struct NgramInfo {
  size_t nClinton;
  size_t nTrump;
  size_t nBoth; // for the number of clinton and NOT trump, use nClinton - nBoth

  size_t nTotal() const {
    return nClinton + nTrump - nBoth;
  }

  OriginalMapping originalTexts;
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

static const int MinMappingCount = 3; // Below this, we lump mappings to "Other" ... and if even "Other" doesn't have this, we nix the mapping

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

void
dumpMappings(std::ostream& os, const OriginalMapping& originalMapping)
{
  size_t nOther = 0;
  for (const auto& m : originalMapping) {
    const auto& original = m.first;
    const size_t count = m.second;
    if (count < MinMappingCount || original.find('\n') != std::string::npos || original.find('\t') != std::string::npos) {
      nOther += count;
    } else {
      os << "\t" << count << "\t" << original << "\n";
    }
  }
  if (nOther != 0) {
    os << "\t" << nOther << "\t\n";
  }
}

}; // namespace ""

int
main(int argc, char** argv) {
  twittok::Tokenizer tokenizer;

  twittok::UnigramInfoTable unigramInfoTable;
  twittok::BigramInfoTable bigramInfoTable;
  twittok::TrigramInfoTable trigramInfoTable;
  size_t nClinton = 0;
  size_t nTrump = 0;
  size_t nBoth = 0;
  size_t nClintonWithBio = 0;
  size_t nTrumpWithBio = 0;
  size_t nBothWithBio = 0;

  unigramInfoTable.gramToInfo.reserve(10000000);
  bigramInfoTable.gramToInfo.reserve(10000000);
  trigramInfoTable.gramToInfo.reserve(10000000);

  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " DATA.csv OUT-TOKENS.tsv OUT-MAPPINGS.tsv" << std::endl;
    exit(1);
  }

  twittok::CsvBioReader reader(argv[1]);

  while (true) {
    twittok::CsvBioReader::Error error;
    twittok::UntokenizedBio untokenizedBio(reader.nextBio(&error));

    if (error == twittok::CsvBioReader::Error::EndOfInput) {
      std::cerr << "Finished reading CSV" << std::endl;
      break;
    }

    if (error != twittok::CsvBioReader::Error::Success) {
      std::cerr << "ERROR reading CSV: " << twittok::CsvBioReader::describeError(error) << std::endl;
      std::cerr << "Skipping bio" << std::endl;
      return 1;
    }

    if (untokenizedBio.followsClinton) nClinton++;
    if (untokenizedBio.followsTrump) nTrump++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) nBoth++;

    if (untokenizedBio.empty()) continue;

    if (untokenizedBio.followsClinton) nClintonWithBio++;
    if (untokenizedBio.followsTrump) nTrumpWithBio++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) nBothWithBio++;

    if ((nClintonWithBio + nTrumpWithBio - nBothWithBio) % 50000 == 0) {
      std::cerr << "Processed " << (nClintonWithBio + nTrumpWithBio - nBothWithBio) << " bios..." << std::endl;
    }

    twittok::Bio bio = twittok::Bio::buildByTokenizing(untokenizedBio, tokenizer);

    auto unigrams = bio.unigrams();
    std::for_each(unigrams.cbegin(), unigrams.cend(), [bio, &unigramInfoTable](auto const& unigram) { unigramInfoTable.add(unigram, bio.followsClinton, bio.followsTrump); });

    auto bigrams = bio.bigrams();
    std::for_each(bigrams.cbegin(), bigrams.cend(), [bio, &bigramInfoTable](auto const& bigram) { bigramInfoTable.add(bigram, bio.followsClinton, bio.followsTrump); });

    auto trigrams = bio.trigrams();
    std::for_each(trigrams.cbegin(), trigrams.cend(), [bio, &trigramInfoTable](auto const& trigram) { trigramInfoTable.add(trigram, bio.followsClinton, bio.followsTrump); });
  }

  std::cout << "Statistics:" << std::endl;
  std::cout << "  Total followers: " << (nClinton + nTrump - nBoth) << std::endl;
  std::cout << "  Clinton followers: " << nClinton << std::endl;
  std::cout << "  Trump followers: " << nTrump << std::endl;
  std::cout << "  (Clinton+Trump) followers: " << nBoth << std::endl;
  std::cout << "  Total followers with bios: " << (nClintonWithBio + nTrumpWithBio - nBothWithBio) << std::endl;
  std::cout << "  Clinton followers with bios: " << nClintonWithBio << std::endl;
  std::cout << "  Trump followers with bios: " << nTrumpWithBio << std::endl;
  std::cout << "  (Clinton+Trump) followers with bios: " << nBothWithBio << std::endl;

  std::ofstream tokensFile(argv[2], std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  std::ofstream mappingsFile(argv[3], std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

  for (const auto& pair : unigramInfoTable.gramToInfo) {
    const auto& token = pair.first;
    const auto& info = pair.second;

    if (info.nTotal() > MinMappingCount) {
      tokensFile << token << "\t" << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\n";

      mappingsFile << token << "\n";
      dumpMappings(mappingsFile, info.originalTexts);
    }
  }

  for (const auto& pair : bigramInfoTable.gramToInfo) {
    const auto& token1 = pair.first;
    for (const auto& pair2 : pair.second.gramToInfo) {
      const auto& token2 = pair2.first;
      const auto& info = pair2.second;

      if (info.nTotal() > MinMappingCount) {
        tokensFile << token1 << " " << token2 << "\t" << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\n";

        mappingsFile << token1 << " " << token2 << "\n";
        dumpMappings(mappingsFile, info.originalTexts);
      }
    }
  }

  for (const auto& pair : trigramInfoTable.gramToInfo) {
    const auto& token1 = pair.first;
    for (const auto& pair2 : pair.second.gramToInfo) {
      const auto& token2 = pair2.first;
      for (const auto& pair3 : pair2.second.gramToInfo) {
        const auto& token3 = pair3.first;
        const auto& info = pair3.second;

        if (info.nTotal() > MinMappingCount) {
          tokensFile << token1 << " " << token2 << " " << token3 << "\t" << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\n";

          mappingsFile << token1 << " " << token2 << " " << token3 << "\n";
          dumpMappings(mappingsFile, info.originalTexts);
        }
      }
    }
  }
}
