/****************************************************************************
 Copyright (c) 2008-2010 Ricardo Quesada
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2011      Zynga Inc.
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

#include "2d/CCActionInterval.h"

#include "2d/CCActionInstant.h"
#include "2d/CCNode.h"
#include "2d/CCSprite.h"
#include "2d/CCSpriteFrame.h"
#include "base/CCDirector.h"
#include "base/CCEventCustom.h"
#include "base/CCEventDispatcher.h"
#include "base/CCScriptSupport.h"
#include "platform/CCStdC.h"

#include <cmath>
#include <cstdarg>
#include <limits>

using namespace std::chrono_literals;

NS_CC_BEGIN

//
// IntervalAction
//

ActionInterval::~ActionInterval()
{
}

bool ActionInterval::initWithDuration(std::chrono::milliseconds d)
{
    _duration = d;
    return true;
}

bool ActionInterval::isDone() const
{
    return _status == Action::Status::DONE;
}

void ActionInterval::step(float dt)
{
    switch (_status)
    {
        case Action::Status::START:
            _status = Action::Status::RUNNING;
            _elapsed = 0ms;
            update(0.f);
            break;
        case Action::Status::RUNNING:
            _elapsed += std::chrono::milliseconds(static_cast<std::size_t>(dt * 1000.f));
            if (_duration == 0ms || (_elapsed > 0ms && _elapsed >= _duration))
            {
                update(1.f);
                _status = Action::Status::DONE;
            }
            else
            {
                update(static_cast<float>(_elapsed.count()) / _duration.count());
            }
            break;
        case Action::Status::UNKNOWN:
            CC_ASSERT(false);
            break;
        case Action::Status::DONE:
            CC_ASSERT(isDone());
            break;
    }
}

ActionInterval* ActionInterval::reverse() const
{
    CCASSERT(false, "must be implemented");
    return nullptr;
}

ActionInterval* ActionInterval::clone() const
{
    CCASSERT(false, "must be implemented");
    return nullptr;
}

//
// Sequence
//

Sequence::Sequence(std::initializer_list<FiniteTimeAction*> actions)
{
    _actions.reserve(actions.size());
    auto d = 0ms;
    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        d += duration;
        _actions.emplace_back(action);
        CC_SAFE_RETAIN(action);
    }
    initWithDuration(d);
}

Sequence::Sequence(Vector<FiniteTimeAction*> const& actions)
{
    _actions.reserve(actions.size());
    auto d = 0ms;
    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        d += duration;
        _actions.emplace_back(action);
        CC_SAFE_RETAIN(action);
    }
    initWithDuration(d);
}

Sequence::~Sequence()
{
    for (auto action : _actions)
    {
        CC_SAFE_RELEASE(action);
    }
}

Sequence* Sequence::create(std::initializer_list<FiniteTimeAction*> actions)
{
    auto ret = new Sequence(actions);
    ret->autorelease();
    return ret;
}

Sequence* Sequence::create(Vector<FiniteTimeAction*> const& actions)
{
    auto ret = new Sequence(actions);
    ret->autorelease();
    return ret;
}

Sequence* Sequence::clone() const
{
    auto ret = new Sequence();
    ret->autorelease();
    ret->_actions.reserve(_actions.size());
    ret->initWithDuration(_duration);
    for (auto action : _actions)
    {
        auto tmp = action->clone();
        ret->_actions.emplace_back(tmp);
        CC_SAFE_RETAIN(tmp);
    }
    return ret;
}

void Sequence::startWithTarget(Node* target)
{
    CC_ASSERT(target != nullptr);

    for (auto action : _actions)
    {
        action->setStatus(Action::Status::START);
    }

    ActionInterval::startWithTarget(target);
}

void Sequence::stop()
{
    for (auto& action : _actions)
    {
        if (action->getStatus() != Action::Status::DONE) {
        	action->stop();
        	action->setStatus(Action::Status::DONE);
        }
    }

    ActionInterval::stop();
}

void Sequence::update(float p)
{
    auto duration = p >= 1.f ? _duration : std::chrono::milliseconds(static_cast<std::size_t>(p * _duration.count()));
    for (auto action : _actions)
    {
        auto const d = action->getDuration();
        switch (action->getStatus())
        {
            case Action::Status::START:
                action->startWithTarget(_target);
                action->setStatus(Action::Status::RUNNING);
                if (d != 0ms)
                {
                    action->update(0.f);
                }
            case Action::Status::RUNNING:
                if (d == 0ms || (duration > 0ms && duration >= d))
                {
                    action->update(1.f);
                    action->stop();
                    action->setStatus(Status::DONE);
                }
                else
                {
                    action->update(static_cast<float>(duration.count()) / d.count());
                }
                break;
            case Action::Status::UNKNOWN:
                CC_ASSERT(false);
                break;
            case Action::Status::DONE:
                break;
        }

        if (d > duration) // optimization to decrease loop iteration at the begining of the action
        {
            break;
        }
        duration -= d;
    }
}

bool Sequence::isDone() const
{
    return _actions.empty() ? true : (_actions.back()->getStatus() == Status::DONE);
}

Sequence* Sequence::reverse() const
{
    auto ret = new Sequence();
    ret->autorelease();
    ret->_actions.reserve(_actions.size());
    ret->initWithDuration(_duration);
    for (auto it = _actions.rbegin(); it != _actions.rend(); ++it)
    {
        auto tmp = (*it)->reverse();
        ret->_actions.emplace_back(tmp);
        CC_SAFE_RETAIN(tmp);
    }
    return ret;
}

//
// Repeat
//

Repeat* Repeat::create(FiniteTimeAction* action, unsigned int times)
{
    Repeat* repeat = new (std::nothrow) Repeat();
    if (repeat && repeat->initWithAction(action, times))
    {
        repeat->autorelease();
        return repeat;
    }

    delete repeat;
    return nullptr;
}

bool Repeat::initWithAction(FiniteTimeAction* action, unsigned int times)
{
    auto d = action->getDuration() * times;

    if (action && ActionInterval::initWithDuration(d))
    {
        _times = times;
        _innerAction = action;
        action->retain();

        _actionInstant = dynamic_cast<ActionInstant*>(action) ? true : false;
        // an instant action needs to be executed one time less in the update method since it uses startWithTarget to execute the action
        // minggo: instant action doesn't execute action in Repeat::startWithTarget(), so comment it.
        //        if (_actionInstant)
        //        {
        //            _times -=1;
        //        }
        _total = 0;

        return true;
    }

    return false;
}

Repeat* Repeat::clone(void) const
{
    // no copy constructor
    return Repeat::create(_innerAction->clone(), _times);
}

Repeat::~Repeat(void)
{
    CC_SAFE_RELEASE(_innerAction);
}

void Repeat::startWithTarget(Node* target)
{
    _total = 0;
    _nextDt = _innerAction->getDuration() / _duration;
    ActionInterval::startWithTarget(target);
    _innerAction->startWithTarget(target);
}

void Repeat::stop(void)
{
    _innerAction->stop();
    ActionInterval::stop();
}

// issue #80. Instead of hooking step:, hook update: since it can be called by any
// container action like Repeat, Sequence, Ease, etc..
void Repeat::update(float p)
{
    if (p >= _nextDt)
    {
        while (p >= _nextDt && _total < _times)
        {
            _innerAction->update(1.0f);
            _total++;

            _innerAction->stop();
            _innerAction->startWithTarget(_target);
            _nextDt = static_cast<float>(_innerAction->getDuration().count()) / _duration.count() * (_total + 1);
        }

        // fix for issue #1288, incorrect end value of repeat
        if (p >= 1.0f && _total < _times)
        {
            _innerAction->update(1.0f);
            _total++;
        }

        // don't set an instant action back or update it, it has no use because it has no duration
        if (!_actionInstant)
        {
            if (_total == _times)
            {
                // minggo: inner action update is invoked above, don't have to invoke it here
                // _innerAction->update(1.f);
                _innerAction->stop();
            }
            else
            {
                // issue #390 prevent jerk, use right update
                float const tmp = p - (_nextDt - static_cast<float>(_innerAction->getDuration().count()) / _duration.count());
                _innerAction->update(tmp > 0.f ? tmp : 0.f);
            }
        }
    }
    else
    {
        if (_total < _times)
        {
            _innerAction->update(std::fmod(p * _times, 1.0f));
        }
    }
}

bool Repeat::isDone(void) const
{
    return _total == _times;
}

Repeat* Repeat::reverse() const
{
    return Repeat::create(_innerAction->reverse(), _times);
}

//
// RepeatForever
//
RepeatForever::~RepeatForever()
{
    CC_SAFE_RELEASE(_innerAction);
}

bool RepeatForever::initWithAction(ActionInterval* action)
{
    CCASSERT(action != nullptr, "action can't be nullptr!");
    if (action == nullptr)
    {
        log("RepeatForever::initWithAction error:action is nullptr!");
        return false;
    }

    action->retain();
    _innerAction = action;

    return true;
}

RepeatForever* RepeatForever::create(ActionInterval* action)
{
    RepeatForever* ret = new (std::nothrow) RepeatForever();
    if (ret && ret->initWithAction(action))
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void RepeatForever::setInnerAction(ActionInterval* action)
{
    if (_innerAction != action)
    {
        CC_SAFE_RELEASE(_innerAction);
        _innerAction = action;
        CC_SAFE_RETAIN(_innerAction);
    }
}

RepeatForever* RepeatForever::clone() const
{
    // no copy constructor
    return RepeatForever::create(_innerAction->clone());
}

void RepeatForever::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _innerAction->startWithTarget(target);
}

void RepeatForever::step(float dt)
{
    _innerAction->step(dt);
    if (!isDone() && _innerAction->isDone())
    {
        auto const duration = _innerAction->getDuration();
        auto const elasped = _innerAction->getElapsed();
        if (elasped > duration)
        {
            auto const diff = (elasped - duration).count() % duration.count();
            _innerAction->startWithTarget(_target);
            // to prevent jerk. issue #390, 1247
            _innerAction->step(0.0f);
            _innerAction->step(static_cast<float>(diff) / 1000.f);
        }
        else
        {
            _innerAction->startWithTarget(_target);
            // to prevent jerk. issue #390, 1247
            _innerAction->step(0.0f);
        }
    }
}

void RepeatForever::update(float)
{
}

RepeatForever* RepeatForever::reverse() const
{
    return RepeatForever::create(_innerAction->reverse());
}

//
// Spawn
//

Spawn::Spawn(std::initializer_list<FiniteTimeAction*> actions)
{
    _actions.reserve(actions.size());
    auto d = 0ms;

    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        d = std::max(d, duration);
    }

    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        if (d == duration) // in the case of the max value no need to add extra DelayTime
        {
            _actions.emplace_back(action);
            CC_SAFE_RETAIN(action);
        }
        else
        {
            // We create a sequence with a DelayTime to make sure that the action will be played backwards if they are reversed
            auto tmp = Sequence::create({action, DelayTime::create(d - duration)});
            _actions.emplace_back(tmp);
            CC_SAFE_RETAIN(tmp);
        }
    }

    d = 0ms;
    for (auto action : _actions)
    {
        auto const duration = action->getDuration();
        d = std::max(d, duration);
    }

    initWithDuration(d);
}

Spawn::Spawn(Vector<FiniteTimeAction*> const& actions)
{
    _actions.reserve(actions.size());
    auto d = 0ms;

    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        d = std::max(d, duration);
    }

    for (auto action : actions)
    {
        auto const duration = action->getDuration();
        if (d == duration) // in the case of the max value no need to add extra DelayTime
        {
            _actions.emplace_back(action);
            CC_SAFE_RETAIN(action);
        }
        else
        {
            // We create a sequence with a DelayTime to make sure that the action will be played backwards if they are reversed
            auto tmp = Sequence::create({action, DelayTime::create(d - duration)});
            _actions.emplace_back(tmp);
            CC_SAFE_RETAIN(tmp);
        }
    }

    d = 0ms;
    for (auto action : _actions)
    {
        auto const duration = action->getDuration();
        d = std::max(d, duration);
    }

    initWithDuration(d);
}

Spawn::~Spawn()
{
    for (auto action : _actions)
    {
        CC_SAFE_RELEASE(action);
    }
}

Spawn* Spawn::create(std::initializer_list<FiniteTimeAction*> actions)
{
    auto ret = new Spawn(actions);
    ret->autorelease();
    return ret;
}

Spawn* Spawn::create(Vector<FiniteTimeAction*> const& actions)
{
    auto ret = new Spawn(actions);
    ret->autorelease();
    return ret;
}

Spawn* Spawn::clone() const
{
    auto ret = new Spawn();
    ret->autorelease();
    ret->_actions.reserve(_actions.size());
    ret->initWithDuration(_duration);
    for (auto action : _actions)
    {
        auto tmp = action->clone();
        ret->_actions.emplace_back(tmp);
        CC_SAFE_RETAIN(tmp);
    }
    return ret;
}

void Spawn::startWithTarget(Node* target)
{
    CC_ASSERT(target != nullptr);

    for (auto action : _actions)
    {
        action->setStatus(Action::Status::START);
    }

    ActionInterval::startWithTarget(target);
}

void Spawn::stop()
{
    for (auto action : _actions)
    {
        if (action->getStatus() != Action::Status::DONE) {
            action->stop();
            action->setStatus(Action::Status::DONE);
        }
    }

    ActionInterval::stop();
}

void Spawn::update(float p)
{
    auto const duration = p >= 1.f ? _duration : std::chrono::milliseconds(static_cast<std::size_t>(p * _duration.count()));
    for (auto action : _actions)
    {
        auto const d = action->getDuration();
        switch (action->getStatus())
        {
            case Action::Status::START:
                action->startWithTarget(_target);
                action->setStatus(Action::Status::RUNNING);
                if (d != 0ms)
                {
                    action->update(0.f);
                }
            case Action::Status::RUNNING:
                if (d == 0ms || (duration > 0ms && duration >= d))
                {
                    action->update(1.f);
                    action->stop();
                    action->setStatus(Status::DONE);
                }
                else
                {
                    action->update(static_cast<float>(duration.count()) / d.count());
                }
                break;
            case Action::Status::UNKNOWN:
                CC_ASSERT(false);
                break;
            case Action::Status::DONE:
                break;
        }
    }
}

bool Spawn::isDone() const
{
    for (auto action : _actions)
    {
        if (action->getStatus() != Action::Status::DONE)
        {
            return false;
        }
    }
    return true;
}

Spawn* Spawn::reverse() const
{
    auto ret = new Spawn();
    ret->autorelease();
    ret->_actions.reserve(_actions.size());
    ret->initWithDuration(_duration);
    for (auto it = _actions.rbegin(); it != _actions.rend(); ++it)
    {
        auto tmp = (*it)->reverse();
        ret->_actions.emplace_back(tmp);
        CC_SAFE_RETAIN(tmp);
    }
    return ret;
}

//
// RotateTo
//

RotateTo* RotateTo::create(std::chrono::milliseconds duration, float dstAngle)
{
    RotateTo* rotateTo = new (std::nothrow) RotateTo();
    if (rotateTo && rotateTo->initWithDuration(duration, dstAngle, dstAngle))
    {
        rotateTo->autorelease();
        return rotateTo;
    }

    delete rotateTo;
    return nullptr;
}

RotateTo* RotateTo::create(std::chrono::milliseconds duration, float dstAngleX, float dstAngleY)
{
    RotateTo* rotateTo = new (std::nothrow) RotateTo();
    if (rotateTo && rotateTo->initWithDuration(duration, dstAngleX, dstAngleY))
    {
        rotateTo->autorelease();
        return rotateTo;
    }

    delete rotateTo;
    return nullptr;
}

RotateTo* RotateTo::create(std::chrono::milliseconds duration, const Vec3& dstAngle3D)
{
    RotateTo* rotateTo = new (std::nothrow) RotateTo();
    if (rotateTo && rotateTo->initWithDuration(duration, dstAngle3D))
    {
        rotateTo->autorelease();
        return rotateTo;
    }

    delete rotateTo;
    return nullptr;
}

RotateTo::RotateTo()
: _is3D(false)
{
}

bool RotateTo::initWithDuration(std::chrono::milliseconds duration, float dstAngleX, float dstAngleY)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _dstAngle.x = dstAngleX;
        _dstAngle.y = dstAngleY;

        return true;
    }

    return false;
}

bool RotateTo::initWithDuration(std::chrono::milliseconds duration, const Vec3& dstAngle3D)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _dstAngle = dstAngle3D;
        _is3D = true;

        return true;
    }

    return false;
}

RotateTo* RotateTo::clone(void) const
{
    // no copy constructor
    auto a = new (std::nothrow) RotateTo();
    if (_is3D)
        a->initWithDuration(_duration, _dstAngle);
    else
        a->initWithDuration(_duration, _dstAngle.x, _dstAngle.y);
    a->autorelease();
    return a;
}

void RotateTo::calculateAngles(float& startAngle, float& diffAngle, float dstAngle)
{
    if (startAngle > 0)
    {
        startAngle = fmodf(startAngle, 360.0f);
    }
    else
    {
        startAngle = fmodf(startAngle, -360.0f);
    }

    diffAngle = dstAngle - startAngle;
    if (diffAngle > 180)
    {
        diffAngle -= 360;
    }
    if (diffAngle < -180)
    {
        diffAngle += 360;
    }
}

void RotateTo::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);

    if (_is3D)
    {
        _startAngle = _target->getRotation3D();
    }
    else
    {
        _startAngle.x = _target->getRotationSkewX();
        _startAngle.y = _target->getRotationSkewY();
    }

    calculateAngles(_startAngle.x, _diffAngle.x, _dstAngle.x);
    calculateAngles(_startAngle.y, _diffAngle.y, _dstAngle.y);
    calculateAngles(_startAngle.z, _diffAngle.z, _dstAngle.z);
}

void RotateTo::update(float time)
{
    if (_target)
    {
        if (_is3D)
        {
            _target->setRotation3D(Vec3(_startAngle.x + _diffAngle.x * time, _startAngle.y + _diffAngle.y * time, _startAngle.z + _diffAngle.z * time));
        }
        else
        {
#if CC_USE_PHYSICS
            if (_startAngle.x == _startAngle.y && _diffAngle.x == _diffAngle.y)
            {
                _target->setRotation(_startAngle.x + _diffAngle.x * time);
            }
            else
            {
                _target->setRotationSkewX(_startAngle.x + _diffAngle.x * time);
                _target->setRotationSkewY(_startAngle.y + _diffAngle.y * time);
            }
#else
            _target->setRotationSkewX(_startAngle.x + _diffAngle.x * time);
            _target->setRotationSkewY(_startAngle.y + _diffAngle.y * time);
#endif // CC_USE_PHYSICS
        }
    }
}

RotateTo* RotateTo::reverse() const
{
    CCASSERT(false, "RotateTo doesn't support the 'reverse' method");
    return nullptr;
}

//
// RotateBy
//

RotateBy* RotateBy::create(std::chrono::milliseconds duration, float deltaAngle)
{
    RotateBy* rotateBy = new (std::nothrow) RotateBy();
    if (rotateBy && rotateBy->initWithDuration(duration, deltaAngle))
    {
        rotateBy->autorelease();
        return rotateBy;
    }

    delete rotateBy;
    return nullptr;
}

RotateBy* RotateBy::create(std::chrono::milliseconds duration, float deltaAngleX, float deltaAngleY)
{
    RotateBy* rotateBy = new (std::nothrow) RotateBy();
    if (rotateBy && rotateBy->initWithDuration(duration, deltaAngleX, deltaAngleY))
    {
        rotateBy->autorelease();
        return rotateBy;
    }

    delete rotateBy;
    return nullptr;
}

RotateBy* RotateBy::create(std::chrono::milliseconds duration, const Vec3& deltaAngle3D)
{
    RotateBy* rotateBy = new (std::nothrow) RotateBy();
    if (rotateBy && rotateBy->initWithDuration(duration, deltaAngle3D))
    {
        rotateBy->autorelease();
        return rotateBy;
    }

    delete rotateBy;
    return nullptr;
}

RotateBy::RotateBy()
: _is3D(false)
{
}

bool RotateBy::initWithDuration(std::chrono::milliseconds duration, float deltaAngle)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _deltaAngle.x = _deltaAngle.y = deltaAngle;
        return true;
    }

    return false;
}

bool RotateBy::initWithDuration(std::chrono::milliseconds duration, float deltaAngleX, float deltaAngleY)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _deltaAngle.x = deltaAngleX;
        _deltaAngle.y = deltaAngleY;
        return true;
    }

    return false;
}

bool RotateBy::initWithDuration(std::chrono::milliseconds duration, const Vec3& deltaAngle3D)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _deltaAngle = deltaAngle3D;
        _is3D = true;
        return true;
    }

    return false;
}

RotateBy* RotateBy::clone() const
{
    // no copy constructor
    auto a = new (std::nothrow) RotateBy();
    if (_is3D)
        a->initWithDuration(_duration, _deltaAngle);
    else
        a->initWithDuration(_duration, _deltaAngle.x, _deltaAngle.y);
    a->autorelease();
    return a;
}

void RotateBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    if (_is3D)
    {
        _startAngle = target->getRotation3D();
    }
    else
    {
        _startAngle.x = target->getRotationSkewX();
        _startAngle.y = target->getRotationSkewY();
    }
}

void RotateBy::update(float time)
{
    // FIXME: shall I add % 360
    if (_target)
    {
        if (_is3D)
        {
            Vec3 v;
            v.x = _startAngle.x + _deltaAngle.x * time;
            v.y = _startAngle.y + _deltaAngle.y * time;
            v.z = _startAngle.z + _deltaAngle.z * time;
            _target->setRotation3D(v);
        }
        else
        {
#if CC_USE_PHYSICS
            if (_startAngle.x == _startAngle.y && _deltaAngle.x == _deltaAngle.y)
            {
                _target->setRotation(_startAngle.x + _deltaAngle.x * time);
            }
            else
            {
                _target->setRotationSkewX(_startAngle.x + _deltaAngle.x * time);
                _target->setRotationSkewY(_startAngle.y + _deltaAngle.y * time);
            }
#else
            _target->setRotationSkewX(_startAngle.x + _deltaAngle.x * time);
            _target->setRotationSkewY(_startAngle.y + _deltaAngle.y * time);
#endif // CC_USE_PHYSICS
        }
    }
}

RotateBy* RotateBy::reverse() const
{
    if (_is3D)
    {
        Vec3 v;
        v.x = -_deltaAngle.x;
        v.y = -_deltaAngle.y;
        v.z = -_deltaAngle.z;
        return RotateBy::create(_duration, v);
    }
    else
    {
        return RotateBy::create(_duration, -_deltaAngle.x, -_deltaAngle.y);
    }
}

//
// MoveBy
//

MoveBy::~MoveBy()
{
}

MoveBy* MoveBy::create(std::chrono::milliseconds duration, Vec2 const& deltaPosition)
{
    return MoveBy::create(duration, Vec3(deltaPosition.x, deltaPosition.y, 0));
}

MoveBy* MoveBy::create(std::chrono::milliseconds duration, Vec3 const& deltaPosition)
{
    MoveBy* ret = new (std::nothrow) MoveBy();

    if (ret && ret->initWithDuration(duration, deltaPosition))
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool MoveBy::initWithDuration(std::chrono::milliseconds duration, Vec2 const& deltaPosition)
{
    return MoveBy::initWithDuration(duration, Vec3(deltaPosition.x, deltaPosition.y, 0));
}

bool MoveBy::initWithDuration(std::chrono::milliseconds duration, Vec3 const& deltaPosition)
{
    bool ret = false;

    if (ActionInterval::initWithDuration(duration))
    {
        _positionDelta = deltaPosition;
        _is3D = true;
        ret = true;
    }

    return ret;
}

MoveBy* MoveBy::clone() const
{
    // no copy constructor
    return MoveBy::create(_duration, _positionDelta);
}

void MoveBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _previousPosition = _startPosition = target->getPosition3D();
}

MoveBy* MoveBy::reverse() const
{
    return MoveBy::create(_duration, -_positionDelta);
}

void MoveBy::update(float t)
{
    if (_target != nullptr)
    {
#if CC_ENABLE_STACKABLE_ACTIONS
        Vec3 const currentPos = _target->getPosition3D();
        Vec3 const diff = currentPos - _previousPosition;
        _startPosition = _startPosition + diff;
        Vec3 const newPos = _startPosition + (_positionDelta * t);
        _target->setPosition3D(newPos);
        _previousPosition = newPos;
#else
        _target->setPosition3D(_startPosition + _positionDelta * t);
#endif // CC_ENABLE_STACKABLE_ACTIONS
    }
}

//
// MoveTo
//

MoveTo::~MoveTo()
{
}

MoveTo* MoveTo::create(std::chrono::milliseconds duration, Vec2 const& position)
{
    return MoveTo::create(duration, Vec3(position.x, position.y, 0));
}

MoveTo* MoveTo::create(std::chrono::milliseconds duration, Vec3 const& position)
{
    MoveTo* ret = new (std::nothrow) MoveTo();

    if (ret && ret->initWithDuration(duration, position))
    {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool MoveTo::initWithDuration(std::chrono::milliseconds duration, Vec2 const& position)
{
    return initWithDuration(duration, Vec3(position.x, position.y, 0));
}

bool MoveTo::initWithDuration(std::chrono::milliseconds duration, Vec3 const& position)
{
    bool ret = false;

    if (ActionInterval::initWithDuration(duration))
    {
        _endPosition = position;
        ret = true;
    }

    return ret;
}

MoveTo* MoveTo::clone() const
{
    // no copy constructor
    return MoveTo::create(_duration, _endPosition);
}

void MoveTo::startWithTarget(Node* target)
{
    MoveBy::startWithTarget(target);
    _positionDelta = _endPosition - target->getPosition3D();
}

MoveTo* MoveTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in MoveTo");
    return nullptr;
}

//
// SkewTo
//
SkewTo* SkewTo::create(std::chrono::milliseconds t, float sx, float sy)
{
    SkewTo* skewTo = new (std::nothrow) SkewTo();
    if (skewTo && skewTo->initWithDuration(t, sx, sy))
    {
        skewTo->autorelease();
        return skewTo;
    }

    delete skewTo;
    return nullptr;
}

bool SkewTo::initWithDuration(std::chrono::milliseconds t, float sx, float sy)
{
    bool bRet = false;

    if (ActionInterval::initWithDuration(t))
    {
        _endSkewX = sx;
        _endSkewY = sy;

        bRet = true;
    }

    return bRet;
}

SkewTo* SkewTo::clone() const
{
    // no copy constructor
    return SkewTo::create(_duration, _endSkewX, _endSkewY);
}

SkewTo* SkewTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in SkewTo");
    return nullptr;
}

void SkewTo::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);

    _startSkewX = target->getSkewX();

    if (_startSkewX > 0)
    {
        _startSkewX = fmodf(_startSkewX, 180.f);
    }
    else
    {
        _startSkewX = fmodf(_startSkewX, -180.f);
    }

    _deltaX = _endSkewX - _startSkewX;

    if (_deltaX > 180)
    {
        _deltaX -= 360;
    }
    if (_deltaX < -180)
    {
        _deltaX += 360;
    }

    _startSkewY = target->getSkewY();

    if (_startSkewY > 0)
    {
        _startSkewY = fmodf(_startSkewY, 360.f);
    }
    else
    {
        _startSkewY = fmodf(_startSkewY, -360.f);
    }

    _deltaY = _endSkewY - _startSkewY;

    if (_deltaY > 180)
    {
        _deltaY -= 360;
    }
    if (_deltaY < -180)
    {
        _deltaY += 360;
    }
}

void SkewTo::update(float t)
{
    _target->setSkewX(_startSkewX + _deltaX * t);
    _target->setSkewY(_startSkewY + _deltaY * t);
}

SkewTo::SkewTo()
: _skewX(0.0)
, _skewY(0.0)
, _startSkewX(0.0)
, _startSkewY(0.0)
, _endSkewX(0.0)
, _endSkewY(0.0)
, _deltaX(0.0)
, _deltaY(0.0)
{
}

//
// SkewBy
//
SkewBy* SkewBy::create(std::chrono::milliseconds t, float sx, float sy)
{
    SkewBy* skewBy = new (std::nothrow) SkewBy();
    if (skewBy && skewBy->initWithDuration(t, sx, sy))
    {
        skewBy->autorelease();
        return skewBy;
    }

    delete skewBy;
    return nullptr;
}

SkewBy* SkewBy::clone() const
{
    // no copy constructor
    return SkewBy::create(_duration, _skewX, _skewY);
}

bool SkewBy::initWithDuration(std::chrono::milliseconds t, float deltaSkewX, float deltaSkewY)
{
    bool ret = false;

    if (SkewTo::initWithDuration(t, deltaSkewX, deltaSkewY))
    {
        _skewX = deltaSkewX;
        _skewY = deltaSkewY;

        ret = true;
    }

    return ret;
}

void SkewBy::startWithTarget(Node* target)
{
    SkewTo::startWithTarget(target);
    _deltaX = _skewX;
    _deltaY = _skewY;
    _endSkewX = _startSkewX + _deltaX;
    _endSkewY = _startSkewY + _deltaY;
}

SkewBy* SkewBy::reverse() const
{
    return SkewBy::create(_duration, -_skewX, -_skewY);
}

ResizeTo* ResizeTo::create(std::chrono::milliseconds duration, const cocos2d::Size& final_size)
{
    ResizeTo* ret = new (std::nothrow) ResizeTo();

    if (ret)
    {
        if (ret->initWithDuration(duration, final_size))
        {
            ret->autorelease();
        }
        else
        {
            delete ret;
            ret = nullptr;
        }
    }

    return ret;
}

ResizeTo* ResizeTo::clone() const
{
    // no copy constructor
    ResizeTo* a = new (std::nothrow) ResizeTo();
    a->initWithDuration(_duration, _finalSize);
    a->autorelease();

    return a;
}

void ResizeTo::startWithTarget(cocos2d::Node* target)
{
    ActionInterval::startWithTarget(target);
    _initialSize = target->getContentSize();
    _sizeDelta = _finalSize - _initialSize;
}

void ResizeTo::update(float time)
{
    if (_target)
    {
        auto new_size = _initialSize + (_sizeDelta * time);
        _target->setContentSize(new_size);
    }
}

bool ResizeTo::initWithDuration(std::chrono::milliseconds duration, const cocos2d::Size& final_size)
{
    if (cocos2d::ActionInterval::initWithDuration(duration))
    {
        _finalSize = final_size;
        return true;
    }

    return false;
}

//
// ResizeBy
//

ResizeBy* ResizeBy::create(std::chrono::milliseconds duration, const cocos2d::Size& deltaSize)
{
    ResizeBy* ret = new (std::nothrow) ResizeBy();

    if (ret)
    {
        if (ret->initWithDuration(duration, deltaSize))
        {
            ret->autorelease();
        }
        else
        {
            delete ret;
            ret = nullptr;
        }
    }

    return ret;
}

ResizeBy* ResizeBy::clone() const
{
    // no copy constructor
    auto a = new (std::nothrow) ResizeBy();
    a->initWithDuration(_duration, _sizeDelta);
    a->autorelease();
    return a;
}

void ResizeBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _previousSize = _startSize = target->getContentSize();
}

ResizeBy* ResizeBy::reverse() const
{
    cocos2d::Size newSize(-_sizeDelta.width, -_sizeDelta.height);
    return ResizeBy::create(_duration, newSize);
}

void ResizeBy::update(float t)
{
    if (_target)
    {
        _target->setContentSize(_startSize + (_sizeDelta * t));
    }
}

bool ResizeBy::initWithDuration(std::chrono::milliseconds duration, const cocos2d::Size& deltaSize)
{
    bool ret = false;

    if (ActionInterval::initWithDuration(duration))
    {
        _sizeDelta = deltaSize;
        ret = true;
    }

    return ret;
}

//
// JumpBy
//

JumpBy* JumpBy::create(std::chrono::milliseconds duration, const Vec2& position, float height, int jumps)
{
    JumpBy* jumpBy = new (std::nothrow) JumpBy();
    if (jumpBy && jumpBy->initWithDuration(duration, position, height, jumps))
    {
        jumpBy->autorelease();
        return jumpBy;
    }

    delete jumpBy;
    return nullptr;
}

bool JumpBy::initWithDuration(std::chrono::milliseconds duration, const Vec2& position, float height, int jumps)
{
    CCASSERT(jumps >= 0, "Number of jumps must be >= 0");
    if (jumps < 0)
    {
        log("JumpBy::initWithDuration error: Number of jumps must be >= 0");
        return false;
    }

    if (ActionInterval::initWithDuration(duration) && jumps >= 0)
    {
        _delta = position;
        _height = height;
        _jumps = jumps;

        return true;
    }

    return false;
}

JumpBy* JumpBy::clone() const
{
    // no copy constructor
    return JumpBy::create(_duration, _delta, _height, _jumps);
}

void JumpBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _previousPos = _startPosition = target->getPosition();
}

void JumpBy::update(float t)
{
    // parabolic jump (since v0.8.2)
    if (_target)
    {
        float frac = fmodf(t * _jumps, 1.0f);
        float y = _height * 4 * frac * (1 - frac);
        y += _delta.y * t;

        float x = _delta.x * t;
#if CC_ENABLE_STACKABLE_ACTIONS
        Vec2 currentPos = _target->getPosition();

        Vec2 diff = currentPos - _previousPos;
        _startPosition = diff + _startPosition;

        Vec2 newPos = _startPosition + Vec2(x, y);
        _target->setPosition(newPos);

        _previousPos = newPos;
#else
        _target->setPosition(_startPosition + Vec2(x, y));
#endif // !CC_ENABLE_STACKABLE_ACTIONS
    }
}

JumpBy* JumpBy::reverse() const
{
    return JumpBy::create(_duration, Vec2(-_delta.x, -_delta.y), _height, _jumps);
}

//
// JumpTo
//

JumpTo* JumpTo::create(std::chrono::milliseconds duration, const Vec2& position, float height, int jumps)
{
    JumpTo* jumpTo = new (std::nothrow) JumpTo();
    if (jumpTo && jumpTo->initWithDuration(duration, position, height, jumps))
    {
        jumpTo->autorelease();
        return jumpTo;
    }

    delete jumpTo;
    return nullptr;
}

bool JumpTo::initWithDuration(std::chrono::milliseconds duration, const Vec2& position, float height, int jumps)
{
    CCASSERT(jumps >= 0, "Number of jumps must be >= 0");
    if (jumps < 0)
    {
        log("JumpTo::initWithDuration error:Number of jumps must be >= 0");
        return false;
    }

    if (ActionInterval::initWithDuration(duration) && jumps >= 0)
    {
        _endPosition = position;
        _height = height;
        _jumps = jumps;

        return true;
    }

    return false;
}

JumpTo* JumpTo::clone() const
{
    // no copy constructor
    return JumpTo::create(_duration, _endPosition, _height, _jumps);
}

JumpTo* JumpTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in JumpTo");
    return nullptr;
}

void JumpTo::startWithTarget(Node* target)
{
    JumpBy::startWithTarget(target);
    _delta.set(_endPosition.x - _startPosition.x, _endPosition.y - _startPosition.y);
}

// Bezier cubic formula:
//    ((1 - t) + t)3 = 1
// Expands to ...
//   (1 - t)3 + 3t(1-t)2 + 3t2(1 - t) + t3 = 1
static inline float bezierat(float a, float b, float c, float d, float t)
{
    return (powf(1 - t, 3) * a + 3 * t * (powf(1 - t, 2)) * b + 3 * powf(t, 2) * (1 - t) * c + powf(t, 3) * d);
}

//
// BezierBy
//

BezierBy* BezierBy::create(std::chrono::milliseconds t, const ccBezierConfig& c)
{
    BezierBy* bezierBy = new (std::nothrow) BezierBy();
    if (bezierBy && bezierBy->initWithDuration(t, c))
    {
        bezierBy->autorelease();
        return bezierBy;
    }

    delete bezierBy;
    return nullptr;
}

bool BezierBy::initWithDuration(std::chrono::milliseconds t, const ccBezierConfig& c)
{
    if (ActionInterval::initWithDuration(t))
    {
        _config = c;
        return true;
    }

    return false;
}

void BezierBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _previousPosition = _startPosition = target->getPosition();
}

BezierBy* BezierBy::clone() const
{
    // no copy constructor
    return BezierBy::create(_duration, _config);
}

void BezierBy::update(float time)
{
    if (_target)
    {
        float xa = 0;
        float xb = _config.controlPoint_1.x;
        float xc = _config.controlPoint_2.x;
        float xd = _config.endPosition.x;

        float ya = 0;
        float yb = _config.controlPoint_1.y;
        float yc = _config.controlPoint_2.y;
        float yd = _config.endPosition.y;

        float x = bezierat(xa, xb, xc, xd, time);
        float y = bezierat(ya, yb, yc, yd, time);

#if CC_ENABLE_STACKABLE_ACTIONS
        Vec2 currentPos = _target->getPosition();
        Vec2 diff = currentPos - _previousPosition;
        _startPosition = _startPosition + diff;

        Vec2 newPos = _startPosition + Vec2(x, y);
        _target->setPosition(newPos);

        _previousPosition = newPos;
#else
        _target->setPosition(_startPosition + Vec2(x, y));
#endif // !CC_ENABLE_STACKABLE_ACTIONS
    }
}

BezierBy* BezierBy::reverse() const
{
    ccBezierConfig r;

    r.endPosition = -_config.endPosition;
    r.controlPoint_1 = _config.controlPoint_2 + (-_config.endPosition);
    r.controlPoint_2 = _config.controlPoint_1 + (-_config.endPosition);

    BezierBy* action = BezierBy::create(_duration, r);
    return action;
}

//
// BezierTo
//

BezierTo* BezierTo::create(std::chrono::milliseconds t, const ccBezierConfig& c)
{
    BezierTo* bezierTo = new (std::nothrow) BezierTo();
    if (bezierTo && bezierTo->initWithDuration(t, c))
    {
        bezierTo->autorelease();
        return bezierTo;
    }

    delete bezierTo;
    return nullptr;
}

bool BezierTo::initWithDuration(std::chrono::milliseconds t, const ccBezierConfig& c)
{
    if (ActionInterval::initWithDuration(t))
    {
        _toConfig = c;
        return true;
    }

    return false;
}

BezierTo* BezierTo::clone() const
{
    // no copy constructor
    return BezierTo::create(_duration, _toConfig);
}

void BezierTo::startWithTarget(Node* target)
{
    BezierBy::startWithTarget(target);
    _config.controlPoint_1 = _toConfig.controlPoint_1 - _startPosition;
    _config.controlPoint_2 = _toConfig.controlPoint_2 - _startPosition;
    _config.endPosition = _toConfig.endPosition - _startPosition;
}

BezierTo* BezierTo::reverse() const
{
    CCASSERT(false, "CCBezierTo doesn't support the 'reverse' method");
    return nullptr;
}

//
// ScaleTo
//
ScaleTo* ScaleTo::create(std::chrono::milliseconds duration, float s)
{
    ScaleTo* scaleTo = new (std::nothrow) ScaleTo();
    if (scaleTo && scaleTo->initWithDuration(duration, s))
    {
        scaleTo->autorelease();
        return scaleTo;
    }

    delete scaleTo;
    return nullptr;
}

ScaleTo* ScaleTo::create(std::chrono::milliseconds duration, float sx, float sy)
{
    ScaleTo* scaleTo = new (std::nothrow) ScaleTo();
    if (scaleTo && scaleTo->initWithDuration(duration, sx, sy))
    {
        scaleTo->autorelease();
        return scaleTo;
    }

    delete scaleTo;
    return nullptr;
}

ScaleTo* ScaleTo::create(std::chrono::milliseconds duration, float sx, float sy, float sz)
{
    ScaleTo* scaleTo = new (std::nothrow) ScaleTo();
    if (scaleTo && scaleTo->initWithDuration(duration, sx, sy, sz))
    {
        scaleTo->autorelease();
        return scaleTo;
    }

    delete scaleTo;
    return nullptr;
}

bool ScaleTo::initWithDuration(std::chrono::milliseconds duration, float s)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _endScaleX = s;
        _endScaleY = s;
        _endScaleZ = s;

        return true;
    }

    return false;
}

bool ScaleTo::initWithDuration(std::chrono::milliseconds duration, float sx, float sy)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _endScaleX = sx;
        _endScaleY = sy;
        _endScaleZ = 1.f;

        return true;
    }

    return false;
}

bool ScaleTo::initWithDuration(std::chrono::milliseconds duration, float sx, float sy, float sz)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _endScaleX = sx;
        _endScaleY = sy;
        _endScaleZ = sz;

        return true;
    }

    return false;
}

ScaleTo* ScaleTo::clone() const
{
    // no copy constructor
    return ScaleTo::create(_duration, _endScaleX, _endScaleY, _endScaleZ);
}

ScaleTo* ScaleTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in ScaleTo");
    return nullptr;
}

void ScaleTo::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _startScaleX = target->getScaleX();
    _startScaleY = target->getScaleY();
    _startScaleZ = target->getScaleZ();
    _deltaX = _endScaleX - _startScaleX;
    _deltaY = _endScaleY - _startScaleY;
    _deltaZ = _endScaleZ - _startScaleZ;
}

void ScaleTo::update(float time)
{
    if (_target)
    {
        _target->setScaleX(_startScaleX + _deltaX * time);
        _target->setScaleY(_startScaleY + _deltaY * time);
        _target->setScaleZ(_startScaleZ + _deltaZ * time);
    }
}

//
// ScaleBy
//

ScaleBy* ScaleBy::create(std::chrono::milliseconds duration, float s)
{
    ScaleBy* scaleBy = new (std::nothrow) ScaleBy();
    if (scaleBy && scaleBy->initWithDuration(duration, s))
    {
        scaleBy->autorelease();
        return scaleBy;
    }

    delete scaleBy;
    return nullptr;
}

ScaleBy* ScaleBy::create(std::chrono::milliseconds duration, float sx, float sy)
{
    ScaleBy* scaleBy = new (std::nothrow) ScaleBy();
    if (scaleBy && scaleBy->initWithDuration(duration, sx, sy, 1.f))
    {
        scaleBy->autorelease();
        return scaleBy;
    }

    delete scaleBy;
    return nullptr;
}

ScaleBy* ScaleBy::create(std::chrono::milliseconds duration, float sx, float sy, float sz)
{
    ScaleBy* scaleBy = new (std::nothrow) ScaleBy();
    if (scaleBy && scaleBy->initWithDuration(duration, sx, sy, sz))
    {
        scaleBy->autorelease();
        return scaleBy;
    }

    delete scaleBy;
    return nullptr;
}

ScaleBy* ScaleBy::clone() const
{
    // no copy constructor
    return ScaleBy::create(_duration, _endScaleX, _endScaleY, _endScaleZ);
}

void ScaleBy::startWithTarget(Node* target)
{
    ScaleTo::startWithTarget(target);
    _deltaX = _startScaleX * _endScaleX - _startScaleX;
    _deltaY = _startScaleY * _endScaleY - _startScaleY;
    _deltaZ = _startScaleZ * _endScaleZ - _startScaleZ;
}

ScaleBy* ScaleBy::reverse() const
{
    return ScaleBy::create(_duration, 1 / _endScaleX, 1 / _endScaleY, 1 / _endScaleZ);
}

//
// Blink
//

Blink* Blink::create(std::chrono::milliseconds duration, int blinks)
{
    Blink* blink = new (std::nothrow) Blink();
    if (blink && blink->initWithDuration(duration, blinks))
    {
        blink->autorelease();
        return blink;
    }

    delete blink;
    return nullptr;
}

bool Blink::initWithDuration(std::chrono::milliseconds duration, int blinks)
{
    CCASSERT(blinks >= 0, "blinks should be >= 0");
    if (blinks < 0)
    {
        log("Blink::initWithDuration error:blinks should be >= 0");
        return false;
    }

    if (ActionInterval::initWithDuration(duration) && blinks >= 0)
    {
        _times = blinks;
        return true;
    }

    return false;
}

void Blink::stop()
{
    if (nullptr != _target)
        _target->setVisible(_originalState);
    ActionInterval::stop();
}

void Blink::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _originalState = target->isVisible();
}

Blink* Blink::clone(void) const
{
    // no copy constructor
    return Blink::create(_duration, _times);
}

void Blink::update(float time)
{
    if (_target && !isDone())
    {
        float slice = 1.0f / _times;
        float m = fmodf(time, slice);
        _target->setVisible(m > slice / 2 ? true : false);
    }
}

Blink* Blink::reverse() const
{
    return Blink::create(_duration, _times);
}

//
// FadeIn
//

FadeIn* FadeIn::create(std::chrono::milliseconds d)
{
    FadeIn* action = new (std::nothrow) FadeIn();
    if (action && action->initWithDuration(d, 255.0f))
    {
        action->autorelease();
        return action;
    }

    delete action;
    return nullptr;
}

FadeIn* FadeIn::clone() const
{
    // no copy constructor
    return FadeIn::create(_duration);
}

void FadeIn::setReverseAction(cocos2d::FadeTo* ac)
{
    _reverseAction = ac;
}

FadeTo* FadeIn::reverse() const
{
    auto action = FadeOut::create(_duration);
    action->setReverseAction(const_cast<FadeIn*>(this));
    return action;
}

void FadeIn::startWithTarget(cocos2d::Node* target)
{
    ActionInterval::startWithTarget(target);

    if (nullptr != _reverseAction)
        this->_toOpacity = this->_reverseAction->_fromOpacity;
    else
        _toOpacity = 255.0f;

    if (target)
        _fromOpacity = target->getOpacity();
}

//
// FadeOut
//

FadeOut* FadeOut::create(std::chrono::milliseconds d)
{
    FadeOut* action = new (std::nothrow) FadeOut();
    if (action && action->initWithDuration(d, 0.0f))
    {
        action->autorelease();
        return action;
    }

    delete action;
    return nullptr;
}

FadeOut* FadeOut::clone() const
{
    // no copy constructor
    return FadeOut::create(_duration);
}

void FadeOut::startWithTarget(cocos2d::Node* target)
{
    ActionInterval::startWithTarget(target);

    if (nullptr != _reverseAction)
        _toOpacity = _reverseAction->_fromOpacity;
    else
        _toOpacity = 0.0f;

    if (target)
        _fromOpacity = target->getOpacity();
}

void FadeOut::setReverseAction(cocos2d::FadeTo* ac)
{
    _reverseAction = ac;
}

FadeTo* FadeOut::reverse() const
{
    auto action = FadeIn::create(_duration);
    action->setReverseAction(const_cast<FadeOut*>(this));
    return action;
}

//
// FadeTo
//

FadeTo* FadeTo::create(std::chrono::milliseconds duration, GLubyte opacity)
{
    FadeTo* fadeTo = new (std::nothrow) FadeTo();
    if (fadeTo && fadeTo->initWithDuration(duration, opacity))
    {
        fadeTo->autorelease();
        return fadeTo;
    }

    delete fadeTo;
    return nullptr;
}

bool FadeTo::initWithDuration(std::chrono::milliseconds duration, GLubyte opacity)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _toOpacity = opacity;
        return true;
    }

    return false;
}

FadeTo* FadeTo::clone() const
{
    // no copy constructor
    return FadeTo::create(_duration, _toOpacity);
}

FadeTo* FadeTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in FadeTo");
    return nullptr;
}

void FadeTo::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);

    if (target)
    {
        _fromOpacity = target->getOpacity();
    }
}

void FadeTo::update(float time)
{
    if (_target)
    {
        _target->setOpacity((GLubyte)(_fromOpacity + (_toOpacity - _fromOpacity) * time));
    }
}

//
// TintTo
//
TintTo* TintTo::create(std::chrono::milliseconds duration, GLubyte red, GLubyte green, GLubyte blue)
{
    TintTo* tintTo = new (std::nothrow) TintTo();
    if (tintTo && tintTo->initWithDuration(duration, red, green, blue))
    {
        tintTo->autorelease();
        return tintTo;
    }

    delete tintTo;
    return nullptr;
}

TintTo* TintTo::create(std::chrono::milliseconds duration, const Color3B& color)
{
    return create(duration, color.r, color.g, color.b);
}

bool TintTo::initWithDuration(std::chrono::milliseconds duration, GLubyte red, GLubyte green, GLubyte blue)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _to = Color3B(red, green, blue);
        return true;
    }

    return false;
}

TintTo* TintTo::clone() const
{
    // no copy constructor
    return TintTo::create(_duration, _to.r, _to.g, _to.b);
}

TintTo* TintTo::reverse() const
{
    CCASSERT(false, "reverse() not supported in TintTo");
    return nullptr;
}

void TintTo::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    if (_target)
    {
        _from = _target->getColor();
    }
}

void TintTo::update(float time)
{
    if (_target)
    {
        _target->setColor(
            Color3B(GLubyte(_from.r + (_to.r - _from.r) * time), (GLubyte)(_from.g + (_to.g - _from.g) * time), (GLubyte)(_from.b + (_to.b - _from.b) * time)));
    }
}

//
// TintBy
//

TintBy* TintBy::create(std::chrono::milliseconds duration, GLshort deltaRed, GLshort deltaGreen, GLshort deltaBlue)
{
    TintBy* tintBy = new (std::nothrow) TintBy();
    if (tintBy && tintBy->initWithDuration(duration, deltaRed, deltaGreen, deltaBlue))
    {
        tintBy->autorelease();
        return tintBy;
    }

    delete tintBy;
    return nullptr;
}

bool TintBy::initWithDuration(std::chrono::milliseconds duration, GLshort deltaRed, GLshort deltaGreen, GLshort deltaBlue)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _deltaR = deltaRed;
        _deltaG = deltaGreen;
        _deltaB = deltaBlue;

        return true;
    }

    return false;
}

TintBy* TintBy::clone() const
{
    // no copy constructor
    return TintBy::create(_duration, _deltaR, _deltaG, _deltaB);
}

void TintBy::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);

    if (target)
    {
        Color3B color = target->getColor();
        _fromR = color.r;
        _fromG = color.g;
        _fromB = color.b;
    }
}

void TintBy::update(float time)
{
    if (_target)
    {
        _target->setColor(Color3B((GLubyte)(_fromR + _deltaR * time), (GLubyte)(_fromG + _deltaG * time), (GLubyte)(_fromB + _deltaB * time)));
    }
}

TintBy* TintBy::reverse() const
{
    return TintBy::create(_duration, -_deltaR, -_deltaG, -_deltaB);
}

//
// DelayTime
//
DelayTime* DelayTime::create(std::chrono::milliseconds d)
{
    DelayTime* action = new (std::nothrow) DelayTime();
    if (action && action->initWithDuration(d))
    {
        action->autorelease();
        return action;
    }

    delete action;
    return nullptr;
}

DelayTime* DelayTime::clone() const
{
    // no copy constructor
    return DelayTime::create(_duration);
}

void DelayTime::update(float time)
{
    CC_UNUSED_PARAM(time);
    return;
}

DelayTime* DelayTime::reverse() const
{
    return DelayTime::create(_duration);
}

//
// ReverseTime
//

ReverseTime* ReverseTime::create(FiniteTimeAction* action)
{
    // casting to prevent warnings
    ReverseTime* reverseTime = new (std::nothrow) ReverseTime();
    if (reverseTime && reverseTime->initWithAction(action->clone()))
    {
        reverseTime->autorelease();
        return reverseTime;
    }

    delete reverseTime;
    return nullptr;
}

bool ReverseTime::initWithAction(FiniteTimeAction* action)
{
    CCASSERT(action != nullptr, "action can't be nullptr!");
    CCASSERT(action != _other, "action doesn't equal to _other!");
    if (action == nullptr || action == _other)
    {
        log("ReverseTime::initWithAction error: action is null or action equal to _other");
        return false;
    }

    if (ActionInterval::initWithDuration(action->getDuration()))
    {
        // Don't leak if action is reused
        CC_SAFE_RELEASE(_other);

        _other = action;
        action->retain();

        return true;
    }

    return false;
}

ReverseTime* ReverseTime::clone() const
{
    // no copy constructor
    return ReverseTime::create(_other->clone());
}

ReverseTime::ReverseTime()
: _other(nullptr)
{
}

ReverseTime::~ReverseTime()
{
    CC_SAFE_RELEASE(_other);
}

void ReverseTime::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _other->startWithTarget(target);
}

void ReverseTime::stop(void)
{
    _other->stop();
    ActionInterval::stop();
}

void ReverseTime::update(float p)
{
    if (_other != nullptr)
    {
        _other->update(1.f - p);
    }
}

ReverseTime* ReverseTime::reverse() const
{
    // FIXME: This looks like a bug
    return (ReverseTime*)_other->clone();
}

//
// Animate
//
Animate* Animate::create(Animation* animation)
{
    Animate* animate = new (std::nothrow) Animate();
    if (animate && animate->initWithAnimation(animation))
    {
        animate->autorelease();
        return animate;
    }

    delete animate;
    return nullptr;
}

Animate::Animate()
: _splitTimes(new std::vector<float>)
, _nextFrame(0)
, _origFrame(nullptr)
, _executedLoops(0)
, _animation(nullptr)
, _frameDisplayedEvent(nullptr)
, _currFrameIndex(0)
{
}

Animate::~Animate()
{
    CC_SAFE_RELEASE(_animation);
    CC_SAFE_RELEASE(_origFrame);
    CC_SAFE_DELETE(_splitTimes);
    CC_SAFE_RELEASE(_frameDisplayedEvent);
}

bool Animate::initWithAnimation(Animation* animation)
{
    CCASSERT(animation != nullptr, "Animate: argument Animation must be non-nullptr");
    if (animation == nullptr)
    {
        log("Animate::initWithAnimation: argument Animation must be non-nullptr");
        return false;
    }

    auto const singleDuration = animation->getDuration();

    if (ActionInterval::initWithDuration(singleDuration * animation->getLoops()))
    {
        _nextFrame = 0;
        setAnimation(animation);
        _origFrame = nullptr;
        _executedLoops = 0;

        _splitTimes->reserve(animation->getFrames().size());

        auto accumUnitsOfTime = 0ms;
        float newUnitOfTimeValue = static_cast<float>(singleDuration.count()) / animation->getTotalDelayUnits().count();

        auto& frames = animation->getFrames();
        for (auto& frame : frames)
        {
            float value = static_cast<float>((accumUnitsOfTime * newUnitOfTimeValue).count()) / singleDuration.count();
            accumUnitsOfTime += frame->getDelayUnits();
            _splitTimes->push_back(value);
        }
        return true;
    }
    return false;
}

void Animate::setAnimation(cocos2d::Animation* animation)
{
    if (_animation != animation)
    {
        CC_SAFE_RETAIN(animation);
        CC_SAFE_RELEASE(_animation);
        _animation = animation;
    }
}

Animate* Animate::clone() const
{
    // no copy constructor
    return Animate::create(_animation->clone());
}

void Animate::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    Sprite* sprite = static_cast<Sprite*>(target);

    CC_SAFE_RELEASE(_origFrame);

    if (_animation->getRestoreOriginalFrame())
    {
        _origFrame = sprite->getSpriteFrame();
        _origFrame->retain();
    }
    _nextFrame = 0;
    _executedLoops = 0;
}

void Animate::stop()
{
    if (_animation->getRestoreOriginalFrame() && _target)
    {
        auto blend = static_cast<Sprite*>(_target)->getBlendFunc();
        static_cast<Sprite*>(_target)->setSpriteFrame(_origFrame);
        static_cast<Sprite*>(_target)->setBlendFunc(blend);
    }

    ActionInterval::stop();
}

void Animate::update(float t)
{
    // if t==1, ignore. Animation should finish with t==1
    if (t < 1.0f)
    {
        t *= _animation->getLoops();

        // new loop?  If so, reset frame counter
        unsigned int loopNumber = (unsigned int)t;
        if (loopNumber > _executedLoops)
        {
            _nextFrame = 0;
            _executedLoops++;
        }

        // new t for animations
        t = fmodf(t, 1.0f);
    }

    auto& frames = _animation->getFrames();
    auto numberOfFrames = frames.size();
    SpriteFrame* frameToDisplay = nullptr;

    for (int i = _nextFrame; i < numberOfFrames; i++)
    {
        float splitTime = _splitTimes->at(i);

        if (splitTime <= t)
        {
            auto blend = static_cast<Sprite*>(_target)->getBlendFunc();
            _currFrameIndex = i;
            AnimationFrame* frame = frames.at(_currFrameIndex);
            frameToDisplay = frame->getSpriteFrame();
            static_cast<Sprite*>(_target)->setSpriteFrame(frameToDisplay);
            static_cast<Sprite*>(_target)->setBlendFunc(blend);

            const ValueMap& dict = frame->getUserInfo();
            if (!dict.empty())
            {
                if (_frameDisplayedEvent == nullptr)
                    _frameDisplayedEvent = new (std::nothrow) EventCustom(AnimationFrameDisplayedNotification);

                _frameDisplayedEventInfo.target = _target;
                _frameDisplayedEventInfo.userInfo = &dict;
                _frameDisplayedEvent->setUserData(&_frameDisplayedEventInfo);
                Director::getInstance()->getEventDispatcher()->dispatchEvent(_frameDisplayedEvent);
            }
            _nextFrame = i + 1;
        }
        // Issue 1438. Could be more than one frame per tick, due to low frame rate or frame delta < 1/FPS
        else
        {
            break;
        }
    }
}

Animate* Animate::reverse() const
{
    auto& oldArray = _animation->getFrames();
    Vector<AnimationFrame*> newArray(oldArray.size());

    if (!oldArray.empty())
    {
        for (auto iter = oldArray.crbegin(); iter != oldArray.crend(); ++iter)
        {
            AnimationFrame* animFrame = *iter;
            if (!animFrame)
            {
                break;
            }

            newArray.pushBack(animFrame->clone());
        }
    }

    Animation* newAnim = Animation::create(newArray, _animation->getDelayPerUnit(), _animation->getLoops());
    newAnim->setRestoreOriginalFrame(_animation->getRestoreOriginalFrame());
    return Animate::create(newAnim);
}

// TargetedAction

TargetedAction::TargetedAction()
: _action(nullptr)
, _forcedTarget(nullptr)
{
}

TargetedAction::~TargetedAction()
{
    // COCOS
    //    CC_SAFE_RELEASE(_forcedTarget);
    //    CC_SAFE_RELEASE(_action);

    // PRETTY SIMPLE
    // Reverse order for Newyork compatibility
    // when _action is a GameSpriteFilterAction
    // if _forcedTarget is released before the action (ref=0), then the gameSpriteFilterAction will brake as it still requires the _forcedTarget to be valide
    // when destroyed
    CC_SAFE_RELEASE(_action);
    CC_SAFE_RELEASE(_forcedTarget);
}

TargetedAction* TargetedAction::create(Node* target, FiniteTimeAction* action)
{
    TargetedAction* p = new (std::nothrow) TargetedAction();
    if (p && p->initWithTarget(target, action))
    {
        p->autorelease();
        return p;
    }

    delete p;
    return nullptr;
}

bool TargetedAction::initWithTarget(Node* target, FiniteTimeAction* action)
{
    if (ActionInterval::initWithDuration(action->getDuration()))
    {
        CC_SAFE_RETAIN(target);
        _forcedTarget = target;
        CC_SAFE_RETAIN(action);
        _action = action;
        return true;
    }
    return false;
}

TargetedAction* TargetedAction::clone() const
{
    // no copy constructor
    // win32 : use the _other's copy object.
    return TargetedAction::create(_forcedTarget, _action->clone());
}

TargetedAction* TargetedAction::reverse() const
{
    // just reverse the internal action
    auto a = new (std::nothrow) TargetedAction();
    a->initWithTarget(_forcedTarget, _action->reverse());
    a->autorelease();
    return a;
}

void TargetedAction::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _action->startWithTarget(_forcedTarget);
}

void TargetedAction::stop()
{
    _action->stop();
}

void TargetedAction::update(float time)
{
    _action->update(time);
}

void TargetedAction::step(float dt)
{
    _action->step(dt);
}

bool TargetedAction::isDone() const
{
    return _action->isDone();
}

void TargetedAction::setForcedTarget(Node* forcedTarget)
{
    if (_forcedTarget != forcedTarget)
    {
        CC_SAFE_RETAIN(forcedTarget);
        CC_SAFE_RELEASE(_forcedTarget);
        _forcedTarget = forcedTarget;
    }
}

// ActionFloat

ActionFloat* ActionFloat::create(std::chrono::milliseconds duration, float from, float to, ActionFloatCallback callback)
{
    auto ref = new (std::nothrow) ActionFloat();
    if (ref && ref->initWithDuration(duration, from, to, callback))
    {
        ref->autorelease();
        return ref;
    }

    delete ref;
    return nullptr;
}

bool ActionFloat::initWithDuration(std::chrono::milliseconds duration, float from, float to, ActionFloatCallback callback)
{
    if (ActionInterval::initWithDuration(duration))
    {
        _from = from;
        _to = to;
        _callback = callback;
        return true;
    }
    return false;
}

ActionFloat* ActionFloat::clone() const
{
    return ActionFloat::create(_duration, _from, _to, _callback);
}

void ActionFloat::startWithTarget(Node* target)
{
    ActionInterval::startWithTarget(target);
    _delta = _to - _from;
}

void ActionFloat::update(float delta)
{
    float value = _to - _delta * (1 - delta);

    if (_callback)
    {
        // report back value to caller
        _callback(value);
    }
}

ActionFloat* ActionFloat::reverse() const
{
    return ActionFloat::create(_duration, _to, _from, _callback);
}

NS_CC_END
