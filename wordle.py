#!/bin/env python3

"""Fun with the wordle game: https://www.nytimes.com/games/wordle/index.html
We look up matching words in /usr/share/dict/words using the info we get for 
the characters on every step of the game.
"""

import os
import sys
import re

import logging
logging.basicConfig(level=os.environ.get('LOGLEVEL', 'INFO'))
log = logging.getLogger('wordle.py')

WORD_LEN = 5
WORD_FILE = '/usr/share/dict/words'
AZ_REGEX = re.compile('[a-z]')

class WordFinder(object):
    words = None

    def __init__(self):
        """Create the object
        """
        # Grey characters are stored in a python set:
        self.greyCharSet = set()
        # Green ones are a 5-element char list: theit positions matter!
        self.greenChars = [''] * WORD_LEN
        # Yellow ones are a 5-element list of char sets. It's because we need
        # to store multiple characters per each position:
        self.yellowChars = [set() for x in range(WORD_LEN)]

        if not self.words:
            # Read the word file, but only keep 5-letter words that only contain 
            # lowercase letters:
            self.words = []
            regex = re.compile('^' + '[a-z]' * WORD_LEN + '$')
            
            for line in open(WORD_FILE):
                line = line.strip()
                if regex.match(line):
                    self.words.append(line)

    def addGreenChar(self, char, pos):
        """Add a green char at a specified position.
        """
        assert(pos >=0  and pos < WORD_LEN)
        assert(len(char) == 1)
        char = char.lower()
        if AZ_REGEX.match(char):
            self.greenChars[pos] = char
            # Also let's remove the char from the grey ones:
            if char in self.greyCharSet:
                self.greyCharSet.remove(char)

    def addYellowChar(self, char, pos):
        """Add a yellow char at a specified position.
        """
        assert(pos >=0  and pos < WORD_LEN)
        assert(len(char) == 1)
        char = char.lower()
        if AZ_REGEX.match(char):
            self.yellowChars[pos].add(char)
            # Also let's remove the char from the grey ones:
            if char in self.greyCharSet:
                self.greyCharSet.remove(char)

    def addGreyChar(self, char):
        """Add a grey char, no positions necessary.
        """
        assert(len(char) == 1)
        if AZ_REGEX.match(char):
            # NOTE: let's make sure we never add chars that are already 
            # in green or yellow sets - wordle can paint the duplicates grey:
            if char in self.greenChars or \
               any([char in x for x in self.yellowChars]):
                return

            self.greyCharSet.add(char)

    def findMatchingWords(self):
        """Filter the word list and return the ones that match the previously
        specified criteria.
        """
        log.debug('Initial set: {} words'.format(len(self.words)))

        # We need a set of all yellow and green characters:
        allYellowAndGreenChars = set()

        # Prepare a regex to exclude characters at the yellow positions:
        yellowExcludeParts = []
        
        for i, chars in enumerate(self.yellowChars):
            allYellowAndGreenChars.update(chars)
            if not chars:
                continue
            yellowExcludeParts.append('.'*i + 
                         '[' + ''.join(chars) + ']' + 
                         '.'*(WORD_LEN-i-1))
        yellowExcludeRegex = '|'.join(yellowExcludeParts)

        allYellowAndGreenChars.update([x for x in self.greenChars if x])
        
        # Prepare a regex to only include words with characters in green positions: 
        greenIncludeRegex = ''.join([(x or '.') for x in self.greenChars])

        # Prepare a regex to exclude words with grey characters: 
        greyExcludeRegex = '[' + ''.join(self.greyCharSet) + ']'

        log.debug('greyExcludeRegex: {}'.format(repr(greyExcludeRegex)))
        log.debug('yellowExcludeRegex: {}'.format(repr(yellowExcludeRegex)))
        log.debug('greenIncludeRegex: {}'.format(repr(greenIncludeRegex))) 
        log.debug('allYellowAndGreenChars: {}'.format(repr(allYellowAndGreenChars))) 

        greyExcludeRegex = None if greyExcludeRegex == '[]' \
                           else re.compile(greyExcludeRegex)
        yellowExcludeRegex = re.compile(yellowExcludeRegex) if yellowExcludeRegex \
                             else None
        greenIncludeRegex = re.compile(greenIncludeRegex)

        result = list(self.words)

        if greyExcludeRegex:
            result = [x for x in result if not greyExcludeRegex.search(x)]
            log.debug('After greyExcludeRegex: {}'.format(len(result)))

        if yellowExcludeRegex:
            result = [x for x in result if not yellowExcludeRegex.search(x)]
            log.debug('After yellowExcludeRegex: {}'.format(len(result)))

        if greenIncludeRegex:
            result = [x for x in result if greenIncludeRegex.search(x)]
            log.debug('After greenIncludeRegex: {}'.format(len(result)))

        # The word can only be considered "good" if all yellow and green 
        # characters are present in it:
        if allYellowAndGreenChars:
            result = [x for x in result \
                      if allYellowAndGreenChars.intersection(x) == allYellowAndGreenChars]
            log.debug('After matching all yellow/green chars: {}'.format(len(result)))

        return result

def main():
    finder = WordFinder()
    iteration = 0

    while True:
        iteration += 1

        log.info('Step {}'.format(iteration))

        print('Enter non-matching (grey) characters (order or delimiters do not matter):')
        line = sys.stdin.readline().strip().lower()
        for ch in line:
            try:
                finder.addGreyChar(ch)
            except:
                pass
                    
        print('Enter 0 or {} yellow characters, ' \
               'use a period (.) for grey or green ones:'.format(WORD_LEN))
        while True:
            line = sys.stdin.readline().strip().lower()
            
            if len(line) not in (0, WORD_LEN):
                log.warning('Wrong number of characters, try again...') 
                continue

            for i, ch in enumerate(line):
                if not AZ_REGEX.match(ch):
                    ch = '.'
                finder.addYellowChar(ch, i)

            break
            
        print('Enter 0 or {} green characters, ' \
               'use a period (.) for grey or yellow ones:'.format(WORD_LEN))
        while True:
            line = sys.stdin.readline().strip().lower()
            
            if len(line) not in (0, WORD_LEN):
                log.warning('Wrong number of characters, try again...') 
                continue

            for i, ch in enumerate(line):
                if not AZ_REGEX.match(ch):
                    ch = '.'
                finder.addGreenChar(ch, i)

            break

        found = finder.findMatchingWords()

        log.info('Found {} matching word(s). Want to see them? [y/N/x]?'.format(len(found)))
        line = sys.stdin.readline().strip().lower()
        
        if 'y' in line or ('x' in line and len(found) < 50):
            print('-' * 5) 
            for word in found:
                print(word)
            print('-' * 5) 

        if 'x' in line:
            return


if __name__ == '__main__':
    if not os.path.exists(WORD_FILE):
        log.error('Word file ({}) not found. Make sure you run this script ' \
                  'in a Unix-like environment'.format(WORD_FILE))
        sys.exit(1)

    main()

