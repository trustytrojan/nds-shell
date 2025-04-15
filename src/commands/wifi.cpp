#include "Commands.hpp"
#include "NetUtils.hpp"

#include <dswifi9.h>
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
			exit(0);
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
			exit(0);
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

void GetSSID(WlanBssDesc *ap)
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
			return false;

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

void Commands::wifi()
{
	const auto ap = findAP();

	if (!ap)
	{
		std::cerr << "\e[41mwifi: ap is null\e[39m\n";
		return;
	}

	if (!ap->ssid_len)
		GetSSID(ap);

	iprintf("Connecting to %.*s\n", ap->ssid_len, ap->ssid);
	ap->auth_type = authMaskToType(ap->auth_mask);
	memset(&auth, 0, sizeof(auth));

	if (ap->auth_type != WlanBssAuthType_Open && !GetPassword(ap))
	{
		std::cerr << "\e[41mwifi: GetPassword failed\e[39m\n";
	}

	if (!wfcBeginConnect(ap, &auth))
	{
		std::cerr << "\e[41mwifi: wfcBeginConnect failed\e[39m\n";
		return;
	}

	bool is_connect = false;
	while (pmMainLoop())
	{
		swiWaitForVBlank();
		scanKeys();

		if (keysDown() & KEY_START)
		{
			std::cerr << "wifi: connection canceled\n";
			return;
		}

		int status = Wifi_AssocStatus();

		consoleClear();
		iprintf("%s\n", connStatus[status]);

		is_connect = status == ASSOCSTATUS_ASSOCIATED;
		if (is_connect || status == ASSOCSTATUS_DISCONNECTED)
			break;
	}

	if (!is_connect)
	{
		std::cout << "\e[41mwifi: connection failed.\e[39m\n";
		return;
	}

	in_addr gateway, subnetMask, dns1, dns2;
	Wifi_GetIPInfo(&gateway, &subnetMask, &dns1, &dns2);

	std::cout << "\e[32mwifi: connection successful.\e[39m\n"
			  << "IP:          " << (in_addr)Wifi_GetIP() << '\n'
			  << "Gateway:     " << gateway << '\n'
			  << "Subnet Mask: " << subnetMask << '\n'
			  << "DNS 1:       " << dns1 << '\n'
			  << "DNS 2:       " << dns2 << '\n';
}
