#pragma once
// WeatherStation.h

#pragma once

#include "IHidTransport.h"
#include <string>

class WeatherStation
{
public:

    explicit WeatherStation(IHidTransport& transport);

    bool readDeviceInfo(std::vector<uint8_t>& response);
    bool readCurrentRecord(std::vector<uint8_t>& response);
    
	uint8_t indoorHumidity() const;
	uint8_t outdoorHumidity() const;
	double absolutePressure() const;
	double relativePressure() const;
    std::string windDirection() const;
    std::string formatWindDirection(double degrees) const;
	std::int32_t windDirectionDegrees() const;
	double windSpeed() const;
	double windGust() const;
	double indoorTemperature() const;
	double outdoorTemperature() const;
    double rainHour() const;
    double rainDay() const;
    double rainWeek() const;
    double rainMonth() const;
    double rainTotal() const;
    double dewPoint() const;
    double feelsLike() const;

private:
    static uint16_t le16(const std::vector<uint8_t>& p,size_t offset);
    bool executeCommand(uint8_t command,std::vector<uint8_t>& response);
    std::vector<uint8_t> lastPacket_;

    IHidTransport& m_transport;
};