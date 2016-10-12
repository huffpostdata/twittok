#include "casefold.h"

#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

namespace {

static const icu::Transliterator* transliterator = NULL;

} // namespace

namespace twittok {

std::string
casefold_and_normalize(const std::string& utf8)
{
	UErrorCode errorCode = U_ZERO_ERROR;

	if (transliterator == NULL) {
		transliterator = icu::Transliterator::createInstance("NFKD; [:M:] Remove", UTRANS_FORWARD, errorCode);
		if (U_FAILURE(errorCode)) {
			throw "Whoops, an ICU error";
		}
	}

	UnicodeString string = icu::UnicodeString::fromUTF8(utf8);
	transliterator->transliterate(string);
	string.foldCase();

  std::string ret;
  string.toUTF8String(ret);
  return ret;
}

} // namespace twittok
