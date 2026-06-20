#pragma once
// WeatherStation.h

#pragma once

#include "IHidTransport.h"

class WeatherStation
{
public:

    explicit WeatherStation(IHidTransport& transport);

    bool readDeviceInfo(std::vector<uint8_t>& response);

    bool readCurrentRecord(std::vector<uint8_t>& response);

    bool readMaxMin(std::vector<uint8_t>& response);
	uint8_t indoorHumidity() const;
	uint8_t outdoorHumidity() const;
	double absolutePressure() const;
	double relativePressure() const;

private:
    static uint16_t le16(const std::vector<uint8_t>& p,size_t offset);
    bool executeCommand(uint8_t command,std::vector<uint8_t>& response);
    std::vector<uint8_t> lastPacket_;

    IHidTransport& m_transport;
};