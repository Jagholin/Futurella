#include "ShaderWrapper.h"
#include "GUIApplication.h"

#include <CEGUI/CEGUI.h>

ShaderProvider* ShaderWrapper::s_defaultProvider = nullptr;

ShaderWrapper::ShaderWrapper()
{
    m_shaderProvider = nullptr;
}
ShaderWrapper::~ShaderWrapper()
{
    shaderProvider()->removeShader(this);
}

void ShaderWrapper::load(osg::Shader::Type type, const std::string& fileName)
{
    // Load shader
    osg::Shader *shader = osg::Shader::readShaderFile(type, fileName);
    if (shader) {
        // Add shader to program
        addShader(shader);

        // What files for changes
        shaderProvider()->addShader(fileName, this);

        // Store shader
        m_shaders.push_back(shader);
    }
}

void ShaderWrapper::setDefaultShaderProvider(ShaderProvider* prov)
{
    s_defaultProvider = prov;
}

void ShaderWrapper::onShaderChanged(const std::string& fileName, const std::string& source)
{
    for (osg::ref_ptr<osg::Shader> shader : m_shaders){
        if (shader) {
            if (shader->getFileName() == fileName) {
                // Reload shader
                shader->setShaderSource(source);
                break;
            }
        }
    }
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

void ShaderProvider::addShader(const std::string &fileName, ShaderWrapper* shader)
{
    Listbox *fileList = static_cast<Listbox*>(System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild("shaderEditor/list"));
    ListboxItem *myItem = new ListboxTextItem(fileName);
    fileList->addItem(myItem);

    m_loadedShaders[fileName] = shader;
}

void ShaderProvider::removeShader(ShaderWrapper* shader)
{
    // TODO
}

void ShaderProvider::registerEvents(GUIApplication* app)
{
    app->addEventHandler("shaderEditor", FrameWindow::EventCloseClicked, Event::Subscriber(&GUIApplication::onWindowCloseClicked, app));
    app->addEventHandler("shaderEditor/list", Listbox::EventSelectionChanged, Event::Subscriber(&ShaderProvider::onShaderSelect, this));
    app->addEventHandler("shaderEditor/source/save", Window::EventMouseClick, Event::Subscriber(&ShaderProvider::onShaderAccept, this));
}

bool ShaderProvider::onShaderSelect(const EventArgs& args)
{
    Window* rootWnd = System::getSingleton().getDefaultGUIContext().getRootWindow();
    Window* editBox = rootWnd->getChild("shaderEditor/source");
    Listbox* list = static_cast<Listbox*>(rootWnd->getChild("shaderEditor/list"));
    std::string fileName = list->getFirstSelectedItem()->getText().c_str();
    //editBox->setText()
    std::ifstream shaderFile(fileName);
    std::ostringstream tempStream;
    tempStream << shaderFile.rdbuf();
    std::string fileContent = tempStream.str();
    editBox->setText(fileContent);
    return true;
}

bool ShaderProvider::onShaderAccept(const EventArgs& args)
{
    Window* rootWnd = System::getSingleton().getDefaultGUIContext().getRootWindow();
    Window* editBox = rootWnd->getChild("shaderEditor/source");
    Listbox* list = static_cast<Listbox*>(rootWnd->getChild("shaderEditor/list"));
    std::string fileName = list->getFirstSelectedItem()->getText().c_str();

    std::string updatedSources = editBox->getText().c_str();
    m_loadedShaders[fileName]->onShaderChanged(fileName, updatedSources);
    std::ofstream shaderFile(fileName);
    std::copy(updatedSources.begin(), updatedSources.end(), std::ostream_iterator<char>(shaderFile));

    return true;
}
