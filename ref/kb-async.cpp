#include <nds.h>
#include <stdio.h>

int main(void) {
	consoleDemoInit();
	keyboardDemoInit();
	keyboardShow();
	while(1) {
		int key = keyboardUpdate();
		if(key > 0)
			iprintf("%c", key);
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}
}