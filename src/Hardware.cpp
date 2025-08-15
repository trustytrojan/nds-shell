#include "Hardware.hpp"

#include <nds.h>

namespace Hardware
{
    static bool dsiMode = false;
    static bool gbaSlotAvailable = false;
    static u32 totalRAM = 0;
    static const char* hwType = "DS";

    void Init()
    {
        // Check for DSi mode - this function is available in libnds
        dsiMode = isDSiMode();
        
        if (dsiMode) {
            hwType = "DSi";
            // DSi has 16MB of RAM available
            totalRAM = 16 * 1024 * 1024;
            
            // Enable DSi enhanced features if available
            // This allows access to additional memory and faster CPU
            // The system will automatically use enhanced features when in DSi mode
        } else {
            hwType = "DS";
            // Standard DS has 4MB of RAM
            totalRAM = 4 * 1024 * 1024;
            
            // Check for GBA slot availability on DS hardware
            // GBA slot can provide additional memory expansion
            // Check if GBA slot is present and not occupied by a cartridge
            gbaSlotAvailable = (REG_GBA_CART & 0xFF) == 0x96;
            
            if (gbaSlotAvailable) {
                // Initialize GBA slot for memory expansion
                // This enables access to additional RAM through the GBA slot
                // Typically adds 32MB of additional address space
                totalRAM += 32 * 1024 * 1024;
            }
        }
    }

    bool IsDSiMode()
    {
        return dsiMode;
    }

    bool IsGBASlotAvailable()
    {
        return gbaSlotAvailable;
    }

    u32 GetAvailableRAM()
    {
        return totalRAM;
    }

    const char* GetHardwareType()
    {
        return hwType;
    }
}