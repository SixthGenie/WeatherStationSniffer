#pragma once

#include "IHidTransport.h"

#include <windows.h>

class WindowsHidTransport
    : public IHidTransport
{
public:

    WindowsHidTransport();
    ~WindowsHidTransport();

    bool open() override;
    void close() override;

    bool writeReport(
        const uint8_t* data,
        size_t length) override;

    bool readReport(
        uint8_t* data,
        size_t length) override;

private:

    HANDLE m_handle;
}; 
