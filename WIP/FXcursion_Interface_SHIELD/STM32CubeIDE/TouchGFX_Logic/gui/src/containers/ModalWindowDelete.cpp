#include <gui/containers/ModalWindowDelete.hpp>

ModalWindowDelete::ModalWindowDelete()
{

}

void ModalWindowDelete::initialize()
{
    ModalWindowDeleteBase::initialize();
}

void ModalWindowDelete::setText(U8* const text)
{
	Unicode::fromUTF8(text,
					  textModalBuffer,
					  TEXTMODAL_SIZE);
	textModal.invalidate();
}

void ModalWindowDelete::setTextUnicode(TEXTS text)
{
	Unicode::snprintf(textModalBuffer,
					  TEXTMODAL_SIZE,
					  "%s",
					  touchgfx::TypedText(text).getText());

	textModal.invalidate();
}
