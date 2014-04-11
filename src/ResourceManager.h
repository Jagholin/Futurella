#pragma once

#include <Magnum/ResourceManager.h>
#include <Magnum/Resource.h>
#include <Magnum/AbstractResourceLoader.h>
#include <Magnum/Mesh.h>
#include <Magnum/Buffer.h>
#include <Magnum/Texture.h>
#include <Magnum/CubeMapTexture.h>

#include <Magnum/Trade/AbstractImporter.h>

#include "ShaderWrapper.h"
#include "magnumdefs.h"

typedef ResourceManager<Mesh, Texture2D, Buffer, CubeMapTexture, ShaderWrapper> FuturellaResourceManager;

// Add ResourceLoader classes 

class MeshLoader : public AbstractResourceLoader<Mesh>
{
public:
    explicit MeshLoader();

private:
    void doLoad(ResourceKey key) override;

    std::unique_ptr<Trade::AbstractImporter> m_importer;
    std::unordered_map<ResourceKey, std::string> m_fileMap;
};

class TextureLoader : public AbstractResourceLoader<Texture2D>
{
public:
    explicit TextureLoader();

private:
    void doLoad(ResourceKey key) override;

    std::unique_ptr<Trade::AbstractImporter> m_importer;
    std::unordered_map<ResourceKey, std::string> m_fileMap;
};

class CubeMapLoader : public AbstractResourceLoader<CubeMapTexture>
{
public:
    explicit CubeMapLoader();

private:
    void doLoad(ResourceKey key) override;

    std::unique_ptr<Trade::AbstractImporter> m_importer;
    std::unordered_map<ResourceKey, std::string> m_fileMap;
};

class ShaderLoader : public AbstractResourceLoader<ShaderWrapper>
{
public:
    explicit ShaderLoader();

private:
    void doLoad(ResourceKey key) override;
    std::unordered_map<ResourceKey, std::string> m_fileMap;
};
