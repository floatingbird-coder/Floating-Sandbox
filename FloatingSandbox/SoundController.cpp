/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundController.h"

#include <Game/Materials.h>

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <regex>

float constexpr SinkingMusicVolume = 80.0f;
float constexpr RepairVolume = 40.0f;
float constexpr SawVolume = 50.0f;
float constexpr SawedVolume = 80.0f;
float constexpr StressSoundVolume = 20.0f;
std::chrono::milliseconds constexpr SawedInertiaDuration = std::chrono::milliseconds(200);
float constexpr WaveSplashTriggerSize = 0.5f;

SoundController::SoundController(
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
    : mResourceLoader(std::move(resourceLoader))
    // State
    , mMasterEffectsVolume(100.0f)
    , mMasterEffectsMuted(false)
    , mMasterToolsVolume(100.0f)
    , mMasterToolsMuted(false)
    , mMasterMusicVolume(100.0f)
    , mMasterMusicMuted(false)
    , mPlayBreakSounds(true)
    , mPlayStressSounds(true)
    , mPlayWindSound(true)
    , mPlaySinkingMusic(true)
    , mLastWaterSplashed(0.0f)
    , mCurrentWaterSplashedTrigger(WaveSplashTriggerSize)
    , mLastWindSpeedAbsoluteMagnitude(0.0f)
    , mWindVolumeRunningAverage()
    // One-shot sounds
    , mMSUOneShotMultipleChoiceSounds()
    , mDslUOneShotMultipleChoiceSounds()
    , mUOneShotMultipleChoiceSounds()
    , mOneShotMultipleChoiceSounds()
    , mCurrentlyPlayingOneShotSounds()
    // Continuous sounds
    , mSawedMetalSound(SawedInertiaDuration)
    , mSawedWoodSound(SawedInertiaDuration)
    , mSawAbovewaterSound()
    , mSawUnderwaterSound()
    , mFlameThrowerSound()
    , mDrawSound()
    , mSwirlSound()
    , mAirBubblesSound()
    , mFloodHoseSound()
    , mRepairStructureSound()
    , mWaveMakerSound()
    , mWaterRushSound()
    , mWaterSplashSound()
    , mWindSound()
    , mTimerBombSlowFuseSound()
    , mTimerBombFastFuseSound()
    , mAntiMatterBombContainedSounds()
    // Music
    , mSinkingMusic(
        SinkingMusicVolume,
        mMasterMusicVolume,
        mMasterMusicMuted,
        std::chrono::seconds::zero(),
        std::chrono::seconds(4))
{
    //
    // Initialize Music
    //

    auto musicNames = mResourceLoader->GetMusicNames();

    for (size_t i = 0; i < musicNames.size(); ++i)
    {
        std::string const & musicName = musicNames[i];

        //
        // Parse filename
        //

        static std::regex const MusicNameRegex(R"((.+)(?:_\d+)?)");
        std::smatch musicNameMatch;
        if (!std::regex_match(musicName, musicNameMatch, MusicNameRegex))
        {
            throw GameException("Music filename \"" + musicName + "\" is not recognized");
        }

        assert(musicNameMatch.size() == 1 + 1);

        mSinkingMusic.AddAlternative(mResourceLoader->GetMusicFilepath(musicName));
    }


    //
    // Initialize Sounds
    //

    auto soundNames = mResourceLoader->GetSoundNames();

    for (size_t i = 0; i < soundNames.size(); ++i)
    {
        std::string const & soundName = soundNames[i];

        // Notify progress
        progressCallback(static_cast<float>(i + 1) / static_cast<float>(soundNames.size()), "Loading sounds...");


        //
        // Load sound buffer
        //

        std::unique_ptr<sf::SoundBuffer> soundBuffer = std::make_unique<sf::SoundBuffer>();
        if (!soundBuffer->loadFromFile(mResourceLoader->GetSoundFilepath(soundName).string()))
        {
            throw GameException("Cannot load sound \"" + soundName + "\"");
        }



        //
        // Parse filename
        //

        std::regex soundTypeRegex(R"(([^_]+)(?:_.+)?)");
        std::smatch soundTypeMatch;
        if (!std::regex_match(soundName, soundTypeMatch, soundTypeRegex))
        {
            throw GameException("Sound filename \"" + soundName + "\" is not recognized");
        }

        assert(soundTypeMatch.size() == 1 + 1);
        SoundType soundType = StrToSoundType(soundTypeMatch[1].str());
        if (soundType == SoundType::Saw)
        {
            std::regex sawRegex(R"(([^_]+)(?:_(underwater))?)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, sawRegex))
            {
                throw GameException("Saw sound filename \"" + soundName + "\" is not recognized");
            }

            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                mSawUnderwaterSound.Initialize(
                    std::move(soundBuffer),
                    SawVolume,
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
            else
            {
                mSawAbovewaterSound.Initialize(
                    std::move(soundBuffer),
                    SawVolume,
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
        }
        else if (soundType == SoundType::Draw)
        {
            mDrawSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::Sawed)
        {
            std::regex mRegex(R"(([^_]+)_([^_]+))");
            std::smatch mMatch;
            if (!std::regex_match(soundName, mMatch, mRegex))
            {
                throw GameException("M sound filename \"" + soundName + "\" is not recognized");
            }

            assert(mMatch.size() == 1 + 2);

            // Parse SoundElementType
            StructuralMaterial::MaterialSoundType materialSound = StructuralMaterial::StrToMaterialSoundType(mMatch[2].str());

            if (StructuralMaterial::MaterialSoundType::Metal == materialSound)
            {
                mSawedMetalSound.Initialize(
                    std::move(soundBuffer),
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }
            else
            {
                mSawedWoodSound.Initialize(
                    std::move(soundBuffer),
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }
        }
        else if (soundType == SoundType::FlameThrower)
        {
            mFlameThrowerSound.Initialize(
                std::move(soundBuffer),
                60.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::Swirl)
        {
            mSwirlSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::AirBubbles)
        {
            mAirBubblesSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::FloodHose)
        {
            mFloodHoseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::RepairStructure)
        {
            mRepairStructureSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::WaveMaker)
        {
            mWaveMakerSound.Initialize(
                std::move(soundBuffer),
                40.0f,
                mMasterToolsVolume,
                mMasterToolsMuted,
                std::chrono::milliseconds(2500),
                std::chrono::milliseconds(5000));
        }
        else if (soundType == SoundType::WaterRush)
        {
            mWaterRushSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::WaterSplash)
        {
            mWaterSplashSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::Wind)
        {
            mWindSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::TimerBombSlowFuse)
        {
            mTimerBombSlowFuseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::TimerBombFastFuse)
        {
            mTimerBombFastFuseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::Break || soundType == SoundType::Destroy || soundType == SoundType::Stress
                || soundType == SoundType::RepairSpring || soundType == SoundType::RepairTriangle)
        {
            //
            // MSU sound
            //

            std::regex msuRegex(R"(([^_]+)_([^_]+)_([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch msuMatch;
            if (!std::regex_match(soundName, msuMatch, msuRegex))
            {
                throw GameException("MSU sound filename \"" + soundName + "\" is not recognized");
            }

            assert(msuMatch.size() == 1 + 4);

            // 1. Parse MaterialSoundType
            StructuralMaterial::MaterialSoundType materialSound = StructuralMaterial::StrToMaterialSoundType(msuMatch[2].str());

            // 2. Parse Size
            SizeType sizeType = StrToSizeType(msuMatch[3].str());

            // 3. Parse Underwater
            bool isUnderwater;
            if (msuMatch[4].matched)
            {
                assert(msuMatch[4].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mMSUOneShotMultipleChoiceSounds[std::make_tuple(soundType, materialSound, sizeType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::LightFlicker)
        {
            //
            // DslU sound
            //

            std::regex dsluRegex(R"(([^_]+)_([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch dsluMatch;
            if (!std::regex_match(soundName, dsluMatch, dsluRegex))
            {
                throw GameException("DslU sound filename \"" + soundName + "\" is not recognized");
            }

            assert(dsluMatch.size() >= 1 + 2 && dsluMatch.size() <= 1 + 3);

            // 1. Parse Duration
            DurationShortLongType durationType = StrToDurationShortLongType(dsluMatch[2].str());

            // 2. Parse Underwater
            bool isUnderwater;
            if (dsluMatch[3].matched)
            {
                assert(dsluMatch[3].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mDslUOneShotMultipleChoiceSounds[std::make_tuple(soundType, durationType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::Wave
                || soundType == SoundType::WindGust
                || soundType == SoundType::TsunamiTriggered
                || soundType == SoundType::AntiMatterBombPreImplosion
                || soundType == SoundType::AntiMatterBombImplosion
                || soundType == SoundType::Snapshot
                || soundType == SoundType::TerrainAdjust
                || soundType == SoundType::Scrub)
        {
            //
            // - one-shot sound
            //

            std::regex sRegex(R"(([^_]+)_\d+)");
            std::smatch sMatch;
            if (!std::regex_match(soundName, sMatch, sRegex))
            {
                throw GameException("- sound filename \"" + soundName + "\" is not recognized");
            }

            assert(sMatch.size() == 1 + 1);

            //
            // Store sound buffer
            //

            mOneShotMultipleChoiceSounds[std::make_tuple(soundType)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::AntiMatterBombContained)
        {
            //
            // - continuous sound
            //

            std::regex sRegex(R"(([^_]+)_\d+)");
            std::smatch sMatch;
            if (!std::regex_match(soundName, sMatch, sRegex))
            {
                throw GameException("- sound filename \"" + soundName + "\" is not recognized");
            }

            assert(sMatch.size() == 1 + 1);

            //
            // Initialize continuous sound
            //

            mAntiMatterBombContainedSounds.AddAlternative(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else
        {
            //
            // U sound
            //

            std::regex uRegex(R"(([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, uRegex))
            {
                throw GameException("U sound filename \"" + soundName + "\" is not recognized");
            }

            assert(uMatch.size() == 1 + 2);

            // 1. Parse Underwater
            bool isUnderwater;
            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mUOneShotMultipleChoiceSounds[std::make_tuple(soundType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
    }
}

SoundController::~SoundController()
{
    Reset();
}

void SoundController::SetPaused(bool isPaused)
{
    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        for (auto & playingSound : playingSoundIt.second)
        {
            if (isPaused)
            {
                playingSound.Sound->pause();
            }
            else
            {
                playingSound.Sound->resume();
            }
        }
    }

    // We don't pause the sounds of those continuous tools that keep "working" while paused;
    // we only pause the sounds of those that stop functioning
    mWaveMakerSound.SetPaused(isPaused);

    mWaterRushSound.SetPaused(isPaused);
    mWaterSplashSound.SetPaused(isPaused);
    mWindSound.SetPaused(isPaused);
    mTimerBombSlowFuseSound.SetPaused(isPaused);
    mTimerBombFastFuseSound.SetPaused(isPaused);
    mAntiMatterBombContainedSounds.SetPaused(isPaused);

    // Sinking music
    if (isPaused)
    {
        if (sf::Sound::Status::Playing == mSinkingMusic.GetStatus())
            mSinkingMusic.Pause();
    }
    else
    {
        if (sf::Sound::Status::Paused == mSinkingMusic.GetStatus())
            mSinkingMusic.Resume();
    }
}

void SoundController::SetMuted(bool isMuted)
{
    sf::Listener::setGlobalVolume(isMuted ? 0.0f : 100.0f);
}

// Master effects

void SoundController::SetMasterEffectsVolume(float volume)
{
    mMasterEffectsVolume = volume;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first != SoundType::Draw
            && playingSoundIt.first != SoundType::Saw
            && playingSoundIt.first != SoundType::FlameThrower
            && playingSoundIt.first != SoundType::Swirl
            && playingSoundIt.first != SoundType::AirBubbles
            && playingSoundIt.first != SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMasterVolume(mMasterEffectsVolume);
            }
        }
    }

    mSawedMetalSound.SetMasterVolume(mMasterEffectsVolume);
    mSawedWoodSound.SetMasterVolume(mMasterEffectsVolume);

    mWaterRushSound.SetMasterVolume(mMasterEffectsVolume);
    mWaterSplashSound.SetMasterVolume(mMasterEffectsVolume);
    mWindSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombSlowFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombFastFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mAntiMatterBombContainedSounds.SetMasterVolume(mMasterEffectsVolume);
}

void SoundController::SetMasterEffectsMuted(bool isMuted)
{
    mMasterEffectsMuted = isMuted;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first != SoundType::Draw
            && playingSoundIt.first != SoundType::Saw
            && playingSoundIt.first != SoundType::FlameThrower
            && playingSoundIt.first != SoundType::Swirl
            && playingSoundIt.first != SoundType::AirBubbles
            && playingSoundIt.first != SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMuted(mMasterEffectsMuted);
            }
        }
    }

    mSawedMetalSound.SetMuted(mMasterEffectsMuted);
    mSawedWoodSound.SetMuted(mMasterEffectsMuted);

    mWaterRushSound.SetMuted(mMasterEffectsMuted);
    mWaterSplashSound.SetMuted(mMasterEffectsMuted);
    mWindSound.SetMuted(mMasterEffectsMuted);
    mTimerBombSlowFuseSound.SetMuted(mMasterEffectsMuted);
    mTimerBombFastFuseSound.SetMuted(mMasterEffectsMuted);
    mAntiMatterBombContainedSounds.SetMuted(mMasterEffectsMuted);
}

// Master tools

void SoundController::SetMasterToolsVolume(float volume)
{
    mMasterToolsVolume = volume;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first == SoundType::Draw
            || playingSoundIt.first == SoundType::Saw
            || playingSoundIt.first == SoundType::FlameThrower
            || playingSoundIt.first == SoundType::Swirl
            || playingSoundIt.first == SoundType::AirBubbles
            || playingSoundIt.first == SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMasterVolume(mMasterToolsVolume);
            }
        }
    }

    mSawAbovewaterSound.SetMasterVolume(mMasterToolsVolume);
    mSawUnderwaterSound.SetMasterVolume(mMasterToolsVolume);
    mFlameThrowerSound.SetMasterVolume(mMasterToolsVolume);
    mDrawSound.SetMasterVolume(mMasterToolsVolume);
    mSwirlSound.SetMasterVolume(mMasterToolsVolume);
    mAirBubblesSound.SetMasterVolume(mMasterToolsVolume);
    mFloodHoseSound.SetMasterVolume(mMasterToolsVolume);
    mRepairStructureSound.SetMasterVolume(mMasterToolsVolume);
    mWaveMakerSound.SetMasterVolume(mMasterToolsVolume);
}

void SoundController::SetMasterToolsMuted(bool isMuted)
{
    mMasterToolsMuted = isMuted;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first == SoundType::Draw
            || playingSoundIt.first == SoundType::Saw
            || playingSoundIt.first == SoundType::FlameThrower
            || playingSoundIt.first == SoundType::Swirl
            || playingSoundIt.first == SoundType::AirBubbles
            || playingSoundIt.first == SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMuted(mMasterToolsMuted);
            }
        }
    }

    mSawAbovewaterSound.SetMuted(mMasterToolsMuted);
    mSawUnderwaterSound.SetMuted(mMasterToolsMuted);
    mFlameThrowerSound.SetMuted(mMasterToolsMuted);
    mDrawSound.SetMuted(mMasterToolsMuted);
    mSwirlSound.SetMuted(mMasterToolsMuted);
    mAirBubblesSound.SetMuted(mMasterToolsMuted);
    mFloodHoseSound.SetMuted(mMasterToolsMuted);
    mRepairStructureSound.SetMuted(mMasterToolsMuted);
    mWaveMakerSound.SetMuted(mMasterToolsMuted);
}

// Master music

void SoundController::SetMasterMusicVolume(float volume)
{
    mMasterMusicVolume = volume;

    mSinkingMusic.SetMasterVolume(volume);
}

void SoundController::SetMasterMusicMuted(bool isMuted)
{
    mMasterMusicMuted = isMuted;

    mSinkingMusic.SetMuted(mMasterMusicMuted);
}

void SoundController::SetPlayBreakSounds(bool playBreakSounds)
{
    mPlayBreakSounds = playBreakSounds;

    if (!mPlayBreakSounds)
    {
        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::Break == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
}

void SoundController::SetPlayStressSounds(bool playStressSounds)
{
    mPlayStressSounds = playStressSounds;

    if (!mPlayStressSounds)
    {
        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::Stress == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
}

void SoundController::SetPlayWindSound(bool playWindSound)
{
    mPlayWindSound = playWindSound;

    if (!mPlayWindSound)
    {
        mWindSound.SetMuted(true);

        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::WindGust == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
    else
    {
        mWindSound.SetMuted(false);
    }
}

void SoundController::SetPlaySinkingMusic(bool playSinkingMusic)
{
    mPlaySinkingMusic = playSinkingMusic;

    if (!mPlaySinkingMusic)
    {
        mSinkingMusic.Stop();
    }
}

// Misc

void SoundController::PlayDrawSound(bool /*isUnderwater*/)
{
    // At the moment we ignore the water-ness
    mDrawSound.Start();
}

void SoundController::StopDrawSound()
{
    mDrawSound.Stop();
}

void SoundController::PlaySawSound(bool isUnderwater)
{
    if (isUnderwater)
    {
        mSawUnderwaterSound.Start();
        mSawAbovewaterSound.Stop();
    }
    else
    {
        mSawAbovewaterSound.Start();
        mSawUnderwaterSound.Stop();
    }

    mSawedMetalSound.Start();
    mSawedWoodSound.Start();
}

void SoundController::StopSawSound()
{
    mSawedMetalSound.Stop();
    mSawedWoodSound.Stop();

    mSawAbovewaterSound.Stop();
    mSawUnderwaterSound.Stop();
}

void SoundController::PlayFlameThrowerSound()
{
    mFlameThrowerSound.Start();
}

void SoundController::StopFlameThrowerSound()
{
    mFlameThrowerSound.Stop();
}

void SoundController::PlaySwirlSound(bool /*isUnderwater*/)
{
    // At the moment we ignore the water-ness
    mSwirlSound.Start();
}

void SoundController::StopSwirlSound()
{
    mSwirlSound.Stop();
}

void SoundController::PlayAirBubblesSound()
{
    mAirBubblesSound.Start();
}

void SoundController::StopAirBubblesSound()
{
    mAirBubblesSound.Stop();
}

void SoundController::PlayFloodHoseSound()
{
    mFloodHoseSound.Start();
}

void SoundController::StopFloodHoseSound()
{
    mFloodHoseSound.Stop();
}

void SoundController::PlayTerrainAdjustSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::TerrainAdjust,
        100.0f,
        true);
}

void SoundController::PlayRepairStructureSound()
{
    mRepairStructureSound.Start();
}

void SoundController::StopRepairStructureSound()
{
    mRepairStructureSound.Stop();
}

void SoundController::PlayWaveMakerSound()
{
    mWaveMakerSound.FadeIn();
}

void SoundController::StopWaveMakerSound()
{
    mWaveMakerSound.FadeOut();
}

void SoundController::PlayScrubSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Scrub,
        100.0f,
        true);
}

void SoundController::PlaySnapshotSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Snapshot,
        100.0f,
        true);
}

void SoundController::Update()
{
    mWaveMakerSound.Update();
    mSinkingMusic.Update();

    // Silence the inertial sounds - this will basically be a nop in case
    // they've just been started or will be started really soon
    mSawedMetalSound.SetVolume(0.0f);
    mSawedWoodSound.SetVolume(0.0f);
}

void SoundController::LowFrequencyUpdate()
{
}

void SoundController::Reset()
{
    //
    // Stop and clear all sounds
    //

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        for (auto & playingSound : playingSoundIt.second)
        {
            assert(!!playingSound.Sound);
            if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
            {
                playingSound.Sound->stop();
            }
        }
    }

    mCurrentlyPlayingOneShotSounds.clear();

    mSawedMetalSound.Reset();
    mSawedWoodSound.Reset();
    mSawAbovewaterSound.Reset();
    mSawUnderwaterSound.Reset();
    mFlameThrowerSound.Reset();
    mDrawSound.Reset();
    mSwirlSound.Reset();
    mAirBubblesSound.Reset();
    mFloodHoseSound.Reset();
    mRepairStructureSound.Reset();
    mWaveMakerSound.Reset();

    mWaterRushSound.Reset();
    mWaterSplashSound.Reset();
    mWindSound.Reset();
    mTimerBombSlowFuseSound.Reset();
    mTimerBombFastFuseSound.Reset();
    mAntiMatterBombContainedSounds.Reset();

    //
    // Reset music
    //

    mSinkingMusic.Reset();

    //
    // Reset state
    //

    mLastWaterSplashed = 0.0f;
    mCurrentWaterSplashedTrigger = WaveSplashTriggerSize;
    mLastWindSpeedAbsoluteMagnitude = 0.0f;
    mWindVolumeRunningAverage.Reset();
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::OnDestroy(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Destroy,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            70.0f,
            true);
    }
}

void SoundController::OnSpringRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::RepairSpring,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            RepairVolume,
            true);
    }
}

void SoundController::OnTriangleRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::RepairTriangle,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            RepairVolume,
            true);
    }
}

void SoundController::OnSawed(
    bool isMetal,
    unsigned int size)
{
    if (isMetal)
        mSawedMetalSound.SetVolume(size > 0 ? SawedVolume : 0.0f);
    else
        mSawedWoodSound.SetVolume(size > 0 ? SawedVolume : 0.0f);
}

void SoundController::OnPinToggled(
    bool isPinned,
    bool isUnderwater)
{
    PlayUOneShotMultipleChoiceSound(
        isPinned ? SoundType::PinPoint : SoundType::UnpinPoint,
        isUnderwater,
        100.0f,
        true);
}

void SoundController::OnSinkingBegin(ShipId /*shipId*/)
{
    if (mPlaySinkingMusic)
    {
        if (sf::SoundSource::Status::Playing != mSinkingMusic.GetStatus())
        {
            mSinkingMusic.Play();
        }
    }
}

void SoundController::OnSinkingEnd(ShipId /*shipId*/)
{
    if (sf::SoundSource::Status::Stopped != mSinkingMusic.GetStatus())
    {
        mSinkingMusic.FadeToStop();
    }
}

void SoundController::OnTsunamiNotification(float /*x*/)
{
    PlayOneShotMultipleChoiceSound(
        SoundType::TsunamiTriggered,
        100.0f,
        true);
}

void SoundController::OnStress(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (mPlayStressSounds
        && !!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Stress,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            StressSoundVolume,
            true);
    }
}

