#include <osg/Drawable>
#include <osg/Texture2D>
#include <osg/Program>
#include <osgGA/GUIEventAdapter>
#include <OpenThreads/Mutex>
#include <CEGUI/CEGUI.h>
#include <deque>

#include <boost/asio.hpp>

class GUIApplication;

class CeguiDrawable: public osg::Drawable
{
public:
	CeguiDrawable();
    CeguiDrawable(const CeguiDrawable& rhs, const osg::CopyOp& op)
    {
        throw std::logic_error("CeguiDrawable shouldn't be copied");
    }

	// We should initialize GUIApplication somewhere after CEGUI itself is initialized,
	// which is udoable from main() function.
	void setGuiApplication(GUIApplication *app);
	void init() const;
    META_Object(futurella, CeguiDrawable);

	void addEvent(const osgGA::GUIEventAdapter&);
	void passEvent(const osgGA::GUIEventAdapter&) const;
protected:
    virtual void drawImplementation( osg::RenderInfo& renderInfo ) const;

protected:
    mutable bool m_initDone;
    static bool m_exists;
	osg::ref_ptr<osg::Texture2D> m_tex;
	osg::ref_ptr<osg::Program> m_pr;
	osg::ref_ptr<osg::StateSet> m_state;

	mutable std::deque<osg::ref_ptr<osgGA::GUIEventAdapter> > m_eventQueue;
	mutable OpenThreads::Mutex m_eventMutex;

	mutable unsigned int m_skipCounter;

	GUIApplication* m_guiApp;
	std::shared_ptr<boost::asio::io_service> m_renderThreadService;

    std::map<int, CEGUI::Key::Scan> m_keyboardMap;
};