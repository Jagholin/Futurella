#include "glincludes.h"
#include <osg/GLExtensions>
#include <iostream>

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

#define LOADGLFUNC(funcname, type) funcname = reinterpret_cast<type>(osg::getGLExtensionFuncPtr(#funcname)); \
    if (funcname == nullptr) std::cerr << "Cant find function " #funcname ", program will self-destruct" << std::endl 

void glFuncsInit()
{
    LOADGLFUNC(glGenBuffers, PFNGLGENBUFFERSPROC);
    LOADGLFUNC(glBufferData, PFNGLBUFFERDATAPROC);
    LOADGLFUNC(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    LOADGLFUNC(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC);
    LOADGLFUNC(glBindBuffer, PFNGLBINDBUFFERPROC);
    LOADGLFUNC(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);
    LOADGLFUNC(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOADGLFUNC(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
    LOADGLFUNC(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC);
    LOADGLFUNC(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC);
    LOADGLFUNC(glVertexAttribDivisor, PFNGLVERTEXATTRIBDIVISORPROC);
    LOADGLFUNC(glDrawArraysInstanced, PFNGLDRAWARRAYSINSTANCEDPROC);
    LOADGLFUNC(glPatchParameteri, PFNGLPATCHPARAMETERIPROC);
    LOADGLFUNC(glTransformFeedbackVaryings, PFNGLTRANSFORMFEEDBACKVARYINGSPROC);
    LOADGLFUNC(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC);
    LOADGLFUNC(glBindBufferBase, PFNGLBINDBUFFERBASEPROC);
    LOADGLFUNC(glGenQueries, PFNGLGENQUERIESPROC);
    LOADGLFUNC(glBeginQuery, PFNGLBEGINQUERYPROC);
    LOADGLFUNC(glEndQuery, PFNGLENDQUERYPROC);
    LOADGLFUNC(glDeleteQueries, PFNGLDELETEQUERIESPROC);
    LOADGLFUNC(glBeginTransformFeedback, PFNGLBEGINTRANSFORMFEEDBACKPROC);
    LOADGLFUNC(glEndTransformFeedback, PFNGLENDTRANSFORMFEEDBACKPROC);
    LOADGLFUNC(glGetQueryObjectiv, PFNGLGETQUERYOBJECTIVPROC);
    LOADGLFUNC(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);
}
