/****************************************************************************
 Copyright (c) 2014-2017 Chukong Technologies Inc.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#pragma once

#include <cocos/audio/include/Export.h>
#include <cocos/platform/CCPlatformConfig.h>
#include <cocos/platform/CCPlatformMacros.h>

#include <chrono>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>

#ifdef ERROR
#    undef ERROR
#endif // ERROR

/**
 * @addtogroup audio
 * @{
 */

NS_CC_BEGIN
namespace experimental
{
    /**
     * @class AudioProfile
     *
     * @brief
     * @js NA
     */
    class EXPORT_DLL AudioProfile
    {
    public:
        // Profile name can't be empty.
        std::string name;
        // The maximum number of simultaneous audio instance.
        unsigned int maxInstances;

        /* Minimum delay in between sounds */
        double minDelay;

        /**
         * Default constructor
         *
         * @lua new
         */
        AudioProfile()
        : maxInstances(0)
        , minDelay(0.0)
        {
        }
    };

    class AudioEngineImpl;

    /**
     * @class AudioEngine
     *
     * @brief Offers a interface to play audio.
     *
     * @note Make sure to call AudioEngine::end() when the audio engine is not needed anymore to release resources.
     * @js NA
     */

    class EXPORT_DLL AudioEngine
    {
    public:
        /** AudioState enum,all possible states of an audio instance.*/
        enum class AudioState
        {
            ERROR = -1,
            INITIALIZING,
            PLAYING,
            PAUSED
        };

        static const int INVALID_AUDIO_ID;

        static const std::chrono::milliseconds TIME_UNKNOWN;

        static bool lazyInit();

        /**
         * Release objects relating to AudioEngine.
         *
         * @warning It must be called before the application exit.
         * @lua endToLua
         */
        static void end();

        /**
         * Gets the default profile of audio instances.
         *
         * @return The default profile of audio instances.
         */
        static AudioProfile* getDefaultProfile();

        /**
         * Play 2d sound.
         *
         * @param filePath The path of an audio file.
         * @param loop Whether audio instance loop or not.
         * @param volume Volume value (range from 0.0 to 1.0).
         * @param profile A profile for audio instance. When profile is not specified, default profile will be used.
         * @return An audio ID. It allows you to dynamically change the behavior of an audio instance on the fly.
         *
         * @see `AudioProfile`
         */
        static int play2d(const std::string& filePath, bool loop = false, float volume = 1.0f, bool isMusic = false, const AudioProfile* profile = nullptr);

        /**
         * Sets whether an audio instance loop or not.
         *
         * @param audioID An audioID returned by the play2d function.
         * @param loop Whether audio instance loop or not.
         */
        static void setLoop(int audioID, bool loop);

        /**
         * Checks whether an audio instance is loop.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return Whether or not an audio instance is loop.
         */
        static bool isLoop(int audioID);

        /**
         * Sets volume for an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @param volume Volume value (range from 0.0 to 1.0).
         */
        static void setVolume(int audioID, float volume);

        /**
         * Gets the volume value of an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return Volume value (range from 0.0 to 1.0).
         */
        static float getVolume(int audioID);

        /**
         * Pause an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         */
        static void pause(int audioID);

        /** Pause all playing audio instances. */
        static void pauseAll();

        /**
         * Resume an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         */
        static void resume(int audioID);

        /** Resume all suspended audio instances. */
        static void resumeAll();

        /**
         * Stop an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         */
        static void stop(int audioID);

        /** Stop all audio instances. */
        static void stopAll();

        /**
         * Sets the current playback position of an audio instance.
         *
         * @param audioID   An audioID returned by the play2d function.
         * @param sec       The offset in seconds from the start to seek to.
         * @return
         */
        [[deprecated("Do not use until setCurrentTime() behavior is specified - see comments in PSAudioEngine.h")]] static bool
        setCurrentTime(int audioID, float sec);

        /**
         * Gets the current playback position of an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return The current playback position of an audio instance.
         */
        static float getCurrentTime(int audioID);

        /**
         * Gets the duration of an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return The duration of an audio instance.
         */
        static std::chrono::milliseconds getDuration(int audioID);

        /**
         * Returns the state of an audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return The status of an audio instance.
         */
        static AudioState getState(int audioID);

        /**
         * Register a callback to be invoked when an audio instance has completed playing.
         *
         * @param audioID An audioID returned by the play2d function.
         * @param callback
         */
        static void setFinishCallback(int audioID, const std::function<void(int, const std::string&)>& callback);

