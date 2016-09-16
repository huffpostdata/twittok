#!/usr/bin/env node

'use strict'

const fs = require('fs')
const regexpu = require('regexpu')
const request = require('sync-request')

const unicode_data = request('GET', 'http://www.unicode.org/Public/UNIDATA/UnicodeData.txt').getBody('utf8');

/**
 * Turns 'L' or 'R' into regex-ready UTF-16 characters.
 *
 * JavaScript regexes (in web browsers) aren't ready for Unicode, because they
 * try to match one character at a time instead of one _codepoint_ at a time.
 * So we only return ranges within the Basic Multilingual Plane (BMP).
 */
function build_category_range(letter) {
  const letter_regex = new RegExp(`^[0-9A-F]+;[^;]+;${letter}`)
  const bmp_regex = /^(([0-9A-CEF][0-9A-F]{3})|(D[0-7][0-9A-F]{2}));/

  const ranges = [];

  let start = null;
  let maybe_end = null;

  unicode_data
    .split('\n')
    .filter(s => letter_regex.test(s))     // Filter for our character class
    .filter(s => bmp_regex.test(s))        // Filter for the BMP
    .map(s => parseInt(s.slice(0, 4), 16)) // to integer
    .forEach(i => {
      if (start === null || maybe_end + 1 != i) {
        if (start !== null) ranges.push([ start, maybe_end ])
        start = i
      }

      maybe_end = i
    })

  ranges.push([ start, maybe_end ])

  function encode(i) {
    return '\\u' + (0x10000 + i).toString(16).slice(1)
  }

  return ranges
    .map(arr => {
      return (arr[0] === arr[1]) ? encode(arr[0]) : `${encode(arr[0])}-${encode(arr[1])}`;
    })
    .join('')
}

const ranges = {
  L: build_category_range('L'),
  N: build_category_range('N'),
  M: build_category_range('M'),
  Nd: build_category_range('Nd')
}

const c_code = fs.readFileSync(`${__dirname}/../src/token_regex.i`, 'utf-8')
const c_escaped_string = c_code.split(/(?:token_re = ")|(?:";)/)[1]
const regex_string = c_escaped_string
  .replace(/\\\\x([a-z0-9A-Z]{2})/g, (_, m) => `\\x${m}`)
  .replace(/\\\\x\{([a-z0-9A-Z]{4})\}/g, (_, m) => `\\u${m}`)
  .replace(/\\\\/g, '\\')
  .replace(/\\p\{Cyrillic\}/g, '\u0400–\u04ff\u0500–\u052f\u2de0–\u2dff\ua640–\ua69f\u1c80–\u1c8f')
  .replace(/\\p([A-Z])/g, (_, m) => ranges[m])
  .replace(/\\p\{([^\}]+)\}/g, (_, m) => ranges[m])

fs.writeFileSync(`${__dirname}/token_regex.js`, `module.exports = /${regex_string}/;\n`, 'utf-8')
