#pragma once

//#include <osg/Program>
#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Shader.h>
#include <map>

using namespace Magnum;

class ShaderProvider;
class GUIApplication;
namespace CEGUI {
    class String;
    class EventArgs;
}
using CEGUI::String;
using CEGUI::EventArgs;

class ShaderWrapper : public AbstractShaderProgram
{
public:
    ShaderWrapper();
    virtual ~ShaderWrapper();

    void load(Shader::Type type, const std::string& fileName);
    void linkit();

    static void setDefaultShaderProvider(ShaderProvider*);
    //virtual void compileGLObjects(osg::State& state) const override;
    void addTransformFeedbackVarying(const std::string& varName);

protected:
    friend class ShaderProvider;

    void onShaderChanged(const std::string& fileName, const std::string& source);
    ShaderProvider* shaderProvider();

    static ShaderProvider* s_defaultProvider;
    ShaderProvider* m_shaderProvider;
    std::vector<Shader*> m_shaders;
    std::vector<std::string> m_feedback;
};

class ShaderProvider
{
public:
    ShaderProvider();

    void addShader(const std::string&, ShaderWrapper* shader);
    void removeShader(ShaderWrapper* shader);
    void registerEvents(GUIApplication* app);
protected:
    
    bool onShaderSelect(const EventArgs& args);
    bool onShaderAccept(const EventArgs& args);
    std::map<std::string, ShaderWrapper*> m_loadedShaders;
};
