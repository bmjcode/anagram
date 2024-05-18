#!/usr/bin/env python3

"""Extract words from Wikipedia's Basic English combined wordlist."""

import re

# Regular expression to extract words from inside wikilinks: [[dance]]
#                                                              ^^^^^
# with or without a parenthetical after: [[pointing]] (at)
#                                          ^^^^^^^^    ^^
# If there's a pipe like in [[wikt:wild|wild]], we will deal with it later.
RX_WORD = re.compile(r'\[\[([^\]]+)\]\](?: \(([a-z]+)\))?')

words = []
with open("basic_english.wikitext", "r") as src:
    for line in src:
        if not line.startswith("'''"):
            continue
        for match in RX_WORD.findall(line):
            # Join multiple words into a phrase
            if isinstance(match, str):
                word = match.strip()
            else:
                word = " ".join(match).strip()

            # If there's a pipe, the right side is the displayed text
            if "|" in word:
                left, right = word.split("|", 1)
                if right:
                    word = right
                elif ":" in left:
                    word = left.split(":")[1]

            if not word in words:
                words.append(word)

words.sort()
for word in words:
    print(word)
