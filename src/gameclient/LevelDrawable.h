#pragma once 

#include "../ShaderWrapper.h"
#include <osg/Drawable>
#include <osg/RenderInfo>

const unsigned int cFeedbackBuffers = 30;

class LevelDrawable : public osg::Drawable
{
public:
    META_Object(futurella, LevelDrawable);

    enum TransformFeedbackMode {
        USE_TRANSFORM_FEEDBACK,
        GEN_TRANSFORM_FEEDBACK,
        NO_TRANSFORM_FEEDBACK
    };

    LevelDrawable(const LevelDrawable & rhs, const osg::CopyOp& op):
        m_graphicsObjects(*this){};
    LevelDrawable();
    virtual ~LevelDrawable();

    virtual osg::BoundingBox computeBound() const;
    virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
    virtual void releaseGLObjects(osg::State*) const override;

    void addAsteroid(osg::Vec3f position, float scale);
    void setUseTesselation(bool useTess);

    void onUpdatePhase();

protected:
    // To not write a bunch of mutable variables,
    // We separate off the structure containing handles for GL objects
    friend class GLObjectsHolder;
    class GLObjectsHolder
    {
    public:
        explicit GLObjectsHolder(const LevelDrawable& ld);
        ~GLObjectsHolder();

        osg::BoundingBox getBound();
        void draw(osg::RenderInfo&);
        void deleteGLObjects();
        void initGLObjects();
        void initGLObjectsForTesselation();
        void initGLObjectsForFBDrawing();

        void invalidateGeometry();

        bool isFeedbackReady() const;
        void resetFeedbackReady();

        void releaseFeedbackBuffers();
        void acquireFeedbackBuffers();

        struct TransformFeedbackObjects
        {
            TransformFeedbackObjects():
                m_TransFeedback(0), m_VBfeedback(0), m_TFquery(0), m_inUse(false)
            {
                // no-op
            }

            void initGLObjects();

            GLuint m_TransFeedback;
            GLuint m_VBfeedback;
            GLuint m_TFquery;
            bool m_inUse;
        };
    protected:
        GLuint m_VBinstanceInfo, m_VAtess, m_VAnorm;
        static GLuint m_VBbasis;
        static TransformFeedbackObjects m_tfArray[cFeedbackBuffers];
        //GLuint m_VBfeedback;
        //GLuint m_TransFeedback;
        //GLuint m_TFquery;
        TransformFeedbackObjects* m_tfeedback;
        bool m_geometryDirty, m_aabbDirty;
        osg::BoundingBox m_aabb;
        const LevelDrawable& m_owner;

        bool m_feedbackReady;
    };
private:

    std::vector<float> m_instanceRawData;
    //std::vector<float> m_normalLines;
    //std::vector<float> m_feedbackPrimitives;
    TransformFeedbackMode m_feedbackMode;
    mutable GLObjectsHolder m_graphicsObjects;
    static osg::ref_ptr<ShaderWrapper> m_tessShader, m_normalShader;
    bool m_useTess, m_newUseTess;
    int m_asteroidCount;
};