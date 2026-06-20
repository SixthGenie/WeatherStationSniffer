#pragma once
// IHidTransport.h

#pragma once

#include <cstdint>
#include <vector>

class IHidTransport
{
public:
    virtual ~IHidTransport() = default;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool writeReport(
        const uint8_t* data,
        size_t length) = 0;

    virtual bool readReport(
        uint8_t* data,
        size_t length) = 0;
};