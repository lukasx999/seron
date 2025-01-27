#!/usr/bin/env python3

import sys
import re
from pprint import pprint

# Get BNF grammar from source code


GRAMMAR_SRC: str = "grammar.c"


def main() -> int:

    with open(GRAMMAR_SRC) as file:
        lines: list[str] = file.readlines()
        matches = [line.strip()[3:] for line in lines if bool(re.search(r"//\s.*::=.*$", line))]

        maxdist: int = 0
        for match in matches:
            if (dist := match.find("::=")) > maxdist:
                maxdist = dist

        for match in matches:
            index = match.index("::=")
            dist = maxdist - index
            new = list(match)
            for _ in range(0, dist):
                new.insert(index, ' ')
            print("".join(new))

    return 0

if __name__ == '__main__':
    sys.exit(main())
