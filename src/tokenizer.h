#ifndef TWITTOK_H
#define TWITTOK_H

#include <vector>
#include <re2/re2.h>

namespace twittok {

class Tokenizer_priv;

class Tokenizer {
public:
  Tokenizer();
  ~Tokenizer();

  std::vector<re2::StringPiece> tokenize(const re2::StringPiece& text) const;
  int re2ProgramSize() const;

private:
  Tokenizer_priv* priv;
};

}; // namespace twittok

#endif /* TWITTOK_H */
