#include "Commands.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <nds.h>
#include <wfc.h>

static const char *const signalStrength[] = {
	"   ",
	"*  ",
	"** ",
	"***",
};

static const char *const authTypes[] = {
	"Open",
	"WEP",
	"WEP",
	"WEP",
	"WPA-PSK-TKIP",
	"WPA-PSK-AES",
	"WPA2-PSK-TKIP",
	"WPA2-PSK-AES",
};

static const char *const connStatus[] = {
	"Disconnected :(",
	"Searching...",
	"Associating...",
	"Obtaining IP address...",
	"Connected!",
};

static inline WlanBssAuthType authMaskToType(unsigned mask)
{
	return mask ? (WlanBssAuthType)(31 - __builtin_clz(mask))
				: WlanBssAuthType_Open;
}

static const WlanBssScanFilter filter = {
	.channel_mask = UINT32_MAX,
	.target_bssid = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};

static WlanBssDesc *GetAPList(unsigned *const count)
{
	if (!wfcBeginScan(&filter))
	{
		*Shell::err << "\e[41mwifi: GetAPList: wfcBeginScan failed\e[39m\n";
		return NULL;
	}

	*Shell::out << "Scanning APs...\n";
	WlanBssDesc *aplist{};

	while (pmMainLoop() && !(aplist = wfcGetScanBssList(count)))
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START)
		{
			*Shell::err << "\e[2Jwifi: connection canceled\n";
			return NULL;
		}
	}

	return aplist;
}

static WlanBssDesc *FindAPInteractive(void)
{
	consoleClear();

	int selected = 0;
	int i;
	int displaytop = 0;
	unsigned count = 0;
	WlanBssDesc *aplist = NULL;
	WlanBssDesc *apselected = NULL;

	static const WlanBssScanFilter filter = {
		.channel_mask = UINT32_MAX,
		.target_bssid = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	};

_rescan:
	aplist = GetAPList(&count);

	if (!aplist || !count)
	{
		iprintf("No APs detected\n");
		return NULL;
	}

	while (pmMainLoop())
	{
		swiWaitForVBlank();
		scanKeys();

		u32 pressed = keysDownRepeat();
		if (pressed & KEY_START)
		{
			*Shell::err << "\e[2Jwifi: connection canceled\n";
			return NULL;
		}
		if (pressed & KEY_A)
		{
			apselected = &aplist[selected];
			break;
		}

		consoleClear();

		if (pressed & KEY_R)
			goto _rescan;

		iprintf("%u APs detected (R = rescan)\n\n", count);

		int displayend = displaytop + 10;
		if (displayend > count)
			displayend = count;

		// display the APs to the user
		for (i = displaytop; i < displayend; i++)
		{
			WlanBssDesc *ap = &aplist[i];
			ap->ssid[sizeof(ap->ssid) - 1] = '\0';

			// display the name of the AP
			iprintf(
				"%s%.29s\n  %s Type:%s\n",
				i == selected ? "*" : " ",
				ap->ssid_len ? ap->ssid : "-- Hidden SSID --",
				signalStrength[wlanCalcSignalStrength(ap->rssi)],
				authTypes[authMaskToType(ap->auth_mask)]);
		}

		// move the selection asterisk
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
	}

	consoleClear();
	return apselected;
}

void GetHiddenSSID(WlanBssDesc *ap)
{
	consoleClear();
	char instr[64];
	uint inlen;

	iprintf("Enter hidden SSID name\n");
	bool ok;
	do
	{
		if (!fgets(instr, sizeof(instr), stdin))
			exit(0);

		instr[strcspn(instr, "\n")] = 0;
		inlen = strlen(instr);

		ok = inlen && inlen <= WLAN_MAX_SSID_LEN;
		if (!ok)
			iprintf("Invalid SSID\n");
	} while (!ok);

	memcpy(ap->ssid, instr, inlen);
	ap->ssid_len = inlen;
}

// Auth data must be in main RAM, not DTCM stack
static WlanAuthData auth{};

bool GetPassword(WlanBssDesc *ap)
{
	char instr[64];
	uint inlen;
	iprintf("Enter %s key\n", authTypes[ap->auth_type]);

	bool ok;
	do
	{
		if (!fgets(instr, sizeof(instr), stdin))
		{
			*Shell::err << "\e[41mwifi: GetPassword: fgets failed\e[39m\n";
			return false;
		}

		instr[strcspn(instr, "\n")] = 0;
		inlen = strlen(instr);

		ok = true;
		if (ap->auth_type < WlanBssAuthType_WPA_PSK_TKIP)
		{
			if (inlen == WLAN_WEP_40_LEN)
				ap->auth_type = WlanBssAuthType_WEP_40;
			else if (inlen == WLAN_WEP_104_LEN)
				ap->auth_type = WlanBssAuthType_WEP_104;
			else if (inlen == WLAN_WEP_128_LEN)
				ap->auth_type = WlanBssAuthType_WEP_128;
			else
				ok = false;
		}
		else if (inlen < 1 || inlen >= WLAN_WPA_PSK_LEN)
		{
			ok = false;
		}

		if (!ok)
			iprintf("Invalid key!\n");
	} while (!ok);

	if (ap->auth_type < WlanBssAuthType_WPA_PSK_TKIP)
	{
		memcpy(auth.wep_key, instr, inlen);
	}
	else
	{
		iprintf("Deriving PMK, please wait\n");
		wfcDeriveWpaKey(&auth, ap->ssid, ap->ssid_len, instr, inlen);
	}

	return true;
}

