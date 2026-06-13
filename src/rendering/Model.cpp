#include <Model.hpp>

void Model::loadGlbModel(std::string path)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices
    );
	
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    processAssimpNode(scene->mRootNode, scene);
}

void Model::Draw(Shader &shader)
{
    for(unsigned int meshIndex = 0; meshIndex < meshes.size(); meshIndex++)
        meshes[meshIndex].Draw(shader);
}

void Model::processAssimpNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processAssimpMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processAssimpNode(node->mChildren[i], scene);
    }
}
Mesh Model::processAssimpMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    extractVerticesFromAssimpMesh(mesh, &vertices);
    extractIndicesFromAssimpMesh(mesh, &indices);
    loadGlbPbrMaterialTextures(mesh, scene, &textures);
    
    return Mesh(vertices, indices, textures);
}

void Model::extractVerticesFromAssimpMesh(aiMesh *mesh, std::vector<Vertex> *vertices)
{
    for(unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++)
    {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec3 position, normal; 
        position.x = mesh->mVertices[vertexIndex].x;
        position.y = mesh->mVertices[vertexIndex].y;
        position.z = mesh->mVertices[vertexIndex].z; 
        vertex.Position = position;

        normal.x = mesh->mNormals[vertexIndex].x;
        normal.y = mesh->mNormals[vertexIndex].y;
        normal.z = mesh->mNormals[vertexIndex].z;
        vertex.Normal = normal;

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 meshCoords;
            meshCoords.x = mesh->mTextureCoords[0][vertexIndex].x; 
            meshCoords.y = mesh->mTextureCoords[0][vertexIndex].y;
            vertex.TexCoords = meshCoords;
        }
        else vertex.TexCoords = glm::vec2(0.0f, 0.0f);  

        vertices->push_back(vertex);
    }
}

void Model::extractIndicesFromAssimpMesh(aiMesh *mesh, std::vector<unsigned int> *indices)
{
    for(unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
    {
        aiFace face = mesh->mFaces[faceIndex];
        
        for(unsigned int index = 0; index < face.mNumIndices; index++)
        {
            indices->push_back(face.mIndices[index]);
        }
    }  
}

void Model::loadGlbPbrMaterialTextures(aiMesh *mesh, const aiScene *scene, std::vector<Texture> *textures)
{
    if(mesh->mMaterialIndex < 0)
        return;

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    // glTF / GLB PBR maps

    auto baseColorMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_BASE_COLOR, "texture_albedo");
    textures->insert(textures->end(), baseColorMaps.begin(), baseColorMaps.end());

    auto normalMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_NORMALS, "texture_normal");
    textures->insert(textures->end(), normalMaps.begin(), normalMaps.end());

    auto metallicMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_UNKNOWN, "texture_metallic");
    textures->insert(textures->end(), metallicMaps.begin(), metallicMaps.end());

    auto roughnessMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
    textures->insert(textures->end(), roughnessMaps.begin(), roughnessMaps.end());

    auto aoMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao");
    textures->insert(textures->end(), aoMaps.begin(), aoMaps.end());

    auto emissiveMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_EMISSIVE, "texture_emissive");
    textures->insert(textures->end(), emissiveMaps.begin(), emissiveMaps.end());

    auto unknownMaps =
        loadTexturesFromAssimpMaterial(scene, material, aiTextureType_UNKNOWN, "texture_metallicRoughness");
    textures->insert(textures->end(), unknownMaps.begin(), unknownMaps.end());
}

std::vector<Texture> Model::loadTexturesFromAssimpMaterial(
    const aiScene *scene,
    aiMaterial *mat,
    aiTextureType type,
    std::string typeName)
{
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool skip = false;

        for (const Texture& tex : textures_loaded)
        {
            if (std::strcmp(tex.path.data(), str.C_Str()) == 0)
            {
                textures.push_back(tex);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;
            bool isSRGB = typeName == "texture_albedo" || typeName == "texture_emissive";

            texture.id = loadTextureFromAssimpPath(scene, str, directory, isSRGB);
            texture.type = typeName;
            texture.path = str.C_Str();

            textures.push_back(texture);
            textures_loaded.push_back(texture);

            std::cout << "Loaded texture: "
                      << texture.path
                      << " as "
                      << texture.type
                      << " id: "
                      << texture.id
                      << '\n';
        }
    }

    return textures;
}

unsigned int Model::loadTextureFromAssimpPath(
    const aiScene *scene,
    const aiString &path,
    const std::string &directory,
    bool isSRGB)
{
    std::string texturePath = path.C_Str();

    if (!texturePath.empty() && texturePath[0] == '*')
    {
        int textureIndex = std::stoi(texturePath.substr(1));

        if (textureIndex < 0 || textureIndex >= static_cast<int>(scene->mNumTextures))
        {
            std::cout << "Invalid embedded texture index: " << texturePath << std::endl;
            return 0;
        }

        const aiTexture *embeddedTexture = scene->mTextures[textureIndex];
        return loadEmbeddedGlbTexture(embeddedTexture, isSRGB);
    }

    return loadExternalTextureFile(texturePath.c_str(), directory, isSRGB);
}

unsigned int Model::loadEmbeddedGlbTexture(
    const aiTexture *texture,
    bool isSRGB)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = nullptr;

    if (texture->mHeight == 0)
    {
        // Compressed texture, usually PNG/JPG embedded in GLB.
        data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(texture->pcData),
            texture->mWidth,
            &width,
            &height,
            &nrComponents,
            0
        );
    }
    else
    {
        // Uncompressed aiTexel data.
        width = texture->mWidth;
        height = texture->mHeight;
        nrComponents = 4;
        data = reinterpret_cast<unsigned char*>(texture->pcData);
    }

    if (!data)
    {
        std::cout << "Failed to load embedded texture" << std::endl;
        return 0;
    }

    GLenum dataFormat = GL_RGB;
    GLenum internalFormat = GL_RGB;

    if (nrComponents == 1)
    {
        dataFormat = GL_RED;
        internalFormat = GL_RED;
    }
    else if (nrComponents == 3)
    {
        dataFormat = GL_RGB;
        internalFormat = isSRGB ? GL_SRGB : GL_RGB;
    }
    else if (nrComponents == 4)
    {
        dataFormat = GL_RGBA;
        internalFormat = isSRGB ? GL_SRGB_ALPHA : GL_RGBA;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (texture->mHeight == 0)
    {
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int Model::loadExternalTextureFile(
    const char *path,
    const std::string &directory,
    bool isSRGB)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);

    if (data)
    {
        GLenum format = GL_RGB;

        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
