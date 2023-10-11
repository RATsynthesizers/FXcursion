#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

#define GUI_NESTING_LEVELS_NUMBER 3

class ModelListener;

class Model
{
public:
    Model();


    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

protected:
    ModelListener* modelListener;
};

#endif // MODEL_HPP
