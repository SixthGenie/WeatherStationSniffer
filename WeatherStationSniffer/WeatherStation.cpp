#include "WeatherStation.h"

#include <cstring>

WeatherStation::WeatherStation(
    IHidTransport& transport)
    : m_transport(transport)
{}

bool WeatherStation::readDeviceInfo(
    std::vector<uint8_t>& packet)
{
    return executeCommand(
        0x01,
        packet);
}

bool WeatherStation::readCurrentRecord(std::vector<uint8_t>& packet)
{
    const auto b = executeCommand(0x04,packet);
    lastPacket_ = packet;
    return b;
}

uint8_t WeatherStation::indoorHumidity() const
{
    return lastPacket_[0x09];
}

uint8_t WeatherStation::outdoorHumidity() const
{
    return lastPacket_[0x16];
}

double WeatherStation::absolutePressure() const
{
    return  le16(lastPacket_, 0x36) / 10.0;
}

double WeatherStation::relativePressure() const
{
    return le16(lastPacket_, 0x38) / 10.0;
}

uint16_t WeatherStation::le16(const std::vector<uint8_t>& p, size_t offset)
{
    return
        static_cast<uint16_t>(p[offset]) |
        (static_cast<uint16_t>(p[offset + 1]) << 8);
}

bool WeatherStation::executeCommand(
    uint8_t command,
    std::vector<uint8_t>& packet)
{
    uint8_t tx[65]{};

    tx[0] = 0;

    tx[1] = 3;
    tx[2] = command;
    tx[3] = tx[1] + tx[2];

    if (!m_transport.writeReport(
        tx,
        sizeof(tx)))
    {
        return false;
    }

    uint8_t rx[128]{};

    uint8_t report[65]{};

    if (!m_transport.readReport(
        report,
        sizeof(report)))
    {
        return false;
    }

    memcpy(
        rx,
        report + 1,
        64);

    uint8_t len = rx[0];

    if (len > 64)
    {
        if (!m_transport.readReport(
            report,
            sizeof(report)))
        {
            return false;
        }

        memcpy(
            rx + 64,
            report + 1,
            64);
    }

    uint8_t checksum = 0;

    for (uint8_t i = 0;
        i < len - 1;
        ++i)
    {
        checksum += rx[i];
    }

    if (checksum != rx[len - 1])
    {
        printf(
            "Checksum mismatch "
            "(%02X != %02X)\n",
            checksum,
            rx[len - 1]);

        return false;
    }

    packet.assign(
        rx,
        rx + len);

    return true;
}