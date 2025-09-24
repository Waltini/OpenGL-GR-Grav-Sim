#define M_PI        3.14159265358979323846264338327950288   /* pi */

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
#include <cmath>
#include <fstream>

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

	ldvec3 pos1{0.0L, 0.0L, 0.0L};
	ldvec3 v1{0.0L, 0.0L, 0.0L};
	long double m1 = 1;

	ldvec3 pos2{ 0.8L, 0.0L, 0.0L };
	ldvec3 v2{ 0.0L, 1.5 * M_PI, 0.0L };
	long double m2 = 3.003e-6;


	celestial_body sun = celestial_body(pos1, v1, m1);
	celestial_body earth = celestial_body(pos2, v2, m2);

	RK45_integration integrator = RK45_integration(1e-8, 1e-10, 0.05);

	bool show = true;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);


	std::ofstream csv_loop_log("rk45_debug_loop_stats.csv", std::ios::trunc);
	std::ofstream csv_debug_log("rk45_debug_log.csv", std::ios::trunc);
	csv_loop_log << "loop num, max err, min err, sum err, err_count, mean h, attempts, accepts, rejects, max con rejects\n"; // Header of the loop stats
	csv_debug_log << "loop num, t, h, err, next_h\n";

	int loop_num = 0;
	long double loop_t = 0.0L;
	long double sim_t = 0.0L;

	int FPS = 60;
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

		long double t_final = 1.0L, h_total = 0.0L;
		long double min_err = 0.0L, max_err = 0.0L, sum_err = 0.0L;
		int attempts = 0, accepts = 0, con_retries = 0, rejected = 0, err_count = 0, max_con_retries = 0;
		while (loop_t < 1.0L/FPS) {
			debug_values result = integrator.step(sun, earth, loop_t);
			bool accepted = result.accepted;
			long double h_attempt = result.attempt_h;
			long double err = result.err_norm_passed;
			long double next_h = result.next_h;
			h_total += h_attempt;

			if (accepted) {
				min_err = std::min(min_err, err);
				max_err = std::max(max_err, err);
				sum_err += err;
				err_count++;
				attempts++;
				accepts++;
				con_retries = 0;
				std::cout << "[ACCEPT] t = " << loop_t 
						  << ", h = " << h_attempt 
						  << ", err = " << err 
						  << ", adapt_h = " << next_h 
						  << std::endl;
			}
			else {
				attempts++;
				rejected++;
				con_retries++;
				max_con_retries = std::max(max_con_retries, con_retries);
				std::cout << "[REJECT] t = " << loop_t 
						  << ", h = " << h_attempt 
						  << ", err = " << err 
						  << std::endl;
			}
			std::cout << "t = " << loop_t << std::endl;
			std::cout << "h = " << h_attempt << std::endl;
			csv_debug_log << loop_num << ", "
						  << loop_t << ", " 
						  << h_attempt << ", " 
						  << err << ", " 
						  << next_h 
						  << "\n";
		}
		loop_t = 0;
		loop_num++;
		std::cout << "[Error Stats] max_err = " << max_err
				  << ", min_err = " << min_err
				  << ", sum_err = " << sum_err
				  << ", err_count = " << err_count
				  << "mean h = " << h_total / attempts
				  << std::endl;
		std::cout << "[Attempt Stats] attempts = " << attempts 
				  << ", accepted = " << accepts 
				  << ", rejected = " << rejected 
				  << " max consecutive retries = " << max_con_retries
				  << std::endl;
		csv_loop_log << loop_num << ", "
					 << max_err << ", "
					 << min_err << ", "
					 << sum_err << ", "
					 << err_count << ", "
					 << h_total / attempts << ", "
					 << attempts << ", "
					 << accepts << ", "
					 << rejected << ", "
					 << max_con_retries
					 << "\n";


		// Imgui Render
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Buffers are swapped and events are checked and called
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	csv_loop_log.close();
	csv_debug_log.close();
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
