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

    /*
     * New telemetry from the audio controller.
     *
     * Deliberately NOT pure, unlike everything above it: a screen that draws no
     * meters should not have to declare that it ignores them, and making this
     * pure would force an empty override into every existing view.
     */
    virtual void telemetryUpdate(const PROTO_TELEMETRY& tTelem)
    {
        (void)tTelem;
    }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
