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

    virtual void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed) = 0;
    virtual void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed) = 0;
    virtual void btnUpUpdate(BOOLEAN bIsFuncPressed) = 0;
    virtual void btnDownUpdate(BOOLEAN bIsFuncPressed) = 0;
    virtual void btnFootUpdate(U8 nID) = 0;

    virtual void encMenuUpdate(S8 nValue) = 0;
    virtual void encParamUpdate(U8 nID, S8 nValue) = 0;
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
