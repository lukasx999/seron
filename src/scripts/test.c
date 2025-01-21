#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "../util.h"
#include "../lexer.h"
#include "../parser.h"



static TokenList lexer_testcase(const char *testcase, const Token *expected, size_t expected_size) {
    printf("[TEST] Testing with: `%s`\n", testcase);

    TokenList tokens = tokenize(testcase);

    bool passed = tokens.size == expected_size;

    for (size_t i=0; i < tokens.size; ++i) {
        Token *tok = &tokens.items[i];
        if (memcmp(tok, expected+i, sizeof(Token)))
            passed = false;
    }

    if (passed)
        printf("[TEST] [Lexer] %s%sPASSED%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_END);
    else
        printf("[TEST] [Lexer] %s%sFAILED%s\n", COLOR_BOLD, COLOR_RED, COLOR_END);

    return tokens;
}

#if 0
static void parser_testcase(const AstNode *expected, TokenList tokens) {
    AstNode *root = parser_parse(&tokens);

    // TODO: this
    parser_free_ast(root);
    tokenlist_destroy(&tokens);
}
#endif

static void test(void) {

    Token case1[] = {
        (Token) { TOK_NUMBER, "123" },
        (Token) { TOK_PLUS,   ""    },
        (Token) { TOK_NUMBER, "2"   },
        (Token) { TOK_MINUS,  ""    },
        (Token) { TOK_NUMBER, "3"   },
    };

    // parser_testcase(NULL, lexer_testcase("123+2-3", case1, ARRAY_LEN(case1)));
    lexer_testcase("123\n\n\t\t    +2-3", case1, ARRAY_LEN(case1));

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

    Token case5[] = {
        (Token) { TOK_KW_FUNCTION, ""     },
        (Token) { TOK_IDENTIFIER,  "main" },
        (Token) { TOK_LPAREN,      ""     },
        (Token) { TOK_IDENTIFIER,  "a"    },
        (Token) { TOK_COMMA,       ""     },
        (Token) { TOK_IDENTIFIER,  "b"    },
        (Token) { TOK_COMMA,       ""     },
        (Token) { TOK_IDENTIFIER,  "c"    },
        (Token) { TOK_RPAREN,      ""     },
        (Token) { TOK_LBRACE,      ""     },
        (Token) { TOK_RBRACE,      ""     },
    };
    lexer_testcase("proc main(a, b, c) {}", case5, ARRAY_LEN(case5));

    Token case6[] = {
        (Token) { TOK_KW_VARDECL, ""    },
        (Token) { TOK_IDENTIFIER, "foo" },
        (Token) { TOK_ASSIGN,     ""    },
        (Token) { TOK_NUMBER,     "1"   },
        (Token) { TOK_PLUS,       ""    },
        (Token) { TOK_LPAREN,     ""    },
        (Token) { TOK_NUMBER,     "2"   },
        (Token) { TOK_MINUS,      ""    },
        (Token) { TOK_NUMBER,     "3"   },
        (Token) { TOK_RPAREN,     ""    },
        (Token) { TOK_SEMICOLON,  ""    },
    };
    lexer_testcase("val foo = 1+(2-3);", case6, ARRAY_LEN(case6));

    Token case7[] = {
        (Token) { TOK_IDENTIFIER, "foo" },
        (Token) { TOK_IDENTIFIER, "bar" },
        (Token) { TOK_SEMICOLON,  ""    },
    };
    lexer_testcase("foo#abcABC12!§$%üäö+#-.,\nbar;", case7, ARRAY_LEN(case7));

    Token case8[] = {
        (Token) { TOK_NUMBER, "1" },
        (Token) { TOK_PLUS,   ""  },
        (Token) { TOK_NUMBER, "2" },
        (Token) { TOK_NUMBER, "3" },
        (Token) { TOK_PLUS,   ""  },
        (Token) { TOK_NUMBER, "4" },
    };
    lexer_testcase("1+2 ## abc#####abc12 ## 3+4", case8, ARRAY_LEN(case8));

    Token case9[] = {
        (Token) { TOK_IDENTIFIER, "a"       },
        (Token) { TOK_ASSIGN,     ""        },
        (Token) { TOK_STRING,     "str"     },
        (Token) { TOK_SEMICOLON,  ""        },
        (Token) { TOK_IDENTIFIER, "b"       },
        (Token) { TOK_ASSIGN,     ""        },
        (Token) { TOK_STRING,     "string"  },
    };
    lexer_testcase("a = \"str\"; b = \"string\"", case9, ARRAY_LEN(case9));

}



int main(void) {

    printf("\n");
    test();
    printf("\n");


    return EXIT_SUCCESS;
}
