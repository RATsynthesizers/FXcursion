#ifndef RECMODULESTEREO_HPP
#define RECMODULESTEREO_HPP

#include <gui_generated/containers/RecModuleStereoBase.hpp>

class RecModuleStereo : public RecModuleStereoBase
{
public:
    RecModuleStereo();
    virtual ~RecModuleStereo() {}

    virtual void initialize();
protected:
};

#endif // RECMODULESTEREO_HPP
