#include "../glincludes.h"
#include "LevelDrawable.h"
#include <osg/GLExtensions>
#include <iostream>

LevelDrawable::LevelDrawable():
basis(0), instanceInfo(0), vao(0),
m_geometryDirty(true),
m_aabbDirty(true),
m_asteroidCount(0)
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
    glDrawArraysInstanced(GL_TRIANGLES, 0, 24, m_asteroidCount);
    glBindVertexArray(0);

    renderInfo.getState()->checkGLErrors("LevelDrawable::drawImplementation");
} 

void 
LevelDrawable::initGeometry() const
{
    if (!glGenBuffers)
        glFuncsInit();
    if (basis == 0)
    {
        glEnable(GL_CULL_FACE);
        glGenBuffers(1, &basis);
        glBindBuffer(GL_ARRAY_BUFFER, basis);
        float vertices[] = {
            -1, 0, 0,
            0, 0, -1,
            0, -1, 0,

            -1, 0, 0,
            0, -1, 0,
            0, 0, 1,

            1, 0, 0,
            0, -1, 0,
            0, 0, -1,

            1, 0, 0,
            0, 0, 1,
            0, -1, 0,

            -1, 0, 0,
            0, 1, 0,
            0, 0, -1,

            -1, 0, 0,
            0, 0, 1,
            0, 1, 0,

            1, 0, 0,
            0, 0, -1,
            0, 1, 0,

            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }
    else
        glBindBuffer(GL_ARRAY_BUFFER, basis);

    if (instanceInfo == 0)
        glGenBuffers(1, &instanceInfo);

    if (vao == 0)
        glGenVertexArrays(1, &vao);

    // While we still have our first VBO active, begin to set VAO up 
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, instanceInfo);
    glBufferData(GL_ARRAY_BUFFER, m_instanceRawData.size() * sizeof(float), &(m_instanceRawData[0]), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_geometryDirty = false;
}

void LevelDrawable::addAsteroid(osg::Vec3f position, float scale)
{
    m_instanceRawData.push_back(position.x());
    m_instanceRawData.push_back(position.y());
    m_instanceRawData.push_back(position.z());
    m_instanceRawData.push_back(scale);
    ++m_asteroidCount;
    m_geometryDirty = true;
    m_aabbDirty = true;
}

osg::BoundingBox LevelDrawable::computeBound() const
{
    if (!m_aabbDirty)
        return m_aabb;
    m_aabb.init();
    unsigned int i = 0;
    while (i < m_instanceRawData.size())
    {
        float x = m_instanceRawData[i++];
        float y = m_instanceRawData[i++];
        float z = m_instanceRawData[i++];
        float w = m_instanceRawData[i++];

        m_aabb.expandBy(osg::BoundingSphere(osg::Vec3f(x, y, z), w));
    }
    m_aabbDirty = false;
    return m_aabb;
}
