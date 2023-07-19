#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp() : _microSecondsSinceEpoch(0) {}
    explicit Timestamp(int64_t microSeconds)
        : _microSecondsSinceEpoch(microSeconds) {}
    static Timestamp now();
    std::string toString() const;

private:
    int64_t _microSecondsSinceEpoch;
};