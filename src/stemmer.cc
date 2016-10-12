#include "stemmer.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <unicode/utf8.h>

#include "casefold.h"
#include "porter2_stemmer.h"

namespace {
bool is_too_long(size_t len)
{
	return len > twittok::stemmer::MaxBytesToStem;
}

/**
 * Tests that the given utf8 string contains a "." but isn't a number.
 */
bool is_url_or_just_dots(const char* utf8, size_t len)
{
	bool has_dot = false;
	bool has_digit = false;
	bool has_other = false;

	for (size_t i = 0; i < len; i++) {
		char c = utf8[i];
		switch(c) {
			case '.': 
				has_dot = true;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				has_digit = true;
				break;
			default:
				has_other = true;
		}
	}

	return has_dot && (has_other || !has_digit);
}

/**
 * Returns true if the given token is sure to be trash.
 */
bool should_abort_stem_right_away(const char* utf8, size_t len)
{
	return is_too_long(len) || is_url_or_just_dots(utf8, len);
}

/**
 * Returns whether the first Unicode codepoint in utf8 is a Letter or Number
 *
 * Assumes non-empty, valid-utf8 string.
 */
bool
first_codepoint_is_alnum(const std::string& utf8) {
	int32_t c;
	U8_GET_UNSAFE(&utf8[0], 0, c);
	return u_isalnum(c);
}

typedef enum NormalizedTokenType {
	Empty,        // this token normalized to zilch
	Symbol,       // this token should disappear because it's punctuation (or similar)
	AsciiLetters, // this token needs Porter2-English stemming applied
	Other         // this token is not pure-ASCII and not invalid. Number? Hashtag? Mention? Leave it alone
} NormalizedTokenType;

NormalizedTokenType
calculate_normalized_token_type(const std::string& utf8)
{
	if (utf8.size() == 0) return Empty;

	if (utf8[0] == '#' || utf8[0] == '@') return Other;

	if (!first_codepoint_is_alnum(utf8)) return Symbol;

	for (size_t i = 0; i < utf8.size(); i++) {
		char c(utf8[i]);
		if (c < 'a' || c > 'z') return Other;
	}

	return AsciiLetters;
}

}; // empty namespace

namespace twittok {

namespace stemmer {

std::string
stem(const char* utf8, size_t len)
{
	if (should_abort_stem_right_away(utf8, len)) {
    return std::string();
	}

  std::string normalized = twittok::casefold_and_normalize(std::string(utf8, len));

	// stem, pass-through, or return empty string
	switch (calculate_normalized_token_type(normalized)) {
		case Empty:
		case Symbol:
      return std::string();
		case Other:
      return normalized;
		case AsciiLetters:
      Porter2Stemmer::stem(normalized); // ick! edits in-place
      return normalized;
    default:
      assert(false);
	}
}

}; // namespace stemmer

}; // namespace twittok
