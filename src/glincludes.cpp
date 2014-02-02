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
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLBINDBUFFERBASEPROC glBindBufferBase = nullptr;
PFNGLGENQUERIESPROC glGenQueries = nullptr;
PFNGLBEGINQUERYPROC glBeginQuery = nullptr;
PFNGLENDQUERYPROC glEndQuery = nullptr;
PFNGLDELETEQUERIESPROC glDeleteQueries = nullptr;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv = nullptr;
PFNGLBEGINTRANSFORMFEEDBACKPROC glBeginTransformFeedback = nullptr;
PFNGLENDTRANSFORMFEEDBACKPROC glEndTransformFeedback = nullptr;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData = nullptr;

#define LOADGLFUNC(funcname, type) funcname = reinterpret_cast<type>(osg::getGLExtensionFuncPtr(#funcname))

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
    glTransformFeedbackVaryings = reinterpret_cast<PFNGLTRANSFORMFEEDBACKVARYINGSPROC>(osg::getGLExtensionFuncPtr("glTransformFeedbackVaryings"));
    glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(osg::getGLExtensionFuncPtr("glDeleteVertexArrays"));
    glBindBufferBase = reinterpret_cast<PFNGLBINDBUFFERBASEPROC>(osg::getGLExtensionFuncPtr("glBindBufferBase"));
    LOADGLFUNC(glGenQueries, PFNGLGENQUERIESPROC);
    LOADGLFUNC(glBeginQuery, PFNGLBEGINQUERYPROC);
    LOADGLFUNC(glEndQuery, PFNGLENDQUERYPROC);
    LOADGLFUNC(glDeleteQueries, PFNGLDELETEQUERIESPROC);
    LOADGLFUNC(glBeginTransformFeedback, PFNGLBEGINTRANSFORMFEEDBACKPROC);
    LOADGLFUNC(glEndTransformFeedback, PFNGLENDTRANSFORMFEEDBACKPROC);
    LOADGLFUNC(glGetQueryObjectiv, PFNGLGETQUERYOBJECTIVPROC);
    LOADGLFUNC(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);
}
