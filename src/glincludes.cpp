#include "glincludes.h"
#include <osg/GLExtensions>

PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = nullptr;
PFNGLPATCHPARAMETERIPROC glPatchParameteri = nullptr;

void glFuncsInit()
{
    glGenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(osg::getGLExtensionFuncPtr("glGenBuffers"));
    glBufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(osg::getGLExtensionFuncPtr("glBufferData"));
    glDeleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(osg::getGLExtensionFuncPtr("glDeleteBuffers"));
    glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(osg::getGLExtensionFuncPtr("glGenVertexArrays"));
    glBindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(osg::getGLExtensionFuncPtr("glBindBuffer"));
    glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(osg::getGLExtensionFuncPtr("glVertexAttribPointer"));
    glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(osg::getGLExtensionFuncPtr("glEnableVertexAttribArray"));
    glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(osg::getGLExtensionFuncPtr("glDisableVertexAttribArray"));
    glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(osg::getGLExtensionFuncPtr("glBindVertexArray"));
    glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(osg::getGLExtensionFuncPtr("glBindFramebuffer"));
    glVertexAttribDivisor = reinterpret_cast<PFNGLVERTEXATTRIBDIVISORPROC>(osg::getGLExtensionFuncPtr("glVertexAttribDivisor"));
    glDrawArraysInstanced = reinterpret_cast<PFNGLDRAWARRAYSINSTANCEDPROC>(osg::getGLExtensionFuncPtr("glDrawArraysInstanced"));
    glPatchParameteri = reinterpret_cast<PFNGLPATCHPARAMETERIPROC>(osg::getGLExtensionFuncPtr("glPatchParameteri"));
}
