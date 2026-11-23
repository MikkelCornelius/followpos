# regex-dfa

A C++17 library that compiles regular expressions into compact DFAs for incremental matching over byte sequences and UTF-8 text.

## Why

Most regex engines are black boxes: they report whether an input matches, but do not expose the automaton used to get that result. This library constructs the DFA directly from the pattern and keeps the matching state available while input is streamed.

That is useful for constrained generation, incremental parsers, and applications that need to find a pattern across tokens rather than only in a complete string.

The project originated as an experiment for [llama.cpp](https://github.com/ggml-org/llama.cpp), where a regular expression is evaluated one generated token at a time. It has no third-party runtime dependencies.

## How it works

The implementation follows the position/`followpos` construction described in [Compilers: Principles, Techniques, and Tools](https://faculty.sist.shanghaitech.edu.cn/faculty/songfu/cav/Dragon-book.pdf), chapter 3.9. This is also known as the Glushkov construction.

The library builds the syntax tree for the augmented pattern `(r)#`, annotates nodes with `nullable`, `firstpos`, and `lastpos`, and computes `followpos`. It does not build an intermediate NFA. Each DFA state is represented by a set of syntax positions and is accepting when it contains the position of the `#` marker.

The generated transition table uses packed `base`/`check`/`next_state` arrays. States are assigned numeric IDs, with state `0` reserved as the dead state.

## Supported patterns

- Concatenation and alternation: `ab`, `a|b`
- Repetition: `a*`
- Quantifiers: `a+`, `a?`, `a{n}`, `a{n,}`, `a{n,m}`
- Parenthesized groups
- Character classes and character ranges: `[abc]`, `[a-z]`, `[一-鿿]`
- UTF-8 literals and ranges
- Escape sequences for whitespace and literal characters
- `\s`, `\f`, `\n`, `\r`, `\t`, `\v`, `\0`, `\cX`, and `\xHH`

Some features are not implemented, including dot matches, negated and POSIX classes, anchors, non-greedy quantifiers, digit/word/whitespace shorthand classes, backreferences, lookarounds, and `\u` escapes. Unsupported constructs throw `std::logic_error`.

## API

The public header is `src/regex-to-dfa.h`. Compile a pattern and feed input one token or byte sequence at a time:

```cpp
#include "regex-to-dfa.h"

#include <list>
#include <string>

regex_dfa::DFA dfa = regex_dfa::regex_to_dfa("<tool_call>");
std::list<size_t> live_paths;

bool matched = regex_dfa::feed("<tool_", dfa, live_paths);
matched = regex_dfa::feed("call>", dfa, live_paths);
```

`feed()` restarts or continues its set of possible DFA paths for each call and returns `true` once an accepting state is reached. The example in `examples/usage.cpp` reads a token list and reports the first token containing a match.

## Building and testing

Requires a C++17 compiler and CMake 3.14 or newer.

```sh
make test
```

This configures `build/` with benchmarks disabled, builds the library and tests, and runs CTest. Test coverage is in `tests/test-regex-dfa.cpp`.

Run the usage example with:

```sh
make usage
```

## Benchmark

An optional benchmark executable compares this implementation with `std::regex` and VectorScan. It is disabled by default and requires the bundled VectorScan sources/build and a discoverable `hs` library:

```sh
make benchmark
```

The benchmark runs isolated workers from `tests/bench/content/` for each engine and pattern.

## Layout

```
src/       library implementation and public header
examples/  incremental-matching example
tests/     unit tests and benchmark sources
Makefile   test, usage, and benchmark targets
```
