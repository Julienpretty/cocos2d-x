#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_EMSCRIPTEN

#include "platform/CCDevice.h"
#include <emscripten/emscripten.h>

NS_CC_BEGIN

int Device::getDPI()
{
    return emscripten_get_device_pixel_ratio() * 96.0;
}

/**
 * To enable or disable accelerometer.
 */
void Device::setAccelerometerEnabled(bool isEnabled)
{
    //TODO EMSCRIPTEN: Implement
}

/**
 * Sets the interval of accelerometer.
 */
void Device::setAccelerometerInterval(float interval)
{
    //TODO EMSCRIPTEN: Implement
}

/**
 * Controls whether the screen should remain on.
 *
 * @param keepScreenOn One flag indicating that the screen should remain on.
 */
void Device::setKeepScreenOn(bool keepScreenOn)
{
    //TODO EMSCRIPTEN: Implement
}

/**
 * Vibrate for the specified amount of time.
 * If vibrate is not supported, then invoking this method has no effect.
 * Some platforms limit to a maximum duration of 5 seconds.
 * Duration is ignored on iOS due to API limitations.
 * @param duration The duration in seconds.
 */
void Device::vibrate(float duration)
{
    //TODO EMSCRIPTEN: Implement
}

/**
 * Gets texture data for text.
 */
Data Device::getTextureDataForText(const char* text, const FontDefinition& textDefinition, TextAlign align, int& width, int& height, bool& hasPremultipliedAlpha)
{
    //TODO EMSCRIPTEN: Implement
    return Data();
}

NS_CC_END

#endif