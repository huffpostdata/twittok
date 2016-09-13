#include "csv_bio_reader.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

class CsvBioReaderTest : public testing::Test {
  class StringIO : public twittok::CsvBioReader::IO {
  public:
    StringIO(const std::string& str)
      : str_(str)
      , start(str.data())
      , end(str.data() + str.size())
    {}

    size_t read(char* buf, size_t len) override {
      if (start + len > end) {
        len = end - start;
      }

      memcpy(buf, start, len);

      start += len;
      return len;
    }

  private:
    std::string str_;
    const char* start;
    const char* end;
  };

protected:
  std::string input;
  std::unique_ptr<twittok::CsvBioReader> reader; // lazily initialized
  twittok::CsvBioReader::Error error;

  twittok::UntokenizedBio next() {
    if (!reader) {
      std::unique_ptr<twittok::CsvBioReader::IO> io(new StringIO(input));
      reader.reset(new twittok::CsvBioReader(std::move(io)));
    }

    return reader->nextBio(&error);
  }
};

#define EXPECT_ERROR(err) EXPECT_EQ(twittok::CsvBioReader::Error::err, error)

TEST_F(CsvBioReaderTest, ReturnsEndOfInput) {
  input = "";
  next();
  EXPECT_ERROR(EndOfInput);
}

TEST_F(CsvBioReaderTest, ReadsOneBio) {
  input = "123,1,1,my bio\n";
  auto bio = next();
  EXPECT_ERROR(Success);
  EXPECT_EQ(123, bio.id);
  EXPECT_EQ(true, bio.followsClinton);
  EXPECT_EQ(true, bio.followsTrump);
  EXPECT_EQ("my bio", bio.utf8);
}

TEST_F(CsvBioReaderTest, ErrorNoNewlineAfterBio) {
  input = "123,1,1,my bio";
  next();
  EXPECT_ERROR(ExpectedNewline);
}

TEST_F(CsvBioReaderTest, ErrorExpectedCommaInClinton) {
  input = "123,11,1,my bio\n";
  next();
  EXPECT_ERROR(ExpectedComma);
}

TEST_F(CsvBioReaderTest, ErrorExpectedCommaInTrump) {
  input = "123,1,11,my bio\n";
  next();
  EXPECT_ERROR(ExpectedComma);
}

TEST_F(CsvBioReaderTest, ErrorExpectedCommaAfterId) {
  input = "123!,1,1\n";
  next();
  EXPECT_ERROR(ExpectedComma);
}

TEST_F(CsvBioReaderTest, EmptyBio) {
  input = "123,1,1,\n";
  auto bio = next();
  EXPECT_ERROR(Success);
  EXPECT_EQ(123, bio.id);
  EXPECT_EQ("", bio.utf8);
}

TEST_F(CsvBioReaderTest, QuotedNewline) {
  input = "1,1,1,\"foo\nbar\"\n";
  EXPECT_ERROR(Success);
  EXPECT_EQ("foo\nbar", next().utf8);
}

TEST_F(CsvBioReaderTest, DoubleQuotes) {
  input = "1,1,1,\"foo\"\"bar\"\n";
  EXPECT_ERROR(Success);
  EXPECT_EQ("foo\"bar", next().utf8);
}

TEST_F(CsvBioReaderTest, TwoBios) {
  input = "1,1,1,foo\n2,1,1,bar\n";
  EXPECT_EQ("foo", next().utf8);
  EXPECT_ERROR(Success);
  EXPECT_EQ("bar", next().utf8);
  EXPECT_ERROR(Success);
  EXPECT_EQ(0, next().id);
  EXPECT_ERROR(EndOfInput);
}
