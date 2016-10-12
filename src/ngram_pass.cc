#include "ngram_pass.h"

#include <iostream>

#include "casefold.h"
#include "ngram_info.h"

namespace {

class NgramToDump {
public:
  std::string original;
  std::string folded;
  size_t n;

  NgramToDump() {} // so it can go in a vector

  NgramToDump(const twittok::NgramInfo::OriginalTexts::Item& item)
    : original(item.string.to_string())
    , folded(twittok::casefold_and_normalize(original))
    , n(item.n)
  {
  }

  bool operator<(const NgramToDump& rhs) const {
    // Sort order is most-common to least-common: that is, high n to low n
    if (n > rhs.n) return true;
    if (n < rhs.n) return false;
    return folded < rhs.folded;
  }
};

void
dumpNgramInfo(const twittok::NgramInfo& info, std::ostream& os, size_t minCount) {
  if (info.nTotal() < minCount) return; // optimization

  // 1. Sort tokens by most to least common
  std::vector<NgramToDump> vector;
  vector.reserve(info.originalTexts.values.size());
  for (const auto& original : info.originalTexts.values) {
    auto ngramToDump = NgramToDump(original);

    // Completely ignore anything with a special character that breaks output
    //
    // These stay in "nVariants" because we do _count_ each occurrence as an
    // alternate spelling. We just won't output all of them.
    if (ngramToDump.original.find('\n') != std::string::npos) continue;
    if (ngramToDump.original.find('\t') != std::string::npos) continue;

    vector.push_back(ngramToDump);
  }

  std::sort(vector.begin(), vector.end());

  // 2. Fold items together: two identical spellings get added together
  // This is an in-place edit of the list, akin to std::unique(). It's O(n^2)
  // but we assume these lists are pretty small. The "out" list is the output;
  // the "in" list is the part of the vector we have not yet inspected. One
  // step involves taking an "in" and comparing with each "out": if we match,
  // we merge the two and move on. Otherwise, we move the "in" item into the
  // "out" list.
  //
  // Notice why we do this _after_ step 1: it's so that we only show the most
  // common capitalization (e.g., "LGBT" instead of "lgbt").
  auto outEnd = vector.begin();

  for (auto inIt = vector.begin(); inIt != vector.end(); ++inIt) {
    const NgramToDump& in = *inIt;
    bool matched = false;

    for (auto outIt = vector.begin(); outIt != outEnd; ++outIt) {
      NgramToDump& out = *outIt;
      if (out.folded == in.folded) {
        // We found a dup
        matched = true;
        out.n += in.n;
        break;
      }
    }

    if (!matched) {
      // Move our input item to the end of the output
      if (outEnd != inIt) *outEnd = in;
      ++outEnd;
    }
  }
  // anything past outEnd is noise
  vector.resize(std::distance(vector.begin(), outEnd));

  // 3. Sort again: this time for output
  std::sort(vector.begin(), vector.end());

  // 4. Output every spelling that has occurs more than minCount times
  if (vector[0].n < minCount) return;

  os << info.nClinton << "\t" << info.nTrump << "\t" << info.nBoth << "\t" << info.nVariants() << "\n";

  for (const auto& item : vector) {
    if (item.n < minCount) return;

    os << item.original << "\n";
  }
}

}; // namespace

namespace twittok {

template<size_t N>
void
NgramPass<N>::dump(std::ostream& os, size_t minCount) const {
  for (const auto& pair : gramToInfo) {
    dumpNgramInfo(pair.second, os, minCount);
  }
}

template<size_t N>
void
NgramPass<N>::scanBios(const std::forward_list<Bio>& bios) {
  size_t n = 0;
  for (const auto& bio : bios) {
    n++;
    if (n % 1000000 == 0) {
      std::cerr << "Pass " << N << ": " << (n / 1000000) << "M bios..." << std::endl;
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

template<size_t N>
std::unordered_set<std::string>
NgramPass<N>::ngramStrings(size_t minCount) const
{
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

template class NgramPass<1>;
template class NgramPass<2>;
template class NgramPass<3>;
template class NgramPass<4>;
template class NgramPass<5>;
template class NgramPass<6>;
template class NgramPass<7>;
template class NgramPass<8>;
template class NgramPass<9>;
template class NgramPass<10>;

}; // namespace twittok
