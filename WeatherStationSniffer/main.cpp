#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>

#include "WindowsHidTransport.h"
#include "WeatherStation.h"

#include <cstdio>

int main()
{
	WindowsHidTransport transport;

	if (!transport.open())
	{
		printf(
			"FT0203A not found\n");

		return 1;
	}

	printf(
		"FT0203A connected\n");

	WeatherStation ws(transport);

	std::vector<uint8_t> packet;
	static std::vector<uint8_t> previous;

	for (;;)
	{
		if (!previous.empty())
		{
			for (size_t i = 0; i < packet.size(); i++)
			{
				if (packet[i] != previous[i])
				{
					printf("[%02X] %02X -> %02X\n",
						(unsigned)i,
						previous[i],
						packet[i]);
				}
			}
		}
		if (ws.readCurrentRecord(packet))
		{
			printf(
				"Received %zu bytes\n",
				packet.size());

			for (size_t i = 0;
				i < packet.size();
				++i)
			{
				printf(
					"%02X ",
					packet[i]);

				if ((i % 16) == 15)
					printf("\n");
			}

			printf("\n");
		}
		else
		{
			printf(
				"Read failed\n");
		}
		//dumpKnownFields(packet);
		std::cout << "Indoor humidity: " << (int)ws.indoorHumidity() << "\n";
		std::cout << "Outdoor humidity: " << (int)ws.outdoorHumidity() << "\n";
		std::cout << "Absolute pressure: " << ws.absolutePressure() << "\n";
		std::cout << "Relative pressure: " << ws.relativePressure() << "\n";
		std::cout << "Wind speed: " << ws.windSpeed() << "\n";
		std::cout << "Wind gust: " << ws.windGust() << "\n";
		std::cout << "Wind direction: " << ws.windDirectionDegrees() <<" - " << ws.windDirection() << "\n";
		std::cout << "indoor temperature: " << ws.indoorTemperature() << "\n";
		std::cout << "Outdoor temperature: " << ws.outdoorTemperature() << "\n";
		std::cout << "Rain hour: " << ws.rainHour() << "\n";
		std::cout << "Rain day: " << ws.rainDay() << "\n";
		std::cout << "Rain week: " << ws.rainWeek() << "\n";
		std::cout << "Rain month: " << ws.rainMonth() << "\n";
		std::cout << "Rain total: " << ws.rainTotal() << "\n";
		std::cout << "Dew point: " << ws.dewPoint() << "\n";
		std::cout << "Feels like: "<<ws.feelsLike() << "\n";
//
		//dumpDecoded(packet.data());

		previous = packet;
		Sleep(16000);
	}
	return 0;
}


