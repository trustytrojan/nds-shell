#include "everything.hpp"

Wifi_AccessPoint *findAP(void)
{
	int i, selected = 0, count = 0, displaytop = 0, pressed = 0;
	static Wifi_AccessPoint ap;
	Wifi_ScanMode();
	do
	{
		scanKeys();
		pressed = keysDown();
		count = Wifi_GetNumAP();
		consoleClear();
		iprintf("%d APs detected\n\n", count);
		int displayend = displaytop + 10;
		if (displayend > count)
			displayend = count;

		for (i = displaytop; i < displayend; i++)
		{
			Wifi_AccessPoint ap;
			Wifi_GetAPData(i, &ap);
			iprintf("%s %.29s\n  Wep:%s Sig:%i\n",
					i == selected ? "*" : " ",
					ap.ssid,
					ap.flags & WFLAG_APDATA_WEP ? "Yes " : "No ",
					ap.rssi * 100 / 0xD0);
		}

		if (pressed & KEY_UP)
		{
			selected--;
			if (selected < 0)
				selected = 0;
			if (selected < displaytop)
				displaytop = selected;
		}

		if (pressed & KEY_DOWN)
		{
			selected++;
			if (selected >= count)
				selected = count - 1;
			displaytop = selected - 9;
			if (displaytop < 0)
				displaytop = 0;
		}

		swiWaitForVBlank();
	} while (!(pressed & KEY_A));

	Wifi_GetAPData(selected, &ap);
	return &ap;
}
