#include <nds.h>
#include <print>

constexpr auto getCPSRString()
{
	switch (getCPSR() & CPSR_MODE_MASK)
	{
	case CPSR_MODE_USER:
		return "USER";
	case CPSR_MODE_FIQ:
		return "FIQ";
	case CPSR_MODE_IRQ:
		return "IRQ";
	case CPSR_MODE_SUPERVISOR:
		return "SUPERVISOR";
	case CPSR_MODE_ABORT:
		return "ABORT";
	case CPSR_MODE_UNDEFINED:
		return "UNDEFINED";
	case CPSR_MODE_SYSTEM:
		return "SYSTEM";
	default:
		return "UNKNOWN";
	}
}

VoidFn g_oldSwiHandler;

const char *const staticMessage = "static message";
const char *message = staticMessage;

/*
I noticed some interesting things when trying to switch to the following CPSR modes
before returning from `my_swi_log()`:
- SUPERVISOR, IRQ: powers off the system
- UNDEFINED, FIQ, ABORT: data abort

The remaining modes, SYSTEM and USER, of course, work fine.
*/
#define SWI_RETURN_MODE CPSR_MODE_SYSTEM

extern "C" void my_swi_log(uint32_t arg /* r0 */)
{
	// ALL exception handlers are entered in SUPERVISOR mode.
	// The stack size for SUPERVISOR code is EXTREMELY small.
	// Stack usage must be kept minimal. Avoid heavy libc calls (especially ANYTHING print-like).
	// Here, to signal to main() that we ran, we just set a char pointer to our own message.
	if (arg == 0x67)
		message = "my_swi_log: 67!";
	else
		message = "my_swi_log: called!";

	// Here, we can set the desired CPSR mode to return to the caller as.
	// If you comment this line out, SPSR is simply the CPSR before the SWI was executed,
	// AKA the mode our caller had. The `movs` call below will reflect that.
	// asm("msr spsr, %0" : : "i"(SWI_RETURN_MODE));

	// `movs` performs the specified move and a `CPSR <- SPSR` move simultaneously.
	// We must do this instead of the compiler-generated `bx lr`, which is just an undonditional jump.
	// If we DON'T do this (by commenting the line out), execution continues in SUPERVISOR mode with a TINY stack,
	// which WILL cause stack smashing due to our use of `std::println` in `main()`!!!!!
	asm("movs pc, lr");
}

int main(void)
{
	defaultExceptionHandler();
	videoSetMode(MODE_0_2D);
	vramSetBankC(VRAM_C_MAIN_BG);

	// put on main display, since exceptions are displayed on sub display
	consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_512x512, 22, 3, true, true);

#ifdef __BLOCKSDS__
	std::println("\e[46mtoolchain: blocks\e[47m");
#else
	std::println("\e[46mtoolchain: dkp\e[47m");
#endif

	g_oldSwiHandler = SystemVectors.swi;
	SystemVectors.swi = (VoidFn)my_swi_log;

#ifdef __BLOCKSDS__
	setVectorBase(0);
#endif

	std::println("current cpsr: \e[46m{}\e[47m", getCPSRString());

	std::println("setting CPSR to USER...");
	setCPSR(CPSR_MODE_USER);
	std::println("current cpsr: \e[46m{}\e[47m", getCPSRString());

	if ((getCPSR() & CPSR_MODE_MASK) == CPSR_MODE_USER)
		std::println("\e[42mwe are USER!\e[47m");
	else
		// this should never happen, unless an emulator allows it
		std::println("\e[41mdid not become USER!\e[47m");

	std::println("trying to become SYSTEM...");
	setCPSR(CPSR_MODE_SYSTEM);
	if ((getCPSR() & CPSR_MODE_MASK) == CPSR_MODE_SYSTEM)
		// this should never happen, unless an emulator allows it
		std::println("\e[41mgained SYSTEM privileges...\e[47m");
	else
		std::println("\e[42mdid not become SYSTEM!\e[47m");

	std::println("message before swi: \e[46m'{}'\e[47m", message);
	std::println("calling swi...");

	// we called as USER
	asm("mov r0, #0x67"); // ARGUMENT PASSING TEST: WORKS!
	asm("swi 0x80");
	// but here we should be whatever SWI_RETURN_MODE is

	std::println("message after swi: \e[46m'{}'\e[47m", message);

	if (message == staticMessage)
		std::println("\e[41mmessage did NOT change\e[47m");
	else
		std::println("\e[42mmessage changed!\e[47m");

	std::println("cpsr after swi: \e[46m{}\e[47m", getCPSRString());

	if ((getCPSR() & CPSR_MODE_MASK) == CPSR_MODE_SYSTEM)
	{
		std::println("we are SYSTEM, calling setVectorBase(1)");

		// if we are not SYSTEM or higher, the ARM9 throws undefined instruction!
		// this calls the `mrc` instruction which is privileged!
		setVectorBase(1);
	}
	else
	{
		std::println("we are NOT SYSTEM, manually resetting SystemVectors.swi");

		// if for some reason we ended up as USER (you can change my_swi_log() to do this!),
		// we don't have the privileges to setVectorBase(), but we can at least set `SystemVectors.swi`
		// back to the original BIOS code, so that the BIOS SWI functions will work.
		SystemVectors.swi = g_oldSwiHandler;
	}

	const auto sqrt100 = swiSqrt(100);
	std::println("swiSqrt(100) = {}", sqrt100); // should print '10'
	if (sqrt100 == 10)
		std::println("\e[42mswiSqrt() is working!\e[47m");
	else
		std::println("\e[41mswiSqrt() is NOT working\e[47m");

	if ((getCPSR() & CPSR_MODE_MASK) != CPSR_MODE_SYSTEM)
	{
		std::println(
			"we are NOT SYSTEM, and are about to call swiWaitForVBlank().\n\e[41mthe ARM9 will now throw 'undefined "
			"instruction'!\e[47m");
	}

	while (true)
		// if we are not SYSTEM or higher, the ARM9 throws undefined instruction!
		// this calls the `mrc` instruction which is privileged!
		swiWaitForVBlank();
}
