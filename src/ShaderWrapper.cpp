#include "ShaderWrapper.h"
#include "GUIApplication.h"

#include <CEGUI/CEGUI.h>

ShaderProvider* ShaderWrapper::s_defaultProvider = nullptr;

ShaderWrapper::ShaderWrapper()
{
    // nop
}
ShaderWrapper::~ShaderWrapper()
{
    // nop
}

void ShaderWrapper::load(osg::Shader::Type type, const std::string& fileName)
{

}

void ShaderWrapper::setDefaultShaderProvider(ShaderProvider* prov)
{
    s_defaultProvider = prov;
}

void ShaderWrapper::onShaderChanged(const std::string& fileName, const std::string& source)
{

}

using namespace CEGUI;
ShaderProvider* ShaderWrapper::shaderProvider()
{
    if (m_shaderProvider)
        return m_shaderProvider;
    return s_defaultProvider;
}

ShaderProvider::ShaderProvider()
{

}

void ShaderProvider::addShader(const std::string, ShaderWrapper* shader)
{

}

void ShaderProvider::removeShader(ShaderWrapper* shader)
{

}

void ShaderProvider::registerEvents(GUIApplication* app)
{
    app->addEventHandler("shaderEditor", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, app));
    app->addEventHandler("shaderEditor/list", Listbox::EventSelectionChanged, Event::Subscriber(&ShaderProvider::onShaderSelect, this));
    app->addEventHandler("shaderEditor/source/saveBtn", Window::EventMouseClick, Event::Subscriber(&ShaderProvider::onShaderAccept, this));
}

bool ShaderProvider::onShaderSelect(const EventArgs& args)
{
    return true;
}

bool ShaderProvider::onShaderAccept(const EventArgs& args)
{
    return true;
}
