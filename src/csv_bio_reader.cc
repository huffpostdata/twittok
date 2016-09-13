#include "csv_bio_reader.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream> // TODO remove this

namespace {

class CFileIO : public twittok::CsvBioReader::IO {
public:
  CFileIO(const char* filename) : file(std::fopen(filename, "rb"))
  {
    if (!file) throw "Could not open file";
  }

  ~CFileIO() { if (file) std::fclose(file); }

  size_t read(char* buf, size_t len) override {
    const size_t ret = std::fread(buf, 1, len, file);
    if (ret != len) {
      if (ret == 0 && std::feof(file)) return 0;
      if (std::ferror(file)) throw "Error reading file";
    }
    return ret;
  }

private:
  std::FILE* file;
};

} // namespace ""

namespace twittok {

CsvBioReader::CsvBioReader(const char* filename)
  : io_(new CFileIO(filename))
  , begin_(0)
  , end_(0)
{
}

#define fillBufOrReturnWithError(error) do { if (bufIsEmpty()) { fillBuf(); if (bufIsEmpty()) { *err = Error::error; return; } } } while(false)
#define fillBufOrReturnValueWithError(value, error) do { if (bufIsEmpty()) { fillBuf(); if (bufIsEmpty()) { *err = Error::error; return value; } } } while(false)

void
CsvBioReader::fillBuf()
{
  size_t len = io_->read(buf_, BufferSize);
  begin_ = buf_;
  end_ = begin_ + len;
}

bool
CsvBioReader::bufIsEmpty() const {
  return begin_ == end_;
}

/**
 * Assumes io points to "[1-9][0-9]*,". Absorbs that and returns a uint64_t.
 *
 * Can fill err with:
 *
 * * ExpectedUint64: if the first character is not [1-9]
 * * ExpectedComma: if a subsequnet character is not [0-9,]
 * * Uint64OutOfRange: if there are too many digits
 */
uint64_t
CsvBioReader::readUint64AndComma(Error* err) {
  uint64_t ret = 0;

  fillBufOrReturnValueWithError(0, ExpectedUint64);

  while (begin_ < end_) {
    char c = *begin_;
    begin_++;

    if (c == ',') return ret; // success!

    if (c < '0' || c > '9') {
      *err = Error::ExpectedComma;
      return ret;
    }
    if (ret == 0 && c == '0') {
      *err = Error::ExpectedUint64;
      return ret;
    }

    uint64_t next = ret * 10 + (c - '0');
    if (next < ret) {
      *err = Error::Uint64OutOfRange;
      return ret;
    }
    ret = next;

    fillBufOrReturnValueWithError(0, ExpectedComma);
  }

  *err = ret == 0 ? Error::ExpectedComma : Error::ExpectedUint64;
  return ret; // EOF
}

/**
 * Reads "0" as false or "1" as true.
 *
 * Sets err Expected0Or1 for any other input.
 *
 * Leaves buf full, or sets ExpectedComma.
 */
bool
CsvBioReader::readBool(Error* err)
{
  fillBufOrReturnValueWithError(false, Expected0Or1);

  char c = *begin_;
  begin_++;

  switch (c) {
    case '0': return false;
    case '1': return true;
    default:
      *err = Error::Expected0Or1;
      return false;
  }
}

/**
 * Consumes a comma. May set ExpectedComma.
 */
void
CsvBioReader::consumeComma(Error* err)
{
  fillBufOrReturnWithError(ExpectedComma);

  if (*begin_ != ',') *err = Error::ExpectedComma;
  begin_++;
}

/**
 * Consumes a newline, or sets ExpectedNewline.
 */
void
CsvBioReader::consumeNewline(Error* err)
{
  fillBufOrReturnWithError(ExpectedNewline);

  if (*begin_ != '\n') *err = Error::ExpectedNewline;
  begin_++;
}

void
CsvBioReader::readSimpleString(char* out, size_t* out_len, size_t max_len, Error* err)
{
  size_t copied = 0;
  while (copied < max_len) {
    size_t len = max_len - copied;
    if (begin_ + len > end_) len = end_ - begin_;

    const char* newline = static_cast<const char*>(memchr(begin_, '\n', len));
    if (newline != NULL) {
      len = newline - begin_;
      memcpy(out, begin_, len);
      *out_len = copied + len;
      begin_ += len;
      return;
    }

    memcpy(out, begin_, len);
    copied += len;
    out += len;
    begin_ += len;
    fillBufOrReturnWithError(ExpectedNewline);
  }

  if (*begin_ == '\n') {
    // Lucky! After copying all the characters, the next one (!bufIsEmpty() here) is the newline
    *out_len = max_len;
    return;
  }

  // More likely: we copied as far as we could and didn't hit a newline
  *err = Error::ExpectedNewline;
}

/**
 * Fills out and out_len with a string of size 0-max_len.
 *
 * We assume begin_ is pointing at a '"'. We read until the first '"' that is
 * not followed by another '"'.
 */
void
CsvBioReader::readQuotedString(char* out, size_t* out_len, size_t max_len, Error* err)
{
  begin_++;
  fillBufOrReturnWithError(ExpectedEndQuote);

  size_t copied = 0;
  while (copied < max_len) {
    size_t len = max_len - copied;
    if (begin_ + len > end_) len = end_ - begin_;

    const char* quote = static_cast<const char*>(memchr(begin_, '"', len));
    if (quote != NULL) {
      len = quote - begin_;
      memcpy(out, begin_, len);
      copied += len;
      out += len;
      begin_ = quote + 1; // consume the quote without writing it to out

      fillBufOrReturnWithError(ExpectedNewline);

      if (*begin_ == '"') {
        // That was a double-quote. Write the one quote to out and keep moving.
        *out = '"';
        out++;
        begin_++;
        copied++;
        fillBufOrReturnWithError(ExpectedEndQuote);
      } else {
        // The quote ended the string.
        *out_len = copied;
        return;
      }
    } else {
      // We read to the end of the buffer without finding a quotation mark.
      // Loop.
      memcpy(out, begin_, len);
      copied += len;
      out += len;
      begin_ += len;
      fillBufOrReturnWithError(ExpectedEndQuote);
    }
  }

  if (*begin_ == '"') {
    // Lucky! After copying all the characters, the next one (!bufIsEmpty() here) is the ending quote
    begin_++;
    fillBufOrReturnWithError(ExpectedNewline);
    if (*begin_ == '"') {
      *err = Error::ExpectedEndQuote;
      return;
    }
    *out_len = max_len;
    return;
  }

  *err = Error::ExpectedEndQuote;
}

/**
 * Fills out and out_len with a string of size 0-max_len.
 *
 * We assume the CSV ends with a newline. That way, an end of file means error.
 *
 * Possible err values:
 *
 * * ExpectedNewline: we went max_len bytes without encountering a newline.
 * * ExpectedEndQuote: we went max_len (unescaped) bytes and didn't encounter a closing quotation mark.
 */
void
CsvBioReader::readString(char* out, size_t* out_len, size_t max_len, Error* err)
{
  fillBufOrReturnWithError(ExpectedNewline);

  if (*begin_ == '\n') return; // empty bio

  if (*begin_ == '"') {
    readQuotedString(out, out_len, max_len, err);
  } else {
    readSimpleString(out, out_len, max_len, err);
  }
}

const char*
CsvBioReader::describeError(Error error)
{
  switch (error) {
    case Error::Success: return "success";
    case Error::EndOfInput: return "end of input";
    case Error::ExpectedUint64: return "expected a uint64";
    case Error::Uint64OutOfRange: return "integer exceeded uint64 limit";
    case Error::ExpectedComma: return "expected comma";
    case Error::Expected0Or1: return "expected '0' or '1'";
    case Error::ExpectedNewline: return "expected '\\n'";
    case Error::ExpectedEndQuote: return "expected '\"'";
    default: assert(false);
  }
}

UntokenizedBio
CsvBioReader::nextBio(Error* err)
{
  *err = Error::Success;

  // Implementation detail: all methods call fillBuf() before returning.
  fillBufOrReturnValueWithError(UntokenizedBio(), EndOfInput);

#define FAIL_IF_ERROR() if(*err != Error::Success) return UntokenizedBio()

  uint64_t id = readUint64AndComma(err);
  FAIL_IF_ERROR();

  bool followsClinton = readBool(err);
  FAIL_IF_ERROR();
  consumeComma(err);
  FAIL_IF_ERROR();

  bool followsTrump = readBool(err);
  FAIL_IF_ERROR();
  consumeComma(err);
  FAIL_IF_ERROR();

  char bytes[UntokenizedBio::MaxBioBytes];
  size_t len = 0; // 0 in case we fail fast

  readString(bytes, &len, UntokenizedBio::MaxBioBytes, err);
  FAIL_IF_ERROR();

  consumeNewline(err);
  FAIL_IF_ERROR();

  return { id, followsClinton, followsTrump, std::string(bytes, len) };
}

} // namespace twittok
