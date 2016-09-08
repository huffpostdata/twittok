#include "tokenizer.h"

#include <re2/re2.h>

namespace {

// static const LazyRE2 token_re = ...
#include "token_regex.i"

}; // namespace

namespace twittok {

struct Tokenizer_priv {
  Tokenizer_priv(const re2::RE2* re_) : re(re_) {}
  ~Tokenizer_priv() { delete re; }

  const re2::RE2* re;
};

Tokenizer::Tokenizer()
  : priv(0)
{
  re2::RE2::Options options;
  options.set_case_sensitive(true);
  options.set_never_capture(true);
  auto re = new re2::RE2(token_re, options);
  priv = new Tokenizer_priv(re);
}

Tokenizer::~Tokenizer()
{
  delete priv;
}

std::vector<re2::StringPiece>
Tokenizer::tokenize(const re2::StringPiece& text) const
{
  std::vector<re2::StringPiece> tokens;

  re2::StringPiece match; // set every iteration
  size_t pos(0);

  while (priv->re->Match(text, pos, text.size(), re2::RE2::Anchor::UNANCHORED, &match, 1)) {
    tokens.push_back(match); // a copy of the StringPiece we just found
    pos = match.end() - text.begin();
  }

  return tokens;
}

int
Tokenizer::re2ProgramSize() const
{
  return this->priv->re->ProgramSize();
}

}; // namespace twittok
