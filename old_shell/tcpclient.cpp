#include "everything.h"

char receiveString[100];
bool sendLoop;
int sock, bytesReceived;

void irqNetworkHandler(void)
{
	switch (bytesReceived = recv(sock, receiveString, 100, 0))
	{
	case -1:
		perror("recv");
		break;
	case 0:
		std::cout << "server disconnected\n";
		sendLoop = false;
		break;
	}
	receiveString[bytesReceived + 1] = 0;
	std::cout << "\e[server>> " << receiveString << "\ntcp> ";
}

void tcpClient(const int _sock)
{
	irqInit();
	irqEnable(IRQ_NETWORK);
	irqEnable(IRQ_WIFI);
	irqSet(IRQ_NETWORK, irqNetworkHandler);
	irqSet(IRQ_WIFI, irqNetworkHandler);
	keyboard->OnKeyPressed = [](const auto key)
	{
		if (key == DVK_FOLD)
		{
			std::cout << "\nclient disconnected\n";
			sendLoop = false;
		}
		else if (key > 0)
			std::cout << (char)key;
	};
	sock = _sock;
	std::string sendString;
	sendLoop = true;
	while (sendLoop)
	{
		std::cout << "tcp> ";
		std::getline(std::cin, sendString);
		if (sendString.empty())
			continue;
		send(sock, sendString.c_str(), sendString.length(), 0);
	}
	resetKeyHandler();
	irqClear(IRQ_NETWORK);
	irqClear(IRQ_WIFI);
}
