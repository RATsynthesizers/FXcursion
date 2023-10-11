#include <gui/containers/ModalWindowDelete.hpp>

ModalWindowDelete::ModalWindowDelete() {

}

void ModalWindowDelete::initialize() {
	ModalWindowDeleteBase::initialize();
}

void ModalWindowDelete::setEffectName(TEXTS effectNameID) {
	Unicode::snprintf(textModalBuffer, TEXTMODAL_SIZE, "%s",
			touchgfx::TypedText(effectNameID).getText());
	textModal.invalidate();
}
