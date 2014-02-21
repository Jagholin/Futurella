#include "../gl_40.h"
#include "LevelDrawable.h"
#include <iostream>

GLuint LevelDrawable::GLObjectsHolder::m_VBbasis = 0;
osg::ref_ptr<ShaderWrapper> LevelDrawable::m_tessShader;
osg::ref_ptr<ShaderWrapper> LevelDrawable::m_normalShader;

class LevelDrawableUpdateCallback : public osg::Drawable::UpdateCallback
{
public:
    virtual void update(osg::NodeVisitor*, osg::Drawable* ld) override
    {
        LevelDrawable* realDrawable = static_cast<LevelDrawable*> (ld);
        realDrawable->onUpdatePhase();
    }
};

LevelDrawable::GLObjectsHolder::GLObjectsHolder(const LevelDrawable& ld):
m_owner(ld)
{
    m_aabbDirty = m_geometryDirty = true;
    m_VBfeedback = m_VBinstanceInfo = m_VAtess = 0;
    m_VAnorm = 0;
    m_TransFeedback = 0;
    m_feedbackReady = false;
    //m_TFquery = 0;
    //m_tessPrimitiveCount = 0;
    //m_tessFeedbackArray = nullptr;
    //m_feedbackWritten = false;
}

LevelDrawable::GLObjectsHolder::~GLObjectsHolder()
{
    //deleteGLObjects();
}

osg::BoundingBox LevelDrawable::GLObjectsHolder::getBound()
{
    if (!m_aabbDirty)
        return m_aabb;
    m_aabb.init();
    unsigned int i = 0;
    while (i < m_owner.m_instanceRawData.size())
    {
        float x = m_owner.m_instanceRawData[i++];
        float y = m_owner.m_instanceRawData[i++];
        float z = m_owner.m_instanceRawData[i++];
        float w = m_owner.m_instanceRawData[i++];

        m_aabb.expandBy(osg::BoundingSphere(osg::Vec3f(x, y, z), w));
    }
    m_aabbDirty = false;
    return m_aabb;
}

void LevelDrawable::GLObjectsHolder::draw(osg::RenderInfo& ri)
{
    glEnable(GL_CULL_FACE);
    if (m_geometryDirty)
    {
        ri.getState()->setCheckForGLErrors(osg::State::NEVER_CHECK_GL_ERRORS);
        initGLObjects();
    }
    if (m_owner.m_feedbackMode == GEN_TRANSFORM_FEEDBACK)
    {
        // Start generating transform feedback
        if (!m_TransFeedback)
        {
            glGenTransformFeedbacks(1, &m_TransFeedback);
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_TransFeedback);

            glGenBuffers(1, &m_VBfeedback);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_VBfeedback);
            glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000000 * 6 * sizeof(float), nullptr, GL_STATIC_DRAW);

            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

            glGenVertexArrays(1, &m_VAnorm);
            glBindVertexArray(m_VAnorm);
            glBindBuffer(GL_ARRAY_BUFFER, m_VBfeedback);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), 0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            //glBindVertexArray(0);

            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_TransFeedback);
        }
        else
            //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_VBfeedback);
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_TransFeedback);

        //glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_TFquery);
        glBeginTransformFeedback(GL_TRIANGLES);

        glBindVertexArray(m_VAtess);
        glDrawArraysInstanced(GL_PATCHES, 0, 24, m_owner.m_asteroidCount);
    }
    else if (m_owner.m_feedbackMode == USE_TRANSFORM_FEEDBACK)
    {
        glBindVertexArray(m_VAnorm);
        glDrawTransformFeedback(GL_TRIANGLES, m_TransFeedback);
    }
    else // NO_TRANSFORM_FEEDBACK
    {
        glBindVertexArray(m_VAtess);
        glDrawArraysInstanced(GL_PATCHES, 0, 24, m_owner.m_asteroidCount);
    }

    glBindVertexArray(0);

    if (m_owner.m_feedbackMode == GEN_TRANSFORM_FEEDBACK)
    {
        glEndTransformFeedback();
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        m_feedbackReady = true;
        //glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

        //GLint oldPrimitiveCount = m_tessPrimitiveCount;
        //glGetQueryObjectiv(m_TFquery, GL_QUERY_RESULT, &m_tessPrimitiveCount);

        //if (m_tessFeedbackArray && oldPrimitiveCount < m_tessPrimitiveCount)
        //{
        //    delete[] m_tessFeedbackArray;
        //    m_tessFeedbackArray = nullptr;
        //}
        //if (!m_tessFeedbackArray)
        //    m_tessFeedbackArray = new float[m_tessPrimitiveCount * 3 * 6];

        //glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tessPrimitiveCount * 3 * 6 * sizeof(float), m_tessFeedbackArray);
        //m_feedbackWritten = true;
    }
    glDisable(GL_CULL_FACE);

