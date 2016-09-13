#include "stemmer.h"

#include "gtest/gtest.h"

#include <cstring>
#include <memory>

using twittok::stemmer::stem;

class StemTest : public testing::Test {
protected:
  void try_stem(const char* in_utf8, const char* expect_out_utf8)
  {
    std::string in_string(in_utf8);
    std::string expect_out_string(expect_out_utf8);
    std::string out_string = stem(in_string.c_str(), in_string.size());

    EXPECT_EQ(expect_out_string, out_string);
  }
};

#define T(in, out, name) TEST_F(StemTest, name) { try_stem(in, out); }

T("", "", no_empty_string)
T(":)", "", no_symbol)
T(".", "", no_period)
T("...", "", no_ellipsis)
T("and", "", no_stopword)
T("And", "", case_insensitive_stopword)
T("13", "13", number)
T("1.3", "1.3", decimal_number)
T("twitter.com", "", url)
T("http://example.org/foo", "", http_url)
T(u8"\u1e9bea", "sea", normalize_nix_accents)
T("FOO", "foo", casefold)
T(u8"CAF\u00C9", "cafe", casefold_while_nixing_accents)
T("word", "word", word)
T("stemming", "stem", stem)
T("#stemming", "#stemming", no_stem_hashtag)
T("@stemming", "@stemming", no_stem_mention)
