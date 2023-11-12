#pragma once

class PumpStatus {
public:
    enum Code {
        off = 0,
        forwards,     // 1
        backwards  // 2
    } _code;

    PumpStatus(Code code) : _code(code) {}

    Code code() const {
        return _code;
    }

    // Implicit conversion to int
    operator int() const {
        return _code;
    }
};