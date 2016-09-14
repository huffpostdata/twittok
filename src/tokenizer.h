#ifndef TWITTOK_H
#define TWITTOK_H

#include <vector>
#include <re2/stringpiece.h>

namespace twittok {

class Tokenizer_priv;

class Tokenizer {
public:
  typedef re2::StringPiece Token;

  Tokenizer();
  ~Tokenizer();

  std::vector<Token> tokenize(const re2::StringPiece& text) const;

private:
  Tokenizer_priv* priv;
};

}; // namespace twittok

#endif /* TWITTOK_H */