void SoundController::OnBreak(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (mPlayBreakSounds
        && !!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Break,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            10.0f,
            true);
    }
}

void SoundController::OnLightFlicker(
    DurationShortLongType duration,
    bool isUnderwater,
    unsigned int size)
{
    PlayDslUOneShotMultipleChoiceSound(
        SoundType::LightFlicker,
        duration,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnWaterTaken(float waterTaken)
{
    // 50 * (-1 / 2.4^(0.3 * x) + 1)
    float rushVolume = 40.f * (-1.f / std::pow(2.4f, std::min(90.0f, 0.3f * std::abs(waterTaken))) + 1.f);
    mWaterRushSound.SetVolume(rushVolume);
    mWaterRushSound.Start();
}

void SoundController::OnWaterSplashed(float waterSplashed)
{
    //
    // Trigger waves
    //

    if (waterSplashed > mLastWaterSplashed)
    {
        if (waterSplashed > mCurrentWaterSplashedTrigger)
        {
            // 12 * (-1 / 1.8^(0.08 * x) + 1)
            float waveVolume = 12.f * (-1.f / std::pow(1.8f, 0.08f * std::min(1800.0f, std::abs(waterSplashed))) + 1.f);

            PlayOneShotMultipleChoiceSound(
                SoundType::Wave,
                waveVolume,
                true);

            // Advance trigger
            mCurrentWaterSplashedTrigger = waterSplashed + WaveSplashTriggerSize;
        }
    }
    else
    {
        // Lower trigger
        mCurrentWaterSplashedTrigger = waterSplashed + WaveSplashTriggerSize;
    }

    mLastWaterSplashed = waterSplashed;


    //
    // Adjust continuous splash sound
    //

    // 12 * (-1 / 1.3^(0.01*x) + 1)
    float splashVolume = 12.f * (-1.f / std::pow(1.3f, 0.01f * std::abs(waterSplashed)) + 1.f);
    mWaterSplashSound.SetVolume(splashVolume);
    mWaterSplashSound.Start();
}

void SoundController::OnWindSpeedUpdated(
    float const /*zeroSpeedMagnitude*/,
    float const baseSpeedMagnitude,
    float const /*preMaxSpeedMagnitude*/,
    float const maxSpeedMagnitude,
    vec2f const & windSpeed)
{
    float const windSpeedAbsoluteMagnitude = windSpeed.length();

    //
    // 1. Calculate volume of continuous sound
    //

    float windVolume;
    if (windSpeedAbsoluteMagnitude >= abs(baseSpeedMagnitude))
    {
        // 20 -> 43:
        // 100 * (-1 / 1.1^(0.3 * x) + 1)
        windVolume = 100.f * (-1.f / std::pow(1.1f, 0.3f * (windSpeedAbsoluteMagnitude - abs(baseSpeedMagnitude))) + 1.f);
    }
    else
    {
        // Raise volume only if goes up
        float const deltaUp = std::max(0.0f, windSpeedAbsoluteMagnitude - mLastWindSpeedAbsoluteMagnitude);

        // 100 * (-1 / 1.1^(0.3 * x) + 1)
        windVolume = 100.f * (-1.f / std::pow(1.1f, 0.3f * deltaUp) + 1.f);
    }

    // Smooth the volume
    float const smoothedWindVolume = mWindVolumeRunningAverage.Update(windVolume);

    // Set the volume
    mWindSound.SetVolume(smoothedWindVolume);
    mWindSound.Start();


    //
    // 2. Decide if time to fire a gust
    //

    if (mPlayWindSound)
    {
        // Detect first arrival to max (gust) level
        if (windSpeedAbsoluteMagnitude > mLastWindSpeedAbsoluteMagnitude
            && abs(maxSpeedMagnitude) - windSpeedAbsoluteMagnitude < 0.001f)
        {
            PlayOneShotMultipleChoiceSound(
                SoundType::WindGust,
                smoothedWindVolume,
                true);
        }
    }

    mLastWindSpeedAbsoluteMagnitude = windSpeedAbsoluteMagnitude;
}

void SoundController::OnBombPlaced(
    BombId /*bombId*/,
    BombType /*bombType*/,
    bool isUnderwater)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::BombAttached,
        isUnderwater,
        100.0f,
        true);
}

