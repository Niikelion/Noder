# Noder — Node toolkit

Noder project is collection of tools designed to help you create applications and libraries using nodes instead of code and allows node execution and compilation from code.

## Contents

[Authors](#Authors)
[Getting started](#Getting-started)

## Authors

- Michał Osiński(Niikelion) - nicraft@tlen.pl

## Getting started

### Building Noder tools

Currently Noder only includes console utilities (noder-cu) and only supports interpreter mode.

Build dependencies:
| Module/Tool | Dependencies |
| ----------- | ------------ |
| Core(always included) | [Parselib](https://github.com/Niikelion/ParseLib) |
| Interpreter(enabled via `NODER_MODULES`) | Core |
| Compiler(enabled via `NODER_MODULES`) | Core, LLVM |
| noder-cu(enabled via `BUILD_TOOLS`) | Interpreter, pybind11 |

Building all tools using cmake:
```sh
$ cmake <path_to_sources> -DBUILD_TOOLS=TRUE -DNODER_MODULES="interpreter"
```

Cmake options:
```
NODER_MODULES:STRING
```
Specifies modules to build separated by `;`. Available modules: `interpreter,compiler`. `all` is equivalent to `interpreter;compiler`. Defaults to `all`.

```
BUILD_TOOLS:BOOL
```
Enabled tools building. Defaults to `FALSE`.

```
BUILD_TESTS:BOOL
```
Build and run tests for Noder components. Defaults to `FALSE`

## License

Source code of Noder Core is licensed under MIT License.
License of every tool is located in root directory of that tool.
