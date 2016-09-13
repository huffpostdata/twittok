#ifndef CSV_BIO_READER_H
#define CSV_BIO_READER_H

#include <memory>

#include "untokenized_bio.h"

namespace twittok {

class CsvBioReader {
public:
  class IO {
  public:
    /**
     * Reads at most len bytes into buf. Returns the number of bytes read.
     *
     * On EOF, returns 0. On error, crashes.
     */
    virtual size_t read(char* buf, size_t len) = 0;

  };

  class NullIO : public IO {
  public:
    size_t read(char* buf, size_t len) override { return 0; }
  };

  enum class Error {
    Success,
    EndOfInput,
    ExpectedUint64,
    Uint64OutOfRange,
    ExpectedComma,
    Expected0Or1,
    ExpectedNewline,
    ExpectedEndQuote
  };

  static const char* describeError(Error error);

  static const size_t MaxLineBytes = UntokenizedBio::MaxBioBytes + 6; // "1,1,[...160 utf-8 characters...]\r\n" -- Twitter uses NFC normalization
  static const size_t BufferSize = MaxLineBytes * 20; // must be at least 2x MaxLineBytes, to account for a tweet of nothing but '"' (CSV-escaped)

  CsvBioReader(std::unique_ptr<IO> io) : io_(std::move(io)), begin_(0), end_(0) {}
  CsvBioReader() : io_(new NullIO()), begin_(0), end_(0) {}
  CsvBioReader(const char* filename);

  /**
   * Returns another Bio.
   *
   * Iff we reach EOF, then the return value will isNull() == true.
   */
  UntokenizedBio nextBio(Error* error);

private:
  bool bufIsEmpty() const;
  void fillBuf();
  uint64_t readUint64AndComma(Error* error);
  bool readBool(Error* error);
  void readSimpleString(char* out, size_t* out_len, size_t max_len, Error* error);
  void readQuotedString(char* out, size_t* out_len, size_t max_len, Error* error);
  void readString(char* out, size_t* out_len, size_t max_len, Error* error);
  void consumeComma(Error* error);
  void consumeNewline(Error* error);

  std::unique_ptr<IO> io_;
  char buf_[BufferSize];
  const char* begin_;
  const char* end_;
};

} // namespace twittok

#endif /* CSV_BIO_READER_H */
