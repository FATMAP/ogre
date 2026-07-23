/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
-----------------------------------------------------------------------------
*/

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreRoot.h"

#include "OgreOSXEGLSupport.h"
#include "OgreOSXEGLWindow.h"
#include "OgreGLUtil.h"

#import <AppKit/NSScreen.h>

namespace Ogre {

    GLNativeSupport* getGLSupport(int)
    {
        return new OSXEGLSupport();
    }

    OSXEGLSupport::OSXEGLSupport() : EGLSupport(CONTEXT_ES)
    {
        mNativeDisplay = EGL_DEFAULT_DISPLAY;
        mGLDisplay = getGLDisplay();
    }

    ConfigOptionMap OSXEGLSupport::getConfigOptions()
    {
        ConfigOptionMap mOptions = EGLSupport::getConfigOptions();

        ConfigOption optFullScreen;
        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back("Yes");
        optFullScreen.possibleValues.push_back("No");
        optFullScreen.currentValue = "No";
        optFullScreen.immutable = false;
        mOptions[optFullScreen.name] = optFullScreen;

        ConfigOption optVideoMode;
        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;

        // Get available resolutions from the main screen
        NSScreen *mainScreen = [NSScreen mainScreen];
        NSDictionary *screenDescription = [mainScreen deviceDescription];
        NSSize displayPixelSize = [[screenDescription objectForKey:NSDeviceSize] sizeValue];

        // Add common resolutions
        optVideoMode.possibleValues.push_back("800 x 600");
        optVideoMode.possibleValues.push_back("1024 x 768");
        optVideoMode.possibleValues.push_back("1280 x 720");
        optVideoMode.possibleValues.push_back("1280 x 1024");
        optVideoMode.possibleValues.push_back("1600 x 900");
        optVideoMode.possibleValues.push_back("1920 x 1080");
        optVideoMode.possibleValues.push_back("2560 x 1440");

        // Add native resolution
        String nativeResolution = StringConverter::toString((int)displayPixelSize.width) + " x " +
                                  StringConverter::toString((int)displayPixelSize.height);
        optVideoMode.possibleValues.push_back(nativeResolution);
        optVideoMode.currentValue = nativeResolution;

        mOptions[optVideoMode.name] = optVideoMode;

        ConfigOption optContentScaling;
        optContentScaling.name = "Content Scaling Factor";
        optContentScaling.immutable = false;
        optContentScaling.possibleValues.push_back("1.0");
        optContentScaling.possibleValues.push_back("2.0");
        optContentScaling.currentValue = "1.0";
        mOptions[optContentScaling.name] = optContentScaling;

        return mOptions;
    }

    OSXEGLSupport::~OSXEGLSupport()
    {
    }

    RenderWindow* OSXEGLSupport::newWindow(
        const String &name, unsigned int width, unsigned int height,
        bool fullScreen, const NameValuePairList *miscParams)
    {
        OSXEGLWindow* window = new OSXEGLWindow(this);
        window->create(name, width, height, fullScreen, miscParams);

        return window;
    }
}
