#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define GLM_ENABALE_EXPERIMENTAL

#include "formulae.h"
#include "integration.h"
#include "shaders_c.h"
#include "celestial_body_class.h"
#include "camera_class.h"
#include "objects.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/include/glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <fstream>
#include <filesystem>

#include <queue>
#include <array>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>


int SCR_WIDTH = 600;
int SCR_HEIGHT = 800;

camera cam(-90.0f, 0.0f, 800.0f / 2.0f, 600.0f / 2.0f, 45.0f);

using dvec3 = glm::dvec3;
using dmat43 = glm::mat<4, 3, double>;

namespace fs = std::filesystem;

std::atomic<bool> pause = false;
std::atomic<bool> halted = false;

std::mutex mtx;
std::condition_variable P_cv;
//std::condition_variable H_cv;

struct state { // Snapshot of the program that the user can return to
	dmat43 vectors;
	double m1;
	double m2;

public:
	// Constructors
	state(dmat43 con_y, double con_mass1, double con_mass2) // dmat43, double, double
		: vectors(con_y), m1(con_mass1), m2(con_mass2) { }
	state(dvec3 con_pos1, dvec3 con_vel1, dvec3 con_pos2, dvec3 con_vel2, double con_mass1, double con_mass2) // dvec3, dvec3, dvec3, dvec3, double, double
		: m1(con_mass1), m2(con_mass2) {
		vectors[0] = con_pos1;
		vectors[1] = con_vel1;
		vectors[2] = con_pos2;
		vectors[3] = con_vel2;
	}

	void print() const {
		// Mass printing
		std::cout << "m1 = " << m1 << std::endl;
		std::cout << "m2 = " << m2 << std::endl;
		// Matrix printing
		for (int i = 0; i <= 3; i++) {
			for (int j = 0; j <= 2; j++) {
				std::cout << vectors[i][j] << std::endl;
			}
		}
	}
};

struct Node {
	state value; // Snapshot of the program that this node represents

	struct Node* next; // Pointer indicating to the next node in the dustack // If there is no next node it is set to nullptr

	Node(state val, struct Node* ptr) // Constructor
		: value(val), next(ptr) {}
};

struct dustack { // A dual stack mimicks the operations of flipping between two stacks with one stack evaporating as soon as new data is added to the other
private:
	Node* front = nullptr; // points to the top of the fallback list
	Node* middle = nullptr; // points to the currently accessed data
	Node* back = nullptr; // points to the top node of the constant list
	int sizeNum = 0; // current size of the entire strucuture

public:
	int size() const { return sizeNum; }

	void push(state val, bool output = false) {
		// Creating the newNode
		Node* newNode = new Node(val, nullptr);

		if (sizeNum == 0) { // the simplest scenario // the list is empty/being pushed to for the first time
			middle = newNode;
			sizeNum = 1;
			if (output) {
				std::cout << "[1] " << newNode << std::endl;
			}
			return;
		}

		middle->next = back; // Place the previous state at the top of the constant list
		back = middle; // Move the back pointer to the new top of the constant list

		if (front) { // if there is a fallback list, it deletes each fallback node and removes the size of the fallback list from the sizeNum
			Node* cull = front;
			front = nullptr; // Sets the front pointer to null
			while (cull) {
				Node* next = cull->next;
				delete cull;
				sizeNum--;
				cull = next;
			}
		}

		middle = newNode; // Adds the new node

		if (output) {
			std::cout << "[" << sizeNum + 1 << "] " << newNode << std::endl;
		}
		sizeNum++; // Increases the size

		if (sizeNum > 20) { // In case of error removes each extra node over the maximum
			Node* run = back;
			for (int i = 0; i < 18; i++) { // runs to the end of the list
				run = run->next;
			}
			Node* cull = run->next;
			run->next = nullptr;
			while (sizeNum > 20) { // deletes any overflow nodes
				Node* next = cull->next;
				delete cull;
				sizeNum--;
				cull = next;
			}
		}
	}

