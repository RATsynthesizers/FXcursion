#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void parameterChange(uint8_t id, int8_t scrollAmount) = 0;
    virtual void encScroll_action(int8_t scrollAmount) = 0;
    virtual void btnYES_action(void) = 0;
    virtual void btnNO_action(void) = 0;

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