int ConnectAP(WlanBssDesc *const ap)
{
	if (!ap->ssid_len)
		GetHiddenSSID(ap);

	iprintf("Connecting to '%.*s'\n", ap->ssid_len, ap->ssid);
	ap->auth_type = authMaskToType(ap->auth_mask);
	memset(&auth, 0, sizeof(auth));

	if (ap->auth_type != WlanBssAuthType_Open && !GetPassword(ap))
	{
		*Shell::err << "\e[41mwifi: GetPassword failed\e[39m\n";
		return EXIT_FAILURE;
	}

	if (!wfcBeginConnect(ap, &auth))
	{
		*Shell::err << "\e[41mwifi: wfcBeginConnect failed\e[39m\n";
		return EXIT_FAILURE;
	}

	auto status = Wifi_AssocStatus(), prevStatus = -1;
	for (; pmMainLoop() && status != ASSOCSTATUS_ASSOCIATED &&
		   status != ASSOCSTATUS_CANNOTCONNECT;
		 status = Wifi_AssocStatus())
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START)
			return 2; // user canceled

		if (status != prevStatus)
		{
			*Shell::out << "\r\e[2K" << connStatus[status];
			prevStatus = status;
		}
	}

	*Shell::out << "\r\e[2K" << connStatus[status] << '\n';

	if (status == ASSOCSTATUS_CANNOTCONNECT)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

void subcommand_connect()
{
	*Shell::out << "Press Start to cancel\n";
	WlanBssDesc *ap{};

	if (Shell::args.size() == 2)
	{
		ap = FindAPInteractive();
	}
	else
	{
		const auto &ssid = Shell::args[2];

		unsigned count;
		const auto aplist = GetAPList(&count);

		if (!aplist || !count)
		{
			*Shell::err << "No APs detected\n";
			return;
		}

		for (int i = 0; i < count; ++i)
		{
			if (!*aplist[i].ssid || !aplist[i].ssid_len)
				continue;

			if (!strncmp(aplist[i].ssid, ssid.c_str(), aplist[i].ssid_len))
			{
				ap = aplist + i;
				break;
			}
		}

		if (!ap)
		{
			*Shell::err << "\e[41mwifi: SSID '" << ssid
						<< "' not found\e[39m\n";
			return;
		}
	}

	if (!ap)
	{
		*Shell::err << "\e[41mwifi: ap is null\e[39m\n";
		return;
	}

	switch (ConnectAP(ap))
	{
	case EXIT_FAILURE:
		*Shell::out << "\e[41mwifi: connection failed.\e[39m\n";
		return;
	case 2:
		*Shell::out << "wifi: connection canceled\n";
		return;
	}

	*Shell::out << "\e[42mwifi: connection successful\e[39m\n";
}

void subcommand_autoconnect()
{
	Wifi_AutoConnect();

	auto status = Wifi_AssocStatus(), prevStatus = -1;
	for (; pmMainLoop() && status != ASSOCSTATUS_ASSOCIATED &&
		   status != ASSOCSTATUS_CANNOTCONNECT;
		 status = Wifi_AssocStatus())
	{
		swiWaitForVBlank();
		scanKeys();

		if (status != prevStatus)
		{
			*Shell::out << "\r\e[2K" << connStatus[status];
			prevStatus = status;
		}
	}

	*Shell::out << "\r\e[2K";

	if (status == ASSOCSTATUS_CANNOTCONNECT)
	{
		*Shell::err << "\e[41mwifi: connection failed\n\e[39m";
		return;
	}

	*Shell::out << "\e[42mwifi: connection successful\e[39m\n";
}

static const auto subcommandsString = R"(subcommands:
  con[nect] [ssid]
  dis[connect]
  auto[connect]
  stat[us]
)";

void Commands::wifi()
{
	if (Shell::args.size() == 1)
	{
		*Shell::out << subcommandsString;
		return;
	}

	const auto &subcommand = Shell::args[1];

	if (subcommand.starts_with("con"))
		subcommand_connect();
	else if (subcommand.starts_with("dis") && Wifi_DisconnectAP())
		*Shell::err << "\e[41mwifi: Wifi_DisconnectAP failed\e[39m\n";
	else if (subcommand.starts_with("stat"))
		*Shell::out << connStatus[Wifi_AssocStatus()] << '\n';
	else if (subcommand.starts_with("auto"))
		subcommand_autoconnect();
}
