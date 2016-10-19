#include "tokenizer.h"

#include <cstdlib>
#include <forward_list>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "bio.h"
#include "csv_bio_reader.h"
#include "untokenized_bio.h"
#include "ngram_pass.h"

#define MAX_LINE_SIZE 1024

namespace {

struct UntokenizedBiosResult {
  std::forward_list<twittok::UntokenizedBio> untokenizedBios;
  std::string error;
  size_t nClinton = 0;
  size_t nTrump = 0;
  size_t nBoth = 0;
  size_t n() const { return nClinton + nTrump - nBoth; }
  size_t nClintonWithBio = 0;
  size_t nTrumpWithBio = 0;
  size_t nBothWithBio = 0;
  size_t nWithBio() const { return nClinton + nTrump - nBoth; }

  void dump(std::ostream& os) const {
    os << "n: " << n() << "\n";
    os << "nClinton: " << nClinton << "\n";
    os << "nTrump: " << nTrump << "\n";
    os << "nBoth: " << nBoth << "\n";
    os << "nWithBio: " << nWithBio() << "\n";
    os << "nClintonWithBio: " << nClintonWithBio << "\n";
    os << "nTrumpWithBio: " << nTrumpWithBio << "\n";
    os << "nBothWithBio: " << nBothWithBio << "\n";
    os << std::flush;
  }
};

UntokenizedBiosResult
readUntokenizedBiosFromFile(const char* csvFilename)
{
  UntokenizedBiosResult result;

  twittok::CsvBioReader reader(csvFilename);

  std::forward_list<twittok::UntokenizedBio> ret;

  while (true) {
    twittok::CsvBioReader::Error error;
    twittok::UntokenizedBio untokenizedBio(reader.nextBio(&error));

    if (error == twittok::CsvBioReader::Error::EndOfInput) {
      break;
    }

    if (error != twittok::CsvBioReader::Error::Success) {
      result.error = twittok::CsvBioReader::describeError(error);
      break;
    }

    if (untokenizedBio.followsClinton) result.nClinton++;
    if (untokenizedBio.followsTrump) result.nTrump++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) result.nBoth++;

    if (untokenizedBio.empty()) continue;

    if (untokenizedBio.followsClinton) result.nClintonWithBio++;
    if (untokenizedBio.followsTrump) result.nTrumpWithBio++;
    if (untokenizedBio.followsClinton && untokenizedBio.followsTrump) result.nBothWithBio++;

    result.untokenizedBios.push_front(untokenizedBio);
  }

  return result;
}

template<int N>
std::unordered_set<std::string>
doPass(
    const std::unordered_set<std::string>& prefixes,
    const std::forward_list<twittok::Bio>& bios,
    std::ostream& os,
    size_t minCount
) {
  twittok::NgramPass<N> pass(prefixes);
  pass.scanBios(bios);
  pass.dump(os, minCount);
  return pass.ngramStrings(minCount);
}

} // namespace ""

int
main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " DATA.csv OUT-TOKENS.txt" << std::endl;
    exit(1);
  }

  std::cerr << "Preparing to write to " << std::string(argv[2]) << std::endl;
  std::ofstream tokensFile(argv[2], std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

  std::cerr << "Reading bios from " << std::string(argv[1]) << std::endl;
  UntokenizedBiosResult untokenizedBios = readUntokenizedBiosFromFile(argv[1]);

  std::cerr << "Outputting statistics on " << untokenizedBios.nWithBio() << " bios" << std::endl;
  untokenizedBios.dump(tokensFile);

  // We tokenize and stem once, instead of every pass.
  //
  // This is 4x faster than tokenizing+stemming each pass, but it gulps
  // memory -- about 1kb/bio. 20M bios => 20GB.
  std::cerr << "Tokenizing and stemming..." << std::endl;
  twittok::Tokenizer tokenizer;
  std::forward_list<twittok::Bio> bios;
  size_t n = 0;
  for (const auto& untokenizedBio : untokenizedBios.untokenizedBios) {
    bios.emplace_front(twittok::Bio::buildByTokenizing(untokenizedBio, tokenizer));
    n++;
    if (n % 1000000 == 0) {
      std::cerr << "Tokenized and stemmed " << (n / 1000000) << "M bios" << std::endl;
    }
  }

  const size_t MinCount = 100;
  std::unordered_set<std::string> prefixes;

  prefixes = doPass<1>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<2>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<3>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<4>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<5>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<6>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<7>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<8>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<9>(prefixes, bios, tokensFile, MinCount);
  prefixes = doPass<10>(prefixes, bios, tokensFile, MinCount);

  return 0;
}
