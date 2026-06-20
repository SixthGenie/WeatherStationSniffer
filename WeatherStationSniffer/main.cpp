#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>

#include "WindowsHidTransport.h"
#include "WeatherStation.h"

#include <cstdio>

typedef void* (__cdecl* UsbRegisterDevice_t)(HWND);
typedef long(__cdecl* UsbReadRecord_t)(unsigned char*);
typedef int(__cdecl* UsbGetConnectStatus_t)();
typedef int(__cdecl* UsbCheckHandle_t)(HWND);
typedef long(__cdecl* UsbReadMaxMin_t)(unsigned char*);
typedef long(__cdecl* UsbReadDeviceInfo_t)(unsigned char*);

void DumpHex(const unsigned char* data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		if ((i % 16) == 0)
		{
			std::cout << "\n"
				<< std::setw(4)
				<< std::setfill('0')
				<< std::hex
				<< i
				<< ": ";
		}

		std::cout
			<< std::setw(2)
			<< std::setfill('0')
			<< std::hex
			<< (int)data[i]
			<< " ";
	}

	std::cout << std::dec << "\n";
}
uint16_t le16(const uint8_t* p)
{
	return p[0] | (p[1] << 8);
}

void DumpDecoded(const uint8_t* r)
{
	printf("IndoorHumidity : %u\n", r[9]);
	printf("OutdoorHumidity: %u\n", r[22]);

	printf("AbsPressure    : %.1f\n",
		le16(&r[54]) / 10.0);

	printf("RelPressure    : %.1f\n",
		le16(&r[56]) / 10.0);

	// Candidate fields that appear to move
	printf("Field07        : %u\n",
		le16(&r[7]));

	printf("Field10        : 0x%04X\n",
		le16(&r[10]));

	printf("Field30        : 0x%04X\n",
		le16(&r[30]));

	printf("Field60        : 0x%04X\n",
		le16(&r[60]));

	printf("Field62        : 0x%04X\n",
		le16(&r[62]));

	printf("Field64        : 0x%04X\n",
		le16(&r[64]));

	printf("Field66        : 0x%04X\n",
		le16(&r[66]));
}

int main1()
{
	HMODULE dll =
		LoadLibraryA("C:\\tmp\\USBDriver.dll");

	if (!dll)
	{
		std::cout << "Failed to load DLL\n";
		return 1;
	}

	auto UsbRegisterDevice = (UsbRegisterDevice_t)GetProcAddress(dll, "?UsbRegisterDevice@@YAPAXPAUHWND__@@@Z");

	auto UsbReadRecord = (UsbReadRecord_t)GetProcAddress(dll, "?UsbReadRecord@@YAJPAE@Z");

	auto UsbGetConnectStatus = (UsbGetConnectStatus_t)GetProcAddress(dll, "?UsbGetConnectStatus@@YAHXZ");
	auto UsbCheckHandle = (UsbCheckHandle_t)GetProcAddress(dll, "?UsbCheckHandle@@YAHPAUHWND__@@@Z");
	auto UsbReadMaxMin = (UsbReadMaxMin_t)GetProcAddress(dll, "?UsbReadMaxMin@@YAJPAE@Z");
	auto UsbReadDeviceInfo = (UsbReadDeviceInfo_t)GetProcAddress(dll, "?UsbReadDeviceInfo@@YAJPAE@Z");

	HWND hwnd = GetConsoleWindow();
	int rc = UsbCheckHandle(GetConsoleWindow());
	std::cout << "CheckHandle = " << rc << "\n";

	if (!UsbRegisterDevice ||
		!UsbReadRecord ||
		!UsbGetConnectStatus)
	{
		std::cout << "Failed to resolve exports\n";
		return 1;
	}
	std::cout << "Connection status = "
		<< UsbGetConnectStatus()
		<< "\n";
	std::cout << "Registering device...\n";

	void* handle = UsbRegisterDevice(hwnd);

	std::cout << "Handle = " << handle << "\n";

	std::cout << "Connection status = "
		<< UsbGetConnectStatus()
		<< "\n";

	std::vector<unsigned char> buffer(4096);

	memset(buffer.data(), 0xcc, buffer.size());

	long result = UsbReadRecord(buffer.data());

	std::cout << "UsbReadRecord returned " << result << "\n";

	DumpHex(buffer.data(), 512);

	memset(buffer.data(), 0xcc, buffer.size());
	result = UsbReadMaxMin(buffer.data());
	std::cout << "UsbReadMaxMin returned " << result << "\n";
	DumpHex(buffer.data(), 512);

	memset(buffer.data(), 0xcc, buffer.size());
	result = UsbReadDeviceInfo(buffer.data());
	std::cout << "UsbReadDeviceInfo returned " << result << "\n";
	DumpHex(buffer.data(), 512);

	std::array<uint8_t, 128> record{};

	std::array<uint8_t, 128> previous{};
	bool first = true;

	for (;;)
	{
		std::array<uint8_t, 128> current{};

		UsbReadRecord(current.data());

		if (!first)
		{
			for (int i = 0; i < 128; i++)
			{
				if (current[i] != previous[i])
				{
					printf("[%02X] %02X -> %02X\n",
						i,
						previous[i],
						current[i]);
				}
			}
		}

		previous = current;
		first = false;
		printf("Indoor Humidity : %u\n", current.data()[9]);
		printf("Outdoor Humidity: %u\n", current.data()[22]);

		printf("Abs Pressure    : %.1f\n",
			le16(&current.data()[54]) / 10.0);

		printf("Rel Pressure    : %.1f\n",
			le16(&current.data()[56]) / 10.0);

		printf("WindCluster     : %02X %02X %02X %02X\n",
			current.data()[58], current.data()[59], current.data()[60], current.data()[61]);

		printf("TempCandidateA  : %02X\n", current.data()[10]);
		printf("TempCandidateB  : %02X\n", current.data()[42]);
		Sleep(16000);
	}


	FreeLibrary(dll);

	return 0;
}
void dumpKnownFields(const std::vector<uint8_t>& p)
{
	printf("Indoor Humidity : %u\n", p[0x16]);
	printf("Outdoor Humidity: %u\n", p[0x07]);

	printf("WindCluster     : %02X %02X %02X %02X\n",
		p[0x3A],
		p[0x3B],
		p[0x3C],
		p[0x3D]);

	printf("TempCandidateA  : %02X\n", p[0x0A]);
	printf("TempCandidateB  : %02X\n", p[0x2A]);
}
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

		previous = packet;
		Sleep(16000);
	}
	return 0;
}


