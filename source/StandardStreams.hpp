#ifndef __STDSTR__
#define __STDSTR__

#include "libdeps.hpp"

struct StandardStreams
{
	std::istream *in;
	std::ostream *out, *err;

	StandardStreams() : in(&std::cin), out(&std::cout), err(&std::cerr) {}
	StandardStreams(const StandardStreams &) = delete;
	StandardStreams &operator=(const StandardStreams &) = delete;
	~StandardStreams()
	{
		// if any stream isnt a standard stream,
		// we can safely assume it is a file stream.
		// remember std::(i/o)fstream and std::(i/o)stream are NOT the same size.
		// we MUST cast the pointers first, then delete them.

		if (in != &std::cin)
		{
			const auto fs = (std::ifstream *)in;
			fs->close();
			delete fs;
		}
		
		if (out != &std::cout)
		{
			const auto fs = (std::ofstream *)out;
			fs->close();
			delete fs;
		}

		if (err != &std::cerr)
		{
			const auto fs = (std::ofstream *)err;
			fs->close();
			delete fs;
		}
	}
};

#endif