#ifndef STEMMER_H
#define STEMMER_H

#include <cstddef>
#include <string>

namespace twittok {

namespace stemmer {

/**
 * Byte limit above which we will not stem a token.
 *
 * We don't care much about non-English tokens, because we don't expect there to
 * be enough to justify counting them.
 */
const size_t MaxBytesToStem = 30; // 30 bytes -> min. 7 codepoints, or 30 English ones

/**
 * Sets out_utf8 and out_len to a stemmed version of utf8, or sets out_len to
 * 0 if no stemming should occur.
 *
 * Stemming is done using the Porter2 ("snowball") English stemming algorithm.
 *
 * We do not stem:
 *
 * * URLs
 * * Words that are too long
 * * Symbols
 *
 * We copy numbers unchanged. That way, "50" and "Cent" will both be deemed
 * significant.
 *
 * All stemmed tokens are case-folded and Unicode-normalized. Since lowercase
 * and uppercase versions of the same character can consume a different number
 * of bytes, it's possible for out_len to be greater than len.
 *
 * len and out_len are measured in bytes, not characters. You may assume that
 * out_len will never exceed double len. utf8 need not be NULL-terminated; and
 * out_utf8 won't be.
 */
std::string stem(const char* utf8, size_t len);

};

}; // namespace twittok

#endif /* STEMMER_H */
