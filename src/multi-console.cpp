#include "Shell.hpp"

#include <nds/arm9/console.h>
#include <sys/iosupport.h>

/*
This file exists in case libnds PR #70 isn't merged.
I realized a bit late that I could just hack what I wanted together without changing libnds lol.
*/

// from libnds:console.c
extern "C" ssize_t con_write(_reent *r, void *fd, const char *ptr, size_t len);

ssize_t new_con_write(_reent *r, void *fd, const char *ptr, size_t len)
{
	PrintConsole *tmpConsole{};
	if (r->deviceData)
		tmpConsole = consoleSelect((PrintConsole *)(r->deviceData));

	const auto ret = con_write(r, fd, ptr, len);

	if (tmpConsole)
		consoleSelect(tmpConsole);

	return ret;
}

int con_open(struct _reent *, void *, const char *, int, int)
{
	return 0;
}

extern const devoptab_t dotab_stdnull; // from newlib:iosupport.c

namespace Shell
{

void InitMultiConsole()
{
	// Just for my sanity, prevent a segfault in newlib:iosupport.c:AddDevice()
	for (int i = 16; i < STD_MAX; ++i)
		devoptab_list[i] = &dotab_stdnull;

	// Init all backgrounds (main & sub) as consoles!
	for (int i = 0; i < NUM_CONSOLES; ++i)
	{
		const auto mainDisplay = i < 4;
		auto &console = consoles[i];
		consoleInit(
			&console,
			i % 4, // layer
			BgType_Text4bpp,
			BgSize_T_256x256,
			(mainDisplay ? 20 : 21) + (i % 4), // mapBase
			3,								   // tileBase
			mainDisplay,
			true); // loadGraphics

		if (!mainDisplay)
			// Limit height to the top edge of keyboard
			console.windowHeight = 14;

		if (i != 0 && i != 4)
			// Only show the first bg of each display initially
			bgHide(consoles[i].bgId);

		// Create/modify devoptab for consoles
		devoptab_t *dot;

		if (i == 0)
		{ // First console, already attached to STD_OUT and STD_ERR.
			// Grab the devive through devoptab_list since it's `static` in console.c
			dot = (devoptab_t *)devoptab_list[STD_OUT];

			// Sanity check
			if (dot != devoptab_list[STD_ERR])
			{
				std::cerr << "\e[41mmulticon: stdout & stderr devices are not the same!\e[39m\n";
				return;
			}
		}
		else
			// All other consoles need a new devoptab
			dot = new devoptab_t;

		dot->name = strdup("con\0\0");
		snprintf((char *)dot->name + 3, 2, "%d", i);
		dot->write_r = new_con_write;
		dot->open_r = con_open;
		dot->deviceData = &console;

		// Add to devoptab_list (can trust it with the fix above)
		AddDevice(dot);

		// Open a stream to the device!
		auto &ostr = con_streams[i];
		ostr.rdbuf()->pubsetbuf(nullptr, 0);
		ostr.open(std::string{dot->name} + ':');
		if (!ostr)
		{
			std::cerr << "\e[41mmulticon: failed to open " << dot->name << "!\e[39m\n";
			continue;
		}

		ostr << "\e[42mnds-shell (" << dot->name << ")\e[39m\n\n";
	}

	// Keep first console selected. Not necessary since new_con_write handles selection, but do it just in case.
	consoleSelect(&consoles[0]);
}

} // namespace Shell
