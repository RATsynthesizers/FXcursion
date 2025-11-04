#ifndef EMPTYMODULESTEREO_HPP
#define EMPTYMODULESTEREO_HPP

#include <gui_generated/containers/EmptyModuleStereoBase.hpp>

class EmptyModuleStereo : public EmptyModuleStereoBase
{
public:
    EmptyModuleStereo();
    virtual ~EmptyModuleStereo() {}

    virtual void initialize();
protected:
};

#endif // EMPTYMODULESTEREO_HPP
