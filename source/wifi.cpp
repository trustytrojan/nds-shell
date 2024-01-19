#include "everything.hpp"

Wifi_AccessPoint *findAP(void);

void PrintWifiStatus(void)
{
	switch (Wifi_AssocStatus())
	{
	case ASSOCSTATUS_ASSOCIATED:
	{
		const auto ip = Wifi_GetIP();
		iprintf("Connected!\nIP: %li.%li.%li.%li", (ip & 0xFF), ((ip >> 8) & 0xFF), ((ip >> 16) & 0xFF), ((ip >> 24) & 0xFF));
		break;
	}
	case ASSOCSTATUS_ASSOCIATING:
		std::cout << "Associating...";
		break;
	case ASSOCSTATUS_AUTHENTICATING:
		std::cout << "Authenticating...";
		break;
	case ASSOCSTATUS_ACQUIRINGDHCP:
		std::cout << "Acquiring DHCP...";
		break;
	case ASSOCSTATUS_DISCONNECTED:
		std::cout << "Disconnected";
		break;
	case ASSOCSTATUS_SEARCHING:
		std::cout << "Searching...";
	}
	std::cout << '\n';
}

void wifi(const Args &args)
{
	if (args.size() == 1)
	{
		std::cout << "usage: wifi <subcommand>\nsubcommands: connect, status\n";
		return;
	}

	const auto &subcommand = args[1];

	if (subcommand == "connect")
	{
		// Find access point (using libnds example code for now...)
		const auto ap = findAP();
		consoleClear();
		Wifi_SetIP(0, 0, 0, 0, 0);

		// Authenticate if needed and connect
		if (ap->flags & WFLAG_APDATA_WEP)
		{
			std::cout << "Password: ";
			std::string password;
			std::getline(std::cin, password);
			WEPMODES wepmode;

			switch (password.size())
			{
			case 5:
				wepmode = WEPMODE_40BIT;
				break;
			case 13:
				wepmode = WEPMODE_128BIT;
				break;
			default:
				std::cout << "Invalid password!\n";
				return;
			}

			Wifi_ConnectAP(ap, wepmode, 0, (unsigned char *)password.c_str());
		}
		else
			Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);

		// Wait until connected or can't connect
		int curr = Wifi_AssocStatus(), prev = -1;
		while (curr != ASSOCSTATUS_ASSOCIATED && curr != ASSOCSTATUS_CANNOTCONNECT)
		{
			if (curr != prev)
				PrintWifiStatus();
			prev = curr;
			curr = Wifi_AssocStatus();
		}

		// Print final status
		PrintWifiStatus();
	}

	else if (subcommand == "status")
	{
		PrintWifiStatus();
	}
}