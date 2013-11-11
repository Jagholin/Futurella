#include <osg/Drawable>
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

class CeguiDrawable: public osg::Drawable
{
public:
    CeguiDrawable():
        m_initDone(false)
    {
        if (m_exists)
            throw std::logic_error("No other instances of CeguiDrawable allowed");
        if (!m_exists)
        {
            m_exists = true;
        }
    }
    CeguiDrawable(const CeguiDrawable& rhs, const osg::CopyOp& op)
    {
        throw std::logic_error("CeguiDrawable shouldn't be copied");
    }
    META_Object(futurella, CeguiDrawable);
protected:
    virtual void drawImplementation( osg::RenderInfo& renderInfo ) const;

protected:
    mutable bool m_initDone;
    static bool m_exists;
};