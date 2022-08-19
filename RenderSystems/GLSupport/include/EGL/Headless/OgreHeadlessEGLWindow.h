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

#ifndef __HeadlessEGLWindow_H__
#define __HeadlessEGLWindow_H__

#include "OgreEGLWindow.h"
#include "OgreHeadlessEGLSupport.h"

namespace Ogre {
    class _OgrePrivate HeadlessEGLWindow : public EGLWindow
    {
    protected:
        HeadlessEGLSupport* mGLSupport;

        virtual void getLeftAndTopFromNativeWindow(int & left, int & top, uint width, uint height);
        virtual void initNativeCreatedWindow(const NameValuePairList *miscParams);
        virtual void createNativeWindow( int &left, int &top, uint &width, uint &height, String &title );
        virtual void resize(unsigned int width, unsigned int height) override;

    public:
        HeadlessEGLWindow(HeadlessEGLSupport* glsupport);
        virtual ~HeadlessEGLWindow();

        /**
         * For now, just delegates to EGLWindow::getCustomAttribute().
         *
         * If such a need arises, we may support a query for mEglSurface here.
         */
        virtual void getCustomAttribute(const String& name, void* pData) override;

        virtual void setFullscreen (bool fullscreen, uint width, uint height) override;

        virtual void create(const String& name, unsigned int width, unsigned int height,
                            bool fullScreen, const NameValuePairList *miscParams) override;

    private:
        ::EGLSurface createPBufferSurface(unsigned int width, unsigned int height) const;
    };
}

#endif
