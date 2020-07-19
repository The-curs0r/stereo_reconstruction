#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <glad.h>
#include <GLFW/glfw3.h>

#include <Windows.h>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using namespace std;

GLFWwindow* window;
const int SCR_WIDTH = 1920;
const int SCR_HEIGHT = 1080;

int w, h, comp;

//vector<unsigned char> left_image;
//  vector<unsigned char> right_image;

unsigned char* left_image;
unsigned char* right_image;

glm::mat3 cameraOne ;
glm::mat3 cameraTwo ;

glm::mat3 calibMatrixOne;
glm::mat3 calibMatrixTwo;

float focalLength;//Initialize
float prinPointX;//Initialize
float prinPointY;//Initialize
float dOffset;//Initialize
float baseline;//Initialize

glm::vec3 points;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int initialize() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Stereo Reconstruction", NULL, NULL);

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);//For Key Input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//For Curs0r Movement
    glfwPollEvents();//Continously Checks For Input
    glfwSetCursorPos(window, 1920 / 2, 1080 / 2);
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    


    return 1;
}



void findPoints() {
    glm::vec3** leftImage = new glm::vec3*[h];
    for (int i = 0; i < h; i++)
        leftImage[i] = new glm::vec3[w];

    glm::vec3** rightImage = new glm::vec3 * [h];
    for (int i = 0; i < h; i++)
        rightImage[i] = new glm::vec3[w];

    int** disp = new int* [h];
    for (int i = 0; i < h; i++)
        disp[i] = new int[w];

    for (int i = 0;i < h;i++)
    {
        for (int j = 0;j < w;j++) {
            glm::vec3 leftImgPixel = glm::vec3((float)*left_image ,(float)*(left_image + 1) ,(float)*(left_image + 2));
            glm::vec3 rightImgPixel = glm::vec3((float)*right_image, (float)*(right_image + 1), (float)*(right_image + 2));
            leftImage[i][j] = leftImgPixel;
            rightImage[i][j] = rightImgPixel;
            left_image += 3;
            right_image += 3;
            if (comp == 4)
            {
                left_image += 1;
                right_image += 1;
            }
            //std::cout << leftImage[i][j][0] << " " << leftImage[i][j][1] << " " << leftImage[i][j][2] << "\n";
        }
        //std::cout << "\n";
    }

    const int dispAbs = 2,windowSizeX=2, windowSizeY = 2;

    ofstream Output_Image("Output.ppm");
    if (Output_Image.is_open())
    {
        Output_Image << "P3\n" << w << " " << h << " 255\n";
    }
    for (int i = 0;i < h;i++) {
        for (int j = 0;j < w;j++) {

            double disparities[2 * dispAbs - 1] = { FLT_MIN };

            for (int d = -dispAbs+1;d < dispAbs;d++) {
                //Calc window cost
                double windowCost = 0.0f;
                double flfr=0.0f, fl2=0.0f, fr2=0.0f;
                for (int k = 0;k <= windowSizeX;k++) {
                    for (int l = 0;l < windowSizeY;l++) {
                        if (j + k >= w || i + l + d >= h || i + l + d < 0 || i + l>=h)
                            continue;
                        double intensityAtPixelLeft = (0.2126 * leftImage[i + l][j + k][0] + 0.7152 * leftImage[i + l][j + k][1] + 0.0722 * leftImage[i + l][j + k][2])/255.0f;
                        double intensityAtPixelRight = (0.2126 * rightImage[i + l + d][j + k][0] + 0.7152 * rightImage[i + l + d][j + k][1] + 0.0722 * rightImage[i + l + d][j + k][2]) / 255.0f;
                        flfr += intensityAtPixelLeft * intensityAtPixelRight;
                        fl2 += intensityAtPixelLeft * intensityAtPixelLeft;
                        fr2 += intensityAtPixelRight * intensityAtPixelRight;
                    }
                }
                windowCost = flfr / (sqrt(fl2*fr2));
                //std::cout << flfr<<" "<<fl2<<" "<< fr2 << " ";
                disparities[d - 1 + dispAbs] = windowCost;
            }
            //std::cout << "\n";

            int bestDisp=0;
            double bestWindowCost = disparities[dispAbs-1];
            for (int d = -dispAbs + 1;d < dispAbs;d++) {
                //std::cout << disparities[d - 1 + dispAbs] << " ";
                if (disparities[d - 1 + dispAbs] > bestWindowCost)
                {
                    bestDisp = d;
                    bestWindowCost = disparities[d - 1 + dispAbs];
                }
            }
            disp[i][j] = bestDisp;
            //if (disp[i][j])
            {
                Output_Image << 255-(int)(255 * (bestWindowCost*10-(int) (bestWindowCost * 10))) << ' ' << 255-(int)(255 * (bestWindowCost * 10 - (int)(bestWindowCost * 10))) << ' ' <<255- (int)(255 * (bestWindowCost * 10 - (int)(bestWindowCost * 10))) << "\n";
                //std::cout << bestWindowCost << "\n";
            }
        }
    }
    Output_Image.close();
    WinExec("cd ..", 1);
    WinExec("magick \"./Output.ppm\" \"./Output.png\"", 1);
    return ;
}

void loadImagesPNG() {

    string imageToLoad = "./Images/L_3.png";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (left_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    imageToLoad = "./Images/R_3.png";
    right_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb_alpha);
    if (right_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    //std::cout << (float)*left_image << " " << (float)*(left_image+1) << " " << (float)*(left_image + 2) << " " << (float)*(left_image + 3)<<"\n";
    return;
}

void loadImagesJPG() {

    string imageToLoad = "./Images/L_1.jpg";
    left_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (left_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    imageToLoad = "./Images/R_1.jpg";
    right_image = stbi_load(imageToLoad.c_str(), &w, &h, &comp, STBI_rgb);
    if (right_image == nullptr) {
        throw(string("Failed to load snow texture"));
    }
    //std::cout << (float)*left_image << " " << (float)*(left_image+1) << " " << (float)*(left_image + 2) << " " << (float)*(left_image + 3)<<"\n";
    return;
}

void genCameraMatrices() {
    cameraOne = glm::mat3(glm::vec3(focalLength, 0.0f, prinPointX), glm::vec3(0.0f, focalLength, prinPointY), glm::vec3(0.0f, 0.0f, 1.0f));
    cameraTwo = glm::mat3(glm::vec3(focalLength, 0.0f, prinPointX + dOffset), glm::vec3(0.0f, focalLength, prinPointY), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat3 temp = glm::mat3(glm::vec3(w,0.0f,0.0f), glm::vec3(0.0f,h,0.0f), glm::vec3(0.0f,0.0f,1.0f));
    calibMatrixOne = temp * cameraOne;
    calibMatrixTwo = temp * cameraTwo;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    
    loadImagesJPG();
    genCameraMatrices();
    findPoints();
    
    if (initialize() < 0)
        return -1;
    
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.4f, 0.7f, 0.1f, 0.0f);   
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
	return 0;
}
