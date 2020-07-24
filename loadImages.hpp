#ifndef LOADIMAGES_HPP
#define LOADIMAGES_HPP

void loadImagesPNG(unsigned char** left_image, unsigned char** right_image, std::string imageToLoadLeft, std::string imageToLoadRight, GLuint& texture, int& width, int& height, int& comp);
void loadImagesJPG(unsigned char** left_image, unsigned char** right_image, std::string imageToLoadLeft, std::string imageToLoadRight, GLuint& texture, int& width, int& height, int& comp);

#endif // !LOADIMAGES_HPP
