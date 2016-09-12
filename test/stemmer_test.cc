#include "stemmer.h"

#include "gtest/gtest.h"

#include <cstring>
#include <memory>

using twittok::stemmer::stem;

class StemTest : public testing::Test {
protected:
  void try_stem(const char* in_utf8, const char* expect_out_utf8)
  {
    size_t len(strlen(in_utf8));
    size_t expect_out_len(strlen(expect_out_utf8));
    size_t out_len;
    std::unique_ptr<char> out_utf8_ptr(new char[len * 2 + 1]);
    char* out_utf8 = out_utf8_ptr.get();
    stem(in_utf8, len, out_utf8, &out_len);

    ASSERT_EQ(expect_out_len, out_len);

    if (out_len > 0) {
      out_utf8[out_len] = '\0'; // for assertion
      EXPECT_STREQ(expect_out_utf8, out_utf8);
    }
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
T(u8"\u1e9b", "s", normalize_nix_accents)
T("FOO", "foo", casefold)
T(u8"CAF\u00C9", "cafe", casefold_while_nixing_accents)
T("word", "word", word)
T("stemming", "stem", stem)
T("#stemming", "#stemming", no_stem_hashtag)
T("@stemming", "@stemming", no_stem_mention)
