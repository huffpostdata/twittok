Finds all the tokens in a `const char*` from Twitter.

A "token" is a unit of language: usually a word. Eliding the details,
tokenization is an opinionated craft. Here are the rules we use in this
library:

* We assume the input strings are Twitter bios or tweets. That means:
  * The input is valid UTF-8.
  * The input length is <=`160*4` bytes. (Twitter bio limit is 160 codepoints.)
  * `#hashtag`, `@username`, `:)` and `http://valid.url` are valid tokens.
* We largely abide by the rules of [NLTK's "casual" tokenizer](https://github.com/nltk/nltk/blob/07bcb7ed51260f5da07be841aebf23235b6af96e/nltk/tokenize/casual.py), and we comment "`NLTK_DEVIATION`" when we don't.
* Be fast. There are lots of tweets out there.
* Perfect is the enemy of fast. There's no such thing as a perfect tokenizer.

# Dependencies

* [re2](https://github.com/google/re2): on Fedora 24: `sudo dnf install re2-devel`
* [cityhash](https://github.com/google/cityhash): on Fedora 24: `git clone https://github.com/google/cityhash.git && cd cityhash && ./configure && make && sudo make install && sudo ldconfig`
