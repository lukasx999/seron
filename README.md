# Seron Programming Language

<img src="./assets/logo.svg" width="200" height="200">

## Features

- Compiled to native Assembly
- Static & Strong Typing
- **No** dependencies

Currently, the only supported backend is x86_64 NASM.

## Build Requirements

- C11 Toolchain
- Nothing else!

## Usage Requirements

- NASM *(for assembly)*
- CC *(for linking with C runtime)*

## Building Compiler

```
$ git clone https://github.com/lukasx999/seron.git
$ cd seronc/src
$ make
$ ./seronc
```

## Building `compile_commands.json` for clangd

```
$ pipx install compiledb
$ compiledb make
```

## Language Features

- [x] Extern Procedures
- [x] Add / Sub / Mul / Div
- [x] Procedure calls
- [x] Inline Assembly
  - [x] Parameters
- [x] Procedures
  - [x] Parameters
  - [x] Returnvalue
- [x] Variables
- [x] While Loop
- [x] If/Else Statement
