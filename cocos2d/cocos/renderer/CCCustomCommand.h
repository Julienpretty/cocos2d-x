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

#ifndef CC_RENDERER_CUSTOMCOMMAND_H
#define CC_RENDERER_CUSTOMCOMMAND_H

#include <cocos/platform/CCPlatformDefine.h>
#include <cocos/platform/CCPlatformMacros.h>
#include <cocos/renderer/CCRenderCommand.h>

#include <cstdint>
#include <functional>

/**
 * @addtogroup renderer
 * @{
 */

NS_CC_BEGIN

class Mat4;

/**
Custom command is used for call custom openGL command which can not be done by other commands,
such as stencil function, depth functions etc. The render command is executed by calling a call back function.
*/
class CC_DLL CustomCommand : public RenderCommand
{
    /**Callback function.*/
    std::function<void()> _func = nullptr;

public:
    CustomCommand();
    CustomCommand(std::function<void()> const& func);
    CustomCommand(CustomCommand const&) = delete;
    CustomCommand& operator=(CustomCommand const&) = delete;
    CustomCommand(CustomCommand&&) noexcept = default;
    CustomCommand& operator=(CustomCommand&&) noexcept = delete;
    ~CustomCommand() override = default;

public:
    /**
     Init function.
     @param globalZOrder GlobalZOrder of the render command.
     @param modelViewTransform When in 3D mode, depth sorting needs modelViewTransform.
     @param flags Use to identify that the render command is 3D mode or not.
     */
    void init(float globalZOrder, const Mat4& modelViewTransform, std::uint32_t flags) final;
    /**
    Init function. The render command will be in 2D mode.
    @param globalZOrder GlobalZOrder of the render command.
    */
    void init(float globalZOrder);

    /**
    Execute the render command and call callback functions.
    */
    void execute();

    inline void setFunc(std::function<void()> const& func) noexcept { _func = func; }
};

NS_CC_END
/**
 end of support group
 @}
 */
#endif // CC_RENDERER_CUSTOMCOMMAND_H
