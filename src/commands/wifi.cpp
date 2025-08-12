#include "Commands.hpp"

#include <dswifi9.h>
#include <nds.h>
#include <wfc.h>

#include <algorithm>

constexpr int USER_CANCEL = 2;

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

static std::span<WlanBssDesc> ScanAPs(const Commands::Context &ctx)
{
	if (!wfcBeginScan(&filter))
	{
		ctx.err << "\e[91mwifi: GetAPList: wfcBeginScan failed\e[39m\n";
		return {};
	}

	ctx.out << "Scanning APs... Press START to cancel\n";
	WlanBssDesc *aplist{};
	unsigned count;

	while (pmMainLoop() && !(aplist = wfcGetScanBssList(&count)))
	{
#ifdef NDSH_THREADING
		threadYield();
		if (!ctx.shell.IsFocused())
			continue;
#else
		swiWaitForVBlank();
		scanKeys();
#endif

		if (keysDown() & KEY_START)
		{
			ctx.err << "wifi: connection canceled\n";
			return {};
		}
	}

	return {aplist, count};
}

int GetHiddenSSID(const Commands::Context &ctx, WlanBssDesc &ap)
{
	ctx.out << "Hidden SSID required. Press START key to cancel\n";

	CliPrompt prompt;
	prompt.setPrompt("SSID: ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	bool ok;
	do
	{
#ifdef NDSH_THREADING
		threadYield();
		if (!ctx.shell.IsFocused())
			continue;
#else
		swiWaitForVBlank();
		scanKeys();
#endif

		if (keysDown() & KEY_START)
			// user canceled
			return USER_CANCEL;

		prompt.update();

		if (prompt.enterPressed())
		{
			const auto &input = prompt.getInput();

			ok = input.size() && input.size() <= WLAN_MAX_SSID_LEN;

			if (!ok)
			{
				ctx.err << "\e[91mwifi: Invalid SSID\n";
				prompt.prepareForNextLine();
				prompt.printFullPrompt(false);
			}
		}
	} while (!ok && pmMainLoop());

	const auto &input = prompt.getInput();
	input.copy(ap.ssid, input.size());
	ap.ssid_len = input.size();

	return EXIT_SUCCESS;
}

// Auth data must be in main RAM, not DTCM stack
static WlanAuthData auth{};

int GetPassword(const Commands::Context &ctx, WlanBssDesc &ap)
{
	ctx.out << authTypes[ap.auth_type] << " detected. Password required. Press START to cancel\n";

	CliPrompt prompt;
	prompt.setOutputStream(ctx.out);
	prompt.setPrompt("Password: ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	bool ok;
	do
	{
#ifdef NDSH_THREADING
		threadYield();
		if (!ctx.shell.IsFocused())
			continue;
#else
		swiWaitForVBlank();
		scanKeys();
#endif

		if (keysDown() & KEY_START)
			// user canceled
			return USER_CANCEL;

		prompt.update();

		if (prompt.enterPressed())
		{
			const auto &input = prompt.getInput();
			ok = true;
			if (ap.auth_type < WlanBssAuthType_WPA_PSK_TKIP)
			{
				if (input.size() == WLAN_WEP_40_LEN)
					ap.auth_type = WlanBssAuthType_WEP_40;
				else if (input.size() == WLAN_WEP_104_LEN)
					ap.auth_type = WlanBssAuthType_WEP_104;
				else if (input.size() == WLAN_WEP_128_LEN)
					ap.auth_type = WlanBssAuthType_WEP_128;
				else
					ok = false;
			}
			else if (input.size() < 1 || input.size() >= WLAN_WPA_PSK_LEN)
			{
				ok = false;
			}

			if (!ok)
			{
				ctx.err << "\e[91mwifi: Invalid key!\e[39m\n";
				prompt.prepareForNextLine();
				prompt.printFullPrompt(false);
			}
		}
	} while (!ok && pmMainLoop());

	const auto &input = prompt.getInput();

	if (ap.auth_type < WlanBssAuthType_WPA_PSK_TKIP)
	{
		memcpy(auth.wep_key, input.c_str(), input.size());
	}
	else
	{
		ctx.out << "Deriving PMK, please wait\n";
		wfcDeriveWpaKey(&auth, ap.ssid, ap.ssid_len, input.c_str(), input.size());
	}

	return 0;
}

static int WaitForConnection(std::ostream &out)
{
	auto status = Wifi_AssocStatus(), prevStatus = -1;
	for (; pmMainLoop() && status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT;
		 status = Wifi_AssocStatus())
	{
#ifdef NDSH_THREADING
		threadYield();
		// TODO: check console focus
#else
		swiWaitForVBlank();
		scanKeys();
#endif

		if (keysDown() & KEY_START)
			// user canceled
			return USER_CANCEL;

		if (status != prevStatus)
		{
			out << "\r\e[2K" << connStatus[status];
			prevStatus = status;
		}
	}

	out << "\r\e[2K" << connStatus[status] << '\n';

	if (status == ASSOCSTATUS_CANNOTCONNECT)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int ConnectAP(const Commands::Context &ctx, WlanBssDesc &ap)
{
	if (!ap.ssid_len)
	{
		const auto rc = GetHiddenSSID(ctx, ap);
		if (rc == EXIT_FAILURE)
			std::cerr << "\e[91mwifi: GetHiddenSSID failed\e[39m\n";
		return EXIT_FAILURE;
	}

	ap.ssid[ap.ssid_len] = '\0';
	ctx.out << "Connecting to '" << ap.ssid << "'\n";

	ap.auth_type = authMaskToType(ap.auth_mask);
	auth = {};

	if (ap.auth_type != WlanBssAuthType_Open)
	{
		if (const auto rc = GetPassword(ctx, ap))
		{
			if (rc == EXIT_FAILURE)
				ctx.err << "\e[91mwifi: GetPassword failed\e[39m\n";
			return rc;
		}
	}

	if (!wfcBeginConnect(&ap, &auth))
	{
		ctx.err << "\e[91mwifi: wfcBeginConnect failed\e[39m\n";
		return EXIT_FAILURE;
	}

	return WaitForConnection(ctx.out);
}

void subcommand_connect(const Commands::Context &ctx)
{
	if (ctx.args.size() < 3)
	{
		ctx.err << "wifi: connect: SSID required\n";
		return;
	}

	ctx.out << "Press Start to cancel\n";
	WlanBssDesc *ap{};

	const auto &ssid = ctx.args[2];
	const auto aplist = ScanAPs(ctx);

	if (aplist.empty())
	{
		ctx.err << "\e[91mwifi: No APs detected\e[39m\n";
		return;
	}

	// this is a projection: map each AP to it's SSID string_view then check against `ssid`
	const auto it = std::ranges::find(aplist, ssid, [](auto &bss) { return std::string_view{bss.ssid, bss.ssid_len}; });

	if (it == aplist.end())
	{
		ctx.err << "\e[91mwifi: SSID '" << ssid << "' not found\e[39m\n";
		return;
	}

	ap = it.base();

	if (!ap)
	{
		// This now cleanly handles user cancellation from FindAPInteractive
		// or other potential null-pointer cases.
		return;
	}

	switch (ConnectAP(ctx, *ap))
	{
	case EXIT_SUCCESS:
		ctx.out << "\e[92mwifi: connection successful\e[39m\n";
		break;
	case EXIT_FAILURE:
		ctx.err << "\e[91mwifi: connection failed\e[39m\n";
		break;
	case USER_CANCEL:
		ctx.out << "wifi: connection canceled\n";
	}
}

void subcommand_autoconnect(std::ostream &ostr)
{
	Wifi_AutoConnect();

	switch (WaitForConnection(ostr))
	{
	case EXIT_SUCCESS:
		ostr << "\e[92mwifi: autoconnect successful\e[39m\n";
		break;
	case EXIT_FAILURE:
		ostr << "\e[91mwifi: autoconnect failed\n\e[39m";
		break;
	case USER_CANCEL:
		ostr << "wifi: autoconnect canceled\n";
	}
}

void subcommand_status(const Commands::Context &ctx)
{
	const auto status = Wifi_AssocStatus();
	ctx.out << connStatus[status];

	if (status == ASSOCSTATUS_ASSOCIATED)
	{
		const auto *ap = wfcGetActiveSlot();
		if (ap && ap->ssid_len > 0)
			ctx.out << ": " << std::string_view{ap->ssid, ap->ssid_len};
	}
	ctx.out << '\n';
}

void subcommand_scan(const Commands::Context &ctx)
{
	const auto aplist = ScanAPs(ctx);

	if (aplist.empty())
	{
		// ScanAPs already prints messages on failure or cancellation,
		// but not if no APs are found.
		ctx.err << "\e[91mwifi: No APs detected\e[39m\n";
		return;
	}

	ctx.out << aplist.size() << " APs detected:\n";
	for (const auto &ap : aplist)
	{
		if (!ap.ssid_len)
			continue;
		ctx.out << signalStrength[wlanCalcSignalStrength(ap.rssi)] << " " << std::string_view{ap.ssid, ap.ssid_len}
				<< '\n';
	}
}

static const auto subcommandsString = R"(subcommands:
  con[nect] <ssid>
  dis[connect]
  auto[connect]
  stat[us]
  scan
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
		ctx.err << "\e[91mwifi: Wifi_DisconnectAP failed\e[39m\n";
	else if (subcommand.starts_with("stat"))
		subcommand_status(ctx);
	else if (subcommand.starts_with("auto"))
		subcommand_autoconnect(ctx.out);
	else if (subcommand.starts_with("scan"))
		subcommand_scan(ctx);
}
