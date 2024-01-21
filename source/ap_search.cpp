#include "everything.hpp"

#define VISIBLE_APS 10

void FindAPInteractive(Wifi_AccessPoint &ap)
{
	int selectedAP = 0, displayTop = 0, pressed;
	Wifi_ScanMode();

	do
	{
		consoleClear();

		const auto numAPs = Wifi_GetNumAP();
		std::cout << numAPs << " APs detected\n\n";

		auto displayEnd = displayTop + VISIBLE_APS;
		if (displayEnd > numAPs)
			displayEnd = numAPs;

		for (auto i = displayTop; i < displayEnd; i++)
		{
			Wifi_GetAPData(i, &ap);
			std::cout << ((i == selectedAP) ? '*' : ' ') << ' ' << ap.ssid << '\n'
					  << "  Wep:" << ((ap.flags & WFLAG_APDATA_WEP) ? "Yes" : "No") << " Sig:" << (ap.rssi * 100 / 0xD0) << '\n';
		}

		scanKeys();
		pressed = keysDown();

		if (pressed & KEY_UP)
		{
			if (selectedAP > 0)
				--selectedAP;
			if (selectedAP < displayTop)
				displayTop = selectedAP;
		}

		if (pressed & KEY_DOWN)
		{
			if (selectedAP < numAPs - 1)
				++selectedAP;
			displayTop = selectedAP - (VISIBLE_APS - 1);
			if (displayTop < 0)
				displayTop = 0;
		}

		swiWaitForVBlank();
	} while (!(pressed & KEY_A));

	Wifi_GetAPData(selectedAP, &ap);
}
