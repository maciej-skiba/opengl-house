#include "common/gl_includes.hpp"
#include "gfx/MeshUtils.hpp"
#include <vector>
#include <stb_image.h>

void CreateBoxVao(unsigned int &VAO, unsigned int& VBO, float* boxVertices, int bufferSize)
{
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(float), boxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void GenerateTerrain(int &height, int &width,GLuint &terrainVAO, GLuint &terrainVBO, GLuint &terrainEBO)
{
    // load height map texture
    int nChannels;
    unsigned char *heightMapData = stbi_load(
        "../assets/heightmaps/iceland_heightmap.png",
        &width, &height, &nChannels,
        0);

    // vertex generation
    std::vector<float> vertices;
    float yScale = 64.0f / 256.0f, yShift = -25.5f;  // apply a scale+shift to the height data
    for(unsigned int i = 0; i < height; i++)
    {
        for(unsigned int j = 0; j < width; j++)
        {
            // retrieve texel for (i,j) tex coord
            unsigned char* texel = heightMapData + (j + width * i) * nChannels;
            // raw height at coordinate
            unsigned char y = texel[0];

            // vertex
            vertices.push_back( -height/2.0f + i );        // v.x
            vertices.push_back( (int)y * yScale + yShift); // v.y
            vertices.push_back( -width/2.0f + j );        // v.z
        }
    }

    // index generation
    std::vector<unsigned int> indices;
    for(unsigned int i = 0; i < height-1; i++)       // for each row a.k.a. each strip
    {
        for(unsigned int j = 0; j < width; j++)      // for each column
        {
            for(unsigned int k = 0; k < 2; k++)      // for each side of the strip
            {
                indices.push_back(j + width * (i + k));
            }
        }
    }

    // register VAO
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),       // size of vertices buffer
        &vertices[0],                          // pointer to first element
        GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &terrainEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int), // size of indices buffer
        &indices[0],                           // pointer to first element
        GL_STATIC_DRAW);
}