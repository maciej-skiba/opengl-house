#pragma once

#include <string>
#include <vector>
#include <gfx/Mesh.hpp>
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model 
{
    public:
        Model(const char *path)
        {
            loadGlbModel(path);
        }
        void Draw(Shader &shader);	
    private:
        // model data
        std::vector<Mesh> meshes;
        std::string directory;
        std::vector<Texture> textures_loaded; 

        void loadGlbModel(std::string path);
        void extractVerticesFromAssimpMesh(aiMesh *mesh, std::vector<Vertex> *vertices);
        void extractIndicesFromAssimpMesh(aiMesh *mesh, std::vector<unsigned int> *indices);
        void loadGlbPbrMaterialTextures(aiMesh *mesh, const aiScene *scene, std::vector<Texture> *textures);
        std::vector<Texture> loadTexturesFromAssimpMaterial(const aiScene *scene, aiMaterial *mat, aiTextureType type, std::string typeName);
        unsigned int loadTextureFromAssimpPath(const aiScene *scene, const aiString &path, const std::string &directory, bool isSRGB);
        unsigned int loadEmbeddedGlbTexture(const aiTexture *texture, bool isSRGB);
        unsigned int loadExternalTextureFile(const char *path, const std::string &directory, bool isSRGB);

        void processAssimpNode(aiNode *node, const aiScene *scene);
        Mesh processAssimpMesh(aiMesh *mesh, const aiScene *scene);
};