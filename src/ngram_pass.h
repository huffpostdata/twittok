#ifndef NGRAM_PASS_H
#define NGRAM_PASS_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "bio.h"
#include "ngram_info.h"

namespace twittok {

/**
 * Given ngrams of length N, tallies ngrams of length N+1.
 */
template<int N>
class NgramPass {
public:
  NgramPass(const std::unordered_set<std::string>& prefixes)
    : prefixes(prefixes)
    {}

  void scanUntokenizedBios(const std::forward_list<Bio>& bios)
  {
    size_t n = 0;
    for (const auto& bio : bios) {
      n++;
      if (n % 100000 == 0) {
        std::cerr << "Pass " << N << ": " << n << " bios..." << std::endl;
      }

      for (const auto& ngram : bio.ngrams<N>()) {
        if (N == 1 || prefixes.find(ngram.prefixGramsString()) != prefixes.end()) {
          NgramInfo& info = gramToInfo[ngram.gramsString()];
          if (bio.followsClinton) info.nClinton++;
          if (bio.followsTrump) info.nTrump++;
          if (bio.followsClinton && bio.followsTrump) info.nBoth++;
          ++info.originalTexts[ngram.original];
        }
      }
    }
  }

  void dump(std::ostream& os, size_t minCount) const {
    for (const auto& pair : gramToInfo) {
      const auto& info = pair.second;

      if (info.nTotal() < minCount) continue;

      os << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\t" << info.nVariants() << "\n";

      for (const auto& m : info.originalTexts.values) {
        const auto& original = m.string;
        const size_t count = m.n;
        if (!original.contains('\n') && !original.contains('\t')) {
          os << count << "\t" << original.to_string() << "\n";
        }
      }
    }
  }

  std::unordered_set<std::string> ngramStrings(size_t minCount) const {
    // This pass, we got some ngrams. Every ngram we see in the _next_ pass will
    // start with an ngram from _this_ pass. But minCount is our threshold:
    // any ngram that appears fewer than minCount times is worthless to us, and
    // so any _prefix_ that appears fewer than minCount times is useless.

    std::unordered_set<std::string> ret;

    for (const auto& it : gramToInfo) {
      if (it.second.nTotal() >= minCount) {
        ret.insert(it.first);
      }
    }

    return ret;
  }

  std::unordered_set<std::string> prefixes; // calculated in previous pass
  std::unordered_map<std::string, NgramInfo> gramToInfo; // calculated this pass
};

} // namespace twittok

#endif /* NGRAM_PASS_H */
