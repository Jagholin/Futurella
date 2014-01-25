#include "LevelDrawable.h"
#include <osg/GLExtensions>


PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDBUFFERPROC glBindBuffer2;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;

PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;

LevelDrawable::LevelDrawable()
{
    m_initialized = false;
}


void 
LevelDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    if (!m_initialized) createVertexBuffers();
    glEnableVertexAttribArray(0);

    glBindBuffer2(GL_ARRAY_BUFFER, basis);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 8*3);
    glBindBuffer2(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(0);
} 

void 
LevelDrawable::createVertexBuffers() const
{
    /// HACKS, hacks hacks all around...
    // well, what else can you do here, i have no idea.
    glGenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(osg::getGLExtensionFuncPtr("glGenBuffers"));
    glBufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(osg::getGLExtensionFuncPtr("glBufferData"));
    glDeleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(osg::getGLExtensionFuncPtr("glDeleteBuffers"));
    glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(osg::getGLExtensionFuncPtr("glGenVertexArrays"));
    glBindBuffer2 = reinterpret_cast<PFNGLBINDBUFFERPROC>(osg::getGLExtensionFuncPtr("glBindBuffer"));
    glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(osg::getGLExtensionFuncPtr("glVertexAttribPointer"));
    glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(osg::getGLExtensionFuncPtr("glEnableVertexAttribArray"));
    glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(osg::getGLExtensionFuncPtr("glDisableVertexAttribArray"));
    
    glGenBuffers(1, &basis);
    glGenBuffers(1, &instanceInfo);
    glGenVertexArrays(1, &vao);

    glBindBuffer2(GL_ARRAY_BUFFER, basis);
    float vertices[] = {
        -1, 0, 0,
        0, -1, 0,
        0, 0, -1,

        -1, 0, 0,
        0, -1, 0,
        0, 0, 1,

        1, 0, 0,
        0, -1, 0,
        0, 0, -1,

        1, 0, 0,
        0, -1, 0,
        0, 0, 1,

        -1, 0, 0,
        0, 1, 0,
        0, 0, -1,

        -1, 0, 0,
        0, 1, 0,
        0, 0, 1,

        1, 0, 0,
        0, 1, 0,
        0, 0, -1,

        1, 0, 0,
        0, 1, 0,
        0, 0, 1,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer2(GL_ARRAY_BUFFER, 0);

    m_initialized = true;
}