#include "Consoles.hpp"
#include "console/console-priv.hpp"

#include <fcntl.h>
#include <nds/arm9/console.h>
#ifndef __BLOCKSDS__
	#include <sys/iosupport.h>
#endif

#include <fstream>
#include <iostream>

#ifndef NDSH_MULTICONSOLE
static MyPrintConsole console;
#endif

#ifdef __BLOCKSDS__
ssize_t con_write(const char *ptr, size_t len)
{
	if (!ptr || len <= 0)
		return -1;

	for (size_t i = 0; i < len; ++i)
	{
		const char chr = ptr[i];
		if (chr == '\e' || console.escBufLen > 0)
			consoleUpdateEscapeSequence(chr);
		else
			myConsolePrintChar(chr);
	}

	return len;
}
#else
ssize_t con_write(_reent *r, void *fd, const char *ptr, size_t len)
{
	if (!ptr || len <= 0)
		return -1;

	const auto myConsole = (MyPrintConsole *)r->deviceData;

	PrintConsole *tmpConsole{};
	if (myConsole)
		tmpConsole = consoleSelect(myConsole);

	for (size_t i = 0; i < len; ++i)
	{
		const char chr = ptr[i];
		if (chr == '\e' || getCurrentConsole()->escBufLen > 0)
			consoleUpdateEscapeSequence(chr);
		else
			myConsolePrintChar(chr);
	}

	if (tmpConsole)
		consoleSelect(tmpConsole);

	return len;
}

int con_open(struct _reent *, void *, const char *, int, int)
{
	return 0;
}
#endif

#ifdef NDSH_MULTICONSOLE
// from newlib/iosupport.c
extern const devoptab_t dotab_stdnull;
#endif

namespace Consoles
{

void Init()
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

#ifdef NDSH_MULTICONSOLE
	void InitMulti();
	InitMulti();
#else
	void InitSingle();
	InitSingle();
#endif

	// Show keyboard on bottom screen (no scrolling animation)
	keyboardDemoInit()->scrollSpeed = 0;
	keyboardShow();

	// TODO: make the keyboard have tcsetattr-like apis for disabling
	// canonical mode (device-local line buffering)!
}

#ifndef NDSH_MULTICONSOLE
void InitSingle()
{
	// handle the simple one-console initialization like back in the old days.

	constexpr auto layer{0};
	constexpr auto type{BgType_Text4bpp};
	constexpr auto size{BgSize_T_256x256};
	constexpr auto mapBase{20}; // 22 is recommended, but as found below, 20 is fine
	constexpr auto tileBase{3};

	consoleInit((PrintConsole *)&console, layer, type, size, mapBase, tileBase, true, true);

	{ // make our console more ANSI-compliant!
		console.bg2Id = bgInit(layer + 1, type, size, mapBase + 1, tileBase);
		console.fontBg2Gfx = bgGetGfxPtr(console.bg2Id);
		console.fontBg2Map = bgGetMapPtr(console.bg2Id);
		console.escBufLen = 0;
		console.fontCurPal2 = 0;
	}

	#ifdef __BLOCKSDS__
	consoleSetCustomStdout(con_write);
	consoleSetCustomStderr(con_write);
	#else
	{ // modify the existing devoptab to use our write() and make it open()able!
		const auto dot = (devoptab_t *)devoptab_list[STD_OUT];

		// Sanity check
		if (dot != devoptab_list[STD_ERR])
		{
			// stdout & stderr devices are not the same!
			// trap here so that HOPEFULLY the PC address points here for me to see 🙏
			__builtin_trap();
		}

		// keep the name "con" since it will be the only one.
		dot->write_r = con_write;
		dot->open_r = con_open;
		dot->deviceData = &console;
	}
	#endif
}
#endif

#ifdef NDSH_MULTICONSOLE
// The fun stuff
MyPrintConsole consoles[NUM_CONSOLES];
std::ofstream streams[NUM_CONSOLES];

std::ostream &GetStream(u8 console)
{
	if (console < 0 || console >= NUM_CONSOLES)
		return std::cout;
	return streams[console];
}

