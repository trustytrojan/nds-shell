#include "deps.h"

void createTerminal(void)
{
	static PrintConsole console;

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleSelect(&console);

	const auto keyboard = keyboardDemoInit();
	keyboardShow();
	keyboard->OnKeyPressed = [](const auto key){ if (key > 0) std::cout << (char)key; };
}

std::vector<std::string> strsplit(const std::string &str, const char delim)
{
	std::istringstream iss(str);
	std::vector<std::string> tokens;
	std::string token;
	while (std::getline(iss, token, delim))
		tokens.push_back(token);
	return tokens;
}

void printWifiStatus(void)
{
	switch (Wifi_AssocStatus())
	{
	case ASSOCSTATUS_ASSOCIATED:
	{
		const auto ip = Wifi_GetIP();
		iprintf("Connected!\nIP: %li.%li.%li.%li", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
	} break;
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

int main(void)
{
	createTerminal();
	Wifi_InitDefault(false);
	std::string line;
	while (1)
	{
		std::cout << "> ";
		std::getline(std::cin, line);
		const auto args = strsplit(line, ' ');
		const auto command = args[0];

		if (command.empty())
			continue;

		if (command == "exit")
			break;

		else if (command == "dns")
		{
			if (args.size() == 1)
			{
				std::cout << "usage: dns <hostname>\n";
				continue;
			}

			const auto hostname = args[1].c_str();
			const auto host = gethostbyname(hostname);

			std::cout << ((host) ? inet_ntoa(*(in_addr *)host->h_addr_list[0]) : "Count not resolve hostname") << '\n';
		}

		else if (command == "wifi")
		{
			if (args.size() == 1)
			{
				std::cout << "usage: wifi <subcommand>\nsubcommands: connect, status\n";
				continue;
			}

			const auto subcommand = args[1];

			if (subcommand == "connect")
			{
				const auto ap = findAP();
				consoleClear();
				Wifi_SetIP(0, 0, 0, 0, 0);

				if (ap->flags & WFLAG_APDATA_WEP)
				{
					std::cout << "Password: ";
					std::string password;
					
					std::getline(std::cin, password);
					auto wepmode = WEPMODE_NONE;
					if (password.size() == 5)
						wepmode = WEPMODE_40BIT;
					else if (password.size() == 13)
						wepmode = WEPMODE_128BIT;
					else
					{
						std::cout << "Invalid password!\n";
						continue;
					}
					Wifi_ConnectAP(ap, wepmode, 0, (unsigned char *)password.c_str());
				}
				else
				{
					Wifi_ConnectAP(ap, WEPMODE_NONE, 0, 0);
				}

				int curr = Wifi_AssocStatus(), prev = -1;
				while (curr != ASSOCSTATUS_ASSOCIATED && curr != ASSOCSTATUS_CANNOTCONNECT)
				{
					if (curr != prev)
						printWifiStatus();
					prev = curr;
					curr = Wifi_AssocStatus();
				}
				printWifiStatus();
			}

			else if (subcommand == "status")
			{
				printWifiStatus();
			}
		}

		else
		{
			std::cout << "unknown command\n";
		}
	}
}
