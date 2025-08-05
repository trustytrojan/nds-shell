#include "Commands.hpp"
#include "Consoles.hpp"

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
	return mask ? (WlanBssAuthType)(31 - __builtin_clz(mask)) : WlanBssAuthType_Open;
}

static const WlanBssScanFilter filter = {
	.channel_mask = UINT32_MAX,
	.target_bssid = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};

static WlanBssDesc *GetAPList(const Commands::Context &ctx, unsigned *const count)
{
	if (!wfcBeginScan(&filter))
	{
		ctx.err << "\e[41mwifi: GetAPList: wfcBeginScan failed\e[39m\n";
		return NULL;
	}

	ctx.out << "Scanning APs... Press START to cancel\n";
	WlanBssDesc *aplist{};

	while (pmMainLoop() && !(aplist = wfcGetScanBssList(count)))
	{
		// swiWaitForVBlank();
		// scanKeys();
		threadYield();

		if (keysDown() & KEY_START)
		{
			ctx.err << "wifi: connection canceled\n";
			return NULL;
		}
	}

	return aplist;
}

static WlanBssDesc *FindAPInteractive(const Commands::Context &ctx)
{
	// consoleClear();

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
	aplist = GetAPList(ctx, &count);

	if (!aplist || !count)
	{
		// iprintf("No APs detected\n");
		ctx.err << "\e[41mwifi: No APs detected\e[39m\n";
		return NULL;
	}

	while (pmMainLoop())
	{
		threadYield();
		// swiWaitForVBlank();
		// scanKeys();

		ctx.out << "\e[2J";

		u32 pressed = keysDown();

		if (pressed & KEY_START)
		{
			ctx.err << "wifi: user canceled\n";
			return NULL;
		}

		if (pressed & KEY_A)
		{
			apselected = &aplist[selected];
			break;
		}

		// consoleClear();
		// ctx.out << "\e[2J";

		if (pressed & KEY_R)
			goto _rescan;

		std::print(ctx.out, "%u APs detected (R = rescan)\n\n", count);

		int displayend = displaytop + 10;
		if (displayend > count)
			displayend = count;

		// display the APs to the user
		for (i = displaytop; i < displayend; i++)
		{
			WlanBssDesc *ap = &aplist[i];
			ap->ssid[sizeof(ap->ssid) - 1] = '\0';

			// display the name of the AP
			std::print(
				ctx.out,
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

	// consoleClear();
	ctx.out << "\e[2J";
	return apselected;
}

int GetHiddenSSID(const Commands::Context &ctx, WlanBssDesc *ap)
{
	ctx.out << "Hidden SSID required. Press START key to cancel\n";

	// consoleClear();
	CliPrompt prompt;
	prompt.setPrompt("SSID: ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	// char instr[64];
	// uint inlen;


	bool ok;
	do
	{
		threadYield();

		if (ctx.shell.console != Consoles::focused_console)
			continue;

		if (keysDown() & KEY_START)
			// user canceled
			return 2;

		prompt.processKeyboard();

		// ctx.out << "Enter hidden SSID name:\n";
		// if (!fgets(instr, sizeof(instr), stdin))
		// {
		// 	ctx.err << "\e[41mwifi: GetHiddenSSID: fgets failed\e[39m\n";
		// 	return EXIT_FAILURE;
		// }

		// instr[strcspn(instr, "\n")] = 0;
		// inlen = strlen(instr);

		if (prompt.enterPressed())
		{
			const auto &input = prompt.getInput();

			ok = input.size() && input.size() <= WLAN_MAX_SSID_LEN;

			if (!ok)
			{
				ctx.err << "\e[41mwifi: Invalid SSID\n";
				prompt.prepareForNextLine();
				prompt.printFullPrompt(false);
			}
		}
	} while (!ok);

	const auto &input = prompt.getInput();
	memcpy(ap->ssid, input.c_str(), input.size());
	ap->ssid_len = input.size();

	return EXIT_SUCCESS;
}

// Auth data must be in main RAM, not DTCM stack
static WlanAuthData auth{};

int GetPassword(const Commands::Context &ctx, WlanBssDesc *ap)
{
	// char instr[64];
	// uint inlen;

	ctx.out << authTypes[ap->auth_type] << " detected. Password required. Press START to cancel\n";

	CliPrompt prompt;
	prompt.setPrompt("Password: ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	bool ok;
	do
	{
		threadYield();

		if (ctx.shell.console != Consoles::focused_console)
			continue;

		if (keysDown() & KEY_START)
			// user canceled
			return 2;

		// because of the stdin impl, characters are not shown
		// ctx.out << "Enter password:\n";
		// if (!fgets(instr, sizeof(instr), stdin))
		// {
		// 	ctx.err << "\e[41mwifi: GetPassword: fgets failed\e[39m\n";
		// 	return false;
		// }

		prompt.processKeyboard();

		// instr[strcspn(instr, "\n")] = 0;
		// inlen = strlen(instr);

		if (prompt.enterPressed())
		{
			const auto &input = prompt.getInput();
			ok = true;
			if (ap->auth_type < WlanBssAuthType_WPA_PSK_TKIP)
			{
				if (input.size() == WLAN_WEP_40_LEN)
					ap->auth_type = WlanBssAuthType_WEP_40;
				else if (input.size() == WLAN_WEP_104_LEN)
					ap->auth_type = WlanBssAuthType_WEP_104;
				else if (input.size() == WLAN_WEP_128_LEN)
					ap->auth_type = WlanBssAuthType_WEP_128;
				else
					ok = false;
			}
			else if (input.size() < 1 || input.size() >= WLAN_WPA_PSK_LEN)
			{
				ok = false;
			}

			if (!ok)
			{
				ctx.err << "\e[41mwifi: Invalid key!\e[39m\n";
				prompt.prepareForNextLine();
				prompt.printFullPrompt(false);
			}
		}
	} while (!ok);

	const auto &input = prompt.getInput();

	if (ap->auth_type < WlanBssAuthType_WPA_PSK_TKIP)
	{
		memcpy(auth.wep_key, input.c_str(), input.size());
	}
	else
	{
		ctx.out << "Deriving PMK, please wait\n";
		wfcDeriveWpaKey(&auth, ap->ssid, ap->ssid_len, input.c_str(), input.size());
	}

	return true;
}

int ConnectAP(const Commands::Context &ctx, WlanBssDesc *const ap)
{
	if (!ap->ssid_len)
	{
		const auto rc = GetHiddenSSID(ctx, ap);
		if (rc == EXIT_FAILURE)
			std::cerr << "\e[41mwifi: GetHiddenSSID failed\e[39m\n";
		return EXIT_FAILURE;
	}

	// iprintf("Connecting to '%.*s'\n", ap->ssid_len, ap->ssid);
	std::print(ctx.out, "Connecting to '%.*s'\n", ap->ssid_len, ap->ssid);
	ap->auth_type = authMaskToType(ap->auth_mask);
	memset(&auth, 0, sizeof(auth));

	if (ap->auth_type != WlanBssAuthType_Open)
	{
		const auto rc = GetPassword(ctx, ap);
		if (rc == EXIT_FAILURE)
			ctx.err << "\e[41mwifi: GetPassword failed\e[39m\n";
		else if (rc == 2)
			ctx.err << "wifi: user canceled\n";
		return EXIT_FAILURE;
	}

	if (!wfcBeginConnect(ap, &auth))
	{
		ctx.err << "\e[41mwifi: wfcBeginConnect failed\e[39m\n";
		return EXIT_FAILURE;
	}

	auto status = Wifi_AssocStatus(), prevStatus = -1;
	for (; pmMainLoop() && status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT;
		 status = Wifi_AssocStatus())
	{
		threadYield();
		// scanKeys(); // this is done in the main thread, trust it

		if (keysDown() & KEY_START)
			// user canceled
			return 2;

		if (status != prevStatus)
		{
			ctx.out << "\r\e[2K" << connStatus[status];
			prevStatus = status;
		}
	}

	ctx.out << "\r\e[2K" << connStatus[status] << '\n';

	if (status == ASSOCSTATUS_CANNOTCONNECT)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

void subcommand_connect(const Commands::Context &ctx)
{
	ctx.out << "Press Start to cancel\n";
	WlanBssDesc *ap{};

	if (ctx.args.size() == 2)
	{
		ap = FindAPInteractive(ctx);
	}
	else
	{
		const auto &ssid = ctx.args[2];

		unsigned count;
		const auto aplist = GetAPList(ctx, &count);

		if (!aplist || !count)
		{
			ctx.err << "\e[41mwifi: No APs detected\e[39m\n";
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
			ctx.err << "\e[41mwifi: SSID '" << ssid << "' not found\e[39m\n";
			return;
		}
	}

	if (!ap)
	{
		ctx.err << "\e[41mwifi: ap is null\e[39m\n";
		return;
	}

	switch (ConnectAP(ctx, ap))
	{
	case EXIT_FAILURE:
		ctx.err << "\e[41mwifi: connection failed.\e[39m\n";
		return;
	case 2:
		ctx.out << "wifi: connection canceled\n";
		return;
	}

	ctx.out << "\e[42mwifi: connection successful\e[39m\n";
}

void subcommand_autoconnect(std::ostream &ostr)
{
	Wifi_AutoConnect();

	// TODO: factor out this loop into a re-usable function
	auto status = Wifi_AssocStatus(), prevStatus = -1;
	for (; pmMainLoop() && status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT;
		 status = Wifi_AssocStatus())
	{
		threadYield();

		// TODO: allow user to cancel

		if (status != prevStatus)
		{
			ostr << "\r\e[2K" << connStatus[status];
			prevStatus = status;
		}
	}

	ostr << "\r\e[2K";

	if (status == ASSOCSTATUS_CANNOTCONNECT)
	{
		ostr << "\e[41mwifi: connection failed\n\e[39m";
		return;
	}

	ostr << "\e[42mwifi: connection successful\e[39m\n";
}

static const auto subcommandsString = R"(subcommands:
  con[nect] [ssid]
  dis[connect]
  auto[connect]
  stat[us]
)";

void Commands::wifi(const Context &ctx)
{
	if (ctx.args.size() == 1)
	{
		ctx.out << subcommandsString;
		return;
	}

	const auto &subcommand = ctx.args[1];

	if (subcommand.starts_with("con"))
		subcommand_connect(ctx);
	else if (subcommand.starts_with("dis") && Wifi_DisconnectAP())
		ctx.err << "\e[41mwifi: Wifi_DisconnectAP failed\e[39m\n";
	else if (subcommand.starts_with("stat"))
		ctx.out << connStatus[Wifi_AssocStatus()] << '\n';
	else if (subcommand.starts_with("auto"))
		subcommand_autoconnect(ctx.out);
}
