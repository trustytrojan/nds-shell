#pragma once

#include <nds.h>

namespace Hardware
{
/**
 * Initialize hardware-specific features including DSi enhanced mode and GBA slot support
 */
void Init();

/**
 * Check if running on DSi hardware
 * @return true if DSi mode is available
 */
bool IsDSiMode();

/**
 * Check if GBA slot is available (DS hardware only)
 * @return true if GBA slot can be used for memory expansion
 */
bool IsGBASlotAvailable();

/**
 * Get total available RAM in bytes
 * @return total RAM available including expansions
 */
u32 GetAvailableRAM();

/**
 * Get hardware type string for display
 * @return human-readable hardware type
 */
const char *GetHardwareType();
} // namespace Hardware