	bool undo() {
		if (back) {
			// Place middle node into fallback list
			middle->next = front;
			front = middle;

			// Move top of constant stack into middle
			middle = back;
			back = back->next;
			middle->next = nullptr;
			return true;
		}
		else {
			return false;
		}
	}

	bool redo() {
		if (front) {
			// Move middle node into constant list
			middle->next = back;
			back = middle;

			// Recall top of fallback list into the middle
			middle = front;
			front = front->next;
			middle->next = nullptr;
			return true;
		}
		else {
			return false;
		}
	}

	state read() const { return middle->value; }

	void debug(bool flag) {
		if (flag) {
			// Middle
			std::cout << "[MIDDLE] printed " << middle << std::endl;
			// Back
			std::cout << "[BACK] printed" << back << std::endl;
			// Front
			std::cout << "[FRONT] printed" << front << std::endl;

			if (sizeNum != 1) {
				// Constant
				if (back) {
					std::cout << "[CONSTANT]" << std::endl;
					Node* next = back;
					while (next) {
						std::cout << "[CONSTANT STATE] printed " << next << std::endl;

						next = next->next;
					}

				}

				// Fallback
				if (front) {
					std::cout << "[FALLBACK]" << std::endl;
					Node* next = front;
					while (next) {
						std::cout << "[FALLBACK STATE] printed " << next << std::endl;

						next = next->next;
					}
				}
			}
		}
		else {
			// Middle
			state crt = middle->value;
			std::cout << "[MIDDLE]" << std::endl;
			crt.print();

			if (sizeNum != 1) {
				// Constant
				if (back) {
					std::cout << "[CONSTANT]" << std::endl;
					Node* next = back;
					while (next) {
						state debug = next->value;

						debug.print();

						next = next->next;
					}
				}

				// Fallback
				if (front) {
					std::cout << "[FALLBACK]" << std::endl;
					Node* next = front;
					while (next) {
						state debug = next->value;

						debug.print();

						next = next->next;
					}
				}
			}
		}
	}
};

struct clsState {
	celestial_body* b1;
	celestial_body* b2;
};

struct ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

struct render_object {
	dvec3 pos;
	dvec3 vel;
	dvec3 accl;
	double mass;
	int body_num;

	// Operations
	bool operator==(const render_object& other) const { // Equality Operator
		return pos == other.pos && vel == other.vel && mass == other.mass; // Compares each composite part ensuring all are similar
	}

	bool operator!=(const render_object& other) const { // Inequality Operator
		return !(*this == other); // Utilises the equality operator
	}
};

// Intialising main functions ~ Informs the compiler the functions exist pretty much
static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window, render_object& b1, render_object& b2, float deltaTime);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int button, int scancode, int action, int mods);

// Formatting Functions
enum infields {
	pos,
	vel,
	mass
};

void ImGui_Input_Vector_Fields(infields intp_type, render_object& edit_obj);

class buffer_box {
private:
	mathState backBuffer; // The back buffer. Used for the background mathematics and the buffer used by the physics thread
	clsState frontBuffer; // The front buffer. Used for displaying the position of the bodies and the buffer used by the rendering function within the main thread
	glm::vec2 mousePos; // The mouse buffer. Stores the location of the most recent mouse input
	dustack editStack; // Custom data structure for undoing and redoing edits
	int GUI_ID; // The GUI ID. Informs the rendering what gui (in context of the bodies) to display at a given moment

	void bufferSet(celestial_body& body1, celestial_body& body2) {
		// Constructor Function
		frontBuffer.b1 = &body1;
		frontBuffer.b2 = &body2;
		backBuffer.y[0] = body1.getPos();
		backBuffer.y[1] = body1.getVel();
		backBuffer.y[2] = body2.getPos();
		backBuffer.y[3] = body2.getVel();
		backBuffer.m1 = body1.getMass();
		backBuffer.m2 = body2.getMass();
		backBuffer.physics_time = 0.0;
	}

