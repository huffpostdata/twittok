#ifndef UNTOKENIZED_BIO_H
#define UNTOKENIZED_BIO_H

namespace twittok {

/**
 * A Twitter bio and accompanying information, untokenized.
 *
 * This uses no dynamic allocation; it's meant to be manipulated on the stack.
 */
struct UntokenizedBio {
  static const int MaxBioCodepoints = 160; // Twitter uses NFC normalization; this is how it counts
  static const int MaxBioBytes = MaxBioCodepoints * 4; // We're UTF-8

  UntokenizedBio() : id(0), followsClinton(false), followsTrump(false), utf8(std::string()) {}
  UntokenizedBio(uint64_t id_, bool followsClinton_, bool followsTrump_, const std::string& utf8_)
    : id(id_), followsClinton(followsClinton_), followsTrump(followsTrump_), utf8(utf8_) {}

  bool isNull() const { return id == 0; }

  uint64_t id;
  bool followsClinton;
  bool followsTrump;
  std::string utf8;
};

} // namespace twittok

#endif /* UNTOKENIZED_BIO_H */
