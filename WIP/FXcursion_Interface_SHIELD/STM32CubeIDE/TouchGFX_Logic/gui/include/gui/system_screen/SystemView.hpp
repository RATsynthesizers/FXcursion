#ifndef SYSTEMVIEW_HPP
#define SYSTEMVIEW_HPP

#include <gui_generated/system_screen/SystemViewBase.hpp>
#include <gui/system_screen/SystemPresenter.hpp>
#include <gui/system_screen/SystemNav.hpp>

typedef struct stInputType
{
	BOOLEAN bIsStereo1;
	BOOLEAN bIsStereo2;

} InputType_t;

class SystemView : public SystemViewBase
{
public:
    SystemView();
    virtual ~SystemView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
protected:

    static const U8 MIXER_Y_POS = 40;
    static const U8 MIXER_X_POSITIONS[4];

    BOOLEAN bIsMixerAdded;
    ChainModuleNumber eMixerPosition;

    InputType_t inputType;
    ModuleSelectType ePrevSelect;
    ModuleSelectType eCurrentSelect;

    /***********************************************************************
     * THE ROW TABLE
     *
     * How many chain rows are on screen, and which container each one is,
     * depends on the two stereo flags: a stereo input pair is ONE row where a
     * mono pair is two. The old navigation code answered that question by
     * hand at every decision point - "if (FALSE == bIsStereo1) monoChain2
     * else stereoChain1" - roughly seventy times, and got it wrong twice.
     *
     * Built once per screen entry, because the flags only change on the Input
     * screen and this view is destroyed and reconstructed on every screen
     * change.
     **********************************************************************/
    typedef struct stChainRowRef
    {
        MonoChain*       pMono;     /**< exactly one of these is non-NULL */
        StereoChain*     pStereo;
        ModuleSelectType eSelect;   /**< the persisted name for this row  */

    } ChainRowRef;

    ChainRowRef aRow[NAV_ROW_MAX];
    U8          nRowQty;

    void buildRowTable();
    void addRow(MonoChain* pMono, StereoChain* pStereo, ModuleSelectType eSelect);

    /** Row index for a persisted select id, or the nearest surviving row. */
    S8 rowIndexForSelect(ModuleSelectType eSelect);

    /** The persisted select id naming a cursor position. */
    ModuleSelectType selectForPos(const NAV_POS& tPos);

    /** What the cursor is on right now, read back from the widgets. */
    NAV_POS currentPos();

    /** What SystemNav needs to know about the grid. */
    NAV_CTX navCtx();

    /* Highlight plumbing. Split so setupScreen can raise a highlight with
       nothing to lower first. */
    void applyHighlight(const NAV_POS& tPos);
    void clearHighlight(const NAV_POS& tPos);
    void moveCursor(const NAV_POS& tOld, const NAV_POS& tNew);

    /* Row-table dispatch, so the callers never care which flavour a row is. */
    void              rowSelect(U8 nRow, ChainModuleNumber eSlot);
    void              rowDeselect(U8 nRow, ChainModuleNumber eSlot);
    ChainModuleNumber rowSelectedSlot(U8 nRow);

    /*
     * Kept because btnYesUpdate and btnNoUpdate still address chains this
     * way. Those two are correct as they stand - they already index rather
     * than repeat themselves - and folding them onto the row table is a
     * separate change, worth making only once the navigation above has been
     * confirmed on hardware.
     */
    MonoChain* monoChain[4] =
    {
    	&monoChain1,
		&monoChain2,
		&monoChain3,
		&monoChain4
    };

    StereoChain* stereoChain[2] =
    {
    	&stereoChain1,
		&stereoChain2
    };
};

#endif // SYSTEMVIEW_HPP
