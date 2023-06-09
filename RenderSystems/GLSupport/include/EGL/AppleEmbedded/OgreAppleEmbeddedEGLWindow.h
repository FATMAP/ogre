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

#ifndef __AppleEmbeddedEGLWindow_H__
#define __AppleEmbeddedEGLWindow_H__

#include "OgreEGLWindow.h"
#include "OgreAppleEmbeddedEGLSupport.h"

#ifdef __OBJC__
// Forward declarations
@class UIWindow;
@class UIView;
@class UIViewController;
#endif

namespace Ogre {
    class _OgrePrivate AppleEmbeddedEGLWindow : public EGLWindow
    {
    private:
#ifdef __OBJC__
        UIView *mView;
#else
        void *mViewPlaceholder;
#endif

        int mMSAA = 0;
        bool mUsingExternalView = false;
        float mContentScalingFactor = 1.f;
        
    protected:
        void resize(unsigned int width, unsigned int height) override;
        void windowMovedOrResized() override;
        
    public:
        AppleEmbeddedEGLWindow(AppleEmbeddedEGLSupport* glsupport);
        void create(const String& name, unsigned int width, unsigned int height,
                    bool fullScreen, const NameValuePairList *miscParams) override;
        
        float getViewPointToPixelScale() override { return 1.0; /*mScale;*/ }
    };
}

#endif
