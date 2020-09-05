#pragma once

#include "Interpreter.hpp"

class DLLACTION NodeCore
{
public:
    struct impl {
        virtual void run() = 0;
    };
    std::unique_ptr<impl> pImpl;
public:
    NodeCore();
};