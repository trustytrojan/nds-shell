#include "Consoles.hpp"

#include <nds/arm9/console.h>
#include <sys/iosupport.h>

#include <fstream>
#include <iostream>

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

namespace Consoles
{

void Init()
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	InitMulti();

	// Show keyboard on bottom screen (no scrolling animation)
	keyboardDemoInit()->scrollSpeed = 0;
	keyboardShow();
}

void InitSingle()
{
	// handle the simple one-console initialization like back in the old days
}

// The fun stuff
PrintConsole consoles[NUM_CONSOLES];
std::ofstream streams[NUM_CONSOLES];

std::ostream &GetStream(u8 console)
{
	if (console < 0 || console >= NUM_CONSOLES)
		return std::cout;
	return streams[console];
}

void InitMulti()
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
				std::cerr << "\e[91mmulticon: stdout & stderr devices are not the same!\e[39m\n";
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
		auto &ostr = streams[i];
		ostr.rdbuf()->pubsetbuf(nullptr, 0);
		ostr.open(std::string{dot->name} + ':');
		if (!ostr)
		{
			std::cerr << "\e[91mmulticon: failed to open " << dot->name << "!\e[39m\n";
			continue;
		}
	}

	// Keep first console selected. Not necessary since new_con_write handles selection, but do it just in case.
	consoleSelect(&consoles[0]);
}

// Console/display switching state
Display focused_display{};
u8 focused_console{}, top_shown_console{}, bottom_shown_console{4};

u8 GetFocusedConsole()
{
	return focused_console;
}

bool IsFocused(u8 console)
{
	return console == focused_console;
}

void updateShownConsoles()
{
	if (focused_console < 4)
		top_shown_console = focused_console;
	else
		bottom_shown_console = focused_console;
}

void switchConsoleLeft()
{
	if (focused_console == 0)
		return;

	if (focused_console == 4)
	{
		// moving to top display
		focused_display = Display::TOP;
		if (top_shown_console != 3)
			// 0, 1, or 2 is shown, hide it
			bgHide(consoles[top_shown_console].bgId);
	}
	else
		// only hide if on same display!
		bgHide(consoles[focused_console].bgId);

	bgShow(consoles[--focused_console].bgId);
	updateShownConsoles();
}

void switchConsoleRight()
{
	if (focused_console == 6)
		return;

	if (focused_console == 3)
	{
		// moving to bottom display
		focused_display = Display::BOTTOM;
		if (bottom_shown_console != 4)
			// 5 or 6 is shown, hide it
			bgHide(consoles[bottom_shown_console].bgId);
	}
	else
		// only hide if on same display!
		bgHide(consoles[focused_console].bgId);

	bgShow(consoles[++focused_console].bgId);
	updateShownConsoles();
}

void toggleFocusedDisplay()
{
	focused_display = (Display) !(bool)focused_display;

	if (focused_display == Display::TOP)
	{ // we are switching to the top
		focused_console = top_shown_console;
	}
	else if (focused_display == Display::BOTTOM)
	{ // we are switching to the bottom
		focused_console = bottom_shown_console;
	}
}

} // namespace Consoles
