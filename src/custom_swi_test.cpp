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

static void my_swi_log(uint32_t arg)
{
	// ALL exception handlers are entered in SUPERVISOR mode.
	// The stack size for SUPERVISOR code is EXTREMELY small.
	// Stack usage must be kept minimal. Avoid heavy libc calls (especially ANYTHING print-like).
	// Here, to signal to main() that we ran, we just set a static char pointer to our own message.
	if (arg == 0x67)
		message = "my_swi_log: 67!";
	else
		message = "my_swi_log: called!";
}

extern "C" bool my_swi_callback(uint32_t swi_arg /* r0 */, uint32_t *regs /* r1 */)
{
	if (swi_arg == 0x80)
	{
		// Here, `regs[0]` is the SWI caller's R0, which is on the stack.
		// It is NOT our R0! Our R0 is `swi_arg`. Do not confuse the two!
		my_swi_log(regs[0]);
		return true;
	}
	return false;
}

// https://problemkaputt.de/gbatek.htm#armopcodesbranchandbranchwithlinkbblbxblxswibkpt
__attribute((naked)) void my_swi_handler()
{
	// Save the caller's registers to the stack.
	asm("stmdb sp!, {r0-r3, r12, lr}");

	/// We are NOT (and should not be) in THUMB mode, so we can just extract the SWI immediate the ARM way.

	// Load SPSR to R1
	asm("mrs r1, spsr");

	// Load the 32-bit immediate from the SWI instruction, which is literally
	// right before the return address (stored in `lr`).
	asm("ldr r0, [lr, #-4]");

	// `bic` stands for "Bit Clear".
	// In the immediate argument, we are specifying the bits to "clear" or "zero out", because internally, it takes the
	// complement of the immediate, then bitwise ANDs it with r0. (Yes, it's a wrapper over `and`, which can also be
	// used here.)
	asm("bic r0, r0, #0xff000000");

	// Move stack pointer to R1. This will be the 2nd argument to my_swi_dispatcher().
	asm("mov r1, sp");

	// Call my_swi_callback().
	// BL means "branch and link".
	asm("bl my_swi_callback");

	// NEW LOGIC: R0 now contains the return value from my_swi_dispatcher (true or false).
	// We compare it to see if we should return to the user or jump to the BIOS.
	asm("cmp r0, #0");

	// Restore the caller's registers from the stack.
	// This is critical: if we are going to the BIOS, the BIOS needs the original R0-R3 back!
	asm("ldmia sp!, {r0-r3, r12, lr}");

	// THIS IS THE KILLER!
	// `movs` performs both the usual move AND a CPSR <- SPSR move.
	// This restores the caller's CPSR mode.
	// We only do this if our dispatcher returned true (NE condition).
	asm("movnes pc, lr");

	// FALLTHROUGH: If dispatcher returned false (EQ), we "Tail-chain" to the original BIOS handler.
	// We load the address of the old handler into R12 and branch to it.
	// Because we restored R0-R3 and didn't change LR, the BIOS will think it was called directly!
	asm("ldr r12, =g_oldSwiHandler");
	asm("ldr r12, [r12]");
	asm("bx  r12");
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
	SystemVectors.swi = my_swi_handler;

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
	asm("mov r0, #0x67"); // This is the argument we pass to `my_swi_log!`
	// If not commented out, message should be "my_swi_log: 67!", otherwise "my_swi_log: called!"
	asm("swi 0x80");
	swiDelay(1); // If all our code above is right, this should not cause an abort!
	// here we should still be USER

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
