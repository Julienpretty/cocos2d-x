/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies

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

#include <cocos/base/CCRef.h>

#include <cocos/base/CCAutoreleasePool.h>
#include <cocos/base/ccMacros.h>
#include <cocos/platform/CCPlatformMacros.h>

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
#    include <algorithm>
#    include <cstddef>
#    include <mutex>
#    include <thread>
#    include <vector>

#    ifdef __APPLE__
#        include <execinfo.h> // For backtrace on IOS
#    endif
#    ifdef __ANDROID__
#        include <dlfcn.h>
#        include <unwind.h> // For backtrace on Android
#    endif
#    include <cxxabi.h> // For demagling

static constexpr std::size_t const MAX_CALLSTACK_SIZE = 32;

string demangle(string call)
{
    int status;
    char* ret = abi::__cxa_demangle(call.c_str(), nullptr, nullptr, &status);

    if (status == 0)
        call = std::string(ret);
    if (ret)
        free(ret);
    return call;
}

#    ifdef __APPLE__
std::string demangle_ios(std::string const& call);
#    endif

#    ifdef __ANDROID__
struct android_backtrace_state
{
    void** current;
    void** end;
};

_Unwind_Reason_Code android_unwind_callback(struct _Unwind_Context* context, void* arg)
{
    android_backtrace_state* state = (android_backtrace_state*)arg;
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc)
    {
        if (state->current == state->end)
            return _URC_END_OF_STACK;
        else
            *state->current++ = reinterpret_cast<void*>(pc);
    }
    return _URC_NO_REASON;
}
#    endif

std::vector<std::string> currentCallStack()
{
    std::vector<std::string> stack;
    void* callstack[MAX_CALLSTACK_SIZE];

#    ifdef __APPLE__
    int i, frames = backtrace(callstack, MAX_CALLSTACK_SIZE);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i)
        stack.push_back(demangle_ios(strs[i]));
    free(strs);
#    endif

#    ifdef __ANDROID__
    android_backtrace_state state;
    state.current = callstack;
    state.end = callstack + MAX_CALLSTACK_SIZE;

    _Unwind_Backtrace(android_unwind_callback, &state);

    int count = (int)(state.current - callstack);

    for (int idx = 0; idx < count; idx++)
    {
        const void* addr = callstack[idx];
        const char* symbol = "";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname)
            symbol = info.dli_sname;
        else
            break;

        stack.push_back(to_string(idx) + " " + demangle(string(symbol)));
    }
#    endif

    return stack;
}

#    ifdef __APPLE__
std::string demangle_ios(std::string const& call)
{
    auto mangled = call;
    auto endName = mangled.find_last_of(" + ");

    if (endName != std::string::npos)
    {
        mangled = call.substr(0, endName - 2);
        auto beginName = mangled.find_last_of(" ");
        if (beginName != string::npos)
        {
            mangled = mangled.substr(beginName + 1, mangled.size() - 1);
            call = call.substr(0, beginName + 1) + demangle(mangled) + call.substr(endName - 2, call.size() - 1);
        }
    }
    return call;
}
#    endif

#endif

NS_CC_BEGIN

Clonable::~Clonable()
{
}

#if CC_ENABLE_SCRIPT_BINDING
static unsigned int uObjectCount = 0;
#endif

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
static void trackRef(Ref* ref);
static void untrackRef(Ref* ref);
#endif

Ref::Ref()
{
#if CC_ENABLE_SCRIPT_BINDING
    _ID = ++uObjectCount;
#endif

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
    trackRef(this);
#endif
}

Ref::~Ref()
{
#if CC_ENABLE_SCRIPT_BINDING
    // if the object is referenced by Lua engine, remove it
    if (_luaID)
    {
        ScriptEngineManager::getInstance()->getScriptEngine()->removeScriptObjectByObject(this);
    }
#    if !CC_ENABLE_GC_FOR_NATIVE_OBJECTS
    else
    {
        ScriptEngineProtocol* pEngine = ScriptEngineManager::getInstance()->getScriptEngine();
        if (pEngine != nullptr && pEngine->getScriptType() == kScriptTypeJavascript)
        {
            pEngine->removeScriptObjectByObject(this);
        }
    }
#    endif // !CC_ENABLE_GC_FOR_NATIVE_OBJECTS
#endif // CC_ENABLE_SCRIPT_BINDING

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
    if (_referenceCount != 0)
        untrackRef(this);
#endif
}

