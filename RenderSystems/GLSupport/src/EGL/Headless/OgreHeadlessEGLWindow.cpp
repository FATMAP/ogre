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
--------------------------------------------------------------------------*/

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreViewport.h"

#include "OgreHeadlessEGLSupport.h"
#include "OgreHeadlessEGLWindow.h"

#include <iostream>
#include <algorithm>
#include <climits>

namespace Ogre {
    HeadlessEGLWindow::HeadlessEGLWindow(HeadlessEGLSupport *glsupport)
        : EGLWindow(glsupport) 
    {
        mGLSupport = glsupport;
        mIsExternal = false;
        mIsExternalGLControl = false;
        mWidth = 0;
        mHeight = 0;
    }

    HeadlessEGLWindow::~HeadlessEGLWindow()
    {
    }

    void HeadlessEGLWindow::getCustomAttribute( const String& name, void* pData )
    {
        EGLWindow::getCustomAttribute(name, pData);
    }

    void HeadlessEGLWindow::getLeftAndTopFromNativeWindow( int & left, int & top, uint width, uint height )
    {
        left = 0;
        top  = 0;
    }

    void HeadlessEGLWindow::initNativeCreatedWindow(const NameValuePairList *miscParams)
    {
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                    "This function is not supposed to be called",
                    __FUNCTION__);
    }

    void HeadlessEGLWindow::createNativeWindow( int &left, int &top, uint &width, uint &height, String &title )
    {
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                    "This function is not supposed to be called",
                    __FUNCTION__);
    }

    void HeadlessEGLWindow::setFullscreen( bool fullscreen, uint width, uint height )
    {
        LogManager::getSingleton().logMessage(
            "HeadlessEGLWindow::setFullScreen: operation not supported");
    }

    void HeadlessEGLWindow::resize(uint width, uint height)
    {
        if (mWidth == width && mHeight == height)
        {
            return;
        }

        EGLSurface oldSurface = mEglSurface;

        // PBuffer surfaces can't be resized, only re-created.
        mEglSurface = createPBufferSurface(width, height);

        static_cast<EGLContext*>(mContext)->_updateInternalResources(mEglDisplay, mEglConfig, mEglSurface);

        eglDestroySurface(mEglDisplay, oldSurface);

        mWidth = width;
        mHeight = height;

        // Notify viewports that our dimensions changed.
        for (auto kv : mViewportList)
        {
            kv.second->_updateDimensions();
        }
    }

    void HeadlessEGLWindow::create(const String& name, uint width, uint height,
                                   bool fullScreen, const NameValuePairList *miscParams)
    {
        mWidth = width;
        mHeight = height;

        uint samples = 0;

        if (miscParams)
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            if ((opt = miscParams->find("FSAA")) != end)
            {
                samples = StringConverter::parseUnsignedInt(opt->second);
            }
        }

        mEglDisplay = mGLSupport->getEglDisplay();

        int minAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
            EGL_RED_SIZE,        5,
            EGL_GREEN_SIZE,      6,
            EGL_BLUE_SIZE,       5,
            EGL_DEPTH_SIZE,      16,
            EGL_SAMPLES,         0,
            EGL_ALPHA_SIZE,      EGL_DONT_CARE,
            EGL_STENCIL_SIZE,    EGL_DONT_CARE,
            EGL_SAMPLE_BUFFERS,  0,
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
            EGL_SAMPLES,        (int)samples,
            EGL_NONE
        };

        mHwGamma = false;
        mEglConfig = mGLSupport->selectGLConfig(minAttribs, maxAttribs);

        mEglSurface = createPBufferSurface(width, height);

        mContext = createEGLContext();
        mWidth = width;
        mHeight = height;

        // Ogre won't render into a window unless it thinks it's visible.
        setVisible(true);
    }

    ::EGLSurface HeadlessEGLWindow::createPBufferSurface(unsigned int width, unsigned int height) const
    {
        const EGLint attribs[] = {
            EGL_WIDTH, (EGLint)width,
            EGL_HEIGHT, (EGLint)height,
            EGL_NONE
        };

        ::EGLSurface surface = eglCreatePbufferSurface(mEglDisplay, mEglConfig, attribs);
        if (surface == EGL_NO_SURFACE)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Fail to create a Pbuffer-based EGLSurface",
                        __FUNCTION__);
        }

        return surface;
    }
}
