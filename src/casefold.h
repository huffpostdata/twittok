#ifndef CASEFOLD_H
#define CASEFOLD_H

#include <string>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>

namespace twittok {

/**
 * Returns a string that can be compared to other strings.
 *
 * The rules are inspired by English: case-insensitive, and Unicode sequences
 * that represent the exact same sequence of characters are treated identically.
 */
std::string
casefold_and_normalize(const std::string& utf8);

}; // namespace twittok

#endif /* CASEFOLD_H */
