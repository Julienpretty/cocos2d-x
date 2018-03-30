#ifndef __CC_EGLViewIMPL_EMSCRIPTEN_H__
#define __CC_EGLViewIMPL_EMSCRIPTEN_H__

#include "base/CCRef.h"
#include "platform/CCCommon.h"
#include "platform/CCGLView.h"
#include "platform/CCGL.h"
#include "math/CCGeometry.h"
#include "CCInjectMouseMove-emscripten.h"

#include <string>
#include <emscripten/html5.h>

NS_CC_BEGIN


class CC_DLL GLViewImpl : public GLView
{
protected:
    GLViewImpl();
    GLViewImpl(GLViewImpl&& other) noexcept =delete;
    GLViewImpl& operator =(GLViewImpl&& other) noexcept =delete;
    GLViewImpl(const GLViewImpl& other) =delete;
    GLViewImpl& operator =(const GLViewImpl& other) =delete;
    ~GLViewImpl();

public:
    static GLViewImpl* create(const std::string& viewName);

    void end() override;
    bool isOpenGLReady() override;
    void swapBuffers() override;
    void setIMEKeyboardState(bool bOpen) override; //TODO
    bool windowShouldClose() override;
	[[deprecated("Use setCursorShape() instead!")]]
    void setCursorVisible(bool isVisible) override;
    void setCursorShape(CursorShape shape) override;
    int getRetinaFactor() const override;
    bool isRetinaDisplay() const override;

    void setViewPortInPoints(float x , float y , float w , float h) override;
    void setScissorInPoints(float x , float y , float w , float h) override;
    Rect getScissorRect() const override;


	// See important setFullscreen() comments below!
	bool	getFullscreen() const noexcept override { return _fullscreen; }

	// Returns true if the requested state doesn't match the current state, and the request was queued.
	// Request is asynchronous by nature. A getFullScreen() call immediately following the setFullscreen() call
	// shall not report the new state correctly - until the fullscreen change event was received (or not) from the browser.
	// If you need quick feedback, subscribe to the EVENT_FULLSCREEN_CHANGED event.
	// Beware that the request may silently fail if this is not invoked from a direct touch action, due
	// to browser security model!
	bool	setFullscreen(bool) noexcept override;


private:

	void	em_webglContextLostEvent() noexcept;
	void	em_webglContextRestoredEvent() noexcept;
	void	em_fullscreenEvent(const EmscriptenFullscreenChangeEvent *) noexcept;
	void	em_mouseEvent(int eventType, const EmscriptenMouseEvent *e) noexcept;
	void	em_wheelEvent(const EmscriptenWheelEvent *) noexcept;

	void	handleMouseMove(float, float) noexcept;
	void	handleMouseOut() noexcept;
	void	handleMouseScroll(float, float, float, float) noexcept;



    void createEGLContext() noexcept;
    void deleteEGLContext() noexcept;

    void registerEvents() noexcept;
    void unregisterEvents() noexcept;
    void updateCanvasSize(int width, int height) noexcept;

public:
	static constexpr const char *EVENT_WINDOW_RESIZED = "glview_window_resized";
	static constexpr const char *EVENT_FULLSCREEN_CHANGED = "glview_fullscreen_changed";

private:
    EGLDisplay _display;
    EGLContext _context;
    EGLSurface _surface;
    EGLConfig _config;

    float _retinaFactor;

	CursorShape		_currentCursorShape;
	InjectMouseMove	_mouseMoveInjector;
	bool			_mouseCaptured;

	bool			_fullscreen;

    cocos2d::Size	_screenSizeBeforeFullscreen;
};

NS_CC_END // end of namespace cocos2d

#endif  // end of __CC_EGLViewIMPL_EMSCRIPTEN_H__
