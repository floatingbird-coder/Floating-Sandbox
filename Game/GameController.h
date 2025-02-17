/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameEventHandlers.h"
#include "GameParameters.h"
#include "IGameController.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "ShipMetadata.h"
#include "StatusText.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageData.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

/*
 * This class is responsible for managing the game, from its lifetime to the user
 * interactions.
 */
class GameController final
    : public IGameController
    , public IWavePhenomenaGameEventHandler
{
public:

    static std::unique_ptr<GameController> Create(
        bool isStatusTextEnabled,
        bool isExtendedStatusTextEnabled,
        std::function<void()> swapRenderBuffersFunction,
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

    std::shared_ptr<GameEventDispatcher> GetGameEventDispatcher()
    {
        assert(!!mGameEventDispatcher);
        return mGameEventDispatcher;
    }

public:

    /////////////////////////////////////////////////////////
    // IGameController
    /////////////////////////////////////////////////////////

    void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterLifecycleEventHandler(handler);
    }

    void RegisterStructuralEventHandler(IStructuralGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStructuralEventHandler(handler);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterWavePhenomenaEventHandler(handler);
    }

    void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStatisticsEventHandler(handler);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterGenericEventHandler(handler);
    }

    ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath) override;
    ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath) override;
    void ReloadLastShip() override;

    RgbImageData TakeScreenshot() override;

    void RunGameIteration() override;
    void LowFrequencyUpdate() override;

    void Update() override;
    void Render() override;

    //
    // Game Control
    //

    void SetPaused(bool isPaused) override;
    void SetMoveToolEngaged(bool isEngaged) override;
    void SetStatusTextEnabled(bool isEnabled) override;
    void SetExtendedStatusTextEnabled(bool isEnabled) override;

    //
    // World probing
    //

    float GetCurrentSimulationTime() const override;
    bool IsUnderwater(vec2f const & screenCoordinates) const override;

    //
    // Interactions
    //

    void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ElementId> & elementId) override;
    void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ShipId> & shipId) override;
    void MoveBy(ElementId elementId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) override;
    void MoveBy(ShipId shipId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) override;
    void RotateBy(ElementId elementId, float screenDeltaY, vec2f const & screenCenter, float inertialScreenDeltaY) override;
    void RotateBy(ShipId shipId, float screenDeltaY, vec2f const & screenCenter, float intertialScreenDeltaY) override;
    void DestroyAt(vec2f const & screenCoordinates, float radiusFraction) override;
    void RepairAt(vec2f const & screenCoordinates, float radiusMultiplier, RepairSessionId sessionId, RepairSessionStepId sessionStepId) override;
    void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    bool ApplyFlameThrowerAt(vec2f const & screenCoordinates) override;
    void DrawTo(vec2f const & screenCoordinates, float strengthFraction) override;
    void SwirlAt(vec2f const & screenCoordinates, float strengthFraction) override;
    void TogglePinAt(vec2f const & screenCoordinates) override;
    bool InjectBubblesAt(vec2f const & screenCoordinates) override;
    bool FloodAt(vec2f const & screenCoordinates, float waterQuantityMultiplier) override;
    void ToggleAntiMatterBombAt(vec2f const & screenCoordinates) override;
    void ToggleImpactBombAt(vec2f const & screenCoordinates) override;
    void ToggleRCBombAt(vec2f const & screenCoordinates) override;
    void ToggleTimerBombAt(vec2f const & screenCoordinates) override;
    void DetonateRCBombs() override;
    void DetonateAntiMatterBombs() override;
    void AdjustOceanSurfaceTo(std::optional<vec2f> const & screenCoordinates) override;
    bool AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    bool ScrubThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) override;
    std::optional<ElementId> GetNearestPointAt(vec2f const & screenCoordinates) const;
    void QueryNearestPointAt(vec2f const & screenCoordinates) const;

    void TriggerTsunami() override;
    void TriggerRogueWave() override;

    //
    // Render controls
    //

    void SetCanvasSize(int width, int height) override;
    void Pan(vec2f const & screenOffset) override;
    void PanImmediate(vec2f const & screenOffset) override;
    void ResetPan() override;
    void AdjustZoom(float amount) override;
    void ResetZoom() override;
    vec2f ScreenToWorld(vec2f const & screenCoordinates) const override;

    //
    // Game parameters
    //

    float GetNumMechanicalDynamicsIterationsAdjustment() const override { return mGameParameters.NumMechanicalDynamicsIterationsAdjustment; }
    void SetNumMechanicalDynamicsIterationsAdjustment(float value) override { mGameParameters.NumMechanicalDynamicsIterationsAdjustment = value; }
    float GetMinNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MinNumMechanicalDynamicsIterationsAdjustment; }
    float GetMaxNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MaxNumMechanicalDynamicsIterationsAdjustment; }

    float GetSpringStiffnessAdjustment() const override { return mParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStiffnessAdjustment(float value) override { mParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStiffnessAdjustment() const override { return GameParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const override { return GameParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const override { return mGameParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value) override { mGameParameters.SpringDampingAdjustment = value; }
    float GetMinSpringDampingAdjustment() const override { return GameParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const override { return GameParameters::MaxSpringDampingAdjustment; }

    float GetSpringStrengthAdjustment() const override { return mParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStrengthAdjustment(float value) override { mParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStrengthAdjustment() const override { return GameParameters::MinSpringStrengthAdjustment;  }
    float GetMaxSpringStrengthAdjustment() const override { return GameParameters::MaxSpringStrengthAdjustment; }

    float GetRotAcceler8r() const override { return mGameParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) override { mGameParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const override { return GameParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const override { return GameParameters::MaxRotAcceler8r; }

    float GetWaterDensityAdjustment() const override { return mGameParameters.WaterDensityAdjustment; }
    void SetWaterDensityAdjustment(float value) override { mGameParameters.WaterDensityAdjustment = value; }
    float GetMinWaterDensityAdjustment() const override { return GameParameters::MinWaterDensityAdjustment; }
    float GetMaxWaterDensityAdjustment() const override { return GameParameters::MaxWaterDensityAdjustment; }

    float GetWaterDragAdjustment() const override { return mGameParameters.WaterDragAdjustment; }
    void SetWaterDragAdjustment(float value) override { mGameParameters.WaterDragAdjustment = value; }
    float GetMinWaterDragAdjustment() const override { return GameParameters::MinWaterDragAdjustment; }
    float GetMaxWaterDragAdjustment() const override { return GameParameters::MaxWaterDragAdjustment; }

    float GetWaterIntakeAdjustment() const override { return mGameParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) override { mGameParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const override { return GameParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const override { return GameParameters::MaxWaterIntakeAdjustment; }

    float GetWaterCrazyness() const override { return mGameParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) override { mGameParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const override { return GameParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const override { return GameParameters::MaxWaterCrazyness; }

    float GetWaterDiffusionSpeedAdjustment() const override { return mGameParameters.WaterDiffusionSpeedAdjustment; }
    void SetWaterDiffusionSpeedAdjustment(float value) override { mGameParameters.WaterDiffusionSpeedAdjustment = value; }
    float GetMinWaterDiffusionSpeedAdjustment() const override { return GameParameters::MinWaterDiffusionSpeedAdjustment; }
    float GetMaxWaterDiffusionSpeedAdjustment() const override { return GameParameters::MaxWaterDiffusionSpeedAdjustment; }

    float GetBasalWaveHeightAdjustment() const override { return mGameParameters.BasalWaveHeightAdjustment; }
    void SetBasalWaveHeightAdjustment(float value) override { mGameParameters.BasalWaveHeightAdjustment = value; }
    float GetMinBasalWaveHeightAdjustment() const override { return GameParameters::MinBasalWaveHeightAdjustment; }
    float GetMaxBasalWaveHeightAdjustment() const override { return GameParameters::MaxBasalWaveHeightAdjustment; }

    float GetBasalWaveLengthAdjustment() const override { return mGameParameters.BasalWaveLengthAdjustment; }
    void SetBasalWaveLengthAdjustment(float value) override { mGameParameters.BasalWaveLengthAdjustment = value; }
    float GetMinBasalWaveLengthAdjustment() const override { return GameParameters::MinBasalWaveLengthAdjustment; }
    float GetMaxBasalWaveLengthAdjustment() const override { return GameParameters::MaxBasalWaveLengthAdjustment; }

    float GetBasalWaveSpeedAdjustment() const override { return mGameParameters.BasalWaveSpeedAdjustment; }
    void SetBasalWaveSpeedAdjustment(float value) override { mGameParameters.BasalWaveSpeedAdjustment = value; }
    float GetMinBasalWaveSpeedAdjustment() const override { return GameParameters::MinBasalWaveSpeedAdjustment; }
    float GetMaxBasalWaveSpeedAdjustment() const override { return GameParameters::MaxBasalWaveSpeedAdjustment; }

    float GetTsunamiRate() const override { return mGameParameters.TsunamiRate; }
    void SetTsunamiRate(float value) override { mGameParameters.TsunamiRate = value; }
    float GetMinTsunamiRate() const override { return GameParameters::MinTsunamiRate; }
    float GetMaxTsunamiRate() const override { return GameParameters::MaxTsunamiRate; }

    float GetRogueWaveRate() const override { return mGameParameters.RogueWaveRate; }
    void SetRogueWaveRate(float value) override { mGameParameters.RogueWaveRate = value; }
    float GetMinRogueWaveRate() const override { return GameParameters::MinRogueWaveRate; }
    float GetMaxRogueWaveRate() const override { return GameParameters::MaxRogueWaveRate; }

    bool GetDoModulateWind() const override { return mGameParameters.DoModulateWind; }
    void SetDoModulateWind(bool value) override { mGameParameters.DoModulateWind = value; }

    float GetWindSpeedBase() const override { return mGameParameters.WindSpeedBase; }
    void SetWindSpeedBase(float value) override { mGameParameters.WindSpeedBase = value; }
    float GetMinWindSpeedBase() const override { return GameParameters::MinWindSpeedBase; }
    float GetMaxWindSpeedBase() const override { return GameParameters::MaxWindSpeedBase; }

    float GetWindSpeedMaxFactor() const override { return mGameParameters.WindSpeedMaxFactor; }
    void SetWindSpeedMaxFactor(float value) override { mGameParameters.WindSpeedMaxFactor = value; }
    float GetMinWindSpeedMaxFactor() const override { return GameParameters::MinWindSpeedMaxFactor; }
    float GetMaxWindSpeedMaxFactor() const override { return GameParameters::MaxWindSpeedMaxFactor; }

    // Heat

    float GetFlameThrowerHeatFlow() const override { return mGameParameters.FlameThrowerHeatFlow; }
    void SetFlameThrowerHeatFlow(float value) override { mGameParameters.FlameThrowerHeatFlow = value; }
    float GetMinFlameThrowerHeatFlow() const override { return GameParameters::MinFlameThrowerHeatFlow; }
    float GetMaxFlameThrowerHeatFlow() const override { return GameParameters::MaxFlameThrowerHeatFlow; }

    float GetFlameThrowerRadius() const override { return mGameParameters.FlameThrowerRadius; }
    void SetFlameThrowerRadius(float value) override { mGameParameters.FlameThrowerRadius = value; }
    float GetMinFlameThrowerRadius() const override { return GameParameters::MinFlameThrowerRadius; }
    float GetMaxFlameThrowerRadius() const override { return GameParameters::MaxFlameThrowerRadius; }

    // Misc

    float GetSeaDepth() const override { return mParameterSmoothers[SeaDepthParameterSmoother].GetValue(); }
    void SetSeaDepth(float value) override { mParameterSmoothers[SeaDepthParameterSmoother].SetValue(value); }
    float GetMinSeaDepth() const override { return GameParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const override { return GameParameters::MaxSeaDepth; }

    float GetOceanFloorBumpiness() const override { return mParameterSmoothers[OceanFloorBumpinessParameterSmoother].GetValue(); }
    void SetOceanFloorBumpiness(float value) override { mParameterSmoothers[OceanFloorBumpinessParameterSmoother].SetValue(value); }
    float GetMinOceanFloorBumpiness() const override { return GameParameters::MinOceanFloorBumpiness; }
    float GetMaxOceanFloorBumpiness() const override { return GameParameters::MaxOceanFloorBumpiness; }

    float GetOceanFloorDetailAmplification() const override { return mParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].GetValue(); }
    void SetOceanFloorDetailAmplification(float value) override { mParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValue(value); }
    float GetMinOceanFloorDetailAmplification() const override { return GameParameters::MinOceanFloorDetailAmplification; }
    float GetMaxOceanFloorDetailAmplification() const override { return GameParameters::MaxOceanFloorDetailAmplification; }

    float GetDestroyRadius() const override { return mGameParameters.DestroyRadius; }
    void SetDestroyRadius(float value) override { mGameParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const override { return GameParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const override { return GameParameters::MaxDestroyRadius; }

    float GetRepairRadius() const override { return mGameParameters.RepairRadius; }
    void SetRepairRadius(float value) override { mGameParameters.RepairRadius = value; }
    float GetMinRepairRadius() const override { return GameParameters::MinRepairRadius; }
    float GetMaxRepairRadius() const override { return GameParameters::MaxRepairRadius; }

    float GetRepairSpeedAdjustment() const override { return mGameParameters.RepairSpeedAdjustment; }
    void SetRepairSpeedAdjustment(float value) override { mGameParameters.RepairSpeedAdjustment = value; }
    float GetMinRepairSpeedAdjustment() const override { return GameParameters::MinRepairSpeedAdjustment; }
    float GetMaxRepairSpeedAdjustment() const override { return GameParameters::MaxRepairSpeedAdjustment; }

    float GetBombBlastRadius() const override { return mGameParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) override { mGameParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const override { return GameParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const override { return GameParameters::MaxBombBlastRadius; }

    float GetAntiMatterBombImplosionStrength() const override { return mGameParameters.AntiMatterBombImplosionStrength; }
    void SetAntiMatterBombImplosionStrength(float value) override { mGameParameters.AntiMatterBombImplosionStrength = value; }
    float GetMinAntiMatterBombImplosionStrength() const override { return GameParameters::MinAntiMatterBombImplosionStrength; }
    float GetMaxAntiMatterBombImplosionStrength() const override { return GameParameters::MaxAntiMatterBombImplosionStrength; }

    float GetFloodRadius() const override { return mGameParameters.FloodRadius; }
    void SetFloodRadius(float value) override { mGameParameters.FloodRadius = value; }
    float GetMinFloodRadius() const override { return GameParameters::MinFloodRadius; }
    float GetMaxFloodRadius() const override { return GameParameters::MaxFloodRadius; }

    float GetFloodQuantity() const override { return mGameParameters.FloodQuantity; }
    void SetFloodQuantity(float value) override { mGameParameters.FloodQuantity = value; }
    float GetMinFloodQuantity() const override { return GameParameters::MinFloodQuantity; }
    float GetMaxFloodQuantity() const override { return GameParameters::MaxFloodQuantity; }

    float GetLuminiscenceAdjustment() const override { return mGameParameters.LuminiscenceAdjustment; }
    void SetLuminiscenceAdjustment(float value) override { mGameParameters.LuminiscenceAdjustment = value; }
    float GetMinLuminiscenceAdjustment() const override { return GameParameters::MinLuminiscenceAdjustment; }
    float GetMaxLuminiscenceAdjustment() const override { return GameParameters::MaxLuminiscenceAdjustment; }

    float GetLightSpreadAdjustment() const override { return mGameParameters.LightSpreadAdjustment; }
    void SetLightSpreadAdjustment(float value) override { mGameParameters.LightSpreadAdjustment = value; }
    float GetMinLightSpreadAdjustment() const override { return GameParameters::MinLightSpreadAdjustment; }
    float GetMaxLightSpreadAdjustment() const override { return GameParameters::MaxLightSpreadAdjustment; }

    bool GetUltraViolentMode() const override { return mGameParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) override { mGameParameters.IsUltraViolentMode = value; }

    bool GetDoGenerateDebris() const override { return mGameParameters.DoGenerateDebris; }
    void SetDoGenerateDebris(bool value) override { mGameParameters.DoGenerateDebris = value; }

    bool GetDoGenerateSparkles() const override { return mGameParameters.DoGenerateSparkles; }
    void SetDoGenerateSparkles(bool value) override { mGameParameters.DoGenerateSparkles = value; }

    bool GetDoGenerateAirBubbles() const override { return mGameParameters.DoGenerateAirBubbles; }
    void SetDoGenerateAirBubbles(bool value) override { mGameParameters.DoGenerateAirBubbles = value; }

    float GetAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - mGameParameters.CumulatedIntakenWaterThresholdForAirBubbles; }
    void SetAirBubblesDensity(float value) override { mGameParameters.CumulatedIntakenWaterThresholdForAirBubbles = GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - value; }
    float GetMinAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles - GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles; }
    float GetMaxAirBubblesDensity() const override { return GameParameters::MaxCumulatedIntakenWaterThresholdForAirBubbles -  GameParameters::MinCumulatedIntakenWaterThresholdForAirBubbles; }

    size_t GetNumberOfStars() const override { return mGameParameters.NumberOfStars; }
    void SetNumberOfStars(size_t value) override { mGameParameters.NumberOfStars = value; }
    size_t GetMinNumberOfStars() const override { return GameParameters::MinNumberOfStars; }
    size_t GetMaxNumberOfStars() const override { return GameParameters::MaxNumberOfStars; }

    size_t GetNumberOfClouds() const override { return mGameParameters.NumberOfClouds; }
    void SetNumberOfClouds(size_t value) override { mGameParameters.NumberOfClouds = value; }
    size_t GetMinNumberOfClouds() const override { return GameParameters::MinNumberOfClouds; }
    size_t GetMaxNumberOfClouds() const override { return GameParameters::MaxNumberOfClouds; }

    //
    // Render parameters
    //

    rgbColor const & GetFlatSkyColor() const override { return mRenderContext->GetFlatSkyColor(); }
    void SetFlatSkyColor(rgbColor const & color) override { mRenderContext->SetFlatSkyColor(color); }

    float GetAmbientLightIntensity() const override { return mRenderContext->GetAmbientLightIntensity(); }
    void SetAmbientLightIntensity(float value) override { mRenderContext->SetAmbientLightIntensity(value); }

    float GetWaterContrast() const override { return mRenderContext->GetWaterContrast(); }
    void SetWaterContrast(float value) override { mRenderContext->SetWaterContrast(value); }

    float GetOceanTransparency() const override { return mRenderContext->GetOceanTransparency(); }
    void SetOceanTransparency(float value) override { mRenderContext->SetOceanTransparency(value); }

    float GetOceanDarkeningRate() const override { return mRenderContext->GetOceanDarkeningRate(); }
    void SetOceanDarkeningRate(float value) override { mRenderContext->SetOceanDarkeningRate(value); }

    bool GetShowShipThroughOcean() const override { return mRenderContext->GetShowShipThroughOcean(); }
    void SetShowShipThroughOcean(bool value) override { mRenderContext->SetShowShipThroughOcean(value); }

    float GetWaterLevelOfDetail() const override { return mRenderContext->GetWaterLevelOfDetail(); }
    void SetWaterLevelOfDetail(float value) override { mRenderContext->SetWaterLevelOfDetail(value); }
    float GetMinWaterLevelOfDetail() const override { return Render::RenderContext::MinWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const override { return Render::RenderContext::MaxWaterLevelOfDetail; }

    ShipRenderMode GetShipRenderMode() const override { return mRenderContext->GetShipRenderMode(); }
    void SetShipRenderMode(ShipRenderMode shipRenderMode) override { mRenderContext->SetShipRenderMode(shipRenderMode); }

    DebugShipRenderMode GetDebugShipRenderMode() const override { return mRenderContext->GetDebugShipRenderMode(); }
    void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode) override { mRenderContext->SetDebugShipRenderMode(debugShipRenderMode); }

    OceanRenderMode GetOceanRenderMode() const override { return mRenderContext->GetOceanRenderMode(); }
    void SetOceanRenderMode(OceanRenderMode oceanRenderMode) override { mRenderContext->SetOceanRenderMode(oceanRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const override { return mRenderContext->GetTextureOceanAvailableThumbnails(); }
    size_t GetTextureOceanTextureIndex() const override { return mRenderContext->GetTextureOceanTextureIndex(); }
    void SetTextureOceanTextureIndex(size_t index) override { mRenderContext->SetTextureOceanTextureIndex(index); }

    rgbColor const & GetDepthOceanColorStart() const override { return mRenderContext->GetDepthOceanColorStart(); }
    void SetDepthOceanColorStart(rgbColor const & color) override { mRenderContext->SetDepthOceanColorStart(color); }

    rgbColor const & GetDepthOceanColorEnd() const override { return mRenderContext->GetDepthOceanColorEnd(); }
    void SetDepthOceanColorEnd(rgbColor const & color) override { mRenderContext->SetDepthOceanColorEnd(color); }

    rgbColor const & GetFlatOceanColor() const override { return mRenderContext->GetFlatOceanColor(); }
    void SetFlatOceanColor(rgbColor const & color) override { mRenderContext->SetFlatOceanColor(color); }

    LandRenderMode GetLandRenderMode() const override { return mRenderContext->GetLandRenderMode(); }
    void SetLandRenderMode(LandRenderMode landRenderMode) override { mRenderContext->SetLandRenderMode(landRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const override { return mRenderContext->GetTextureLandAvailableThumbnails(); }
    size_t GetTextureLandTextureIndex() const override { return mRenderContext->GetTextureLandTextureIndex(); }
    void SetTextureLandTextureIndex(size_t index) override { mRenderContext->SetTextureLandTextureIndex(index); }

    rgbColor const & GetFlatLandColor() const override { return mRenderContext->GetFlatLandColor(); }
    void SetFlatLandColor(rgbColor const & color) override { mRenderContext->SetFlatLandColor(color); }

    VectorFieldRenderMode GetVectorFieldRenderMode() const override { return mRenderContext->GetVectorFieldRenderMode(); }
    void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) override { mRenderContext->SetVectorFieldRenderMode(VectorFieldRenderMode); }

    bool GetShowShipStress() const override { return mRenderContext->GetShowStressedSprings(); }
    void SetShowShipStress(bool value) override { mRenderContext->SetShowStressedSprings(value); }

    bool GetDrawHeatOverlay() const override { return mRenderContext->GetDrawHeatOverlay(); }
    void SetDrawHeatOverlay(bool value) override { mRenderContext->SetDrawHeatOverlay(value); }

    ShipFlameRenderMode GetShipFlameRenderMode() const override { return mRenderContext->GetShipFlameRenderMode(); }
    void SetShipFlameRenderMode(ShipFlameRenderMode shipFlameRenderMode) override { mRenderContext->SetShipFlameRenderMode(shipFlameRenderMode); }

    float GetShipFlameSizeAdjustment() const override { return mRenderContext->GetShipFlameSizeAdjustment(); }
    void SetShipFlameSizeAdjustment(float value) override { mRenderContext->SetShipFlameSizeAdjustment(value); }
    float GetMinShipFlameSizeAdjustment() const override { return Render::RenderContext::MinShipFlameSizeAdjustment; }
    float GetMaxShipFlameSizeAdjustment() const override { return Render::RenderContext::MaxShipFlameSizeAdjustment; }

    //
    // Interaction parameters
    //

    bool GetShowTsunamiNotifications() const override { return mShowTsunamiNotifications; }
    void SetShowTsunamiNotifications(bool value) override { mShowTsunamiNotifications = value; }

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::function<void()> swapRenderBuffersFunction,
        std::unique_ptr<GameEventDispatcher> gameEventDispatcher,
        std::unique_ptr<StatusText> statusText,
        MaterialDatabase materialDatabase,
        std::shared_ptr<ResourceLoader> resourceLoader);

    virtual void OnTsunami(float x) override;

    void InternalUpdate();

    void InternalRender();

    static void SmoothToTarget(
        float & currentValue,
        float startingValue,
        float targetValue,
        std::chrono::steady_clock::time_point startingTime);

    void Reset(std::unique_ptr<Physics::World> newWorld);

    void OnShipAdded(
        ShipDefinition shipDefinition,
        std::filesystem::path const & shipDefinitionFilepath,
        ShipId shipId);

    void PublishStats(std::chrono::steady_clock::time_point nowReal);

private:

    //
    // Our current state
    //

    GameParameters mGameParameters;
    std::filesystem::path mLastShipLoadedFilepath;
    bool mIsPaused;
    bool mIsMoveToolEngaged;

    // When set, will be uploaded to the RenderContext to display the flame thrower
    std::optional<std::tuple<vec2f, float>> mFlameThrowerToRender;

    class TsunamiNotificationStateMachine
    {
    public:

        TsunamiNotificationStateMachine(std::shared_ptr<Render::RenderContext> renderContext);

        ~TsunamiNotificationStateMachine();

        /*
         * When returns false, the state machine is over.
         */
        bool Update();

    private:

        std::shared_ptr<Render::RenderContext> mRenderContext;
        RenderedTextHandle mTextHandle;

        enum class StateType
        {
            RumblingFadeIn,
            Rumbling1,
            WarningFadeIn,
            Warning,
            WarningFadeOut,
            Rumbling2,
            RumblingFadeOut
        };

        StateType mCurrentState;
        float mCurrentStateStartTime;
    };

    std::optional<TsunamiNotificationStateMachine> mTsunamiNotificationStateMachine;

    //
    // The parameters that we own
    //

    bool mShowTsunamiNotifications;


    //
    // The doers
    //

    std::shared_ptr<Render::RenderContext> mRenderContext;
    std::function<void()> const mSwapRenderBuffersFunction;
    std::shared_ptr<GameEventDispatcher> mGameEventDispatcher;
    std::shared_ptr<ResourceLoader> mResourceLoader;
    std::shared_ptr<StatusText> mStatusText;


    //
    // The world
    //

    std::unique_ptr<Physics::World> mWorld;
    MaterialDatabase mMaterialDatabase;


    //
    // The current render parameters that we're smoothing to
    //

    static constexpr int SmoothMillis = 500;

    float mCurrentZoom;
    float mTargetZoom;
    float mStartingZoom;
    std::chrono::steady_clock::time_point mStartZoomTimestamp;

    vec2f mCurrentCameraPosition;
    vec2f mTargetCameraPosition;
    vec2f mStartingCameraPosition;
    std::chrono::steady_clock::time_point mStartCameraPositionTimestamp;


    //
    // Parameter smoothing
    //

    /*
     * All reads and writes of the parameters managed by a smoother go through the smoother.
     *
     * An underlying assumption is that the target value communicated to the smoother is the actual final parameter
     * value that will be enforced - in other words, no clipping occurs.
     */
    class ParameterSmoother
    {
    public:

        ParameterSmoother(
            std::function<float()> getter,
            std::function<void(float)> setter,
            std::chrono::milliseconds trajectoryTime)
            : mGetter(std::move(getter))
            , mSetter(std::move(setter))
            , mTrajectoryTime(trajectoryTime)
        {
            mStartValue = mTargetValue = mCurrentValue = mGetter();
            mCurrentTimestamp = mEndTimestamp = GameWallClock::GetInstance().Now();
        }

        float GetValue() const
        {
            return mCurrentValue;
        }

        void SetValue(float value)
        {
            mStartValue = mCurrentValue;
            mTargetValue = value;

            mCurrentTimestamp = GameWallClock::GetInstance().Now();
            mEndTimestamp =
                mCurrentTimestamp
                + mTrajectoryTime
                + std::chrono::milliseconds(1); // Just to make sure we do an update
        }

        void Update(GameWallClock::time_point now)
        {
            if (mCurrentTimestamp < mEndTimestamp)
            {
                // Advance

                mCurrentTimestamp = std::min(now, mEndTimestamp);

                float const leftFraction = (mTrajectoryTime == std::chrono::milliseconds::zero())
                    ? 0.0f
                    : static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(mEndTimestamp - mCurrentTimestamp).count())
                      / static_cast<float>(mTrajectoryTime.count());

                // We want the sinusoidal to be between Pi/4 and Pi/2;
                //  beginning of trajectory => leftFraction = 1.0 => phase = Pi/4
                //  end of trajectory => leftFraction = 0.0 => phase = Pi/2
                float const phase =
                    Pi<float> / 4.0f
                    + Pi<float> / 4.0f * (1.0f - leftFraction);

                // We want the value of the sinusoidal to be:
                //  beginning of trajectory => phase= Pi/4 => progress = 0
                //  end of trajectory => phase= Pi/2 => progress = 1
                float const progress =
                    (sin(phase) - sin(Pi<float> / 4.0f))
                    / (1.0f - sin(Pi<float> / 4.0f));

                mCurrentValue =
                    mStartValue
                    + (mTargetValue - mStartValue) * progress;

                mSetter(mCurrentValue);
            }
        }

    private:

        std::function<float()> const mGetter;
        std::function<void(float)> const mSetter;
        std::chrono::milliseconds mTrajectoryTime;

        float mStartValue;
        float mTargetValue;
        float mCurrentValue;
        GameWallClock::time_point mCurrentTimestamp;
        GameWallClock::time_point mEndTimestamp;
    };

    static constexpr size_t SpringStiffnessAdjustmentParameterSmoother = 0;
    static constexpr size_t SpringStrengthAdjustmentParameterSmoother = 1;
    static constexpr size_t SeaDepthParameterSmoother = 2;
    static constexpr size_t OceanFloorBumpinessParameterSmoother = 3;
    static constexpr size_t OceanFloorDetailAmplificationParameterSmoother = 4;

    std::vector<ParameterSmoother> mParameterSmoothers;

    //
    // Stats
    //

    uint64_t mTotalFrameCount;
    uint64_t mLastFrameCount;
    std::chrono::steady_clock::time_point mRenderStatsOriginTimestampReal;
    std::chrono::steady_clock::time_point mRenderStatsLastTimestampReal;
    std::chrono::steady_clock::duration mTotalUpdateDuration;
    std::chrono::steady_clock::duration mLastTotalUpdateDuration;
    std::chrono::steady_clock::duration mTotalRenderDuration;
    std::chrono::steady_clock::duration mLastTotalRenderDuration;
    GameWallClock::time_point mOriginTimestampGame;
    int mSkippedFirstStatPublishes;
};
