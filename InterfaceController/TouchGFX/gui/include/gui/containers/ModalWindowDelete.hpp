#ifndef MODALWINDOWDELETE_HPP
#define MODALWINDOWDELETE_HPP

#include <gui_generated/containers/ModalWindowDeleteBase.hpp>

class ModalWindowDelete : public ModalWindowDeleteBase
{
public:
    ModalWindowDelete();
    virtual ~ModalWindowDelete() {}

    virtual void initialize();

    virtual void setEffectName(TEXTS effectNameID);
protected:
};

#endif // MODALWINDOWDELETE_HPP
