#ifndef NGRAM_INFO_TABLE_H
#define NGRAM_INFO_TABLE_H

#include <unordered_map>

#include "bio.h"
#include "ngram_info.h"
#include "ngram.h"

namespace twittok {

template<int N>
class NgramInfoTable {
public:
  void add(const Bio& bio, const Ngram<N>& ngram) {
    NgramInfo& info = gramToInfo[ngram.gramsString()];
    if (bio.followsClinton) info.nClinton++;
    if (bio.followsTrump) info.nTrump++;
    if (bio.followsClinton && bio.followsTrump) info.nBoth++;
    ++info.originalTexts[ngram.original];
  }

  std::unordered_map<std::string, NgramInfo> gramToInfo;
};

}; // namespace twittok

#endif /* NGRAM_INFO_TABLE_H */