	void edit(dvec3 pos1_edit, dvec3 vel1_edit, dvec3 pos2_edit, dvec3 vel2_edit, double m1_edit, double m2_edit) { // Changes the values within each buffer.
		// Pack Matrix
		dmat43 dim_edits = { pos1_edit, vel1_edit, pos2_edit, vel2_edit };

		//Back Buffer Edits
		backBuffer.y = dim_edits; // Apply dimensional edits to the backbuffer
		backBuffer.m1 = m1_edit; // Apply the mass edits of b1 to the backbuffer
		backBuffer.m2 = m2_edit; // Apply the mass edits of b2 to the backbuffer

		//Front Buffer Edits
		celestial_body& b1 = *frontBuffer.b1; // Dereference b1
		celestial_body& b2 = *frontBuffer.b2; // Dereference b2

		b1.setMass(m1_edit);
		b2.setMass(m2_edit);

		b1.setPos(pos1_edit);
		b1.setVel(vel1_edit);

		b2.setPos(pos2_edit);
		b2.setVel(vel2_edit);
	}

public:
	buffer_box(celestial_body& body1, celestial_body& body2) {
		bufferSet(body1, body2);
	}

	void changeBuffers() {
		// Copies Backbuffer into Frontbuffer
		dmat43 y = backBuffer.y;
		celestial_body& b1 = *frontBuffer.b1;
		celestial_body& b2 = *frontBuffer.b2;

		b1.setPos(y[0]);
		b1.setVel(y[1]);
		b2.setPos(y[2]);
		b2.setVel(y[3]);
	}

	mathState readBackBuffer() const { return backBuffer; } // returns the back buffer
	clsState readFrontBuffer() const { return frontBuffer; } // returns the back buffer

	void physicsStateUpdate(const dmat43 state) {
		backBuffer.y = state;
	} // Updates the backbuffer with a new state and new time

	void applyEdits(dvec3 pos1_edit, dvec3 vel1_edit, dvec3 pos2_edit, dvec3 vel2_edit, double m1_edit, double m2_edit) { // Update the back buffer and front buffer with a completely new state / simulation
		edit(pos1_edit, vel1_edit, pos2_edit, vel2_edit, m1_edit, m2_edit);

		state val(pos1_edit, vel1_edit, pos2_edit, vel2_edit, m1_edit, m2_edit);

		editStack.push(val);
	}
	void applyEdits(state &edit_state) { // Update the back buffer and front buffer with a completely new state / simulation (state parameter)
		edit(edit_state.vectors[0], edit_state.vectors[1], edit_state.vectors[2], edit_state.vectors[3], edit_state.m1, edit_state.m2);

		editStack.push(edit_state);
	}

	void undoState() {
		editStack.undo();
		state new_val = editStack.read();
		dmat43 vectors = new_val.vectors;
		edit(vectors[0], vectors[1], vectors[2], vectors[3], new_val.m1, new_val.m2);
	}

	void redoState() {
		editStack.redo();
		state new_val = editStack.read();
		dmat43 vectors = new_val.vectors;
		edit(vectors[0], vectors[1], vectors[2], vectors[3], new_val.m1, new_val.m2);
	}

	state readState() { return editStack.read(); }

	// Debug Functions
	void debugBackBuffer() const {
		dmat43 y = backBuffer.y;

		for (int i = 0; i < 4; i = i + 2) {
			std::cout << "bckbuf pos = ";
			for (int j = 0; j < 3; j++) {
				std::cout << std::setprecision(20) << y[i][j] << ", ";
			}
			std::cout << "\n";
			std::cout << "bckbuf vel = ";
			for (int j = 0; j < 3; j++) {
				std::cout << std::setprecision(20) << y[i + 1][j] << ", ";
			}
			std::cout << "\n";
		}
		std::cout << std::setprecision(20) << "bckbuf m1 = " << backBuffer.m1 << "\n";
		std::cout << std::setprecision(20) << "bckbuf m2 = " << backBuffer.m2 << "\n";
	}

	void debugFrontBuffer() const {
		celestial_body& b1 = *frontBuffer.b1;
		celestial_body& b2 = *frontBuffer.b2;

		b1.print();
		b2.print();
	}