void initConsole(u8 idx, bool mainDisplay, int layer)
{
	if (!mainDisplay && layer >= 2)
		// this would screw up the keyboard
		return;

	// not gonna bother with L & R for switching between same-display consoles
	auto &console = consoles[idx];

	constexpr auto type{BgType_Text4bpp};
	constexpr auto size{BgSize_T_256x256};
	const auto mapBase{(mainDisplay ? 20 : 21) + layer};
	constexpr auto tileBase{3};

	consoleInit(
		(PrintConsole *)&console, layer, BgType_Text4bpp, BgSize_T_256x256, mapBase, tileBase, mainDisplay, true);

	if (!mainDisplay)
		// Limit height to the top edge of keyboard
		console.windowHeight = 14;

	{ // make our console more ANSI-compliant!
		console.bg2Id = mainDisplay ? bgInit(layer + 1, type, size, mapBase + 1, tileBase)
									: bgInitSub(layer + 1, type, size, mapBase + 1, tileBase);
		console.fontBg2Gfx = bgGetGfxPtr(console.bg2Id);
		console.fontBg2Map = bgGetMapPtr(console.bg2Id);
		console.escBufLen = 0;
		console.fontCurPal2 = 0; // bg color: black
	}

	if (idx == 1)
	{
		bgHide(console.bgId);
		if (console.bg2Id != -1)
			bgHide(console.bg2Id);
	}

	// Create/modify devoptab for consoles, making them open()able devices,
	// and giving them our new write() for better escape sequence handling!
	devoptab_t *dot;

	if (idx == 0)
	{ // First console, already attached to STD_OUT and STD_ERR.
		// Grab the device through devoptab_list since it's `static` in console.c
		dot = (devoptab_t *)devoptab_list[STD_OUT];

		// Sanity check
		if (dot != devoptab_list[STD_ERR])
		{
			// stdout & stderr devices are not the same!
			// trap here so that HOPEFULLY the PC address points here for me to see 🙏
			__builtin_trap();
		}
	}
	else
		// All other consoles need a new devoptab
		dot = new devoptab_t;

	// Rename all consoles to be `conN` where `N` is a 0-based index
	dot->name = strdup("con\0\0");
	snprintf((char *)dot->name + 3, 2, "%d", idx);

	// Attach write()/open() callbacks as mentioned
	dot->write_r = con_write;
	dot->open_r = con_open;

	// VERY IMPORTANT: attach PrintConsole* to devoptab's "user data", so that
	// we can extract and select the console in con_write()
	dot->deviceData = &console;

	if (idx != 0)
		// New device: add to devoptab_list
		AddDevice(dot);

	// Open a file stream to the console!
	auto &ostr = streams[idx];
	ostr.rdbuf()->pubsetbuf(nullptr, 0);
	ostr.open(std::string{dot->name} + ':');
	if (!ostr)
	{
		// i don't think this will ever be visible...
		std::cerr << "\e[91mmulticon: failed to open " << dot->name << "!\e[39m\n";
		return;
	}
}

void InitMulti()
{
	// Just for my sanity, prevent a segfault in newlib:iosupport.c:AddDevice()
	for (int i = 16; i < STD_MAX; ++i)
		devoptab_list[i] = &dotab_stdnull;

	// This is the maximum we can do, while keeping all consoles in double-BG mode for ANSI background colors.
	// We could have a fourth single-BG console, but that would break consistency.
	// The last BG (7) is reserved by the keyboard.
	initConsole(0, true, 0);
	initConsole(1, true, 2);
	initConsole(2, false, 0);

	// Keep first console selected. Not necessary since new_con_write handles selection, but do it just in case.
	consoleSelect(&consoles[0]);
}

// Console/display switching state
static Display focused_display{};
static u8 focused_console{}, top_shown_console{}, bottom_shown_console{2};

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
	if (focused_console < 2)
		top_shown_console = focused_console;
	else
		bottom_shown_console = focused_console;
}

void showConsole(u8 idx)
{
	auto &console = consoles[idx];
	bgShow(console.bgId);
	if (console.bg2Id != -1)
		bgShow(console.bg2Id);
}

void hideConsole(u8 idx)
{
	auto &console = consoles[idx];
	bgHide(console.bgId);
	if (console.bg2Id != -1)
		bgHide(console.bg2Id);
}

void switchConsoleLeft()
{
	if (focused_console == 0)
		return;

	if (focused_console == 2)
	{
		// moving to top display
		focused_display = Display::TOP;
		if (top_shown_console != 1)
		{
			// 0 is shown, hide it
			hideConsole(top_shown_console);
		}
	}
	else
		// only hide if on same display!
		hideConsole(focused_console);

	showConsole(--focused_console);
	updateShownConsoles();
	consoleSelect(consoles + focused_console);
}

void switchConsoleRight()
{
	if (focused_console == 2)
		return;

	if (focused_console == 1)
	{
		// moving to bottom display
		focused_display = Display::BOTTOM;
		/*if (bottom_shown_console != 4)
			// 5 or 6 is shown, hide it
			hideConsole(bottom_shown_console);*/
	}
	else
		// only hide if on same display!
		hideConsole(focused_console);

	showConsole(++focused_console);
	updateShownConsoles();
	consoleSelect(consoles + focused_console);
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

	consoleSelect(consoles + focused_console);
}
#endif

} // namespace Consoles
