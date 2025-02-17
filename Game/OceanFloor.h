/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "GameParameters.h"
#include "ImageFileTools.h"
#include "ResourceLoader.h"

#include <GameCore/GameMath.h>

#include <memory>

namespace Physics
{

class OceanFloor
{
public:

    OceanFloor(ResourceLoader & resourceLoader);

    void Update(GameParameters const & gameParameters);

    void Upload(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    bool AdjustTo(
        float x1,
        float targetY1,
        float x2,
        float targetY2);

    float GetHeightAt(float x) const
    {
        //
        // Find sample index and interpolate in-between that sample and the next
        //

        // Fractional index in the sample array
        float const sampleIndexF = (x + GameParameters::HalfMaxWorldWidth) / Dx;

        // Integral part
        int64_t sampleIndexI = FastTruncateInt64(sampleIndexF);

        // Fractional part within sample index and the next sample index
        float sampleIndexDx = sampleIndexF - sampleIndexI;

        // FUTURE: the following checks are temporary; as long as we have multiple mechanical
        // iterations per step, each of the interim steps might exceed the world boundaries. The checks
        // might be removed once the mechanical simulation may guarantee that each position update
        // is followed by a world boundary trim
        if (sampleIndexI < 0)
            return mSamples[0].SampleValue;
        else if (sampleIndexI > SamplesCount)
            return mSamples[SamplesCount].SampleValue;
        else
        {
            assert(sampleIndexI >= 0 && sampleIndexI <= SamplesCount);
            assert(sampleIndexDx >= 0.0f && sampleIndexDx <= 1.0f);

            return mSamples[sampleIndexI].SampleValue
                + mSamples[sampleIndexI].SampleValuePlusOneMinusSampleValue * sampleIndexDx;
        }
    }

private:

    // The number of samples for the entire world width;
    // a higher value means more resolution, at the expense of cache misses
    static constexpr int64_t SamplesCount = 2048;

    // The x step of the samples
    static constexpr float Dx = GameParameters::MaxWorldWidth / static_cast<float>(SamplesCount);

    // What we store for each sample
    struct Sample
    {
        float SampleValue;
        float SampleValuePlusOneMinusSampleValue; // Delta w/next
    };

    // The current samples (plus 1 to account for x==MaxWorldWidth)
    std::unique_ptr<Sample[]> mSamples;

    // The bump map samples (plus 1 to account for x==MaxWorldWidth),
    // between -H/2 and H/2
    std::unique_ptr<float[]> const mBumpMapSamples;

    // The game parameters for which we're current
    float mCurrentSeaDepth;
    float mCurrentOceanFloorBumpiness;
    float mCurrentOceanFloorDetailAmplification;
};

}