	void debugEditLog(bool flag) {
		editStack.debug(flag);
	}

	glm::vec2 getMousePos() const { return mousePos; }
	void setMousePos(const glm::vec2 position) { mousePos = position; }
};

// Nil State
dvec3 empty_vector{ 0.0,0.0,0.0 };
double empty_double = 0.0;
state nil(empty_vector, empty_vector, empty_vector, empty_vector, empty_double, empty_double);

// Initial State
// Body 1 Characteristics
dvec3 pos1{ 0.0, 0.0, 0.0 };
dvec3 v1{ 0.0, 0.0, 0.0 };
double m1 = 1;

// Body 2 Characteristics
dvec3 pos2{ 1.0, 0.0, 0.0 };
dvec3 v2{ 0.0, 2.0 * M_PI, 0.0 };
double m2 = 3.003e-6;

// Creates the celestial body objects
celestial_body body1(pos1, v1, m1);
celestial_body body2(pos2, v2, m2);

buffer_box bufbx = buffer_box(body1, body2);

state earth_sun(pos1, v1, pos2, v2, m1, m2);

state presets[5] = { state(pos1, v1, pos2, v2, m1, m2), nil, nil, nil, nil };

void physics_thread(GLFWwindow* window, RK45_integration& integrator, double sim_speed) {
	double ct, lt = 0.0, accum_t = 0.0;
	double physics_dt = 0.033;
	dmat43 mat{ dvec3{ 0.0 }, dvec3{ 0.0 }, dvec3{ 0.0 }, dvec3{ 0.0 } };
	integrate_result result(mat, 0.0, 0, 0, 0, 0.0); 

	int count = 0, accepts = 0, rejects = 0;

	while (!glfwWindowShouldClose(window)) {
		//halted = true; // Momentarily updates the halted variable so it may be caught if the ImGUI pause function has started
		//H_cv.notify_one(); // Notifies the rendering function IF IT IS WAITING, if it's not waitng nothing happens and this just goes onto the next step

		double lock_time_start = glfwGetTime();

		std::unique_lock<std::mutex> lock_unique(mtx);
		P_cv.wait(lock_unique, [] { return !pause; }); // Mutex unqiue_lock checks if the physics thread has been set to pause
		lock_unique.unlock(); // Unlocks unique_lock

		double lock_time_end = glfwGetTime();
		double lock_duration = lock_time_end - lock_time_start;

		//halted = false; // After checking that it doesn't need to halted it then sets the halted variable to false

		mathState BackBuffer = bufbx.readBackBuffer(); // Reads the current backbuffer

		ct = glfwGetTime();
		double delta = ct - lt - lock_duration; // change in time since last
		lt = ct;
		accum_t += delta;

		while (accum_t >= physics_dt) {
			accum_t -= physics_dt;
			result = integrator.step(BackBuffer, physics_dt); // Steps through the physics given the current state within the backbuffer

			bufbx.physicsStateUpdate(result.state_y);

			std::lock_guard<std::mutex> lock(mtx);
			bufbx.changeBuffers();
		}

		count += result.count;
		accepts += result.accepts;
		rejects += result.rejects;

		//std::cout << "accept ratio = " << accepts / count << std::endl;
		//std::cout << "reject ratio = " << rejects / count << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

void render(GLFWwindow* window, int FPS, glm::vec4 background, bool show, const char* glsl_version) {

	// GLFW Set
	glfwMakeContextCurrent(window); // sets the context of the window to current on the thread

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls    

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Initialises GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialise GLAD" << std::endl;
	}

	fs::path vertexPath = fs::path("assets") / "shader.vs";
	fs::path fragmentPath = fs::path("assets") / "shader.fs";
	Shader myShader(vertexPath.string().c_str(), fragmentPath.string().c_str()); // Points my shader class to my vertex and fragment shader files

	// Render Objects
	render_object body1, body2, body1_edit, body2_edit;
	body1.body_num = 1; body1_edit.body_num = 1;
	body2.body_num = 2; body2_edit.body_num = 2;

	bool edit_f = false; // editing flag to indicate if the user is editing properties
	bool checkpt_f = false;

	glm::mat4 view;	// view matrix, representative of the current position of the where the point of view originates and in what direction
	glm::mat4 projection; // projection matrix, responsible for dictating what is in view / what can be seen

	float deltaTime = 0.0f;	// Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame
	
	objects::Cube cubehead1(1.0f, glm::vec3(1.0f, 0.0f, 0.0f), pos1);
	objects::Cube cubehead2(1.0f, glm::vec3(1.0f, 0.0f, 0.0f), pos2);

	objects::arrow v_arrow_1(glm::vec3{ 0.0f, 2.0f, 0.0f }, glm::vec3{ 0.0f, 3.0f, 0.0f }, glm::vec3{ 1.0f, 0.0f, 0.0f }, 0.5f, 32);
	objects::arrow v_arrow_2(glm::vec3{ 0.0f, 2.0f, 0.0f }, glm::vec3{ 0.0f, 3.0f, 0.0f }, glm::vec3{ 1.0f, 0.0f, 0.0f }, 0.5f, 32);

	objects::arrow a_arrow_1(glm::vec3{ 0.0f, 2.0f, 0.0f }, glm::vec3{ 0.0f, 3.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f, 32);
	objects::arrow a_arrow_2(glm::vec3{ 0.0f, 2.0f, 0.0f }, glm::vec3{ 0.0f, 3.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f, 32);

	//CylinderArrow cylhead1(1.0f, 2.0f, 32, glm::vec3{ 0.0f, 2.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 1.0f, 0.0f, 0.0f });
	//CylinderArrow cylhead2(0.5f, 1.0f, 32, glm::vec3{ 0.0f, -2.0f, 0.0f }, glm::vec3{ 2.0f, -1.0f, 0.0f }, glm::vec3{ 1.0f, 0.0f, 0.0f });

	while (!glfwWindowShouldClose(window))
	{
		// Front Buffer Snapshot
		clsState snapshot = bufbx.readFrontBuffer(); // grabs the pointers for the celestial body class objects
		celestial_body b1 = *snapshot.b1; // De-referenced Body 1
		celestial_body b2 = *snapshot.b2; // De-referenced Body 2

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Snapshot segmenting
		body1.pos = b1.getPos();
		body1.vel = b1.getVel();
		body1.mass = b1.getMass();
		body2.pos = b2.getPos();
		body2.vel = b2.getVel();
		body2.mass = b2.getMass();
		if (!edit_f) {
			body1_edit.pos = body1.pos;
			body1_edit.vel = body1.vel;
			body1_edit.mass = body1.mass;
			body2_edit.pos = body2.pos;
			body2_edit.vel = body2.vel;
			body2_edit.mass = body2.mass;
		}

		// Relative acceleration of the current state
		dvec3 a_rel = PN_acceleration(body1.pos, body2.pos, body1.vel, body2.vel, body1.mass, body2.mass);
		dvec3 a1, a2;
		resolve_rel_accel(a_rel, a1, a2, m1, m2); // Seperates the individual accelerations of each body given the mass ratio
		body1.accl = a1;
		body2.accl = a2;

		// Input processing
		process_input(window, body1, body2, deltaTime);
		glClearColor(background.x, background.y, background.z, background.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Imgui Frame Start
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Test GUI
		if (show) {
			ImGui::Begin("Hello World!");
			if (ImGui::Button("Pause")) {
				pause = true;
				P_cv.notify_one();
			}
			if (ImGui::Button("Unpause")) {
				pause = false;
				edit_f = false;
				checkpt_f = false;
				P_cv.notify_one();
			}
			// Vector Input Fields
			ImGui_Input_Vector_Fields(pos, body1_edit);
			ImGui_Input_Vector_Fields(vel, body1_edit);
			ImGui_Input_Vector_Fields(mass, body1_edit);
			ImGui_Input_Vector_Fields(pos, body2_edit);
			ImGui_Input_Vector_Fields(vel, body2_edit);
			ImGui_Input_Vector_Fields(mass, body2_edit);
			if (body1_edit != body1 || body2_edit != body2) {
				pause = true;
				P_cv.notify_one();
				if (!checkpt_f) {
					bufbx.applyEdits(body1.pos, body1.vel, body2.pos, body2.vel, body1.mass, body2.mass);
					checkpt_f = true;
				}

				bufbx.applyEdits(body1_edit.pos, body1_edit.vel, body2_edit.pos, body2_edit.vel, body1_edit.mass, body2_edit.mass);
			}

			ImGui::End();
		}

		cam.look(view);
		cam.project(projection, SCR_HEIGHT, SCR_WIDTH);

		myShader.use();
		// uniform assigning
		myShader.setMat4("view", view); // sets the calculated view matrix to the uniform variable view matrix called within the vertex shader
		myShader.setMat4("projection", projection); // sets the calculated projection matrix to the uniform variable projection matrix called within the vertex shader

		//// Body 1
		cubehead1.setTransform(body1.pos, 1.0f);

		cubehead1.draw(&myShader);

		//// Body 2
		cubehead2.setTransform(body2.pos, 1.0f);

		cubehead2.draw(&myShader);

		v_arrow_1.transform(body1.pos, body1.vel);
		v_arrow_2.transform(body2.pos, body2.vel);
		a_arrow_1.transform(body1.pos, body1.accl);
		a_arrow_2.transform(body2.pos, body2.accl);

		v_arrow_1.draw(&myShader);
		v_arrow_2.draw(&myShader);
		a_arrow_1.draw(&myShader);
		a_arrow_2.draw(&myShader);

		// Main Menu Bar
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
					pause = true;
					P_cv.notify_one();
					bufbx.undoState();
				}
				if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
					pause = true;
					P_cv.notify_one();
					bufbx.redoState();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Camera")) {
				if (ImGui::BeginMenu("Settings")) {
					cam.settings();
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Reset Camera", "Ctrl+R")) {
					cam.resetCommand();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Presets")) {
				if (ImGui::BeginMenu("Save")) {
					if (ImGui::MenuItem("Preset 1")) {
						presets[0] = bufbx.readState();
					}
					if (ImGui::MenuItem("Preset 2")) {
						presets[1] = bufbx.readState();
					}
					if (ImGui::MenuItem("Preset 3")) {
						presets[2] = bufbx.readState();
					}
					if (ImGui::MenuItem("Preset 4")) {
						presets[3] = bufbx.readState();
					}
					if (ImGui::MenuItem("Preset 5")) {
						presets[4] = bufbx.readState();
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Load")) {
					if (ImGui::MenuItem("Preset 1")) {
						pause = true;
						P_cv.notify_one();
						bufbx.applyEdits(presets[0]);
					}
					if (ImGui::MenuItem("Preset 2")) {
						pause = true;
						P_cv.notify_one();
						bufbx.applyEdits(presets[1]);
					}
					if (ImGui::MenuItem("Preset 3")) {
						pause = true;
						P_cv.notify_one();
						bufbx.applyEdits(presets[2]);
					}
					if (ImGui::MenuItem("Preset 4")) {
						pause = true;
						P_cv.notify_one();
						bufbx.applyEdits(presets[3]);
					}
					if (ImGui::MenuItem("Preset 5")) {
						pause = true;
						P_cv.notify_one();
						bufbx.applyEdits(presets[4]);
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// Imgui Render
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Buffers are swapped and events are checked and called
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}


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
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // sets the framebuffer size callback GLFW will use to the one I programmed
	glfwSetErrorCallback(glfw_error_callback); // sets the error callback GLFW will use to the one I programmed
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	RK45_integration integrator(1e-8, 1e-10, 0.05);

	bool show = true;
	glm::vec4 background(0.5f, 0.5f, 0.5f, 1.0f);

	int FPS = 60;

	integrator.setDebug(false);

	bufbx.applyEdits(pos1, v1, pos2, v2, m1, m2);

	// Threading
	std::thread p(physics_thread, window, std::ref(integrator), 1.0f);

	render(window, FPS, background, show, glsl_version);

	p.join();

	// As soon as the window is set to close, the while loop is passed and then Imgui and glfw is terminated
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
};



// Function definitions ~ kept below the main loop for formatting
static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW ERROR %d: %s\n", error, description); // formatted print ~ using the format of a C error document it prints GLFW ERROR then its error number and then the description
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glfwGetWindowSize(window, &SCR_HEIGHT, &SCR_WIDTH);
	glViewport(0, 0, width, height); // Details how OpenGL should map its NDC (Normalised Device Coordinates) to the display
}

void process_input(GLFWwindow* window, render_object& b1, render_object& b2, float deltaTime) {
	// Camera
	// -------------------------------------------------------------------------------
	// Camera Moving
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cam.move(forward, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cam.move(backward, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cam.move(left, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cam.move(right, deltaTime);

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
		double xpos, ypos;

		glfwGetCursorPos(window, &xpos, &ypos); // grabs the position of cursour

		bufbx.setMousePos((glm::vec2)(xpos, ypos));
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		cam.setLooking(true);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		cam.setLooking(false);
	}
}

void key_callback(GLFWwindow* window, int button, int scancode, int action, int mods) {
	if (button == GLFW_KEY_R && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
		cam.resetCommand();
	}
	if (button == GLFW_KEY_Z && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
		pause = true;
		P_cv.notify_one();
		bufbx.undoState();
	}
	if (button == GLFW_KEY_Y && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
		pause = true;
		P_cv.notify_one();
		bufbx.redoState();
	}
	if (button == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		pause = false;
		P_cv.notify_one();

		glfwSetWindowShouldClose(window, true);
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	cam.looking(xpos, ypos);
}

void ImGui_Input_Vector_Fields(infields intp_type, render_object& edit_obj) {
	ImGui::PushItemWidth(150);

	switch (intp_type) {
	case pos:
	case vel: {
		glm::dvec3* vec = (intp_type == pos) ? &edit_obj.pos : &edit_obj.vel; // selects which vector to use
		const char* label_name = (intp_type == pos) ? "Position" : "Velocity"; // selects the display label for the vector input fields

		char label_x[64], label_y[64], label_z[64];
		snprintf(label_x, sizeof(label_x), "##b%i_%s_x", edit_obj.body_num, label_name); // reformats ID label_x to contain the relevant information
		snprintf(label_y, sizeof(label_y), "##b%i_%s_y", edit_obj.body_num, label_name); // reformats ID label_y to contain the relevant information
		snprintf(label_z, sizeof(label_z), "##b%i_%s_z", edit_obj.body_num, label_name); // reformats ID label_z to contain the relevant information

		ImGui::Text("%s", label_name); // display label
		ImGui::SameLine(); ImGui::InputDouble(label_x, &vec->x, 0.01, 1.0, "%e"); // input field for the x component of the vector
		ImGui::SameLine(); ImGui::InputDouble(label_y, &vec->y, 0.01, 1.0, "%e"); // input field for the y component of the vector
		ImGui::SameLine(); ImGui::InputDouble(label_z, &vec->z, 0.01, 1.0, "%e"); // input field for the z component of the vector
		break;
	}

	case mass: {
		char label_m[64];
		snprintf(label_m, sizeof(label_m), "##b%i_mass", edit_obj.body_num); // reformats the ID label mass to contain the relevant information
		ImGui::Text("Mass");
		double last_mass = edit_obj.mass;
		ImGui::SameLine(); ImGui::InputDouble(label_m, &edit_obj.mass, 0.01, 1.0, "%e"); // input field for the mass
		if (edit_obj.mass <= 0) {
			edit_obj.mass = last_mass;
		}
		break;
	}

	default:
		break;
	}

	ImGui::PopItemWidth();
}