#include "../glincludes.h"
#include "LevelDrawable.h"
#include <osg/GLExtensions>
#include <iostream>

LevelDrawable::LevelDrawable():
m_geometryDirty(true)
{
    setUseDisplayList(false);
    setSupportsDisplayList(false);
}

void 
LevelDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    if (m_geometryDirty)
        initGeometry();
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 24, 2);
    glBindVertexArray(0);

    renderInfo.getState()->checkGLErrors("LevelDrawable::drawImplementation");
} 

void 
LevelDrawable::initGeometry() const
{
    if (!glGenBuffers)
        glFuncsInit();
    glGenBuffers(1, &basis);
    glGenBuffers(1, &instanceInfo);
    glGenVertexArrays(1, &vao);

    glBindBuffer(GL_ARRAY_BUFFER, basis);
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

    // While we still have our first VBO active, begin to set VAO up 
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, instanceInfo);
    float instancePos[] = {
        0, 0.5, 0,
        0.5, 0, 0,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(instancePos), instancePos, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(1, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_geometryDirty = false;
}