        /**
         * Gets the maximum number of simultaneous audio instance of AudioEngine.
         */
        static int getMaxAudioInstance() { return _maxInstances; }

        /**
         * Sets the maximum number of simultaneous audio instance for AudioEngine.
         *
         * @param maxInstances The maximum number of simultaneous audio instance.
         */
        static bool setMaxAudioInstance(int maxInstances);

        /**
         * Uncache the audio data from internal buffer.
         * AudioEngine cache audio data on ios,mac, and win32 platform.
         *
         * @warning This can lead to stop related audio first.
         * @param filePath Audio file path.
         */
        static void uncache(const std::string& filePath);

        /**
         * Uncache all audio data from internal buffer.
         *
         * @warning All audio will be stopped first.
         */
        [[deprecated("Do not use")]] static void uncacheAll();

        /**
         * Gets the audio profile by id of audio instance.
         *
         * @param audioID An audioID returned by the play2d function.
         * @return The audio profile.
         */
        static AudioProfile* getProfile(int audioID);

        /**
         * Gets an audio profile by name.
         *
         * @param profileName A name of audio profile.
         * @return The audio profile.
         */
        static AudioProfile* getProfile(const std::string& profileName);

        /**
         * Preload audio file.
         *
         * PSG: The caller is responsible for deduplicating calls for a given filePath.
         * PSG: Invoking preload() on a filePath while there is a pending or effective
         * PSG: preload on the same filePath will result in undefined (possibly implementation-dependant)
         * PSG: behavior.
         *
         * @param filePath The file path of an audio.
         */
        static void preload(const std::string& filePath) { preload(filePath, nullptr); }

        /**
         * Preload audio file.
         *
         * PSG: The caller is responsible for deduplicating calls for a given filePath.
         * PSG: Invoking preload() on a filePath while there is a pending or effective
         * PSG: preload on the same filePath will result in undefined (possibly implementation-dependant)
         * PSG: behavior.
         *
         * @param filePath The file path of an audio.
         * @param callback A callback which will be called after loading is finished.
         */
        static void preload(const std::string& filePath, std::function<void(bool isSuccess)> callback);

        /**
         * Gets playing audio count.
         */
        static int getPlayingAudioCount();

        /**
         * Whether to enable playing audios
         * @note If it's disabled, current playing audios will be stopped and the later 'preload', 'play2d' methods will take no effects.
         */
        static void setEnabled(bool isEnabled);
        /**
         * Check whether AudioEngine is enabled.
         */
        static bool isEnabled();

    protected:
        static void addTask(const std::function<void()>& task);
        static void remove(int audioID);

        struct ProfileHelper final
        {
            AudioProfile profile;
            std::list<int> audioIDs;
            double lastPlayTime = 0.0;

            ProfileHelper() = default;
            ProfileHelper(ProfileHelper const&) = delete;
            ProfileHelper& operator=(ProfileHelper const&) = delete;
            ProfileHelper(ProfileHelper&&) noexcept = delete;
            ProfileHelper& operator=(ProfileHelper&&) noexcept = delete;
            ~ProfileHelper() = default;
        };

        struct AudioInfo final
        {
            std::string filePath;
            ProfileHelper* profileHelper = nullptr;

            float volume = 1.f;
            bool loop = false;
            std::chrono::milliseconds duration = TIME_UNKNOWN;
            AudioState state = AudioState::INITIALIZING;

            AudioInfo() = default;
            AudioInfo(AudioInfo const&) = delete;
            AudioInfo& operator=(AudioInfo const&) = delete;
            AudioInfo(AudioInfo&&) noexcept = delete;
            AudioInfo& operator=(AudioInfo&&) noexcept = delete;
            ~AudioInfo() = default;
        };

        // audioID,audioAttribute
        static std::unordered_map<int, AudioInfo> _audioIDInfoMap;

        // audio file path,audio IDs
        static std::unordered_multimap<std::string, int> _audioPathIDMap;

        // profileName,ProfileHelper
        static std::unordered_map<std::string, ProfileHelper> _audioPathProfileHelperMap;

        static unsigned int _maxInstances;

        static ProfileHelper* _defaultProfileHelper;

        static AudioEngineImpl* _audioEngineImpl;

        class AudioEngineThreadPool;
        static AudioEngineThreadPool* s_threadPool;

        static bool _isEnabled;

        friend class AudioEngineImpl;
    };

} // namespace experimental
NS_CC_END

// end group
/// @}
