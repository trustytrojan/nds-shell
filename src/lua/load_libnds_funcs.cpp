#include "my_state.hpp"

void my_state::load_libnds_funcs()
{
	// clang-format off
	create_named_table("libnds",
		"setBrightness", setBrightness, // this was just for testing, leave it in anyway for fun
		"pmMainLoop", pmMainLoop,
#ifdef NDSH_THREADING
		"threadYield", threadYield
#else
		"threadYield", [] {}
#endif
	);
	// clang-format on
}
