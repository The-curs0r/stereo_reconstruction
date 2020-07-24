#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <Windows.h>
#include <fstream>

#include "loadImages.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void loadImagesPNG(unsigned char** left_image, unsigned char** right_image, string imageToLoadLeft, string imageToLoadRight, GLuint& texture, int& width, int& height, int& comp) {

    *left_image = stbi_load(imageToLoadLeft.c_str(), &width, &height, &comp, STBI_rgb_alpha);
    if (*left_image == nullptr) {
        std::cout << "Failed to load left image" << "\n";
        throw(string("Failed to load left image"));
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (comp == 3) {
        *left_image = stbi_load(imageToLoadLeft.c_str(), &width, &height, &comp, STBI_rgb);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, *left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (comp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, *left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    *right_image = stbi_load(imageToLoadRight.c_str(), &width, &height, &comp, STBI_rgb_alpha);
    if (*right_image == nullptr) {
        std::cout << "Failed to load right image" << "\n";
        throw(string("Failed to load right image"));
    }
    if (comp == 3) {
        *right_image = stbi_load(imageToLoadRight.c_str(), &width, &height, &comp, STBI_rgb);
    }
    std::cout << comp << "\n";

    return;
}

void loadImagesJPG(unsigned char** left_image, unsigned char** right_image, string imageToLoadLeft, string imageToLoadRight, GLuint& texture, int& width, int& height, int& comp) {
    *left_image = stbi_load(imageToLoadLeft.c_str(), &width, &height, &comp, STBI_rgb);
    if (*left_image == nullptr) {
        throw(string("Failed to load left image"));
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (comp == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, *left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else if (comp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, *left_image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    *right_image = stbi_load(imageToLoadRight.c_str(), &width, &height, &comp, STBI_rgb);
    if (*right_image == nullptr) {
        throw(string("Failed to load right image"));
    }
    return;
}