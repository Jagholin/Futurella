#include "ResourceManager.h"

#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Directory.h>

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Trade/ImageData.h>
#include "luawrapper/selene.h"

#ifdef _DEBUG
#define MAGNUM_PLUGINS_IMPORTER_DIR "/plugins-d"
#else
#define MAGNUM_PLUGINS_IMPORTER_DIR "/plugins"
#endif
// TODO

MeshLoader::MeshLoader()
{
    PluginManager::Manager<Trade::AbstractImporter> plugManager(MAGNUM_PLUGINS_IMPORTER_DIR);

    if (!(plugManager.load("ObjImporter") & PluginManager::LoadState::Loaded))
    {
        Debug() << "Cannot find ObjImporter plugin";
        std::exit(1);
    }

    m_importer = plugManager.instance("ObjImporter");

    auto fileNameList = Corrade::Utility::Directory::list("./meshes", Corrade::Utility::Directory::Flag::SkipDirectories);

    for (std::string fileName : fileNameList)
    {
        m_fileMap[fileName] = fileName;
    }
}

void MeshLoader::doLoad(ResourceKey key)
{
    if (m_fileMap.count(key) == 0)
    {
        setNotFound(key);
        return;
    }
    std::string fileName = Corrade::Utility::Directory::join("./meshes", m_fileMap[key]);

    if (!m_importer->openFile(fileName))
    {
        Debug() << "Couldn't import data from file" << fileName;
        setNotFound(key);
        return;
    }

    std::optional<Mesh> mesh;
    std::unique_ptr<Buffer> verts, inds;

    std::tie(mesh, verts, inds) = MeshTools::compile(m_importer->mesh3D(0).value(), BufferUsage::StaticDraw);

    if (verts)
    {
        FuturellaResourceManager::instance().set(m_importer->mesh3DName(0) + "-vert", verts.release());
    }
    if (inds)
    {
        FuturellaResourceManager::instance().set(m_importer->mesh3DName(0) + "-inds", inds.release());
    }
    set(key, new Mesh(std::move(*mesh)));
}

TextureLoader::TextureLoader()
{
    PluginManager::Manager<Trade::AbstractImporter> plugManager(MAGNUM_PLUGINS_IMPORTER_DIR);

    if (!(plugManager.load("PngImporter") & PluginManager::LoadState::Loaded))
    {
        Debug() << "Cannot find ObjImporter plugin";
        std::exit(1);
    }

    m_importer = plugManager.instance("PngImporter");

    auto fileNameList = Corrade::Utility::Directory::list("./images", Corrade::Utility::Directory::Flag::SkipDirectories);

    for (std::string fileName : fileNameList)
    {
        m_fileMap[fileName] = fileName;
    }
}

void TextureLoader::doLoad(ResourceKey key)
{
    if (m_fileMap.count(key) == 0)
    {
        setNotFound(key);
        return;
    }
    std::string fileName = Corrade::Utility::Directory::join("./images", m_fileMap[key]);

    if (!m_importer->openFile(fileName))
    {
        Debug() << "Couldn't import data from file" << fileName;
        setNotFound(key);
        return;
    }

    std::optional<Trade::ImageData2D> image = m_importer->image2D(0);

    if (!image)
    {
        setNotFound(key);
        return;
    }
    set(key, &(new Texture2D())->setImage(0, TextureFormat::RGBA8, *image));
}

CubeMapLoader::CubeMapLoader()
{
    PluginManager::Manager<Trade::AbstractImporter> plugManager(MAGNUM_PLUGINS_IMPORTER_DIR);

    if (!(plugManager.load("PngImporter") & PluginManager::LoadState::Loaded))
    {
        Debug() << "Cannot find ObjImporter plugin";
        std::exit(1);
    }

    m_importer = plugManager.instance("PngImporter");

    auto fileNameList = Corrade::Utility::Directory::list("./images", Corrade::Utility::Directory::Flag::SkipDirectories);

    for (std::string fileName : fileNameList)
    {
        m_fileMap[fileName] = fileName;
    }
}

void CubeMapLoader::doLoad(ResourceKey key)
{
    static sel::State luae;
    std::unique_ptr<CubeMapTexture> result{ new CubeMapTexture };

    if (m_fileMap.count(key) == 0)
    {
        setNotFound(key);
        return;
    }
    std::string fileN = Corrade::Utility::Directory::join("./images", m_fileMap[key]);

    luae["POSITIVE_X"] = (int)CubeMapTexture::Coordinate::PositiveX;
    luae["NEGATIVE_X"] = (int)CubeMapTexture::Coordinate::NegativeX;
    luae["POSITIVE_Y"] = (int)CubeMapTexture::Coordinate::PositiveY;
    luae["NEGATIVE_Y"] = (int)CubeMapTexture::Coordinate::NegativeY;
    luae["POSITIVE_Z"] = (int)CubeMapTexture::Coordinate::PositiveZ;
    luae["NEGATIVE_Z"] = (int)CubeMapTexture::Coordinate::NegativeZ;

    luae["CubeMapTexture"].SetObj(*result, "loadImage", [this](CubeMapTexture& tex, int face, std::string fileName) -> bool{
        if (!m_importer->openFile(fileName))
        {
            Debug() << "Cannot open file" << fileName;
            return false;
        }
        std::optional<Trade::ImageData2D> image = m_importer->image2D(0);
        if (!image)
            return false;
        tex.setImage((CubeMapTexture::Coordinate)face, 0, TextureFormat::RGB8, *image);
        return true;
    });

    luae["loaded"] = [this, key, &result](){
        set(key, result.release());
    };

    luae["notFound"] = [this, key](){
        setNotFound(key);
    };

    luae.Load(fileN);
}

ShaderLoader::ShaderLoader()
{
    auto fileNameList = Corrade::Utility::Directory::list("./shaders", Corrade::Utility::Directory::Flag::SkipDirectories);

    for (std::string fileName : fileNameList)
    {
        m_fileMap[fileName] = fileName;
    }
}

void ShaderLoader::doLoad(ResourceKey key)
{
    static sel::State luae;
    std::unique_ptr<ShaderWrapper> result{ new ShaderWrapper };

    if (m_fileMap.count(key) == 0)
    {
        setNotFound(key);
        return;
    }
    std::string fileN = Corrade::Utility::Directory::join("./shaders", m_fileMap[key]);

    luae["VERTEX"] = Shader::Type::Vertex;
    luae["GEOMETRY"] = Shader::Type::Geometry;
    luae["TESSCONTROL"] = Shader::Type::TessellationControl;
    luae["TESSEVAL"] = Shader::Type::TessellationEvaluation;
    luae["FRAGMENT"] = Shader::Type::Fragment;

    luae["ShaderProgram"].SetObj(*result, "loadShader", &ShaderWrapper::load,
        "link", &ShaderWrapper::linkit,
        "addVaryings", &ShaderWrapper::addTransformFeedbackVarying);

    luae["loaded"] = [this, key, &result]() {
        set(key, result.release());
    };
    luae["notFound"] = [this, key]() {
        setNotFound(key);
    };

    luae.Load(fileN);
}
