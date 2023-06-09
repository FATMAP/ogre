/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Copyright (c) 2023 FATMAP

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

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include "OgreGLRenderSystemCommon.h"

#include "OgreAppleEmbeddedEGLSupport.h"
#include "OgreAppleEmbeddedEGLWindow.h"
#include "OgreViewport.h"

#include <UIKit/UIKit.h>

#include <iostream>
#include <algorithm>
#include <climits>
#include <cmath>

namespace Ogre {
    AppleEmbeddedEGLWindow::AppleEmbeddedEGLWindow(AppleEmbeddedEGLSupport *glsupport)
        : EGLWindow(glsupport),
          mView(nil)
    {
    }

    void AppleEmbeddedEGLWindow::resize(uint width, uint height)
    {
        // Convert from points to pixels.
        width = std::max<long>(1, std::lround(width * mContentScalingFactor));
        height = std::max<long>(1, std::lround(height * mContentScalingFactor));

        if (!mActive || (mWidth == width && mHeight == height))
            return;

        mWidth = width;
        mHeight = height;

        // Notify viewports of resize
        ViewportList::iterator it = mViewportList.begin();
        while (it != mViewportList.end())
            (*it++).second->_updateDimensions();
    }

    void AppleEmbeddedEGLWindow::windowMovedOrResized()
    {
        if(mActive)
        {
            CGRect frame = [mView frame];
            CGFloat width  = frame.size.width;
            CGFloat height = frame.size.height;

            resize(width, height);
        }
    }

    void AppleEmbeddedEGLWindow::create(const String& name, uint width, uint height,
                                        bool fullScreen, const NameValuePairList *miscParams)
    {
        // Convert from points to pixels.
        width = std::max<long>(1, std::lround(width * mContentScalingFactor));
        height = std::max<long>(1, std::lround(height * mContentScalingFactor));

        mName = name;
        mWidth = width;
        mHeight = height;
        mLeft = 0;
        mTop = 0;
        mIsFullScreen = fullScreen;
        mIsExternal = true;  // We only support external windows.

        if (miscParams)
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            if ((opt = miscParams->find("externalViewHandle")) != end)
            {
                mView = (__bridge UIView *)(void*)StringConverter::parseSizeT(opt->second);
                mUsingExternalView = true;
                LogManager::getSingleton().logMessage("iOS: Using an external view handle");
            }

            if ((opt = miscParams->find("externalViewControllerHandle")) != end)
            {
                UIViewController* viewController =
                    (__bridge UIViewController *)(void*)StringConverter::parseSizeT(opt->second);
                if (viewController.view != nil)
                {
                    mView = (UIView *)viewController.view;
                    mUsingExternalView = true;
                    LogManager::getSingleton().logMessage(
                        "iOS: Using an external view handle obtained from the external view controller");
                }
            }

            if ((opt = miscParams->find("contentScalingFactor")) != end)
            {
                mContentScalingFactor = Ogre::StringConverter::parseReal(opt->second);
            }

            if((opt = miscParams->find("FSAA")) != end)
            {
                mMSAA = Ogre::StringConverter::parseInt(opt->second);
            }
        }

        if (!mView)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "ANGLE-based GLES2 backend doesn't support creating a window on its own. "
                        "Either pass \"externalViewHandle\" or \"externalViewControllerHandle\" with view already set.",
                        __FUNCTION__);
        }

        int minAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 5,
            EGL_GREEN_SIZE, 6,
            EGL_RED_SIZE, 5,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
        };

        int maxAttribs[] = {
            EGL_RED_SIZE,       8,
            EGL_GREEN_SIZE,     8,
            EGL_BLUE_SIZE,      8,
            EGL_DEPTH_SIZE,     24,
            EGL_ALPHA_SIZE,     8,
            EGL_STENCIL_SIZE,   8,
            EGL_SAMPLE_BUFFERS, 1,
            EGL_SAMPLES, mMSAA,
            EGL_NONE
        };

        mEglConfig = mGLSupport->selectGLConfig(minAttribs, maxAttribs);
        mEglDisplay = mGLSupport->getGLDisplay();

        mEglSurface = createSurfaceFromWindow(
            mEglDisplay, (__bridge EGLNativeWindowType)[mView layer]);

        mContext = createEGLContext();
        mContext->setCurrent();

        eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, (EGLint*)&mWidth);
        eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, (EGLint*)&mHeight);
        EGL_CHECK_ERROR

        finaliseWindow();
    }
}
