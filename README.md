# Seron Programming Language

<img src="./assets/logo.svg" width="200" height="200">

## Features

- Compiled to native Assembly
- Static & Strong Typing
- Familiar C/Rust-style syntax
- Easy interop with C APIs
- Building requires no dependencies

Currently, the only supported backend is x86_64 NASM.

## Build Requirements

- C11 Toolchain

## Usage Requirements

- NASM *(for assembling)*
- CC *(for linking with C runtime)*

## Building Compiler

```
$ git clone https://github.com/lukasx999/seron.git
$ cd seron/seronc
$ make
$ ./seronc
```

## Developer Tooling

Currently, only syntax highlighting is available for Vim. See `editor/`.

## Building `compile_commands.json` for clangd

Requires `pipx` to be installed

```
$ pipx install compiledb
$ compiledb make
```
