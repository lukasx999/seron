#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./util.h"
#include "./lexer.h"



void lexer_testcase(const char *src, const Token *testcase, size_t case_size) {
    printf("[TEST] Testing with: `%s`\n", src);

    TokenList tokens = tokenize(src);

    bool passed = tokens.size == case_size;

    for (size_t i=0; i < tokens.size; ++i) {
        Token *tok = &tokens.items[i];
        if (memcmp(tok, testcase+i, sizeof(Token)))
            passed = false;
    }

    if (passed)
        printf("[TEST] %s%sPASSED%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_END);
    else
        printf("[TEST] %s%sFAILED%s\n", COLOR_BOLD, COLOR_RED, COLOR_END);

    tokenlist_destroy(&tokens);
}

void test_lexer(void) {

    Token case1[] = {
        (Token) { TOK_NUMBER, "123" },
        (Token) { TOK_PLUS,   ""    },
        (Token) { TOK_NUMBER, "2"   },
        (Token) { TOK_MINUS,  ""    },
        (Token) { TOK_NUMBER, "3"   },
    };
    lexer_testcase("123+2-3", case1, ARRAY_LEN(case1));

    Token case2[] = {
        (Token) { TOK_NUMBER,   "4312" },
        (Token) { TOK_PLUS,     ""     },
        (Token) { TOK_NUMBER,   "12"   },
        (Token) { TOK_MINUS,    ""     },
        (Token) { TOK_NUMBER,   "4"    },
        (Token) { TOK_SLASH,    ""     },
        (Token) { TOK_NUMBER,   "2"    },
        (Token) { TOK_ASTERISK, ""     },
        (Token) { TOK_NUMBER,   "3"    },
    };
    lexer_testcase("4312 + 12-4/2*3", case2, ARRAY_LEN(case2));

    Token case3[] = {
        (Token) { TOK_IDENTIFIER, "foo_123" },
        (Token) { TOK_ASSIGN,     ""        },
        (Token) { TOK_NUMBER,     "123"     },
        (Token) { TOK_PLUS,       ""        },
        (Token) { TOK_NUMBER,     "456"     },
    };
    lexer_testcase("foo_123 = 123 + 456", case3, ARRAY_LEN(case3));

    Token case4[] = {
        (Token) { TOK_IDENTIFIER, "func" },
        (Token) { TOK_LPAREN,     ""     },
        (Token) { TOK_RPAREN,     ""     },
        (Token) { TOK_EQUALS,     ""     },
        (Token) { TOK_NUMBER,     "1"    },
    };
    lexer_testcase("func() == 1", case4, ARRAY_LEN(case4));


}


int main(void) {

    puts("");
    test_lexer();
    puts("");

    return EXIT_SUCCESS;
}
