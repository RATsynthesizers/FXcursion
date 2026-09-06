#ifndef STATUSBAR_HPP
#define STATUSBAR_HPP

#include <gui_generated/containers/StatusBarBase.hpp>

class StatusBar : public StatusBarBase
{
public:
    StatusBar();
    virtual ~StatusBar() {}

    virtual void initialize();

    void startMove();
    void stopMove();
    void updateProjectName();
    void updateBattery();
    void updateBPM();
protected:

    /*
     * Refill a text buffer from the model WITHOUT invalidating.
     *
     * Split out so initialize() can prime all three buffers the way it always
     * did - the container is about to be drawn for the first time anyway - and
     * the public update* methods can be "refill and repaint". Previously
     * initialize() carried its own copy of each format call, which is how
     * three of the eight bad const casts came to exist.
     */
    void formatProjectName();
    void formatBattery();
    void formatBPM();
};

#endif // STATUSBAR_HPP
