#!/usr/bin/env ruby

OutFile = "token_regex.h"

# The twitter-text package has a great URL regex, but we have to convert it to
# RE2 syntax.
#
# Then we'll nix the GTLD/CCTLD restriction. Twitter uses it because it makes
# no sense for Twitter to link to invalid URLs. We don't want it, because we
# don't care whether a URL is "valid": we care that it's a token.
#
# Finally, we can't use _any_ of Twitter's pre-written code. here's why:
# * Objective-C: too complex
# * JavaScript: regex built around UCS-2 surrogates, not Unicode
# * Java: too hard to build
# * Ruby: no way to convert from Regexp to String (comments and "(?i-mx" abound)
#
# So we copy the Ruby code, but we use Strings instead of Regexps, and then we
# encode those strings into RE2 syntax instead of into Ruby Regexps.
#
# Deviations from twitter-text marked as DEVIATE_FROM_TWITTER

REGEXEN = {}

def regex_range(from, to = nil) # :nodoc:
  # DEVIATE_FROM_TWITTER twitter-text regex-quotes these, but they're just codepoints....
  # We write them out as strings; we'll encode them at the end
  if to
    "#{from.chr(Encoding::UTF_8)}-#{to.chr(Encoding::UTF_8)}"
  else
    "#{from.chr(Encoding::UTF_8)}"
  end
end

# DEVIATE_FROM_TWITTER we don't need String#split. Don't be verbose about UNICODE_SPACES
UNICODE_SPACES = "\x09-\x0d \u0085\u00a0\u1680\u180e\u2000-\u200a\u2028\u2029\u202f\u205f\u3000"

# Character not allowed in Tweets
INVALID_CHARACTERS = "\ufffe\ufeff\uffff\u202a-\u202e"

# Latin accented characters
# Excludes 0xd7 from the range (the multiplication sign, confusable with "x").
# Also excludes 0xf7, the division sign
LATIN_ACCENTS = [
      regex_range(0xc0, 0xd6),
      regex_range(0xd8, 0xf6),
      regex_range(0xf8, 0xff),
      regex_range(0x0100, 0x024f),
      regex_range(0x0253, 0x0254),
      regex_range(0x0256, 0x0257),
      regex_range(0x0259),
      regex_range(0x025b),
      regex_range(0x0263),
      regex_range(0x0268),
      regex_range(0x026f),
      regex_range(0x0272),
      regex_range(0x0289),
      regex_range(0x028b),
      regex_range(0x02bb),
      regex_range(0x0300, 0x036f),
      regex_range(0x1e00, 0x1eff)
].join('').freeze

PUNCTUATION_CHARS = '!"#$%&\'()*+,-./:;<=>?@\\[\\]^_`{|}~'
SPACE_CHARS = " \t\n\x0B\f\r"
CTRL_CHARS = "\x00-\x1F\x7F"

# Generated from unicode_regex/unicode_regex_groups.scala, more inclusive than Ruby's \p{L}\p{M}
# DEVIATE_FROM_TWITTER re2 uses Unicode 8.0, which should be enough
HASHTAG_LETTERS_AND_MARKS = "\\p{L}\\p{M}"

# Generated from unicode_regex/unicode_regex_groups.scala, more inclusive than Ruby's \p{Nd}
# DEVIATE_FROM_TWITTER re2 uses Unicode 8.0, which should be enough
HASHTAG_NUMERALS = "\\p{Nd}"

HASHTAG_SPECIAL_CHARS = "_\u200c\u200d\ua67e\u05be\u05f3\u05f4\uff5e\u301c\u309b\u309c\u30a0\u30fb\u3003\u0f0b\u0f0c\u00b7"

HASHTAG_LETTERS_NUMERALS = "#{HASHTAG_LETTERS_AND_MARKS}#{HASHTAG_NUMERALS}#{HASHTAG_SPECIAL_CHARS}"
HASHTAG_LETTERS_NUMERALS_SET = "[#{HASHTAG_LETTERS_NUMERALS}]"
HASHTAG_LETTERS_SET = "[#{HASHTAG_LETTERS_AND_MARKS}]"

