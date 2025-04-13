#include "Commands.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <wfc.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

// for printing ip addresses without first converting to a string
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip)
{
	return ostr << (ip.s_addr & 0xFF) << '.' << ((ip.s_addr >> 8) & 0xFF) << '.'
				<< ((ip.s_addr >> 16) & 0xFF) << '.'
				<< ((ip.s_addr >> 24) & 0xFF);
}

namespace fs = std::filesystem;

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
	//---------------------------------------------------------------------------------
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

		u32 pressed = keysDown();
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

namespace Commands
{

void help()
{
	std::cout << "commands: ";
	auto itr = MAP.cbegin();
	for (; itr != MAP.cend(); ++itr)
		std::cout << itr->first << ' ';
	std::cout << '\n';
}

void ls()
{
	const auto &path =
		(Shell::args.size() == 1) ? Shell::env["PWD"] : Shell::args[1];

	if (!fs::exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : fs::directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			std::cout << "\e[44m" << filename << "\e[39m ";
		else
			std::cout << filename << ' ';
		std::cout << '\n';
	}
}

void cd()
{
	if (Shell::args.size() == 1)
	{
		Shell::env["PWD"] = "/";
		return;
	}

	const auto &path = Shell::args[1];

	if (!fs::exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	Shell::env["PWD"] = path;
}

void cat()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!fs::exists(Shell::args[1]))
	{
		std::cerr << "file '" << Shell::args[1] << "' does not exist\n";
		return;
	}

	std::ifstream file(Shell::args[1]);

	if (!file)
	{
		std::cerr << "cannot open file\n";
		return;
	}

	std::cout << file.rdbuf();
	file.close();
}

void rm()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!fs::exists(Shell::args[1]))
	{
		std::cerr << "file '" << Shell::args[1] << "' does not exist\n";
		return;
	}

	if (!fs::remove(Shell::args[1]))
		std::cerr << "failed to remove '" << Shell::args[1] << "'\n";
}

void echo()
{
	for (auto itr = Shell::args.cbegin() + 1; itr < Shell::args.cend(); ++itr)
		std::cout << *itr << ' ';
	std::cout << '\n';
}

void dns()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(Shell::args[1].c_str());
	if (!host)
	{
		perror("gethostbyname");
		return;
	}

	std::cout << *(in_addr *)host->h_addr_list[0] << '\n';
}

void wifi()
{
	consoleClear();
	WlanBssDesc *const ap = findAP();

	if (!ap)
	{
		std::cerr << "\e[41mwifi: ap is null\e[39m\n";
		return;
	}

	consoleClear();
	char instr[64];
	uint inlen;

	// Auth data must be in main RAM, not DTCM stack
	static WlanAuthData auth;

	if (!ap->ssid_len)
	{
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

	iprintf("Connecting to %.*s\n", ap->ssid_len, ap->ssid);
	ap->auth_type = authMaskToType(ap->auth_mask);
	memset(&auth, 0, sizeof(auth));

	if (ap->auth_type != WlanBssAuthType_Open)
	{
		iprintf("Enter %s key\n", authTypes[ap->auth_type]);
		bool ok;
		do
		{
			if (!fgets(instr, sizeof(instr), stdin))
				exit(0);

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
	}

	if (!wfcBeginConnect(ap, &auth))
	{
		std::cerr << "\e[41mwifi: wfcBeginConnect returned false\e[39m\n";
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

	if (is_connect)
		std::cout << "\e[32mwifi: connection successful.\e[39m\n";
	else
	 	std::cout << "\e[41mwifi: connection failed.\e[39m\n";
}

void http()
{
	if (Shell::args.size() != 3)
	{
		std::cerr << "usage: http <method> <url>\n";
		return;
	}

	// convert to uppercase
	auto method = Shell::args[1];
	std::ranges::transform(method, method.begin(), toupper);

	// check against supported methods
	static const std::unordered_set<std::string> httpMethods{
		"GET", "POST", "PUT", "DELETE"};
	if (std::find(httpMethods.begin(), httpMethods.cend(), method) ==
		httpMethods.cend())
	{
		std::cerr << "\e[41minvalid http method\e[39m\n";
		return;
	}

	// parse out address and path from url
	// the string.h functions are easier to use for this kind of stuff
	auto addr = Shell::args[2].c_str();

	// take out http:// from url if its there
	if (!strncmp(addr, "http://", 7))
		addr += 7;

	const char *path;
	const auto slashPtr = strchr(addr, '/');
	if (!slashPtr)
		path = "/";
	else
	{
		// we lose the / here, but we're saving what could be a long copy
		// we can insert a / back when constructing the request, see below
		*slashPtr = 0;
		path = slashPtr + 1;
	}

	// parse the address
	sockaddr_in sain;
	if (!NetUtils::parseAddress(addr, 80, sain))
	{
		NetUtils::printError("http");
		return;
	}

	// open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		return;
	}
	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	// construct and send the request
	std::stringstream ss;
	ss << method << ' ' << (slashPtr ? "/" : "") << path
	   << " HTTP/1.1\r\nHost: " << addr
	   << "\r\nUser-Agent: Nintendo DS\r\n\r\n";
	const auto request = ss.str();
	if (send(sock, request.c_str(), request.size(), 0) == -1)
	{
		perror("send");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	// print the response
	char responseBuffer[BUFSIZ + 1];
	responseBuffer[BUFSIZ] = 0;
	int bytesReceived;
	while ((bytesReceived = recv(sock, responseBuffer, BUFSIZ, 0)) > 0)
	{
		responseBuffer[bytesReceived] = 0;
		std::cout << responseBuffer;
	}

	if (bytesReceived == -1)
	{
		perror("recv");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	if (close(sock) == -1)
		perror("close");

	std::cerr << "\e45mreached end of function\e39m\n";
}

} // namespace Commands
