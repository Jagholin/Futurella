#pragma once

#include <osg/Program>
#include <map>

class ShaderProvider;
class GUIApplication;
namespace CEGUI {
    class String;
    class EventArgs;
}
using CEGUI::String;
using CEGUI::EventArgs;

class ShaderWrapper : public osg::Program
{
public:
    ShaderWrapper();
    virtual ~ShaderWrapper();

    void load(osg::Shader::Type type, const std::string& fileName);

    static void setDefaultShaderProvider(ShaderProvider*);

protected:
    friend class ShaderProvider;

    void onShaderChanged(const std::string& fileName, const std::string& source);
    ShaderProvider* shaderProvider();

    static ShaderProvider* s_defaultProvider;
    ShaderProvider* m_shaderProvider;
    std::vector<osg::ref_ptr<osg::Shader>> m_shaders;
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
