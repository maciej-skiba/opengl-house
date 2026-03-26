#include "common/gl_includes.hpp"
#include "gfx/MeshUtils.hpp"

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