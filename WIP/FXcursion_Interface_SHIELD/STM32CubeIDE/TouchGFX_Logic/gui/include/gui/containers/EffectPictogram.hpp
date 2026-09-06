#ifndef EFFECTPICTOGRAM_HPP
#define EFFECTPICTOGRAM_HPP

#include <gui_generated/containers/EffectPictogramBase.hpp>

/**
 * One effect in the pictogram row on the effect settings screen.
 *
 * Three independent visual states, which is why they are applied together
 * rather than one at a time:
 *
 *   selected  the cursor is here          selectBox visible
 *   editing   its parameters are shown    editingBox + pictEditing visible
 *   moving    it is being reordered       selectBox outline turns white
 *
 * setState collapses them into a single repaint and skips it entirely when
 * nothing changed. That matters here: the row is four containers wide and the
 * naive "call select() then edit() then move() on all four" costs twelve
 * invalidates per encoder step, each of which can become its own dirty rect.
 */
class EffectPictogram : public EffectPictogramBase
{
public:
    EffectPictogram();
    virtual ~EffectPictogram() {}

    virtual void initialize();

    virtual void setEffect(FXChainItemInfo newEffectInfo);
    virtual FXChainItemInfo getEffect();

    virtual void select(bool select);
    virtual void edit(bool edit);
    virtual void move(bool move);

    /** Apply all three at once. Repaints only if something actually changed. */
    virtual void setState(bool bSelect, bool bEdit, bool bMove);

protected:
    FXChainItemInfo effectInfo;

    bool bSelected;
    bool bEditing;
    bool bMoving;

    /*
     * The normal outline colour is whatever Designer put in the Base - captured
     * rather than hardcoded, so recolouring the shape in the Designer keeps
     * working and does not silently disagree with this file.
     */
    touchgfx::colortype normalOutline;
    touchgfx::colortype movingOutline;

    void applyState();
};

#endif // EFFECTPICTOGRAM_HPP
