/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2022 FATMAP

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

#include <EGL/eglext.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Ogre {
    GLNativeSupport* getGLSupport(int profile)
    {
        return new HeadlessEGLSupport(profile);
    }

    HeadlessEGLSupport::HeadlessEGLSupport(int profile) : EGLSupport(profile)
    {
        mGLDisplay = selectEglDisplay();

        if (eglInitialize(mGLDisplay, &mEGLMajor, &mEGLMinor) == EGL_FALSE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Couldn`t initialize EGLDisplay.",
                        __FUNCTION__);
        }

        // This only works after eglInitialize().
        printPlatformInfo(mGLDisplay);

        // With headless rendering there is no display.
        // Yet, we still have to fill this. So, fill it with zeros.
        mCurrentMode = {};

        mVideoModes.push_back(mCurrentMode);

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
                mFSAALevels.push_back(samples);
            }
        }

        free(glConfigs);
    }

    HeadlessEGLSupport::~HeadlessEGLSupport()
    {
        if (mGLDisplay)
        {
            eglTerminate(mGLDisplay);
        }
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

    ::EGLDisplay HeadlessEGLSupport::selectEglDisplay()
    {
        // No, we can't make the OGRE_HEADLESS_EGL_DEVICE_IDX a window parameter.
        // If we did, different windows may be assigned different EGL devices,
        // and that would mean different EGLDisplay values for different windows.
        // Ogre doesn't support such a setup.
        if (const char* eglDeviceIdxStr = getenv("OGRE_HEADLESS_EGL_DEVICE_IDX"))
        {
            const int eglDeviceIdx = StringConverter::parseInt(eglDeviceIdxStr, -1);
            if (eglDeviceIdx < 0)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "Invalid value in OGRE_HEADLESS_EGL_DEVICE_IDX environment variable. "
                            "A non-negative integer was expected.",
                            __FUNCTION__);
            }

#if defined(EGL_EXT_device_base)
            auto eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress(
                "eglQueryDevicesEXT");
            if (!eglQueryDevicesEXT)
#endif
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "OGRE_HEADLESS_EGL_DEVICE_IDX environment variable is set, yet this "
                            "EGL implementation misses the device enumeration extension.",
                            __FUNCTION__);
            }

#if defined(EGL_EXT_device_base)
            EGLint numDevices = 0;
            eglQueryDevicesEXT(0, nullptr, &numDevices);

            std::vector<EGLDeviceEXT> devices(numDevices);
            eglQueryDevicesEXT(numDevices, devices.data(), &numDevices);

            if (eglDeviceIdx >= numDevices)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                            "The EGL device requested by the OGRE_HEADLESS_EGL_DEVICE_IDX "
                            "environment variable doesn't exist.",
                            __FUNCTION__);
            }

            printf("Selecting EGL device #%d, requested by OGRE_HEADLESS_EGL_DEVICE_IDX\n",
                   (int)eglDeviceIdx);

            // In case it crashes shortly after.
            fflush(stdout);

            return eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT,
                                         devices[eglDeviceIdx], nullptr);
#endif
        }

        printf("Selecting the default EGL platform. Use the OGRE_HEADLESS_EGL_DEVICE_IDX "
               "environment variable to force a particular EGL device. Use eglinfo to list "
               "EGL devices.\n");

        // In case it crashes shortly after.
        fflush(stdout);

        return eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }

    void HeadlessEGLSupport::printPlatformInfo(EGLDisplay dpy)
    {
        printf("EGL vendor string: %s\n", eglQueryString(dpy, EGL_VENDOR));

#ifdef EGL_MESA_query_driver
        const char* extensions = eglQueryString(dpy, EGL_EXTENSIONS);
        if (extensions && strstr(extensions, "EGL_MESA_query_driver"))
        {
            auto eglGetDisplayDriverName = (PFNEGLGETDISPLAYDRIVERNAMEPROC)eglGetProcAddress(
                "eglGetDisplayDriverName");

            if (eglGetDisplayDriverName)
            {
                printf("EGL driver name: %s\n", eglGetDisplayDriverName(dpy));
            }
        }
#endif
        // In case it crashes shortly after.
        fflush(stdout);
    }
}
