#include "tokenizer.h"

#include <array>
#include <cstdlib>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <string>

#include "bio.h"
#include "csv_bio_reader.h"
#include "untokenized_bio.h"
#include "ngram.h"
#include "string_ref.h"
#include "sparsepp.h"

#define MAX_LINE_SIZE 1024

namespace std {

template<> struct hash<twittok::StringRef> {
  size_t operator()(const twittok::StringRef& str) { return str.hash(); }
};

}

namespace twittok {

typedef spp::sparse_hash_map<twittok::StringRef, int32_t> OriginalMapping;

struct NgramInfo {
  size_t nClinton;
  size_t nTrump;
  size_t nBoth; // for the number of clinton and NOT trump, use nClinton - nBoth

  size_t nTotal() const {
    return nClinton + nTrump - nBoth;
  }

  OriginalMapping originalTexts;
};

class NgramInfoTable {
public:
  template<int N>
  void add(const Bio& bio, const Ngram<N> ngram) {
    NgramInfo& info = gramToInfo[ngram.gramsString()];
    if (bio.followsClinton) info.nClinton++;
    if (bio.followsTrump) info.nTrump++;
    if (bio.followsClinton && bio.followsTrump) info.nBoth++;
    ++info.originalTexts[ngram.original];
  }

  spp::sparse_hash_map<std::string, NgramInfo> gramToInfo;
};

} // namespace twittok

namespace {

static const int MinMappingCount = 3; // Below this, we lump mappings to "Other" ... and if even "Other" doesn't have this, we nix the mapping

void
dumpMappings(std::ostream& os, const twittok::OriginalMapping& originalMapping)
{
  size_t nOther = 0;
  for (const auto& m : originalMapping) {
    const auto& original = m.first;
    const size_t count = m.second;
    if (count < MinMappingCount || original.contains('\n') || original.contains('\t')) {
      nOther += count;
    } else {
      os << "\t" << count << "\t" << original.to_string() << "\n";
    }
  }
  if (nOther != 0) {
    os << "\t" << nOther << "\t\n";
  }
}

std::forward_list<twittok::UntokenizedBio>
readUntokenizedBioFromFile(const char* csvFilename)
{
  std::cout << "Reading CSV file " << csvFilename << "..." << std::endl;

  size_t nClinton = 0;
  size_t nTrump = 0;
  size_t nBoth = 0;
  size_t nClintonWithBio = 0;
  size_t nTrumpWithBio = 0;
  size_t nBothWithBio = 0;

  twittok::CsvBioReader reader(csvFilename);

  std::forward_list<twittok::UntokenizedBio> ret;

  while (true) {
    twittok::CsvBioReader::Error error;
    twittok::UntokenizedBio untokenizedBio(reader.nextBio(&error));

    if (error == twittok::CsvBioReader::Error::EndOfInput) {
      std::cerr << "Finished reading CSV" << std::endl;
      break;
    }

    if (error != twittok::CsvBioReader::Error::Success) {
      std::cerr << "ERROR reading CSV: " << twittok::CsvBioReader::describeError(error) << std::endl;
      return std::forward_list<twittok::UntokenizedBio>();
    }

    if (untokenizedBio.followsClinton) nClinton++;
    if (untokenizedBio.followsTrump) nTrump++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) nBoth++;

    if (untokenizedBio.empty()) continue;

    if (untokenizedBio.followsClinton) nClintonWithBio++;
    if (untokenizedBio.followsTrump) nTrumpWithBio++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) nBothWithBio++;

    if ((nClintonWithBio + nTrumpWithBio - nBothWithBio) % 500000 == 0) {
      std::cerr << "Read " << (nClintonWithBio + nTrumpWithBio - nBothWithBio) << " bios..." << std::endl;
    }

    ret.push_front(untokenizedBio);
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

  return ret;
}

} // namespace ""

int
main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " DATA.csv OUT-TOKENS.txt" << std::endl;
    exit(1);
  }

  std::forward_list<twittok::UntokenizedBio> untokenizedBios = readUntokenizedBioFromFile(argv[1]);

  twittok::Tokenizer tokenizer;
  twittok::NgramInfoTable ngrams;
  ngrams.gramToInfo.reserve(100*1000*1000); // speed up the start

  size_t n = 0;

  for (const auto& untokenizedBio : untokenizedBios) {
    twittok::Bio bio = twittok::Bio::buildByTokenizing(untokenizedBio, tokenizer);

    n++;
    if (n % 100000 == 0) {
      std::cerr << "Processed " << n << " bios..." << std::endl;
    }

    for (const auto& ngram : bio.unigrams()) { ngrams.add(bio, ngram); }
    for (const auto& ngram : bio.bigrams()) { ngrams.add(bio, ngram); }
    for (const auto& ngram : bio.trigrams()) { ngrams.add(bio, ngram); }
  }

  std::cout << "Dumping results" << std::endl;

  std::ofstream tokensFile(argv[2], std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

  for (const auto& pair : ngrams.gramToInfo) {
    const auto& token = pair.first;
    const auto& info = pair.second;

    if (info.nTotal() > MinMappingCount) {
      tokensFile << token << "\t" << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\n";
      dumpMappings(tokensFile, info.originalTexts);
    }
  }

  return 0;
}