# DEVIATE_FROM_TWITTER: :valid_hashtag, like other Twitter regexes, actually
# matches some stuff _preceding_ the hashtag (i.e., a boundary). We do boundary
# matching ourselves, so we don't need it.
#
# Also, nix grouping.
# Also, nix negative lookahead: RE2 doesn't support it, and it's probably super-rare anyway
#HASHTAG = "(\A|[^&#{HASHTAG_LETTERS_NUMERALS}])(#|＃)(?!\ufe0f|\u20e3)(#{HASHTAG_LETTERS_NUMERALS_SET}*#{HASHTAG_LETTERS_SET}#{HASHTAG_LETTERS_NUMERALS_SET}*)"
#
# DEVIATE_FROM_TWITTER: allow hashtags without letters. That makes things 10%
# faster. Plus, one can argue that "#1" should be a tag, anyway.
#HASHTAG = "[#＃]#{HASHTAG_LETTERS_NUMERALS_SET}*#{HASHTAG_LETTERS_SET}#{HASHTAG_LETTERS_NUMERALS_SET}*"
HASHTAG = "[#＃]#{HASHTAG_LETTERS_NUMERALS_SET}+"
REGEXEN[:valid_hashtag] = "(?:#{HASHTAG})"

# DEVIATE_FROM_TWITTER: same for :valid_mention_or_list:
# * Nix :valid_mention_preceding_chars
# * Nix grouping
# * Don't worry about {1,20} or {0,24}: just use + and *. 33% speedup with re2.
REGEXEN[:at_signs] = "[@＠]"
REGEXEN[:valid_mention_or_list] = "(?:#{REGEXEN[:at_signs]}[a-zA-Z0-9_]+(?:\\/[a-zA-Z][a-zA-Z0-9_\\-]*)?)"

DOMAIN_VALID_CHARS = "[^#{PUNCTUATION_CHARS}#{SPACE_CHARS}#{CTRL_CHARS}#{INVALID_CHARACTERS}#{UNICODE_SPACES}]"
REGEXEN[:valid_subdomain] = "(?:(?:#{DOMAIN_VALID_CHARS}(?:[_-]|#{DOMAIN_VALID_CHARS})*)?#{DOMAIN_VALID_CHARS}\\.)"
# DEVIATE_FROM_TWITTER: :valid_domain doesn't check GTLD or CCTLD (see above).
# That means :valid_domain_name has no "." at the end; instead, valid_punycode has it at the beginning
# It also means (valid_subdomain)+ instead of (valid_subdomain)*
REGEXEN[:valid_domain_name] = "(?:(?:#{DOMAIN_VALID_CHARS}(?:[-]|#{DOMAIN_VALID_CHARS})*)?#{DOMAIN_VALID_CHARS})"
REGEXEN[:valid_punycode] = "(?:\\.xn--[0-9a-z]+)"
REGEXEN[:valid_domain] = "(?:#{REGEXEN[:valid_subdomain]}+#{REGEXEN[:valid_domain_name]}(?:#{REGEXEN[:valid_punycode]})?)"

REGEXEN[:valid_port_number] = "[0-9]+"

REGEXEN[:valid_general_url_path_chars] = "[a-zA-Z\\p{Cyrillic}0-9!\\*';:=\\+\\,\\.\\$\\/%#\\[\\]\\-_~&\\|@#{LATIN_ACCENTS}]"
# Allow URL paths to contain up to two nested levels of balanced parens
#  1. Used in Wikipedia URLs like /Primer_(film)
#  2. Used in IIS sessions like /S(dfd346)/
#  3. Used in Rdio URLs like /track/We_Up_(Album_Version_(Edited))/
REGEXEN[:valid_url_balanced_parens] = "(?:#{REGEXEN[:valid_general_url_path_chars]}+|(?:#{REGEXEN[:valid_general_url_path_chars]}*\\(#{REGEXEN[:valid_general_url_path_chars]}+\\)#{REGEXEN[:valid_general_url_path_chars]}*))"

# Valid end-of-path chracters (so /foo. does not gobble the period).
#   1. Allow =&# for empty URL parameters and other URL-join artifacts
REGEXEN[:valid_url_path_ending_chars] = "[a-zA-Z\\p{Cyrillic}0-9=_#\\/\\+\\-#{LATIN_ACCENTS}]|(?:#{REGEXEN[:valid_url_balanced_parens]})"
REGEXEN[:valid_url_path] = "(?:(?:#{REGEXEN[:valid_general_url_path_chars]}*(?:#{REGEXEN[:valid_url_balanced_parens]}#{REGEXEN[:valid_general_url_path_chars]}*)*#{REGEXEN[:valid_url_path_ending_chars]})|(?:#{REGEXEN[:valid_general_url_path_chars]}+\\/))"

