<p align="center"><img src="assets/project-logo.png" alt="guider-logo" width="256" height="256" /></p>

# Noder — Node toolkit

Noder project is collection of tools designed to help you create applications and libraries using nodes instead of code and allows node execution and compilation from code.

## Contents

[Authors](#Authors)
[Getting started](#Getting-started)
[Using tools](#Using-tools)

## Authors

- Michał Osiński(Niikelion) - nicraft@tlen.pl

## Getting started

### Building Noder

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

```
LLVM_CONFIG:FILEPATH
```
Specifies path to llvm_config executable. Can be combined with `LLVM_CONFIG_D` for the cmake's multigenerators.

```
LLVM_CONFIG_D:FILEPATH
```
Specifies path to llvm_config executable for debug builds if using the cmake's multigenerators.

# Using tools

## Noder-cu

Run for command list:
```sh
noder-cu -h
```

## License

Source code of Noder Core is licensed under [MIT](LICENSE) License.
License of every tool is located in root directory of that tool.
