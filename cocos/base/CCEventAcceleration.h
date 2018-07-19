/****************************************************************************
 Copyright (c) 2013-2016 Chukong Technologies Inc.

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

#ifndef CC_BASE_EVENTACCELERATION_H
#define CC_BASE_EVENTACCELERATION_H

#include "base/CCEvent.h"
#include "base/ccTypes.h"
#include "platform/CCPlatformMacros.h"
#include "platform/CCPlatformDefine.h"

/**
 * @addtogroup base
 * @{
 */

NS_CC_BEGIN

/** @class EventAcceleration
 * @brief Accelerometer event.
 */
class CC_DLL EventAcceleration final : public Event
{
public:
    EventAcceleration() = default;
    EventAcceleration(const Acceleration& acc);
    EventAcceleration(EventAcceleration const&) = default;
    EventAcceleration& operator=(EventAcceleration const&) = default;
    EventAcceleration(EventAcceleration&&) noexcept = default;
    EventAcceleration& operator=(EventAcceleration&&) noexcept = default;
    ~EventAcceleration() final;

private:
    Acceleration _acc;
    friend class EventListenerAcceleration;
};

NS_CC_END

// end of base group
/// @}

#endif // CC_BASE_EVENTACCELERATION_H
