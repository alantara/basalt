#include <GLFW/glfw3.h>
#include <iostream>

int main() {
  glfwInit();
  GLFWwindow* window = glfwCreateWindow(680, 420, "Vulkan 1.3", nullptr, nullptr);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
}
