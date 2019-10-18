/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
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

#ifndef CC_PLATFORM_IOS_VIEWIMPLIOS_H
#define CC_PLATFORM_IOS_VIEWIMPLIOS_H

#include <cocos/platform/CCPlatformConfig.h>
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#    include <cocos/base/CCRef.h>
#    include <cocos/platform/CCCommon.h>
#    include <cocos/platform/CCGLView.h>

#    include <cmath>
#    include <limits>

NS_CC_BEGIN

/** Class that represent the OpenGL View
 */
class CC_DLL GLViewImpl : public GLView
{
public:
    /** creates a GLViewImpl with a objective-c CCEAGLViewImpl instance */
    static GLViewImpl* createWithEAGLView(void* eaGLView);

    /** creates a GLViewImpl with a title name in fullscreen mode */
    static GLViewImpl* create(const std::string& viewName);

    /** creates a GLViewImpl with a title name, a rect and the zoom factor */
    static GLViewImpl* createWithRect(const std::string& viewName, const Rect& rect, float frameZoomFactor = 1.0f);

    /** creates a GLViewImpl with a name in fullscreen mode */
    static GLViewImpl* createWithFullScreen(const std::string& viewName);

    static void convertAttrs();
    static void* _pixelFormat;
    static int _depthFormat;

    /** sets the content scale factor */
    bool setContentScaleFactor(float contentScaleFactor) override;

    /** returns the content scale factor */
    float getContentScaleFactor() const override;

    /** returns whether or not the view is in Retina Display mode */
    bool isRetinaDisplay() const override { return std::abs(getContentScaleFactor() - 2.0f) < std::numeric_limits<float>::epsilon(); }

    /** returns the objective-c CCEAGLView instance */
    void* getEAGLView() const override { return _eaglview; }

    // overrides
    bool isOpenGLReady() override;
    void end() override;
    void swapBuffers() override;
    void setIMEKeyboardState(bool bOpen) override;

protected:
    GLViewImpl();
    ~GLViewImpl() override;

    bool initWithEAGLView(void* eaGLView);
    bool initWithRect(const std::string& viewName, const Rect& rect, float frameZoomFactor);
    bool initWithFullScreen(const std::string& viewName);

    // the objective-c CCEAGLView instance
    void* _eaglview;
};

NS_CC_END

#endif // CC_PLATFORM_IOS

#endif // CC_PLATFORM_IOS_VIEWIMPLIOS_H
