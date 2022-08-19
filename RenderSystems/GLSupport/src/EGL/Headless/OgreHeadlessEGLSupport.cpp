/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2008 Renato Araujo Oliveira Filho <renatox@gmail.com>
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

#include "OgreHeadlessEGLSupport.h"
#include "OgreHeadlessEGLWindow.h"

#include "OgreGLUtil.h"

namespace Ogre {
    GLNativeSupport* getGLSupport(int profile)
    {
        return new HeadlessEGLSupport(profile);
    }

    HeadlessEGLSupport::HeadlessEGLSupport(int profile) : EGLSupport(profile)
    {
        mRandr = false;
        mNativeDisplay = EGL_DEFAULT_DISPLAY;

        // This call has side effects that we need. In particular, it calls
        // eglIntialize() the first time it's called. It also fills mGLDisplay for us.
        getGLDisplay();

        // With headless rendering there is no display.
        // Yet, we still have to fill this. So, fill it with zeros.
        mCurrentMode.first.first = 0;
        mCurrentMode.first.second = 0;
        mCurrentMode.second = 0;

        mOriginalMode = mCurrentMode;

        mVideoModes.push_back(mCurrentMode);

        EGLConfig *glConfigs;
        int config, nConfigs = 0;

        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_NONE
        };

        glConfigs = chooseGLConfig(attribs, &nConfigs);

        for (config = 0; config < nConfigs; config++)
        {
            int caveat, samples;

            getGLConfigAttrib(glConfigs[config], EGL_CONFIG_CAVEAT, &caveat);

            if (caveat != EGL_SLOW_CONFIG)
            {
                getGLConfigAttrib(glConfigs[config], EGL_SAMPLES, &samples);
                mSampleLevels.push_back(StringConverter::toString(samples));
            }
        }

        free(glConfigs);

        removeDuplicates(mSampleLevels);
    }

    HeadlessEGLSupport::~HeadlessEGLSupport()
    {
        if (mGLDisplay)
        {
            eglTerminate(mGLDisplay);
        }
    }

    void HeadlessEGLSupport::switchMode(uint& width, uint& height, short& frequency)
    {
        LogManager::getSingleton().logMessage(
            "HeadlessEGLSupport::switchMode: operation not supported");
    }

    RenderWindow* HeadlessEGLSupport::newWindow(const String &name,
                                                unsigned int width, unsigned int height,
                                                bool fullScreen,
                                                const NameValuePairList *miscParams)
    {
        EGLWindow* window = new HeadlessEGLWindow(this);
        window->create(name, width, height, fullScreen, miscParams);

        return window;
    }
}
