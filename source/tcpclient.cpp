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
		sendLoop = false;
		break;
	}
	receiveString[bytesReceived + 1] = 0;
	std::cout << "server: " << receiveString << '\n';
}

void tcpClient(const int _sock)
{
	irqSet(IRQ_NETWORK, irqNetworkHandler);
	keyboard->OnKeyPressed = [](const auto key)
	{
		if (key == DVK_MENU)
			sendLoop = false;
		else if (key > 0)
			std::cout << (char)key;
	};
	sock = _sock;
	std::string sendString;
	while (sendLoop)
	{
		std::cout << "tcp> ";
		std::getline(std::cin, sendString);
		if (sendString.empty())
			continue;
		send(sock, sendString.c_str(), sendString.length(), 0);
	}
	std::cout << "disconnected\n";
	resetKeyHandler();
	irqClear(IRQ_NETWORK);
}
