# regex-dfa

A small C++ library that compiles a regular expression into a DFA and exposes its internal structure, so the automaton can be stepped through one symbol at a time.

> **Status:** work in progress. The API is not stable and most of it is not implemented yet.

## Why

Most regex engines are black boxes. They tell you whether a pattern matches, but not how, and they offer no way to customize the matching process. `std::regex`, for example, can only match against a complete string.

This library builds the DFA directly from the pattern and keeps it accessible. That makes it possible to feed input incrementally (streaming) and inspect the current state at any point, such as which symbols are still allowed next.

The project started as a feature for [llama.cpp](https://github.com/ggml-org/llama.cpp), where constraining generation token by token needs exactly this kind of access, but it has no dependencies and can be used for anything. Naming and style follow llama.cpp conventions to make an eventual merge easy.

## How it works

The algorithms implemented are based on the teachings of [Dragon Book](https://faculty.sist.shanghaitech.edu.cn/faculty/songfu/cav/Dragon-book.pdf), particularly chapter 3.9. The regex to DFA algorithm is refered to as `followpos` but is also known under the name `Glushkov`.

The DFA is constructed directly from the syntax tree of the augmented pattern `(r)#`, using the `nullable`, `firstpos`, `lastpos` and `followpos` functions (the algorithm from the "Dragon Book"). No intermediate NFA is built. A DFA state is a set of positions in the pattern, and a state is accepting if it contains the position of the `#` end marker.

## Building and testing

Requires a C++17 compiler and CMake 3.14 or newer.

```
make test
```

This configures, builds, and runs the tests with CTest. Tests live in `tests/` and follow the `test-*.cpp` convention used by llama.cpp.

## Layout

```
src/     library sources and header
tests/   test programs
```
