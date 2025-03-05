# Seron Compiler

## Pipeline

- Lexical Analysis -> Tokenlist
- Recursive Descent Parsing -> AST
- Symboltable Construction -> Symboltable
- Typechecking
- Semantic Analysis
- Code Generation -> Assembly
*(External Tools)*
- Assembly (NASM) -> Object code
- Linking (LD / CC) -> ELF64 Binary
