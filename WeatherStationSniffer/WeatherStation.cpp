#include "WeatherStation.h"

#include <cstring>
#include <cmath>
WeatherStation::WeatherStation(IHidTransport& transport)
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

std::int32_t WeatherStation::windDirectionDegrees() const
{
    return lastPacket_[0x3d];
}
std::string WeatherStation::formatWindDirection(double degrees) const
{
    // 16-point compass directions array
    static const std::vector<std::string> compass = {
        "N", "NNE", "NE", "ENE",
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
    };

    // Sanitize and normalize input to ensure it falls strictly between 0 and 359.99
    double normalized = std::fmod(degrees, 360.0);
    if (normalized < 0) {
        normalized += 360.0;
    }

    // Add 11.25 degrees to offset the boundaries, then divide by the 22.5 degree index width
    int index = static_cast<int>((normalized + 11.25) / 22.5);

    // Wrap index 16 back around to 0 (North)
    return compass[index % 16];
}
std::string WeatherStation::windDirection() const
{
    // Extract the raw byte value (your dumps report values like 180 and 194)
    double degrees = static_cast<double>(lastPacket_[0x3D]);

    // Convert to text string
    return formatWindDirection(degrees);
}
static uint16_t readLE16(const uint8_t* p)
{
    return p[0] | (p[1] << 8);
}

static double decodeOffsetTemperature(uint16_t raw)
{
    return (raw - 400) / 10.0;
}
double WeatherStation::windSpeed() const
{
    return lastPacket_[0x3A] * 0.1;
}

double WeatherStation::windGust() const
{
    return le16(lastPacket_, 0x3b) * 0.00625;
}
double farenheitToCelsius(double f)
{
	return (f - 32) * (5.0 / 9.0);
}
double WeatherStation::indoorTemperature() const
{
    // Extract 12-bit temperature value from the 2-byte cluster
    int16_t raw = le16(lastPacket_, 0x07) & 0x0FFF;
    const auto f =  (raw - 400) * 0.1;
    return farenheitToCelsius(f);
}

double WeatherStation::outdoorTemperature() const
{
    // Extract 12-bit temperature value, masking out flag bits in the upper byte
        // 0x0A and 0x0B contain 2B A4 -> le16 makes it 0xA42B. 
        // Masking with 0x0FFF strips the 'A', leaving 0x042B (1067)
    int16_t raw = le16(lastPacket_, 0x0A) & 0x0FFF;

    // Convert from raw tenths of a degree Fahrenheit scale
    double tempF = (raw - 400) * 0.1;

    return farenheitToCelsius(tempF);
}

double WeatherStation::rainHour() const
{
    // Offset 0x3F
    uint16_t raw = le16(lastPacket_, 0x3F);
    // If the data is cleared/null, return 0.0 directly
    if ((raw & 0x0FFF) == 0x0FFF || raw == 0xA7FA) return 0.0;
    return (raw >> 4) * 0.1;
}

double WeatherStation::rainDay() const
{
    // Offset 0x41
    uint16_t raw = le16(lastPacket_, 0x41);
    if ((raw & 0x0FFF) == 0x0FFF || raw == 0x7FA7) return 0.0;
    return (raw >> 4) * 0.1;
}

double WeatherStation::rainWeek() const
{
    // Offset 0x43
    uint16_t raw = le16(lastPacket_, 0x43);
    if ((raw & 0x0FFF) == 0x0FFF || raw == 0xFAA7) return 0.0;
    return (raw >> 4) * 0.1;
}

double WeatherStation::rainMonth() const
{
    auto raw = le16(lastPacket_, 0x45);
    return (raw >> 4) * 0.1;
}

double WeatherStation::rainTotal() const
{
    return le16(lastPacket_, 0x48) * 0.1;
}
double WeatherStation::dewPoint() const
{
    double T = outdoorTemperature();
    double RH = outdoorHumidity();

    // Constants for the Magnus-Tetens formula
    const double a = 17.625;
    const double b = 243.04;

    double alpha = ((a * T) / (b + T)) + std::log(RH / 100.0);
    double dewPointC = (b * alpha) / (a - alpha);

    return dewPointC;
}
double WeatherStation::feelsLike() const
{
    double T_C = outdoorTemperature();
    double W_MS = windSpeed(); // Wind speed in m/s
    double RH = outdoorHumidity();

    // 1. Wind Chill (Valid for Temp <= 10 C and Wind > 1.3 m/s)
    if (T_C <= 10.0 && W_MS > 1.3) {
        double W_KMH = W_MS * 3.6; // Convert m/s to km/h
        double windChill = 13.12 + (0.6215 * T_C) - (11.37 * std::pow(W_KMH, 0.16)) + (0.3965 * T_C * std::pow(W_KMH, 0.16));
        return windChill;
    }

    // 2. Heat Index (Valid for Temp >= 26.7 C)
    if (T_C >= 26.7) {
        // Convert to Fahrenheit for standard NOAA Heat Index regression
        double T = (T_C * 9.0 / 5.0) + 32.0;
        double HI = 0.5 * (T + 61.0 + ((T - 68.0) * 1.2) + (RH * 0.094));

        if (HI >= 80.0) {
            HI = -42.379 + 2.04901523 * T + 10.14333127 * RH - 0.22475541 * T * RH
                - 0.00683783 * T * T - 0.05481717 * RH * RH + 0.00122874 * T * T * RH
                + 0.00085282 * T * RH * RH - 0.00000199 * T * T * RH * RH;
        }
        return (HI - 32.0) * 5.0 / 9.0; // Convert back to Celsius
    }

    // 3. Default to current temperature for mild weather
    return T_C;
}
uint16_t WeatherStation::le16(const std::vector<uint8_t>& p, size_t offset)
{
    return
        static_cast<uint16_t>(p[offset]) |
        (static_cast<uint16_t>(p[offset + 1]) << 8);
}

bool WeatherStation::executeCommand(uint8_t command,std::vector<uint8_t>& packet)
{
    uint8_t tx[65]{};

    tx[0] = 0;

    tx[1] = 3;
    tx[2] = command;
    tx[3] = tx[1] + tx[2];

    if (!m_transport.writeReport(tx,sizeof(tx)))
    {
        return false;
    }

    uint8_t rx[128]{};

    uint8_t report[65]{};

    if (!m_transport.readReport(report,sizeof(report)))
    {
        return false;
    }

    memcpy(rx,report + 1,64);

    uint8_t len = rx[0];

    if (len > 64)
    {
        if (!m_transport.readReport(report,sizeof(report)))
        {
            return false;
        }

        memcpy(rx + 64,report + 1,64);
    }

    uint8_t checksum = 0;

    for (uint8_t i = 0;i < len - 1;++i)
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

    packet.assign(rx,rx + len);

    return true;
}