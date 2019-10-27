#!/bin/env python

"""
It deosn't mttaer in waht oredr the ltteers in a wrod are, the olny iprmoetnt
tihng is taht the frist and lsat ltteer be at the rghit pclae...

Let's prove the famous statement wrong!

Usage:
word_shuffle.py             : process and print the default text
word_shuffle.py -           : type your text and hit Ctrl+D when you're done
word_shuffle.py <filename>  : process text from <filename>
"""

import os
import sys
import re
import random

DEFAULT_TEXT = \
"""The account proposed by Richard Shillcock and colleagues, 
also suggests another mechanism that could be at work in the meme. 
They propose a model of word recognition in which each word is split 
in half since the information at the retina is split between the 
two hemispheres of the brain when we read. In some of the simulations 
of their model, Richard Shillcock simulates the effect of jumbling 
letters in each half of the word. It seems that keeping letters in 
the appropriate half of the word, reduces the difficulty of reading 
jumbled text. This approach was used in generating example (1) above, 
but not for (2) or (3).
None of the words that have reordered letters create another word 
(wouthit vs witohut). We know from existing work, that words that can 
be confused by swapping interior letters (e.g. salt and slat) are more 
difficult to read. To make an easy to read jumbled word you should 
therefore avoid making other words.
"""

def shuffle_chars(s):
    """Randomly shuffle characters in a string and return a new string
    """
    assert(len(s) >= 2)

    result = s

    while result == s:
        pairs = [(x, random.random()) for x in s]
        pairs.sort(cmp=lambda *arg: cmp(arg[0][1], arg[1][1]))
        result = ''.join([x[0] for x in pairs])

    return result

def shuffle_word(w):
    """Keep first and last chars and shuffle the ones in between.
    Also preserve leading and trailing punctuation chars.
    """
    PUNCTUATION = '.,!?`~:;-+/"\'<>[](){}'
    pre = ''
    suf = ''
    mid = w

    # Strip the punctuation off both sides:
    while mid and mid[0] in PUNCTUATION:
        pre += mid[0]
        mid = mid[1:]

    while mid and mid[-1] in PUNCTUATION:
        suf += mid[-1]
        mid = mid[:-1]

    # If the stripped part is 3 chars or shorter,
    # we would not be able to shuffle the middle:
    if len(mid) <= 3:
        return w

    # Strip the first and last and shuffle the middle:
    pre += mid[0]
    suf = mid[-1] + suf
    mid = mid[1:-1]

    # Now join all the parts back together:
    return pre + shuffle_chars(mid) + suf


def shuffle_line(s):
    """Process a line of text
    """
    newWords = []

    for word in s.split(' '):
        # Skip numbers and short strings:
        if word.isdigit() or len(word) <= 3:
            newWords.append(word)
        else:
            # Process the word by keeping the first and the last chars
            # and shuffling the ones in between:
            newWords.append(shuffle_word(word))

    return ' '.join(newWords)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        text = DEFAULT_TEXT.splitlines()
    elif sys.argv[1] == '-':
        text = sys.stdin.readlines()
    else:
        text = file(sys.argv[1]).readlines()

    text = [x.decode('utf-8') for x in text]

    sys.stderr.write('Output:\n')

    for line in text:
        print shuffle_line(line.rstrip())