void SoundController::OnBombRemoved(
    BombId /*bombId*/,
    BombType /*bombType*/,
    std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        PlayUOneShotMultipleChoiceSound(
            SoundType::BombDetached,
            *isUnderwater,
            100.0f,
            true);
    }
}

void SoundController::OnBombExplosion(
    BombType bombType,
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        BombType::AntiMatterBomb == bombType
            ? SoundType::AntiMatterBombExplosion
            : SoundType::BombExplosion,
        isUnderwater,
        std::max(
            100.0f,
            50.0f * size),
        true);
}

void SoundController::OnRCBombPing(
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::RCBombPing,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnTimerBombFuse(
    BombId bombId,
    std::optional<bool> isFast)
{
    if (!!isFast)
    {
        if (*isFast)
        {
            // Start fast

            // See if this bomb is emitting a slow fuse sound; if so, remove it
            // and update slow fuse sound
            mTimerBombSlowFuseSound.StopSoundForObject(bombId);

            // Start fast fuse sound
            mTimerBombFastFuseSound.StartSoundForObject(bombId);
        }
        else
        {
            // Start slow

            // See if this bomb is emitting a fast fuse sound; if so, remove it
            // and update fast fuse sound
            mTimerBombFastFuseSound.StopSoundForObject(bombId);

            // Start slow fuse sound
            mTimerBombSlowFuseSound.StartSoundForObject(bombId);
        }
    }
    else
    {
        // Stop the sound, whichever it is
        mTimerBombSlowFuseSound.StopSoundForObject(bombId);
        mTimerBombFastFuseSound.StopSoundForObject(bombId);
    }
}

void SoundController::OnTimerBombDefused(
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::TimerBombDefused,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnAntiMatterBombContained(
    BombId bombId,
    bool isContained)
{
    if (isContained)
    {
        // Start sound
        mAntiMatterBombContainedSounds.StartSoundAlternativeForObject(bombId);
    }
    else
    {
        // Stop sound
        mAntiMatterBombContainedSounds.StopSoundAlternativeForObject(bombId);
    }
}

void SoundController::OnAntiMatterBombPreImploding()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::AntiMatterBombPreImplosion,
        100.0f,
        true);
}

