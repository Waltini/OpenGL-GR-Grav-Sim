#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "formulae.h"
#include "celestial_body_class.h"
#include "integration.h"

#include <iostream>

// Intialising main functions ~ Informs the compiler the functions exist pretty much
static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 800;

int main(int, char**)
{
	// Intialising GLFW
	if (!glfwInit())
	{
		return -1;
	}

	// Context
	const char* glsl_version = "#version 330 core";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Informs GLFW I'm using OpenGL Version 3.x
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Informs GLFW I'm using OpenGL Version x.3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// Window Creation
	GLFWwindow* window = glfwCreateWindow(SCR_HEIGHT, SCR_WIDTH, "Hello World!!!", NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// GLFW Set
	glfwMakeContextCurrent(window); // sets the context of the window to current on the thread
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // sets the framebuffer size callback GLFW will use to the one I programmed
	glfwSetErrorCallback(glfw_error_callback); // sets the error callback GLFW will use to the one I programmed

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls    

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialises GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialise GLAD" << std::endl;
		return -1;
	}

	//glm::dvec3 pos1("words", "1.0f", 1.0f);
	//glm::dvec3 pos2(0.0f, "0.0f", 0.0f);
	//glm::dvec3 vel1(1.0f, 1.0f, "1.0f");
	//glm::dvec3 vel2(0.0f, 0.0, 0.0f);
	//double m1 = 1;
	//double m2 = 2;

	ldvec3 pos {0.0L, 0.0L, 0.0L};
	ldvec3 v {0.0L, 0.0L, 0.0L};
	long double m = 1;

	celestial_body body1 = celestial_body(pos, v, m);

	body1.print();

	//double dt = 0.0001;

	//stepRK4(body1, body2, dt, true);

	//pos1 = body1.getPos();
	//pos2 = body2.getPos();
	//vel1 = body1.getVel();
	//vel2 = body2.getVel();

	//std::cout << "pos1 = " << pos1.x << " " << pos1.y << " " << pos1.z << std::endl;
	//std::cout << "pos2 = " << pos2.x << " " << pos2.y << " " << pos2.z << std::endl;
	//std::cout << "vel1 = " << vel1.x << " " << vel1.y << " " << vel1.z << std::endl;
	//std::cout << "vel2 = " << vel2.x << " " << vel2.y << " " << vel2.z << std::endl;

	bool show = false;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);

	// Graphics Loop
	while (!glfwWindowShouldClose(window))
	{
		// Input processing
		process_input(window);
		glClearColor(background.x, background.y, background.z, background.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// Imgui Frame Start
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show) {
			ImGui::Begin("Hello World!");
			ImGui::Text("Tester Text");
			ImGui::End();
		}

		// render processing here
		// ...

		// Imgui Render
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Buffers are swapped and events are checked and called
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	// As soon as the window is set to close, the while loop is passed and then Imgui and glfw is terminated
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
};

// Function definitions ~ kept below the main loop for formatting
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW ERROR %d: %s\n", error, description); // formatted print ~ using the format of a C error document it prints GLFW ERROR then its error number and then the description
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height); // Details how OpenGL should map its NDC (Normalised Device Coordinates) to the display
}

void process_input(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) // Terminate command
		glfwSetWindowShouldClose(window, true); // Causes while loop to break
}
