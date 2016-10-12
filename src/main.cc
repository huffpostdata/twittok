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

template<int N>
std::unordered_set<std::string>
doPass(
    const std::unordered_set<std::string>& prefixes,
    const std::forward_list<twittok::Bio>& bios,
    std::ostream& os,
    size_t minCount
) {
  twittok::NgramPass<N> pass(prefixes);
  pass.scanUntokenizedBios(bios);
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

  auto untokenizedBios = readUntokenizedBioFromFile(argv[1]);

  std::ofstream tokensFile(argv[2], std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

  // We tokenize and stem once, instead of every pass.
  //
  // This is 4x faster than tokenizing+stemming each pass, but it gulps
  // memory -- about 1kb/bio. 20M bios => 20GB.
  std::cerr << "Tokenizing and stemming..." << std::endl;
  twittok::Tokenizer tokenizer;
  std::forward_list<twittok::Bio> bios;
  for (const auto& untokenizedBio : untokenizedBios) {
    bios.emplace_front(twittok::Bio::buildByTokenizing(untokenizedBio, tokenizer));
  }

  const size_t MinCount = 50;
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
