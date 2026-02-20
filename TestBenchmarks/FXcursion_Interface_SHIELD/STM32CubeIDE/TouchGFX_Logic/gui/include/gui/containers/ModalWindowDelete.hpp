#ifndef MODALWINDOWDELETE_HPP
#define MODALWINDOWDELETE_HPP

#include <gui_generated/containers/ModalWindowDeleteBase.hpp>

class ModalWindowDelete : public ModalWindowDeleteBase
{
public:
    ModalWindowDelete();
    virtual ~ModalWindowDelete() {}

    virtual void initialize();

    void setText(U8* const text);
    void setTextUnicode(TEXTS text);
protected:
};

#endif // MODALWINDOWDELETE_HPP
