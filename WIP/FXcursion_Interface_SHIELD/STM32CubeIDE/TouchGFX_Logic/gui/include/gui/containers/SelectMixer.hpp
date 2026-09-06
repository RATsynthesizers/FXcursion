#ifndef SELECTMIXER_HPP
#define SELECTMIXER_HPP

#include <gui_generated/containers/SelectMixerBase.hpp>

class SelectMixer : public SelectMixerBase
{
public:
    SelectMixer();
    virtual ~SelectMixer() {}

    virtual void initialize();
protected:
};

#endif // SELECTMIXER_HPP
