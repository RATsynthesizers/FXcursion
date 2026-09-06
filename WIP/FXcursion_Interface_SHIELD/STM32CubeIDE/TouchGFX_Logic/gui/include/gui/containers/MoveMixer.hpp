#ifndef MOVEMIXER_HPP
#define MOVEMIXER_HPP

#include <gui_generated/containers/MoveMixerBase.hpp>

class MoveMixer : public MoveMixerBase
{
public:
    MoveMixer();
    virtual ~MoveMixer() {}

    virtual void initialize();
protected:
};

#endif // MOVEMIXER_HPP
