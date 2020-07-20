#ifndef CONTROL_HPP
#define CONTROL_HPP

void calcMatrixFromInp(GLFWwindow* window);
glm::mat4 getViewMatrix();
glm::mat4 getProjMatrix();
glm::vec3 getPosition();
glm::vec3 getFrontViewDirection();
glm::vec3 getDirection();
#endif // !CONTROL_HPP