Ref::Ref(const Ref& other)
: _referenceCount(1)
{
#if CC_ENABLE_SCRIPT_BINDING
    _ID = ++uObjectCount;
    _luaID = 0;
    _scriptObject = nullptr;
    _rooted = false;
#endif
}

Ref& Ref::operator=(const Ref& other)
{
    _referenceCount = 1;
#if CC_ENABLE_SCRIPT_BINDING
    _ID = ++uObjectCount;
    _luaID = 0;
    _scriptObject = nullptr;
    _rooted = false;
#endif
    return *this;
}

void Ref::retain()
{
#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
    if (_trackRetainRelease)
        _retainList.emplace_back(currentCallStack());
#endif
    CCASSERT(_referenceCount > 0, "reference count should be greater than 0");
    ++_referenceCount;
}

void Ref::release()
{
#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
    if (_trackRetainRelease)
        _releaseList.emplace_back(currentCallStack());
#endif
    CCASSERT(_referenceCount > 0, "reference count should be greater than 0");
    --_referenceCount;

    if (_referenceCount == 0)
    {
#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
        auto poolManager = PoolManager::getInstance();
        if (!poolManager->getCurrentPool()->isClearing() && poolManager->isObjectInPools(this))
        {
            // Trigger an assert if the reference count is 0 but the Ref is still in autorelease pool.
            // This happens when 'autorelease/release' were not used in pairs with 'new/retain'.
            //
            // Wrong usage (1):
            //
            // auto obj = Node::create();   // Ref = 1, but it's an autorelease Ref which means it was in the autorelease pool.
            // obj->autorelease();   // Wrong: If you wish to invoke autorelease several times, you should retain `obj` first.
            //
            // Wrong usage (2):
            //
            // auto obj = Node::create();
            // obj->release();   // Wrong: obj is an autorelease Ref, it will be released when clearing current pool.
            //
            // Correct usage (1):
            //
            // auto obj = Node::create();
            //                     |-   new Node();     // `new` is the pair of the `autorelease` of next line
            //                     |-   autorelease();  // The pair of `new Node`.
            //
            // obj->retain();
            // obj->autorelease();  // This `autorelease` is the pair of `retain` of previous line.
            //
            // Correct usage (2):
            //
            // auto obj = Node::create();
            // obj->retain();
            // obj->release();   // This `release` is the pair of `retain` of previous line.
            CCASSERT(false, "The reference shouldn't be 0 because it is still in autorelease pool.");
        }
#endif

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0
        untrackRef(this);
#endif
        delete this;
    }
}

Ref* Ref::autorelease()
{
    PoolManager::getInstance()->getCurrentPool()->addObject(this);
    return this;
}

unsigned int Ref::getReferenceCount() const
{
    return _referenceCount;
}

#if defined(CC_REF_LEAK_DETECTION) && CC_REF_LEAK_DETECTION > 0

static std::vector<Ref*> __refAllocationList;
static std::mutex __refMutex;

void Ref::printLeaks()
{
    std::lock_guard<std::mutex> refLockGuard(__refMutex);
    // Dump Ref object memory leaks
    if (__refAllocationList.empty())
    {
        log("[memory] All Ref objects successfully cleaned up (no leaks detected).\n");
    }
    else
    {
        log("[memory] WARNING: %d Ref objects still active in memory.\n", (int)__refAllocationList.size());

        for (const auto& ref : __refAllocationList)
        {
            CC_ASSERT(ref);
            const char* type = typeid(*ref).name();
            log("[memory] LEAK: Ref object '%s' still active with reference count %d.\n", (type ? type : ""), ref->getReferenceCount());
        }
    }
}

static void trackRef(Ref* ref)
{
    std::lock_guard<std::mutex> refLockGuard(__refMutex);
    CCASSERT(ref, "Invalid parameter, ref should not be null!");

    // Create memory allocation record.
    __refAllocationList.push_back(ref);
}

static void untrackRef(Ref* ref)
{
    std::lock_guard<std::mutex> refLockGuard(__refMutex);
    auto iter = std::find(__refAllocationList.begin(), __refAllocationList.end(), ref);
    if (iter == __refAllocationList.end())
    {
        log("[memory] CORRUPTION: Attempting to free (%s) with invalid ref tracking record.\n", typeid(*ref).name());
        return;
    }

    __refAllocationList.erase(iter);
}

#endif // CC_REF_LEAK_DETECTION

NS_CC_END
