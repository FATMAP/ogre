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

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include "OgreGLRenderSystemCommon.h"

#include "OgreOSXEGLSupport.h"
#include "OgreOSXEGLWindow.h"
#include "OgreViewport.h"

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include <iostream>
#include <algorithm>
#include <climits>
#include <cmath>

@interface OgreMetalView : NSView
@end

@implementation OgreMetalView
+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (CALayer*)makeBackingLayer
{
    return [CAMetalLayer layer];
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}
@end

namespace Ogre {
    OSXEGLWindow::OSXEGLWindow(OSXEGLSupport *glsupport)
        : EGLWindow(glsupport),
          mView(nil),
          mWindow(nil)
    {
    }

    void OSXEGLWindow::resize(uint width, uint height)
    {
        // Convert from points to pixels if needed
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

    void OSXEGLWindow::windowMovedOrResized()
    {
        if(mActive && mView)
        {
            NSRect frame = [mView frame];
            CGFloat width  = frame.size.width;
            CGFloat height = frame.size.height;

            resize(width, height);
        }
    }

    void OSXEGLWindow::create(const String& name, uint width, uint height,
                              bool fullScreen, const NameValuePairList *miscParams)
    {
        mName = name;
        mWidth = width;
        mHeight = height;
        mLeft = 0;
        mTop = 0;
        mIsFullScreen = fullScreen;

        mContentScalingFactor = 1.0f;

        if (miscParams)
        {
            NameValuePairList::const_iterator opt;
            NameValuePairList::const_iterator end = miscParams->end();

            if ((opt = miscParams->find("externalWindowHandle")) != end)
            {
                mWindow = (__bridge NSWindow *)(void*)StringConverter::parseSizeT(opt->second);
                mUsingExternalWindow = true;
                mIsExternal = true;
                LogManager::getSingleton().logMessage("macOS: Using an external window handle");

                if (mWindow && [mWindow contentView])
                {
                    mView = [mWindow contentView];
                    mUsingExternalView = true;
                }
            }

            if ((opt = miscParams->find("externalViewHandle")) != end)
            {
                mView = (__bridge NSView *)(void*)StringConverter::parseSizeT(opt->second);
                mUsingExternalView = true;
                mIsExternal = true;
                LogManager::getSingleton().logMessage("macOS: Using an external view handle");
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

        // Create window if needed
        if (!mWindow && !mUsingExternalWindow)
        {
            NSRect frame = NSMakeRect(0, 0, width, height);
            NSUInteger styleMask = NSWindowStyleMaskTitled |
                                   NSWindowStyleMaskClosable |
                                   NSWindowStyleMaskMiniaturizable;

            if (!fullScreen)
            {
                styleMask |= NSWindowStyleMaskResizable;
            }

            mWindow = [[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:styleMask
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
            [mWindow setTitle:[NSString stringWithUTF8String:name.c_str()]];
            [mWindow center];
            mUsingExternalWindow = false;
        }

        // Create view if needed
        if (!mView && mWindow)
        {
            NSRect contentRect = [[mWindow contentView] frame];
            mView = [[OgreMetalView alloc] initWithFrame:contentRect];
            [mView setWantsLayer:YES];
            [mWindow setContentView:mView];
            mUsingExternalView = false;
        }

        if (!mView)
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "ANGLE-based GLES2 backend requires either \"externalWindowHandle\" or \"externalViewHandle\".",
                        __FUNCTION__);
        }

        // Get actual dimensions from view
        NSRect viewFrame = [mView frame];
        width = viewFrame.size.width;
        height = viewFrame.size.height;

        // Convert from points to pixels
        width = std::max<long>(1, std::lround(width * mContentScalingFactor));
        height = std::max<long>(1, std::lround(height * mContentScalingFactor));

        mWidth = width;
        mHeight = height;

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

        // Set up the metal layer
        CAMetalLayer *metalLayer = (CAMetalLayer*)[mView layer];
        if ([metalLayer isKindOfClass:[CAMetalLayer class]])
        {
            metalLayer.contentsScale = mContentScalingFactor;
        }

        mEglSurface = createSurfaceFromWindow(
            mEglDisplay, (__bridge EGLNativeWindowType)[mView layer]);

        mContext = createEGLContext();
        mContext->setCurrent();

        eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, (EGLint*)&mWidth);
        eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, (EGLint*)&mHeight);
        EGL_CHECK_ERROR

        finaliseWindow();

        // Show window if we created it
        if (!mUsingExternalWindow && mWindow)
        {
            [mWindow makeKeyAndOrderFront:nil];
        }
    }

    void OSXEGLWindow::destroy()
    {
        if (mWindow && !mUsingExternalWindow)
        {
            [mWindow close];
            mWindow = nil;
        }

        if (mView && !mUsingExternalView)
        {
            mView = nil;
        }

        EGLWindow::destroy();
    }

    bool OSXEGLWindow::isClosed() const
    {
        if (mWindow && !mUsingExternalWindow)
        {
            return ![mWindow isVisible];
        }
        return mClosed;
    }

    void OSXEGLWindow::reposition(int left, int top)
    {
        if (mWindow && !mIsFullScreen)
        {
            NSRect frame = [mWindow frame];

            // Convert coordinates (Ogre uses top-left origin, macOS uses bottom-left)
            NSScreen *screen = [mWindow screen];
            if (!screen)
                screen = [NSScreen mainScreen];

            CGFloat screenHeight = [screen frame].size.height;
            CGFloat windowHeight = frame.size.height;

            frame.origin.x = left;
            frame.origin.y = screenHeight - top - windowHeight;

            [mWindow setFrameOrigin:frame.origin];

            mLeft = left;
            mTop = top;
        }
    }

    bool OSXEGLWindow::isVisible() const
    {
        if (mWindow)
        {
            return [mWindow isVisible];
        }
        return mActive;
    }

    void OSXEGLWindow::setVisible(bool visible)
    {
        if (mWindow && !mUsingExternalWindow)
        {
            if (visible)
            {
                [mWindow makeKeyAndOrderFront:nil];
            }
            else
            {
                [mWindow orderOut:nil];
            }
        }
    }
}
