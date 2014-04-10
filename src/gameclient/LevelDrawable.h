#pragma once 

#include "../ShaderWrapper.h"
//#include <osg/Drawable>
//#include <osg/RenderInfo>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Buffer.h>
#include "../ShaderWrapper.h"
using namespace Magnum;
typedef SceneGraph::Object<SceneGraph::RigidMatrixTransformation3D> Object3D;

const unsigned int cFeedbackBuffers = 27; // 3*3*3

class LevelDrawable : public Object3D, SceneGraph::Drawable3D
{
public:
    enum TransformFeedbackMode {
        USE_TRANSFORM_FEEDBACK,
        GEN_TRANSFORM_FEEDBACK,
        NO_TRANSFORM_FEEDBACK
    };

    LevelDrawable(const LevelDrawable & rhs):
        m_graphicsObjects(*this){};
    LevelDrawable();
    virtual ~LevelDrawable();

    //virtual osg::BoundingB computeBound() const;
    //virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
    //virtual void releaseGLObjects(osg::State*) const override;

    void addAsteroid(Vector3 position, float scale);
    void setUseTesselation(bool useTess);

    void onUpdatePhase();

protected:
    void draw(const Matrix4& transform, SceneGraph::AbstractCamera3D& camera) override;

    // To not write a bunch of mutable variables,
    // We separate off the structure containing handles for GL objects
    friend class GLObjectsHolder;
    class GLObjectsHolder
    {
    public:
        explicit GLObjectsHolder(const LevelDrawable& ld);
        ~GLObjectsHolder();

        //osg::BoundingBox getBound();
        void draw(/*osg::RenderInfo&*/);
        void deleteGLObjects();
        void initGLObjects();

        void invalidateGeometry();

        bool isFeedbackReady() const;
        void resetFeedbackReady();

        void releaseFeedbackBuffers();
        void acquireFeedbackBuffers();

        struct TransformFeedbackObjects
        {
            TransformFeedbackObjects():
                m_TransFeedback(0), /*m_VBfeedback(0), m_VAfeedback(0),*/ m_TFquery(0), m_inUse(false)
            {
                // no-op
            }

            void initGLObjects();

            GLuint m_TransFeedback;
            //GLuint m_VBfeedback;
            Buffer m_VBfeedback;
            //GLuint m_VAfeedback;
            Mesh m_Mfeedback;
            GLuint m_TFquery;
            bool m_inUse;
        };
    protected:
        //GLuint m_VBinstanceInfo, m_VAtess;
        //static GLuint m_VBbasis;
        Buffer m_VBinstanceInfo, m_VBbasis;
        static TransformFeedbackObjects m_tfArray[cFeedbackBuffers];
        TransformFeedbackObjects* m_tfeedback;
        bool m_geometryDirty/*, m_aabbDirty*/;
        //osg::BoundingBox m_aabb;
        const LevelDrawable& m_owner;

        bool m_feedbackReady;
    };
private:

    std::vector<float> m_instanceRawData;
    TransformFeedbackMode m_feedbackMode;
    GLObjectsHolder m_graphicsObjects;
    static ShaderWrapper m_tessShader, m_drawFeedbackShader, m_noTessShader;
    bool m_useTess, m_newUseTess;
    int m_asteroidCount;
};