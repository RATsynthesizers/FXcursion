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

    virtual void btnYesUpdate(void) = 0;
    virtual void btnNoUpdate(void) = 0;
    virtual void btnUpUpdate(void) = 0;
    virtual void btnDownUpdate(void) = 0;
    virtual void btnFootUpdate(U8 nID) = 0;

    virtual void encMenuUpdate(S8 nValue) = 0;
    virtual void encParamUpdate(U8 nID, S8 nValue) = 0;
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
