/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AboutDialog.h"
#include "EventTickerPanel.h"
#include "HelpDialog.h"
#include "LoggingDialog.h"
#include "PreferencesDialog.h"
#include "ProbePanel.h"
#include "SettingsDialog.h"
#include "ShipLoadDialog.h"
#include "SoundController.h"
#include "ToolController.h"
#include "UIPreferencesManager.h"
#include "UpdateChecker.h"

#include <Game/GameController.h>
#include <Game/GameEventHandlers.h>
#include <Game/ResourceLoader.h>

#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * The main window of the game's GUI.
 */
class MainFrame
    : public wxFrame
    , public ILifecycleGameEventHandler
    , public IGenericGameEventHandler
{
public:

    static constexpr bool StartInFullScreenMode = true;
    static constexpr bool StartWithStatusText = true;
    static constexpr bool StartWithExtendedStatusText = false;
    static constexpr int CursorStep = 30;
    static constexpr int PowerBarThickness = 2;

public:

    MainFrame(wxApp * mainApp);

    virtual ~MainFrame();

private:

    //
    // Canvas
    //

    std::unique_ptr<wxGLCanvas> mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    //
    // Controls that we're interacting with
    //

    wxBoxSizer * mMainFrameSizer;
    wxMenuItem * mPauseMenuItem;
    wxMenuItem * mStepMenuItem;
    wxMenu * mToolsMenu;
    wxMenuItem * mRCBombsDetonateMenuItem;
    wxMenuItem * mAntiMatterBombsDetonateMenuItem;
    wxMenuItem * mShowEventTickerMenuItem;
    wxMenuItem * mShowProbePanelMenuItem;
    wxMenuItem * mShowStatusTextMenuItem;
    wxMenuItem * mShowExtendedStatusTextMenuItem;
    wxMenuItem * mFullScreenMenuItem;
    wxMenuItem * mNormalScreenMenuItem;
    wxMenuItem * mMuteMenuItem;
    std::unique_ptr<EventTickerPanel> mEventTickerPanel;
    std::unique_ptr<ProbePanel> mProbePanel;

    //
    // Dialogs
    //

    std::unique_ptr<ShipLoadDialog> mShipLoadDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;
    std::unique_ptr<SettingsDialog> mSettingsDialog;
    std::unique_ptr<PreferencesDialog> mPreferencesDialog;
    std::unique_ptr<HelpDialog> mHelpDialog;
    std::unique_ptr<AboutDialog> mAboutDialog;

    //
    // Timers
    //

    std::unique_ptr<wxTimer> mPostInitializeTimer;
    std::unique_ptr<wxTimer> mGameTimer;
    std::unique_ptr<wxTimer> mLowFrequencyTimer;
    std::unique_ptr<wxTimer> mCheckUpdateTimer;

private:

    //
    // Event handlers
    //

    // App
    void OnPostInitializeTrigger(wxTimerEvent & event);
    void OnMainFrameClose(wxCloseEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnKeyDown(wxKeyEvent & event);
    void OnGameTimerTrigger(wxTimerEvent & event);
    void OnLowFrequencyTimerTrigger(wxTimerEvent & event);
    void OnCheckUpdateTimerTrigger(wxTimerEvent & event);
    void OnIdle(wxIdleEvent & event);
    void OnShipFileChosen(fsShipFileChosenEvent & event);

    // Main GL canvas
    void OnMainGLCanvasResize(wxSizeEvent& event);
    void OnMainGLCanvasLeftDown(wxMouseEvent& event);
    void OnMainGLCanvasLeftUp(wxMouseEvent& event);
    void OnMainGLCanvasRightDown(wxMouseEvent& event);
    void OnMainGLCanvasRightUp(wxMouseEvent& event);
    void OnMainGLCanvasMouseMove(wxMouseEvent& event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent& event);
    void OnMainGLCanvasCaptureMouseLost(wxCloseEvent& event);

    // Menu
    void OnZoomInMenuItemSelected(wxCommandEvent& event);
    void OnZoomOutMenuItemSelected(wxCommandEvent& event);
    void OnAmbientLightUpMenuItemSelected(wxCommandEvent& event);
    void OnAmbientLightDownMenuItemSelected(wxCommandEvent& event);
    void OnPauseMenuItemSelected(wxCommandEvent& event);
    void OnStepMenuItemSelected(wxCommandEvent& event);
    void OnResetViewMenuItemSelected(wxCommandEvent& event);
    void OnLoadShipMenuItemSelected(wxCommandEvent& event);
    void OnReloadLastShipMenuItemSelected(wxCommandEvent& event);
    void OnSaveScreenshotMenuItemSelected(wxCommandEvent& event);

    void OnMoveMenuItemSelected(wxCommandEvent& event);
    void OnMoveAllMenuItemSelected(wxCommandEvent& event);
    void OnSmashMenuItemSelected(wxCommandEvent& event);
    void OnSliceMenuItemSelected(wxCommandEvent& event);
    void OnFlameThrowerMenuItemSelected(wxCommandEvent& event);
    void OnGrabMenuItemSelected(wxCommandEvent& event);
    void OnSwirlMenuItemSelected(wxCommandEvent& event);
    void OnPinMenuItemSelected(wxCommandEvent& event);
    void OnInjectAirBubblesMenuItemSelected(wxCommandEvent& event);
    void OnFloodHoseMenuItemSelected(wxCommandEvent& event);
    void OnTimerBombMenuItemSelected(wxCommandEvent& event);
    void OnRCBombMenuItemSelected(wxCommandEvent& event);
    void OnImpactBombMenuItemSelected(wxCommandEvent& event);
    void OnAntiMatterBombMenuItemSelected(wxCommandEvent& event);
    void OnWaveMakerMenuItemSelected(wxCommandEvent& event);
    void OnAdjustTerrainMenuItemSelected(wxCommandEvent& event);
    void OnRepairStructureMenuItemSelected(wxCommandEvent& event);
    void OnScrubMenuItemSelected(wxCommandEvent& event);
    void OnRCBombDetonateMenuItemSelected(wxCommandEvent& event);
    void OnAntiMatterBombDetonateMenuItemSelected(wxCommandEvent& event);
    void OnTriggerTsunamiMenuItemSelected(wxCommandEvent& event);
    void OnTriggerRogueWaveMenuItemSelected(wxCommandEvent& event);

    void OnOpenSettingsWindowMenuItemSelected(wxCommandEvent& event);
    void OnOpenPreferencesWindowMenuItemSelected(wxCommandEvent& event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent& event);
    void OnShowEventTickerMenuItemSelected(wxCommandEvent& event);
    void OnShowProbePanelMenuItemSelected(wxCommandEvent& event);
    void OnShowStatusTextMenuItemSelected(wxCommandEvent& event);
    void OnShowExtendedStatusTextMenuItemSelected(wxCommandEvent& event);
    void OnFullScreenMenuItemSelected(wxCommandEvent& event);
    void OnNormalScreenMenuItemSelected(wxCommandEvent& event);
    void OnMuteMenuItemSelected(wxCommandEvent& event);
    void OnHelpMenuItemSelected(wxCommandEvent& event);
    void OnAboutMenuItemSelected(wxCommandEvent& event);
    void OnCheckForUpdatesMenuItemSelected(wxCommandEvent& event);
    void OnOpenHomePageMenuItemSelected(wxCommandEvent& event);
    void OnOpenCodePageMenuItemSelected(wxCommandEvent& event);

    //
    // Game event handler
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    virtual void OnGameReset() override
    {
        mCurrentShipTitles.clear();

        UpdateFrameTitle();
    }

    virtual void OnShipLoaded(
        unsigned int /*id*/,
        std::string const & name,
        std::optional<std::string> const & author) override
    {
        std::string shipTitle = name;
        if (!!author)
            shipTitle += " - by " + *author;

        mCurrentShipTitles.push_back(shipTitle);

        UpdateFrameTitle();
    }

    virtual void OnBombPlaced(
        BombId /*bombId*/,
        BombType bombType,
        bool /*isUnderwater*/) override
    {
        if (BombType::RCBomb == bombType)
        {
            ++mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
        else if (BombType::AntiMatterBomb == bombType)
        {
            ++mCurrentAntiMatterBombCount;
            mAntiMatterBombsDetonateMenuItem->Enable(mCurrentAntiMatterBombCount > 0);
        }
    }

    virtual void OnBombRemoved(
        BombId /*bombId*/,
        BombType bombType,
        std::optional<bool> /*isUnderwater*/) override
    {
        if (BombType::RCBomb == bombType)
        {
            assert(mCurrentRCBombCount > 0u);
            --mCurrentRCBombCount;
            mRCBombsDetonateMenuItem->Enable(mCurrentRCBombCount > 0);
        }
        else if (BombType::AntiMatterBomb == bombType)
        {
            assert(mCurrentAntiMatterBombCount > 0u);
            --mCurrentAntiMatterBombCount;
            mAntiMatterBombsDetonateMenuItem->Enable(mCurrentAntiMatterBombCount > 0);
        }
    }

private:

    inline void AfterGameRender()
    {
        if (!mHasWindowBeenShown)
        {
            this->Show(true);

            if (StartInFullScreenMode)
                this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);

            mHasWindowBeenShown = true;
        }
    }

    void ResetState();

    void UpdateFrameTitle();

    void OnError(
        std::string const & message,
        bool die);

    void PostGameStepTimer();

    void StartLowFrequencyTimer();

private:

    wxApp * const mMainApp;

    //
    // Helpers
    //

    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<GameController> mGameController;
    std::shared_ptr<SoundController> mSoundController;
    std::unique_ptr<ToolController> mToolController;
    std::shared_ptr<UIPreferencesManager> mUIPreferencesManager;
    std::unique_ptr<UpdateChecker> mUpdateChecker;


    //
    // State
    //

    bool mHasWindowBeenShown;
    bool mHasStartupTipBeenChecked;
    std::vector<std::string> mCurrentShipTitles;
    size_t mCurrentRCBombCount;
    size_t mCurrentAntiMatterBombCount;
    bool mIsShiftKeyDown;
};
