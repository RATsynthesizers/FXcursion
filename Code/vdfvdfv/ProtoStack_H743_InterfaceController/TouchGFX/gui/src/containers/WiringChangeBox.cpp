#include <gui/containers/WiringChangeBox.hpp>

WiringChangeBox::WiringChangeBox() {

}

void WiringChangeBox::initialize() {
	WiringChangeBoxBase::initialize();
}

void WiringChangeBox::setText(uint8_t connectDelete, uint8_t i) {
	if (connectDelete == 1) {
		Unicode::fromUTF8((const uint8_t*) "Connect", TextBuffer1, TEXTBUFFER1_SIZE);
	} else if (connectDelete == 2) {
		Unicode::fromUTF8((const uint8_t*) "Delete", TextBuffer1, TEXTBUFFER1_SIZE);
	}
	const uint8_t io[10] = "";
	if (i < 3) {
		strcpy((char*) io, "Input ");
		char buf[2];
		sprintf(buf, "%d", i + 1);
		strcat((char*) io, buf);
	} else {
		strcpy((char*) io, "Output ");
		char buf[2];
		sprintf(buf, "%d", (i - 3) + 1);
		strcat((char*) io, buf);
	}

	Unicode::fromUTF8(io, TextBuffer2, TEXTBUFFER2_SIZE);
	Text.invalidate();
}
