/*
 * Copyright (c) 2012 cocos2d-x.org
 * http://www.cocos2d-x.org
 *
 * Copyright 2012 Stewart Hamilton-Arrandale.
 * http://creativewax.co.uk
 *
 * Modified by Yannick Loriot.
 * http://yannickloriot.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Converted to c++ / cocos2d-x by Angus C
 */

#ifndef CC_EXTENSIONS_GUI_CONTROLEXTENSION_COLOURPICKER_H
#define CC_EXTENSIONS_GUI_CONTROLEXTENSION_COLOURPICKER_H

#include <cocos/extensions/ExtensionExport.h>
#include <cocos/extensions/ExtensionMacros.h>
#include <cocos/extensions/GUI/CCControlExtension/CCControl.h>
#include <cocos/extensions/GUI/CCControlExtension/CCControlUtils.h>

namespace cocos2d
{
    class Event;
    class Ref;
    class Sprite;
    class Touch;
    struct Color3B;
} // namespace cocos2d

NS_CC_EXT_BEGIN

class ControlHuePicker;
class ControlSaturationBrightnessPicker;

/**
 * @addtogroup GUI
 * @{
 * @addtogroup control_extension
 * @{
 */

class CC_EX_DLL ControlColourPicker : public Control
{
public:
    static ControlColourPicker* create();
    /**
     * @js ctor
     * @lua new
     */
    ControlColourPicker();
    /**
     * @js NA
     * @lua NA
     */
    ~ControlColourPicker() override;

    bool init() override;

    void setColor(const Color3B& colorValue) override;
    void setEnabled(bool bEnabled) override;

    // virtual ~ControlColourPicker();
    void hueSliderValueChanged(Ref* sender, Control::EventType controlEvent);
    void colourSliderValueChanged(Ref* sender, Control::EventType controlEvent);

    virtual ControlSaturationBrightnessPicker* getcolourPicker() const noexcept { return _colourPicker; }
    virtual void setcolourPicker(ControlSaturationBrightnessPicker* colourPicker) noexcept;

    virtual ControlHuePicker* getHuePicker() const noexcept { return _huePicker; }
    virtual void setHuePicker(ControlHuePicker* huePicker) noexcept;

    virtual Sprite* getBackground() const noexcept { return _background; }
    virtual void setBackground(Sprite* background) noexcept;

protected:
    void updateControlPicker();
    void updateHueAndControlPicker();
    bool onTouchBegan(Touch* touch, Event* pEvent) override;

    HSV _hsv;
    ControlSaturationBrightnessPicker* _colourPicker = nullptr;
    ControlHuePicker* _huePicker = nullptr;
    Sprite* _background = nullptr;
};

// end of GUI group
/// @}
/// @}

NS_CC_EXT_END

#endif // CC_EXTENSIONS_GUI_CONTROLEXTENSION_COLOURPICKER_H