#ifdef _DEBUG
    ri.getState()->checkGLErrors("LevelDrawable::drawImplementation");
#endif
}

void LevelDrawable::GLObjectsHolder::initGLObjectsForTesselation()
{
    if (m_VBbasis == 0)
    {
        glGenBuffers(1, &m_VBbasis);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBbasis);
        float vertices[] = {
            -1, 0, 0, 0, -1, -1,
            0, 0, -1, 0, -1, -1,
            0, -1, 0, 0, 0.875, 0,

            -1, 0, 0, 1, -1, -1,
            0, -1, 0, 1, 0.125, 0,// << This one requires some special attention when texture coordinates are generated
            0, 0, 1, 1, -1, -1,

            1, 0, 0, 0, -1, -1,
            0, -1, 0, 0, 0.625, 0,
            0, 0, -1, 0, -1, -1,

            1, 0, 0, 0, -1, -1,
            0, 0, 1, 0, -1, -1,
            0, -1, 0, 0, 0.375, 0,

            -1, 0, 0, 0, -1, -1,
            0, 1, 0, 0, 0.875, 1,
            0, 0, -1, 0, -1, -1,

            -1, 0, 0, 1, -1, -1,
            0, 0, 1, 1, -1, -1, // << As well as this one.
            0, 1, 0, 1, 0.125, 1,

            1, 0, 0, 0, -1, -1,
            0, 0, -1, 0, -1, -1,
            0, 1, 0, 0, 0.625, 1,

            1, 0, 0, 0, -1, -1,
            0, 1, 0, 0, 0.375, 1,
            0, 0, 1, 0, -1, -1
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }
    else
        glBindBuffer(GL_ARRAY_BUFFER, m_VBbasis);

    if (m_VBinstanceInfo == 0)
        glGenBuffers(1, &m_VBinstanceInfo);

    if (m_VAtess == 0)
        glGenVertexArrays(1, &m_VAtess);

    // While we still have our first VBO active, begin to set VAO up 
    glBindVertexArray(m_VAtess);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)(4 * sizeof(float)));
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBinstanceInfo);
    glBufferData(GL_ARRAY_BUFFER, m_owner.m_instanceRawData.size() * sizeof(float), &(m_owner.m_instanceRawData[0]), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(4 * sizeof(float)));
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //if (!m_TFquery)
    //    glGenQueries(1, &m_TFquery);
    //glGenTransformFeedbacks(1, &m_TransFeedback);

    m_geometryDirty = false;
}

void LevelDrawable::GLObjectsHolder::deleteGLObjects()
{
    if (m_VAtess)
        glDeleteVertexArrays(1, &m_VAtess);
//     if (m_VBbasis)
//         glDeleteBuffers(1, &m_VBbasis);
    if (m_VBinstanceInfo)
        glDeleteBuffers(1, &m_VBinstanceInfo);
    if (m_VBfeedback)
        glDeleteBuffers(1, &m_VBfeedback);
    //if (m_TFquery)
    //    glDeleteQueries(1, &m_TFquery);
    if (m_VAnorm)
        glDeleteVertexArrays(1, &m_VAnorm);
//     if (m_VBnorm)
//         glDeleteVertexArrays(1, &m_VBnorm);
    m_VAtess = m_VBinstanceInfo = m_VBfeedback = 0;
    m_VAnorm = 0;
    //m_TFquery = 0;
    m_geometryDirty = true;
}

void LevelDrawable::GLObjectsHolder::invalidateGeometry()
{
    m_aabbDirty = m_geometryDirty = true;
}

//bool LevelDrawable::GLObjectsHolder::hasFeedback()
//{
    //return m_feedbackWritten;
//}

//float* LevelDrawable::GLObjectsHolder::getTessFeedback()
//{
    //return m_tessFeedbackArray;
//}

//unsigned int LevelDrawable::GLObjectsHolder::getPrimitiveCount()
//{
//    return m_tessPrimitiveCount;
//}

void LevelDrawable::GLObjectsHolder::initGLObjects()
{
    if (m_owner.m_feedbackMode == USE_TRANSFORM_FEEDBACK)
        initGLObjectsForFBDrawing();
    else
        initGLObjectsForTesselation();
}

