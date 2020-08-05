# Noder — Node toolkit

Noder project is collection of tools designed to help you create applications and libraries using nodes instead of code and to allow node execution and compilation from code.

## Authors

- Michał Osiński(Niikelion) - nicraft@tlen.pl

## Noder Gui(not implemented)

Visual tool for creating node programs. Provides functionality to create projects, interpret nodes on the fly and compile them to binary form.

## Noder Command Utils(not implemented)

Command line tool for creating node programs. Provides same functionality as Noder Gui and operates on same projects.

## Noder Core

Core library that is used in every tool from Noder toolkit. Can be integrated into C++ projects to minimize overhead introduced by external command line tools.
Provides both interpreter and compiler utilities and allows transfer of node enviroment between them.

## Bindings to other languages(not implemented)

Shared library for bindings exposes C style API for Noder Core that can be used to create bindings to other languages.

## License

Source code of Noder Core is licensed under MIT License.
License of every tool is located in root directory of that tool.