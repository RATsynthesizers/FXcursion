#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void encParam_action(uint8_t id, int8_t scrollAmount) = 0;
    virtual void encSelect_action(int8_t scrollAmount) = 0;
    virtual void btnYES_action(void) = 0;
    virtual void btnNO_action(void) = 0;
    virtual void btnUP_action(void) = 0;
    virtual void btnDOWN_action(void) = 0;
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