void SoundController::OnAntiMatterBombImploding()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::AntiMatterBombImplosion,
        100.0f,
        false);
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::PlayMSUOneShotMultipleChoiceSound(
    SoundType soundType,
    StructuralMaterial::MaterialSoundType materialSound,
    unsigned int size,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    // Convert size
    SizeType sizeType;
    if (size < 2)
        sizeType = SizeType::Small;
    else if (size < 10)
        sizeType = SizeType::Medium;
    else
        sizeType = SizeType::Large;

    LogDebug("MSUSound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(materialSound),
        ",",
        static_cast<int>(sizeType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, sizeType, isUnderwater));
    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // Find a smaller one
        for (int s = static_cast<int>(sizeType) - 1; s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, static_cast<SizeType>(s), isUnderwater));
            if (it != mMSUOneShotMultipleChoiceSounds.end())
            {
                break;
            }
        }
    }

    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // Find this or smaller size with different underwater
        for (int s = static_cast<int>(sizeType); s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, static_cast<SizeType>(s), !isUnderwater));
            if (it != mMSUOneShotMultipleChoiceSounds.end())
            {
                break;
            }
        }
    }

    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayDslUOneShotMultipleChoiceSound(
    SoundType soundType,
    DurationShortLongType duration,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    LogDebug("DslUSound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(duration),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mDslUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, duration, isUnderwater));
    if (it == mDslUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayUOneShotMultipleChoiceSound(
    SoundType soundType,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    LogDebug("USound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, isUnderwater));
    if (it == mUOneShotMultipleChoiceSounds.end())
    {
        // Find different underwater
        it = mUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, !isUnderwater));
    }

    if (it == mUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayOneShotMultipleChoiceSound(
    SoundType soundType,
    float volume,
    bool isInterruptible)
{
    LogDebug("Sound: <",
        static_cast<int>(soundType),
        ">");

    //
    // Find vector
    //

    auto it = mOneShotMultipleChoiceSounds.find(std::make_tuple(soundType));
    if (it == mOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::ChooseAndPlayOneShotMultipleChoiceSound(
    SoundType soundType,
    OneShotMultipleChoiceSound & sound,
    float volume,
    bool isInterruptible)
{
    //
    // Choose sound buffer
    //

    sf::SoundBuffer * chosenSoundBuffer = nullptr;

    assert(!sound.SoundBuffers.empty());
    if (1 == sound.SoundBuffers.size())
    {
        // Nothing to choose
        chosenSoundBuffer = sound.SoundBuffers[0].get();
    }
    else
    {
        assert(sound.SoundBuffers.size() >= 2);

        // Choose randomly, but avoid choosing the last-chosen sound again
        size_t chosenSoundIndex = GameRandomEngine::GetInstance().ChooseNew(
            sound.SoundBuffers.size(),
            sound.LastPlayedSoundIndex);

        chosenSoundBuffer = sound.SoundBuffers[chosenSoundIndex].get();

        sound.LastPlayedSoundIndex = chosenSoundIndex;
    }

    assert(nullptr != chosenSoundBuffer);

    PlayOneShotSound(
        soundType,
        chosenSoundBuffer,
        volume,
        isInterruptible);
}

void SoundController::PlayOneShotSound(
    SoundType soundType,
    sf::SoundBuffer * soundBuffer,
    float volume,
    bool isInterruptible)
{
    assert(nullptr != soundBuffer);

    //
    // Make sure there isn't already a sound with this sound buffer that started
    // playing too recently;
    // if there is, adjust its volume
    //

    auto & thisTypeCurrentlyPlayingSounds = mCurrentlyPlayingOneShotSounds[soundType];

    auto const now = GameWallClock::GetInstance().Now();
    auto const minDeltaTimeSoundForType = GetMinDeltaTimeSoundForType(soundType);

    for (auto & playingSound : thisTypeCurrentlyPlayingSounds)
    {
        assert(!!playingSound.Sound);
        if (playingSound.Sound->getBuffer() == soundBuffer
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - playingSound.StartedTimestamp) < minDeltaTimeSoundForType)
        {
            playingSound.Sound->addVolume(volume);
            return;
        }
    }


    //
    // Make sure there's room for this sound
    //

    auto const maxPlayingSoundsForThisType = GetMaxPlayingSoundsForType(soundType);

    if (thisTypeCurrentlyPlayingSounds.size() >= maxPlayingSoundsForThisType)
    {
        ScavengeStoppedSounds(thisTypeCurrentlyPlayingSounds);

        if (thisTypeCurrentlyPlayingSounds.size() >= maxPlayingSoundsForThisType)
        {
            // Need to stop the (expendable) sound that's been playing for the longest
            ScavengeOldestSound(thisTypeCurrentlyPlayingSounds);
        }
    }

    assert(thisTypeCurrentlyPlayingSounds.size() < maxPlayingSoundsForThisType);



    //
    // Create and play sound
    //

    std::unique_ptr<GameSound> sound = std::make_unique<GameSound>(
        *soundBuffer,
        volume,
        mMasterEffectsVolume,
        mMasterEffectsMuted);

    sound->play();

    thisTypeCurrentlyPlayingSounds.emplace_back(
        soundType,
        std::move(sound),
        now,
        isInterruptible);
}

void SoundController::ScavengeStoppedSounds(std::vector<PlayingSound> & playingSounds)
{
    for (auto it = playingSounds.begin(); it != playingSounds.end(); /*incremented in loop*/)
    {
        assert(!!it->Sound);
        if (sf::Sound::Status::Stopped == it->Sound->getStatus())
        {
            // Scavenge
            it = playingSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SoundController::ScavengeOldestSound(std::vector<PlayingSound> & playingSounds)
{
    assert(!playingSounds.empty());

    //
    // Two choices, in order of priority:
    // 1) Interruptible
    // 2) Non interruptible
    //

    size_t iOldestInterruptibleSound = std::numeric_limits<size_t>::max();
    auto oldestInterruptibleSoundStartTimestamp = GameWallClock::time_point::max();
    size_t iOldestNonInterruptibleSound = std::numeric_limits<size_t>::max();
    auto oldestNonInterruptibleSoundStartTimestamp = GameWallClock::time_point::max();
    for (size_t i = 0; i < playingSounds.size(); ++i)
    {
        if (playingSounds[i].StartedTimestamp < oldestNonInterruptibleSoundStartTimestamp)
        {
            iOldestNonInterruptibleSound = i;
            oldestNonInterruptibleSoundStartTimestamp = playingSounds[i].StartedTimestamp;
        }

        if (playingSounds[i].StartedTimestamp < oldestInterruptibleSoundStartTimestamp
            && playingSounds[i].IsInterruptible)
        {
            iOldestInterruptibleSound = i;
            oldestInterruptibleSoundStartTimestamp = playingSounds[i].StartedTimestamp;
        }
    }

    size_t iSoundToStop;
    if (oldestInterruptibleSoundStartTimestamp != GameWallClock::time_point::max())
    {
        iSoundToStop = iOldestInterruptibleSound;
    }
    else
    {
        assert((oldestNonInterruptibleSoundStartTimestamp != GameWallClock::time_point::max()));
        iSoundToStop = iOldestNonInterruptibleSound;
    }

    assert(!!playingSounds[iSoundToStop].Sound);
    playingSounds[iSoundToStop].Sound->stop();
    playingSounds.erase(playingSounds.begin() + iSoundToStop);
}