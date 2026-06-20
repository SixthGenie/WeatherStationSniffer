#include "WindowsHidTransport.h"

#include <setupapi.h>
#include <hidsdi.h>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib,"hid.lib")

WindowsHidTransport::WindowsHidTransport()
    : m_handle(INVALID_HANDLE_VALUE)
{}

WindowsHidTransport::~WindowsHidTransport()
{
    close();
}
void WindowsHidTransport::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}
bool WindowsHidTransport::writeReport(
    const uint8_t* data,
    size_t length)
{
    DWORD written = 0;

    return WriteFile(
        m_handle,
        data,
        (DWORD)length,
        &written,
        nullptr) != FALSE;
}
bool WindowsHidTransport::readReport(
    uint8_t* data,
    size_t length)
{
    DWORD read = 0;

    return ReadFile(
        m_handle,
        data,
        (DWORD)length,
        &read,
        nullptr) != FALSE;
}
bool WindowsHidTransport::open()
{
    GUID guid;

    HidD_GetHidGuid(&guid);

    HDEVINFO devInfo =
        SetupDiGetClassDevs(
            &guid,
            nullptr,
            nullptr,
            DIGCF_PRESENT |
            DIGCF_DEVICEINTERFACE);

    if (devInfo == INVALID_HANDLE_VALUE)
        return false;

    SP_DEVICE_INTERFACE_DATA ifData{};
    ifData.cbSize = sizeof(ifData);

    for (DWORD i = 0;
        SetupDiEnumDeviceInterfaces(
            devInfo,
            nullptr,
            &guid,
            i,
            &ifData);
        ++i)
    {
        DWORD needed = 0;

        SetupDiGetDeviceInterfaceDetail(
            devInfo,
            &ifData,
            nullptr,
            0,
            &needed,
            nullptr);

        auto detail =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA)
            malloc(needed);

        detail->cbSize =
            sizeof(
                SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(
            devInfo,
            &ifData,
            detail,
            needed,
            nullptr,
            nullptr))
        {
            free(detail);
            continue;
        }

        HANDLE h =
            CreateFile(
                detail->DevicePath,
                GENERIC_READ |
                GENERIC_WRITE,
                FILE_SHARE_READ |
                FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);

        if (h == INVALID_HANDLE_VALUE)
        {
            free(detail);
            continue;
        }

        HIDD_ATTRIBUTES attr{};
        attr.Size = sizeof(attr);

        if (HidD_GetAttributes(
            h,
            &attr))
        {
            if (attr.VendorID == 0x1130 &&
                attr.ProductID == 0x0829)
            {
                m_handle = h;

                free(detail);

                SetupDiDestroyDeviceInfoList(
                    devInfo);

                return true;
            }
        }

        CloseHandle(h);
        free(detail);
    }

    SetupDiDestroyDeviceInfoList(
        devInfo);

    return false;
}