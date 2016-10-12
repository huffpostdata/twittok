#include <cassert>
#include <cstddef>
#include <cstring>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <unicode/utypes.h>

#include <string>

#include "stemmer.h"
#include "porter2_stemmer.h"

namespace {

struct StringRef {
	const char* utf8;
	const size_t len;

	StringRef(const char* utf8, size_t len) : utf8(utf8), len(len) {}
	StringRef(const char* utf8_null_terminated) : utf8(utf8_null_terminated), len(strlen(utf8_null_terminated)) {}

	bool operator<(const StringRef& rhs) const {
		int cmp = std::strncmp(utf8, rhs.utf8, std::min(len, rhs.len));
		if (cmp == 0) {
			return len < rhs.len; // shorter string comes first
		} else {
			return cmp < 0; // higher-in-the-alphabet string comes first
		}
	}

	bool operator==(const StringRef& rhs) const {
		return len == rhs.len && strncmp(utf8, rhs.utf8, len) == 0;
	}
};

// These max buffer sizes are kind of hand-wavy. Don't trust them.
const size_t MaxNormalizedUCharLen = twittok::stemmer::MaxBytesToStem * 4;
const size_t MaxNormalizedLen = MaxNormalizedUCharLen * 2;

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

static const icu::Transliterator* transliterator = NULL;
void
casefold_and_normalize(const char* utf8, size_t len, char* out_utf8, size_t* out_len)
{
	UErrorCode errorCode = U_ZERO_ERROR;

	if (transliterator == NULL) {
		transliterator = icu::Transliterator::createInstance("NFKD; [:M:] Remove", UTRANS_FORWARD, errorCode);
		if (U_FAILURE(errorCode)) {
			throw "Whoops, an ICU error";
		}
	}

	// Parse out a UnicodeString, allocated entirely on the stack.
	UChar utf16[MaxNormalizedUCharLen];
	int32_t utf16_len;
	u_strFromUTF8(
		utf16, MaxNormalizedUCharLen, &utf16_len,
		utf8, len,
		&errorCode
	);
	if (U_FAILURE(errorCode)) {
		throw "Whoops, an ICU error parsing UTF-8";
	}
	UnicodeString string(utf16, utf16_len, MaxNormalizedUCharLen);

	// The main stuff: normalize; nix accents; casefold
	transliterator->transliterate(string);
	string.foldCase();

	// Write UTF-8 to the output arguments.
	int32_t out_len32;
	u_strToUTF8(
			out_utf8, MaxNormalizedLen, &out_len32,
			string.getBuffer(), string.length(),
			&errorCode
	);
	if (U_FAILURE(errorCode)) {
		throw "Whoops, an ICU error during to-utf8";
	}

	*out_len = static_cast<size_t>(out_len32);
}

std::string
porter2_stem(const char* utf8, size_t len)
{
	std::string string(utf8, len);
	Porter2Stemmer::stem(string);
  return string;
}

/**
 * Returns whether the first Unicode codepoint in utf8 is a Letter or Number
 *
 * Assumes non-empty, valid-utf8 string.
 */
bool
first_codepoint_is_alnum(const char* utf8) {
	int32_t c;
	U8_GET_UNSAFE(utf8, 0, c);
	return u_isalnum(c);
}

typedef enum NormalizedTokenType {
	Empty,        // this token normalized to zilch
	Symbol,       // this token should disappear because it's punctuation (or similar)
	AsciiLetters, // this token needs Porter2-English stemming applied
	Other         // this token is not pure-ASCII and not invalid. Number? Hashtag? Mention? Leave it alone
} NormalizedTokenType;

NormalizedTokenType
calculate_normalized_token_type(const char* utf8, size_t len)
{
	if (len == 0) return Empty;

	if (utf8[0] == '#' || utf8[0] == '@') return Other;

	if (!first_codepoint_is_alnum(utf8)) return Symbol;

	for (size_t i = 0; i < len; i++) {
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

	char normalized_utf8[MaxNormalizedLen];
	size_t normalized_len;
	casefold_and_normalize(utf8, len, normalized_utf8, &normalized_len);

	// stem, pass-through, or return empty string
	switch (calculate_normalized_token_type(normalized_utf8, normalized_len)) {
		case Empty:
		case Symbol:
      return std::string();
			break;
		case Other:
      return std::string(normalized_utf8, normalized_len);
			break;
		case AsciiLetters:
			return porter2_stem(normalized_utf8, normalized_len);
			break;
    default:
      assert(false);
	}
}

}; // namespace stemmer

}; // namespace twittok
