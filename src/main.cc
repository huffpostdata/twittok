#include "tokenizer.h"

#include <iostream>
#include <string>

#define MAX_LINE_SIZE 1024

int main(int argc, char** argv) {
  twittok::Tokenizer tokenizer;

  std::cout << "Regex compiled. Size: " << tokenizer.re2ProgramSize() << std::endl;

  size_t n_processed(0);
  size_t n_tokens(0);

  for (std::string line; std::getline(std::cin, line);) {
    std::cout << "LINE: " << line << std::endl;

    re2::StringPiece bio(line);

    std::vector<re2::StringPiece> tokens = tokenizer.tokenize(bio);
    n_processed++;
    n_tokens += tokens.size();

    if (n_processed == 100000) { std::cout << n_tokens << std::endl; return 0; }

    std::cout << "TOKENS (" << tokens.size() << "):" << std::endl;

    for (auto const& token: tokens) {
      std::cout << "   [" << (token.data() - bio.data()) << ":" << (token.data() - bio.data() + token.size()) << "] " << token.as_string() << std::endl;
    }
  }
}