REGEXEN[:valid_url_query_chars] = "[a-zA-Z0-9!?\\*'\\(\\);:&=\\+\\$\\/%#\\[\\]\\-_\\.,~|@]"
REGEXEN[:valid_url_query_ending_chars] = "[a-zA-Z0-9_&=#\\/\\-]"
# DEVIATE_FROM_TWITTER: nix :valid_url_preceding_chars, and don't group
REGEXEN[:valid_url] = "(?:(?:https?:\\/\\/)?#{REGEXEN[:valid_domain]}(?::(#{REGEXEN[:valid_port_number]}))?(?:/#{REGEXEN[:valid_url_path]}*)?(?:\\?#{REGEXEN[:valid_url_query_chars]}*#{REGEXEN[:valid_url_query_ending_chars]})?)"

# Now add other regexes, from NLTK
# Source: https://github.com/nltk/nltk/blob/07bcb7ed51260f5da07be841aebf23235b6af96e/nltk/tokenize/casual.py

# NLTK_DEVIATE add "S" to emoticon
EMOTICON = "(?:[<>]?[:;=8][\\-o\\*\\']?[\\)\\]\\(\\[dDpPS/\\:\\}\\{@\\|\\\\]|[\\)\\]\\(\\[dDpPS/\\:\\}\\{@\\|\\\\][\\-o\\*\\']?[:;=8][<>]?|<3)"
ASCII_ARROW = "(?:-+>|<-+)"
HTML_TAG = "(?:<[^>\\s]+>)"
# NLTK_DEVIATE "phone number" is a bad-idea regex. But numbers with lots of
# dashes are great. Adjusted NUMBER to allow dashes within.
# NLTK_DEVIATE use \pL instead of [a-z]
# NLTK_DEVIATE rearrange (but keep logic the same) to speed up RE2 by 10%
WORD_WITH_APOSTROPHES_OR_DASHES = "(?:(?:\\pL+['\\-_]+)+\\pL+)"
# NLTK_DEVIATE use \pN instead of \d
NUMBER = "(?:[+\\-]?\\pN+([,/.:\\-]\\pN+)*[+\\-]?)"
# NLTK_DEVIATE use \pL\pN instead of \w
WORD_WITHOUT_APOSTROPHES_OR_DASHES = "(?:[\\pL\\pN_]+)"
# NLTK_DEVIATE nix ELLIPSIS_DOTS. NON_WHITESPACE fills that niche.
# NLTK_DEVIATE use REGEXEN[:space] -- there's more meat to it
# NLTK_DEVIATE also, avoid words: '"I' should be two tokens
NON_WHITESPACE = "(?:[^#{UNICODE_SPACES}\\pL\\pN]+)"

# The grand total...
#TOKEN = "#{REGEXEN[:valid_url]}|#{PHONE_NUMBER}|#{EMOTICON}|#{ASCII_ARROW}|#{REGEXEN[:valid_mention_or_list]}|#{REGEXEN[:valid_hashtag]}|#{WORD_WITH_APOSTROPHES_OR_DASHES}|#{NUMBER}|#{WORD_WITHOUT_APOSTROPHES_OR_DASHES}|#{ELLIPSIS_DOTS}|#{NON_WHITESPACE}"
TOKEN = "#{REGEXEN[:valid_url]}|#{EMOTICON}|#{ASCII_ARROW}|#{REGEXEN[:valid_mention_or_list]}|#{REGEXEN[:valid_hashtag]}|#{WORD_WITH_APOSTROPHES_OR_DASHES}|#{NUMBER}|#{WORD_WITHOUT_APOSTROPHES_OR_DASHES}|#{NON_WHITESPACE}"

# Converts from a Ruby String to a RE2-ready const char*
def quote_re2(re)
  re
    .inspect # expand non-ASCII; quote; backslash-escape things: all great for C++
    .gsub('\\#', '#')                                          # The one drawback to "inspect"
    .gsub(/\\u([0-9a-f]{4})/i) { "\\\\x{#{$1}}" }              # \u1234 -> \x{1234}
    .gsub(/\\u/, "\\\\x")                                      # \u{12345} -> \x{12345}
    .gsub(/([\u0080-\u{10ffff}])/) { "\\\\x{#{$1.ord.to_s(16).rjust(4, '0')}}" } # become ASCII
    # Remember: "\\\\" in Ruby becomes "\\" in C, which is the "\" character that we know and love
end

puts "Writing to src/token_regex.i: static const char* token_re = \"...\";"

open('src/token_regex.i', 'w') { |f| f.write("// Generated by ../build-regex/generate-c++.rb\n\nstatic const char* token_re = #{quote_re2(TOKEN)};\n") }
