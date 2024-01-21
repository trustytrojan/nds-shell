#include "everything.hpp"

void ProcessLine(const std::string &line)
{
	// save the hassle
	if (line.empty())
		return;

	const auto lineEnd = line.cend();
	Args args;
	std::string currentArg;

	StandardStreams stdio;

	for (auto itr = line.cbegin(); itr < lineEnd; ++itr)
		switch (*itr)
		{
		case ' ':
		case '\t':
		case '\n':
			if (currentArg.empty())
				break;
			args.push_back(currentArg);
			currentArg.clear();
			break;

		case '"':
			for (++itr; *itr != '"' && itr < lineEnd; ++itr)
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
			if (itr == lineEnd)
			{
				std::cerr << "\e[41mshell: closing '\"' not found\e[39m\n";
				return;
			}
			args.push_back(currentArg);
			currentArg.clear();
			break;

		case '<':
		{
			if (stdio.in != &std::cin)
			{
				std::cerr << "\e[41mshell: stdin is already reading from a file\e[39m\n";
				return;
			}

			std::string filepath;

			for (++itr; !isspace(*itr) && itr < lineEnd; ++itr)
			{
				filepath += *itr;
			}

			if (filepath.empty())
			{
				std::cerr << "\e[41mshell: expected filepath after '<'\e[39m\n";
				return;
			}

			if (!std::filesystem::exists(filepath))
			{
				std::cerr << "\e[41mshell: '" << filepath << "' does not exist\e[39m\n";
				return;
			}

			if (!*(stdio.in = new std::ifstream(filepath)))
			{
				std::cerr << "\e[41mshell: failed to open:\e[39m " << filepath << '\n';
				return;
			}

			break;
		}

		case '>':
		{
			if (stdio.out != &std::cout)
			{
				std::cerr << "\e[41mshell: stdout is already being redirected\e[39m\n";
				return;
			}

			std::string filepath;

			for (++itr; !isspace(*itr) && itr < lineEnd; ++itr)
			{
				filepath += *itr;
			}

			if (filepath.empty())
			{
				std::cerr << "\e[41mshell: expected filepath after '>'\e[39m\n";
				return;
			}

			if (!*(stdio.out = new std::ofstream(filepath)))
			{
				std::cerr << "\e[41mshell: failed to open:\e[39m " << filepath << '\n';
				return;
			}

			break;
		}

		case '2':
		{
			// if the next character isnt '>', then break
			if (*(++itr) != '>')
			{
				--itr;
				break;
			}

			if (stdio.err != &std::cerr)
			{
				std::cerr << "\e[41mshell: stderr is already being redirected\e[39m\n";
				return;
			}

			std::string filepath;

			for (++itr; !isspace(*itr) && itr < lineEnd; ++itr)
			{
				filepath += *itr;
			}

			if (filepath.empty())
			{
				std::cerr << "\e[41mshell: expected filepath after '>'\e[39m\n";
				return;
			}

			if (!*(stdio.err = new std::ofstream(filepath)))
			{
				std::cerr << "\e[41mshell: failed to open:\e[39m " << filepath << '\n';
				return;
			}

			break;
		}

		default:
			currentArg += *itr;
		}

	// don't forget to push the last argument (currentArg)!
	if (!currentArg.empty())
		args.push_back(currentArg);

	RunCommand(args, stdio);
}