void LevelDrawable::GLObjectsHolder::initGLObjectsForFBDrawing()
{
}

bool LevelDrawable::GLObjectsHolder::isFeedbackReady() const
{
    return m_feedbackReady;
}

LevelDrawable::LevelDrawable():
m_asteroidCount(0),
m_graphicsObjects(*this)
{
    setUseDisplayList(false);
    setSupportsDisplayList(false);

    if (m_tessShader == nullptr)
    {
        m_tessShader = new ShaderWrapper;
        m_tessShader->load(osg::Shader::VERTEX, "shader/vs_octahedron.txt");
        m_tessShader->load(osg::Shader::TESSCONTROL, "shader/tc_octahedron.txt");
        m_tessShader->load(osg::Shader::TESSEVALUATION, "shader/te_octahedron.txt");
        m_tessShader->load(osg::Shader::GEOMETRY, "shader/gs_octahedron.txt");
        m_tessShader->load(osg::Shader::FRAGMENT, "shader/fs_octahedron.txt");

        m_tessShader->addTransformFeedbackVarying("tfPos");
        m_tessShader->addTransformFeedbackVarying("tfNormal");
        m_tessShader->addTransformFeedbackVarying("tfTexCoord");
        m_tessShader->addTransformFeedbackVarying("tfTexNumber");
    }

    if (m_normalShader == nullptr)
    {
        m_normalShader = new ShaderWrapper;
        m_normalShader->load(osg::Shader::VERTEX, "shader/vs_asteroids.txt");
        m_normalShader->load(osg::Shader::FRAGMENT, "shader/fs_octahedron.txt");
    }

    getOrCreateStateSet()->setAttributeAndModes(m_tessShader, osg::StateAttribute::ON);

    setUpdateCallback(new LevelDrawableUpdateCallback);
    m_feedbackMode = GEN_TRANSFORM_FEEDBACK;

    // IMPORTANT: If you use UpdateCallback above to change this object,
    // set the data variance to dynamic.
    if (m_feedbackMode != NO_TRANSFORM_FEEDBACK)
        setDataVariance(osg::Object::DYNAMIC);
}

LevelDrawable::~LevelDrawable()
{
    // HacK: shouldn't work at all, replace it.
    releaseGLObjects(nullptr);
}

void 
LevelDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    m_graphicsObjects.draw(renderInfo);
} 

void LevelDrawable::addAsteroid(osg::Vec3f position, float scale)
{
    m_instanceRawData.push_back(position.x());
    m_instanceRawData.push_back(position.y());
    m_instanceRawData.push_back(position.z());
    m_instanceRawData.push_back(scale);
    m_instanceRawData.push_back(std::rand() % 16);
    ++m_asteroidCount;
    m_graphicsObjects.invalidateGeometry();
}

osg::BoundingBox LevelDrawable::computeBound() const
{
    return m_graphicsObjects.getBound();
}

void LevelDrawable::releaseGLObjects(osg::State* s) const
{
    m_graphicsObjects.deleteGLObjects();
}

void LevelDrawable::onUpdatePhase()
{
    // TODO: if u want to keep this,
    // keep in mind that the rendering of the same LevelDrawable runs in parallel in 
    // a separate thread!!!!!!

    //return;

    if (m_feedbackMode != GEN_TRANSFORM_FEEDBACK || !m_graphicsObjects.isFeedbackReady())
        return;
    //float *tessResults = m_graphicsObjects.getTessFeedback();
    //unsigned int primitives = m_graphicsObjects.getPrimitiveCount();

    // generate second array of normal lines
    //m_normalLines.clear();
    //m_feedbackPrimitives.clear();
    /*for (unsigned int i = 0; i < primitives * 3; ++i)
    {
        m_normalLines.push_back(tessResults[0]);
        m_normalLines.push_back(tessResults[1]);
        m_normalLines.push_back(tessResults[2]);

        m_normalLines.push_back(tessResults[0] + tessResults[3] * 0.2);
        m_normalLines.push_back(tessResults[1] + tessResults[4] * 0.2);
        m_normalLines.push_back(tessResults[2] + tessResults[5] * 0.2);

        m_feedbackPrimitives.push_back(tessResults[0]);
        m_feedbackPrimitives.push_back(tessResults[1]);
        m_feedbackPrimitives.push_back(tessResults[2]);

        tessResults += 6;
    }*/

    m_feedbackMode = USE_TRANSFORM_FEEDBACK;
    getOrCreateStateSet()->setAttributeAndModes(m_normalShader, osg::StateAttribute::ON);
    //m_graphicsObjects.invalidateGeometry();
}
