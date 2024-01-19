#include "everything.hpp"

void ProcessLine(const std::string &line)
{
	// save the hassle
	if (line.empty())
		return;

	const auto lineEnd = line.cend();
	Args args;
	std::string currentArg;

	for (auto itr = line.cbegin(); itr != lineEnd; ++itr)
		switch (*itr)
		{
		case ' ':
		case '\t':
		case '\n':
			args.push_back(currentArg);
			currentArg.clear();
			break;

		case '\\':
		{
			const char c = *(++itr);
			switch (c)
			{
			case '"':
			case '\n':
			case '\t':
			case '\\':
				currentArg += c;
				break;
			default:
				--itr;
			}
			break;
		}

		case '"':
			if (!currentArg.empty())
			{
				currentArg += '"';
				break;
			}

			for (++itr; *itr != '"' && itr != lineEnd; ++itr)
				switch (*itr)
				{
				case '\\':
					if (*(++itr) == '"')
						currentArg += '"';
					else
					{
						currentArg += '\\';
						--itr;
					}
					break;
				default:
					currentArg += *itr;
				}

			if (itr == line.cend())
			{
				std::cout << "Parse error: closing '\"' not found\n";
				return;
			}

			args.push_back(currentArg);
			currentArg.clear();

			break;

		default:
			currentArg += *itr;
		}

	// don't forget to push the last argument (currentArg)!
	if (!currentArg.empty())
		args.push_back(currentArg);

	RunCommand(args);
}
