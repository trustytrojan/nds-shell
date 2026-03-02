#include "my_state.hpp"

void my_state::load_libnds_funcs()
{
	// clang-format off
	create_named_table("libnds",
		"setBrightness", setBrightness, // this was just for testing, leave it in anyway for fun
		"pmMainLoop",
#ifdef __BLOCKSDS__
		[] { return true; }
#else
		pmMainLoop
#endif
		,
		"threadYield",
#ifdef NDSH_THREADING
		threadYield
#else
		[] { swiWaitForVBlank(); }
#endif
	);
	// clang-format on
}
