#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "EscapeSequences.hpp"
#include "NetUtils.hpp"

#include <dswifi9.h>
#include <nds.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <iostream>

static const char *const signalStrength[] = {
	"[   ]",
	"[.  ]",
	"[.i ]",
	"[.iI]",
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

static WlanBssDesc *findAP(void)
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
	if (!wfcBeginScan(&filter))
		return NULL;

	iprintf("Scanning APs...\n");

	while (pmMainLoop() && !(aplist = wfcGetScanBssList(&count)))
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START)
		{
			std::cerr << "\e[2Jwifi: connection canceled\n";
			return NULL;
		}
	}

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
			std::cerr << "\e[2Jwifi: connection canceled\n";
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

	return apselected;
}

bool GetSSID(WlanBssDesc *ap)
{
	CliPrompt prompt{"SSID: "};
	std::string line;

	while (true)
	{
		std::cout << "Enter hidden SSID name\n";
		prompt.getLine(line);
		if (line.empty() || line.length() > WLAN_MAX_SSID_LEN)
			std::cout << "Invalid SSID\n"_brightred;
		else
			break;
	}

	memcpy(ap->ssid, line.c_str(), line.length());
	ap->ssid_len = line.length();
	return true;
}

// Auth data must be in main RAM, not DTCM stack
static WlanAuthData auth;

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
			std::cerr << "wifi: GetPassword: fgets failed\n"_brightred;
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
			std::cerr << "Invalid key!\n"_brightred;
	} while (!ok);

	if (ap->auth_type < WlanBssAuthType_WPA_PSK_TKIP)
	{
		memcpy(auth.wep_key, instr, inlen);
	}
	else
	{
		std::cout << "Deriving PMK, please wait\n";
		wfcDeriveWpaKey(&auth, ap->ssid, ap->ssid_len, instr, inlen);
	}

	return true;
}

void Commands::wifi()
{
	const auto ap = findAP();

	if (!ap)
	{
		std::cerr << "wifi: ap is null\n"_brightred;
		return;
	}

	if (!ap->ssid_len)
		GetSSID(ap);

	iprintf("Connecting to %.*s\n", ap->ssid_len, ap->ssid);
	ap->auth_type = authMaskToType(ap->auth_mask);
	memset(&auth, 0, sizeof(auth));

	if (ap->auth_type != WlanBssAuthType_Open && !GetPassword(ap))
	{
		std::cerr << "wifi: GetPassword failed\n"_brightred;
		return;
	}

	if (!wfcBeginConnect(ap, &auth))
	{
		std::cerr << "wifi: wfcBeginConnect failed\n"_brightred;
		return;
	}

	bool is_connect = false;
	while (pmMainLoop())
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START)
		{
			std::cout << "wifi: connection canceled\n";
			return;
		}

		int status = Wifi_AssocStatus();

		consoleClear();
		std::cout << connStatus[status] << "\nPress start to cancel";

		is_connect = status == ASSOCSTATUS_ASSOCIATED;
		if (is_connect || status == ASSOCSTATUS_DISCONNECTED)
			break;
	}

	if (!is_connect)
	{
		std::cout << "wifi: connection failed.\n"_brightred;
		return;
	}

	in_addr gateway, subnetMask, dns1, dns2;
	Wifi_GetIPInfo(&gateway, &subnetMask, &dns1, &dns2);

	std::cout << "wifi: connection successful.\n"_green
			  << "IP:          " << (in_addr)Wifi_GetIP() << '\n'
			  << "Gateway:     " << gateway << '\n'
			  << "Subnet Mask: " << subnetMask << '\n'
			  << "DNS 1:       " << dns1 << '\n'
			  << "DNS 2:       " << dns2 << '\n';
}
