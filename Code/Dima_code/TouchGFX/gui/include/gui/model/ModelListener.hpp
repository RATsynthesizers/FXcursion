#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 320

class ModelListener
{
public:
    ModelListener() : model(0) {};
    
    virtual ~ModelListener() {};

    void bind(Model* m)
    {
        model = m;
    }

    virtual void changeScreenLeft() = 0;
    virtual void changeScreenRight() = 0;
    virtual void scrollWindow(int8_t scrollAmount) = 0;
    virtual void parameterChange() = 0;

    uint8_t MenuPosition[GUI_NESTING_LEVELS_NUMBER];

protected:
    Model* model;
};



#endif // MODELLISTENER_HPP
