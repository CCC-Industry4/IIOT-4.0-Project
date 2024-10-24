// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <mutex>
#include <map>
#include <cstring>
#include "read/read.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif


//MEAGAN BEGIN IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}
//END IMAGE


//This struct defines a "thing" (AKA a PCB) with all the data points it manages
struct Thing {
	int count = 0;
	int sensor1 = 0;
	double sensor2 = 0.0;
	std::string in = "000000000";
	std::array<bool, 4> out;
	std::string message = "Null";
	int stepSize = 0;
	int stepSpeed = 0;
};

struct Label { //These labels are used to
	std::string device_name;	
	bool enabled = false;
	size_t device_index = 0;
};


const int THING_COUNT = 30;						//The number of things in the GUI, must be adjusted in the map as well
const int STRING_SIZE = 12;
												//These arrays/vectors are used to manage the GUI inputs  
static char msg[THING_COUNT][STRING_SIZE];
std::vector<int> step(THING_COUNT, 0);
std::vector<int> spd(THING_COUNT, 5);

static std::map<std::string, Label> device_list {
	{"Thing 01",{"thing1", false, 0}},
	{"Thing 02",{"thing2", false, 1}},
	{"Thing 03",{"thing3", false, 2}},
	{"Thing 04",{"thing4", false, 3}},
	{"Thing 05",{"thing5", false, 4}},
	{"Thing 06",{"thing6", false, 5}},
	{"Thing 07",{"thing7", false, 6}},
	{"Thing 08",{"thing8", false, 7}},
	{"Thing 09",{"thing9", false, 8}},
	{"Thing 10",{"thing10", false, 9}},
	{"Thing 11",{"thing11", false, 10}},
	{"Thing 12",{"thing12", false, 11}},
	{"Thing 13",{"thing13", false, 12}},
	{"Thing 14",{"thing14", false, 13}},
	{"Thing 15",{"thing15", false, 14}},
	{"Thing 16",{"thing16", false, 15}},
	{"Thing 17",{"thing17", false, 16}},
	{"Thing 18",{"thing18", false, 17}},
	{"Thing 19",{"thing19", false, 18}},
	{"Thing 20",{"thing20", false, 19}},
	{"Thing 21",{"thing21", false, 20}},
	{"Thing 22",{"thing22", false, 21}},
	{"Thing 23",{"thing23", false, 22}},
	{"Thing 24",{"thing24", false, 23}},
	{"Thing 25",{"thing25", false, 24}},
	{"Thing 26",{"thing26", false, 25}},
	{"Thing 27",{"thing27", false, 26}},
	{"Thing 28",{"thing28", false, 27}},
	{"Thing 29",{"thing29", false, 28}},
	{"Thing 30",{"thing30", false, 29}},
};

//Hash Table of Things and its Thing Protector
std::mutex thingProtector;
std::unordered_map<std::string, Thing> things;
const std::string PIPE_PATH = "log";								//Name of the FIFO to connect the client and the GUI

static void glfw_error_callback(int error, const char* description){
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

bool isNumC(const char* num){										//A function to check if a C-string is a num (cause dearImgui likes C-strings) 
	if (std::string(num) == "") return false;
	for (size_t i = 0; i < strlen(num); i++){
		if (i == 0 and num[i] == '-') continue;
		if (!isdigit(num[i])) return false;
	}
	return true;
}

void parsing(){
	using std::cout;												//Getting rid of a lot of stds
	using std::endl;
	using std::string;
	using namespace std::chrono_literals;
	string line, device, s2, sensor;
	std::ifstream ins(PIPE_PATH);									//Opening the named pipe thats being fed all the data
	if (!ins) {
		cout << "Pipe not opened!\n";
		exit(1);
	}
	while (true){
		std::this_thread::sleep_for(10ms);							//Pausing the parsing thread to provide time so the GUI can read the data
		string s = readline(ins);									//Reading first line from mosquitto (is it publishing/namespace stuff)
		if (!ins) continue;											//If there's no data skip
		if (s == "") continue;
		if (s.find("PUBLISH") == std::string::npos) continue;		//If the first line does not have PUBLISH in it, skip

		std::stringstream sts1(s);									//Create a string stream to split the first line with the namespace stuff
		string word = readline(sts1, '\'');							//Read in the first part of the line up to where the namespace is, eventually throw it away
		word = readline(sts1, '\'');								//Read the second part that contains the namespace

		std::stringstream sts2(word);								//Make another string stream for that segment of line 1
		line = readline(sts2, '/');									//This will read what line it is (line1, line2, etc...)
		device = readline(sts2, '/'); 								//This will read what device it is  (thing1, thing2, etc...)
		s2 = readline(sts2, '/');									//This will catch either out, in, or count
		if (s2 == "count") sensor = "count";						//If it is count, call the "sensor" count
		else sensor = readline(sts2, '\'');							//Otherwise the sensor is in the next line (sensor1, sensor2, message, stepSpeed, stepSize, etc...)

		string data = readline(ins, '\n');  						//Next line from the named pipe will have the data that we will write to the hash table knowing information we found earlier

		if(!ins){
			std::cout << "Error state\n";
			continue;
		}

		std::lock_guard<std::mutex> lock(thingProtector);			//The lock guard used to prevent two threads reading and writing to the hash table at the same time
		//The next lines map the data into the hash table
		if (sensor == "count"){
			things[device].count = std::stoi(data);
		}
		else if (sensor == "sensor1"){
			things[device].sensor1 = std::stoi(data);
		}
		else if (sensor == "sensor2"){
			things[device].sensor2 = std::stod(data);
		}
		else if (sensor == "message"){
			things[device].message = data;
		}
		else if (sensor == "stepperStep"){ //size -3000 - 3000
			int step = 0;
			if (isNumC(data.c_str())) step = std::stoi(data);
			if (step >= 2000) things[device].stepSize = 2000;
			else if (step <= -2000) things[device].stepSize = -2000;
			else things[device].stepSize = step;
		}
		else if (sensor == "stepperSpeed"){ //speed 1-20
			int speed = 0;
			if (isNumC(data.c_str())) speed = std::stoi(data);
			if (speed >= 20) things[device].stepSpeed = 20;
			else if (speed <= 1) things[device].stepSpeed = 1;
			else things[device].stepSpeed = speed;
		}
		else if (sensor == "digitalIn"){
			things[device].in = data;
		}
		else if (sensor == "out1"){
			if (data == "0" || data == "False") things[device].out[0] = 0;
			else if (data == "1" || data == "True") things[device].out[0] = 1;
		}
		else if (sensor == "out2"){
			if (data == "0" || data == "False") things[device].out[1] = 0;
			else if (data == "1" || data == "True") things[device].out[1] = 1;
		}
		else if (sensor == "out3"){
			if (data == "0" || data == "False") things[device].out[2] = 0;
			else if (data == "1" || data == "True") things[device].out[2] = 1;
		}
		else if (sensor == "out4"){
			if (data == "0" || data == "False") things[device].out[3] = 0;
			else if (data == "1" || data == "True") things[device].out[3] = 1;
		}
	}   
}


// Main code
int main(int, char**)
{
	setenv("MESA_GL_VERSION_OVERRIDE", "4.3", true); 		//Lying to the computer to say we have the right GL version to run DearImgui
	system((std::string("rm ") + PIPE_PATH).c_str());		//Removing previous named pipes
	system((std::string("mkfifo ") + PIPE_PATH).c_str());	//Creating a new named pipe
	system("sudo killall mosquitto_sub");					//Killing all old mosquitto subs to prevent multiple rewrites of the same data
	system("(sudo mosquitto_sub -d -t line1/# > log)&");	//Making a new mosquitto sub
	std::thread t1(parsing);								//Spawning a thread for the parser
	t1.detach();											//Detaching it so it doesn't freeze the GUI


	//Meagan -- TODO: Mess with this to fix lights
//	int light = 1;
	for (std::pair<const std::string, Label> &p : device_list) {
		
		system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out1\" -m \"") + std::to_string(0) + "\" > /dev/null)&").c_str());
		system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out2\" -m \"") + std::to_string(0) + "\" > /dev/null)&").c_str());
		system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out3\" -m \"") + std::to_string(0) + "\" > /dev/null)&").c_str());
		system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out4\" -m \"") + std::to_string(0) + "\" > /dev/null)&").c_str());
//		light++;
	}

	//THIS WORKS FOR AN INDIVIDUAL LIGHT WITHIN A SPECIFIC THING
//	system((std::string("(sudo mosquitto_pub -d -t \"line1/thing10/out/out1\" -m \"") + std::to_string(0) + "\" > /dev/null)&").c_str());
	
	//THIS IS AN EXAMPLE OF HOW TO PUBLISH TO THE LIGHTS
//	system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out4\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "IOT Project GUI", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	
//MEAGAN IMAGE RENDERING
	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;
	bool ret = LoadTextureFromFile("background.png", &my_image_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);
//END IMAGE

	//TODO
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontFromFileTTF("usr/include/imgui/misc/fonts/Cousine-Regular.ttf", 14);
	//Also good to know:
	//ImFont* font1 = io.Fonts->AddFontFromFile("font.ttf", size_pixels);
	//ImGui::PushFont(font1);
	//ImGui::PopFont();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	// Our state

	//Meagan - Possible Windows
	bool show_demo_window = false;
	bool show_list = false;
	bool show_picture = true;
	bool show_menu_bar = true;
	//Meagan - Types of Viewer
	bool small_screen = false;
	bool large_screen = false;

	//Meagan - Color Definitions 
	ImColor backgroundColor(4, 71, 28);
	ImColor black(0, 0, 0);
	ImColor white(255, 255, 255);
	ImColor titleColor(115, 169, 66);
	ImColor titleColor_active(251, 97, 7);
	ImColor checkColor();
//	ImColor buttonGreen(3, 192, 60);
//	ImColor buttonRed(255, 0, 0);
	ImVec4 currentButton(0, 0, 0, 1);	
	ImVec4 buttonGreen = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
	ImVec4 buttonRed = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	//Background Color
	ImVec4 clear_color = ImVec4(black);

	//Meagan - Sets us up to allow for window positioning
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 base_pos = viewport->Pos;
	ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
	float maxView_x = viewportSize.x;
	float maxView_y = viewportSize.y;

	//TODO THIS IS FOR SCALING 
	ImGuiStyle& style = ImGui::GetStyle(); 						//These lines make everything bigger
	style.ScaleAllSizes(1.f); //Change this number to adjust the scaling
		

	//Meagan - Color Styling
	style.Colors[ImGuiCol_WindowBg] = ImVec4(backgroundColor);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(black);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(titleColor);
	style.Colors[ImGuiCol_Button] = ImVec4(black);
	style.Colors[ImGuiCol_ButtonHovered] = currentButton;
	style.Colors[ImGuiCol_ButtonActive] = currentButton;
	style.Colors[ImGuiCol_FrameBg] = ImVec4(black);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(black);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(black);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(white);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(backgroundColor);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(backgroundColor);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(backgroundColor);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(backgroundColor);

	//Meagan - Hidden Columns
	bool showDetails = false;

	// Main loop
#ifdef __EMSCRIPTEN__
	// For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
	// You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
	io.IniFilename = nullptr;
	EMSCRIPTEN_MAINLOOP_BEGIN
#else
		while (!glfwWindowShouldClose(window))
#endif
		{
			//std::thread t1(parsing);
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).

			//BEGIN MENU BAR
			if (show_menu_bar){
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(white));
				ImGui::BeginMainMenuBar();

				if (ImGui::BeginMenu("Menu")){
					ImGui::MenuItem("List of Things", NULL, &show_list);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Choose Your Viewer")){
					ImGui::MenuItem("Small Screen", NULL, &small_screen);
					if (small_screen) {
						large_screen = false;
					}
					ImGui::MenuItem("Large Screen", NULL, &large_screen);
					if (large_screen){
						small_screen = false;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Things")){
					for (std::pair<const std::string, Label> &p : device_list) {
						ImGui::MenuItem(p.first.c_str(), NULL, &p.second.enabled);
					}
					ImGui::EndMenu();
				}
				
				ImGui::EndMainMenuBar();
				ImGui::PopStyleColor();
			}
			//END MENU BAR

			//BEGIN STYLING BASED ON WINDOW SIZE
			if (small_screen){
			}
			if (large_screen){
			}
			//END STYLING BASED ON WINDOW SIZE

			//BEGIN DEMO WINDOW
			if (show_demo_window)// ImGui::ShowDemoWindow(&show_demo_window);
			// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
			{

				//std::thread t1(parsing);
				//static float f = 0.0f;
				static int counter = 0;
				ImGui::Begin("Welcome to the IOT Project!");            // Create a window called "Hello, world!" and append into it.
				ImGui::Text("Here's a list of options:");               // Display some text (you can use a format strings too)
				ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
				ImGui::Checkbox("List of Things", &show_list);
				//ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
				if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
					counter++;
				ImGui::SameLine();
				ImGui::Text("counter = %d", counter);
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
				ImGui::End();
			}
			//END DEMO WINDOW

			//BEGIN PICTURE WINDOW
			ImGui::SetNextWindowPos(ImVec2(base_pos.x, base_pos.y + 25)); //Set the position of the next window
			ImGui::SetNextWindowSize(ImVec2(maxView_x, maxView_y), ImGuiCond_FirstUseEver);
			ImVec2 test;
			if (show_picture){
				ImGui::Begin("Example Picture Window");
				//ImGui::Text("pointer = %p", my_image_texture);
				//ImGui::Text("size = %f x %f", my_image_width * .5, my_image_height * .5);
				ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width * 0.5, my_image_height * 0.5));
				
				test = ImGui::GetWindowSize();

				ImGui::End();
			}
			//END PICTURE WINDOW

			//BEGIN LIST OF THINGS WINDOW
			// 3. Show another simple window.


			if (show_list){
				ImGui::Begin("List of Things", &show_list);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
				ImGui::Text("Select the thing you would like to see data for:");

			//BEGIN FOR LOOP TO LIST THINGS
				for (std::pair<const std::string, Label> &p : device_list) {
					ImGui::Checkbox(p.first.c_str(), &p.second.enabled);
				}
				if (ImGui::Button("Close Me")) show_list = false;
				ImGui::End();
			}
			//END FOR LOOP TO LIST THINGS

//			ImGui::SetNextWindowSizeConstraints(ImVec2(test.x, test.y), ImVec2(test.x, test.y));
			ImGui::SetNextWindowPos(ImVec2(base_pos.x, base_pos.y + 25)); //Set the position of the next window
			if (showDetails){
				ImGui::SetNextWindowSize(ImVec2(test.x - 10, 0));
			}
			else {
				ImGui::SetNextWindowSize(ImVec2(my_image_width * 0.5, 0));
			}
			//BEGIN FIGURING OUT WHICH THING YOU CLICKED AND DISPLAYS THE DATA FOR THE SENSORS
			for (std::pair<const std::string, Label> &p : device_list){
				if (p.second.enabled){
					ImGui::Begin(p.first.c_str(), &p.second.enabled, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
					if (!showDetails){
						ImGui::Checkbox("Show Details", &showDetails);
						ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width * 0.5, my_image_height * 0.5));
					}
					else {
					ImGui::Columns(2);
					ImGui::SetColumnWidth(0, my_image_width * 0.5);

					Thing thing;
					{
						std::lock_guard<std::mutex> lock(thingProtector);
						thing = things[p.second.device_name];
					}
					ImGui::Checkbox("Show Details", &showDetails);
					if (showDetails) {
						ImGui::SameLine();
						ImGui::Text("Updated: True");
					}
					ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width * 0.5, my_image_height * 0.5));

					ImGui::NextColumn();
					//BEGIN DETAILS COLLAPSE
					if (ImGui::CollapsingHeader("Details:", ImGuiTreeNodeFlags_None)){
						ImGui::AlignTextToFramePadding();
						ImGui::Text("Count: "); 
						ImGui::SameLine();
						ImGui::SmallButton(std::to_string(thing.count).c_str());
/*
						if (thing.out[0] == 1) ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
						else ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
	
						if (ImGui::SameLine(300), ImGui::SmallButton("OUT 0")){
							bool bit = !(thing.out[0]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out1\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}

						ImGui::PopStyleColor();
*/
						ImGui::Text("Sensor1: ");
						ImGui::SameLine();
						ImGui::SmallButton(std::to_string(thing.sensor1).c_str());
/*
						if (thing.out[1] == 1) ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
						else ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);

						if (ImGui::SameLine(300), ImGui::SmallButton("OUT 1")){
							bool bit = !(thing.out[1]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out2\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
*/
						ImGui::Text("Sensor2: ");
						ImGui::SameLine();
						ImGui::SmallButton(std::to_string(thing.sensor2).c_str());

/*
						if (thing.out[2] == 1) ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
						else ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
						if (ImGui::SameLine(300), ImGui::SmallButton("OUT 2")){
							bool bit = !(thing.out[2]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out3\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
*/
						ImGui::Text("Message: ");
						ImGui::SameLine();
						ImGui::SmallButton(thing.message.c_str());

/*
						if (thing.out[3] == 1) ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
						else ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
						if (ImGui::SameLine(300), ImGui::SmallButton("OUT 3")){
							bool bit = !(thing.out[3]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out4\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
*/
						ImGui::Text("Digital In: ");
						ImGui::SameLine();
						ImGui::SmallButton(thing.in.c_str());

						ImGui::Text("Stepper Run: ");
						ImGui::SameLine();
						std::string outputStr = "";
						if (thing.stepSize == 0){
							outputStr = "Stopped";
						}
						else if (thing.stepSize == -1){
							outputStr = "Reversed";
						}
						else if (thing.stepSize == 1){
							outputStr = "Forward";
						}
						ImGui::SmallButton(outputStr.c_str());
						//ImGui::SmallButton(std::to_string(thing.stepSize).c_str());

						ImGui::Text("Stepper Speed: ");
						ImGui::SameLine();
						ImGui::SmallButton(std::to_string(thing.stepSpeed).c_str());
					}//END DETAILS COLLAPSE

					//BEGIN LIGHTS COLLAPSE
					if (ImGui::CollapsingHeader("Lights:", ImGuiTreeNodeFlags_None)){
						//Light 0
						if (thing.out[0] == 1){
							ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
							currentButton = buttonGreen;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						else{
							ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
							currentButton = buttonRed;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
	
						if (ImGui::SmallButton("OUT 0")){
							bool bit = !(thing.out[0]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out1\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
						
						//Light 1
						if (thing.out[1] == 1){
							ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
							currentButton = buttonGreen;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						else{
							ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
							currentButton = buttonRed;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}

						if (ImGui::SmallButton("OUT 1")){
							bool bit = !(thing.out[1]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out2\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
						
						//Light 2
						if (thing.out[2] == 1){
							ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
							currentButton = buttonGreen;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						else{
							ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
							currentButton = buttonRed;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						if (ImGui::SmallButton("OUT 2")){
							bool bit = !(thing.out[2]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out3\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();

						//Light 3
						if (thing.out[3] == 1){
							ImGui::PushStyleColor(ImGuiCol_Button, buttonGreen);
							currentButton = buttonGreen;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						else{
							ImGui::PushStyleColor(ImGuiCol_Button, buttonRed);
							currentButton = buttonRed;
							style.Colors[ImGuiCol_ButtonHovered] = currentButton;
						}
						if (ImGui::SmallButton("OUT 3")){
							bool bit = !(thing.out[3]);
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/out4\" -m \"") + std::to_string(bit) + "\" > /dev/null)&").c_str());
						}
						ImGui::PopStyleColor();
					}//END LIGHT COLLAPSE


					//BEGIN PUBLISH COLLAPSE
					if (ImGui::CollapsingHeader("Publish Info:", ImGuiTreeNodeFlags_None)){

						//ImGui::Text("Want to Publish?");
/*					
						if (ImGui::SmallButton("Message")){
							if (std::string(msg[p.second.device_index]) != "") system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/message\" -m \"") + std::string(msg[p.second.device_index]) + "\" > /dev/null)&").c_str());	
						}
						ImGui::InputText("Send Message", msg[p.second.device_index], IM_ARRAYSIZE(msg[p.second.device_index]));

*/
					//MEAGAN 09/06
						ImGui::PushItemWidth(-1);
						ImGui::InputText("##Type Message", msg[p.second.device_index], IM_ARRAYSIZE(msg[p.second.device_index]));
						ImGui::PopItemWidth();
						ImGui::Spacing();
						ImGui::SameLine(ImGui::GetColumnWidth() * (1.0/4.0));
						if (ImGui::SmallButton("Update Message")){
							if (std::string(msg[p.second.device_index]) != "") system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/message\" -m \"") + std::string(msg[p.second.device_index]) + "\" > /dev/null)&").c_str());	
						}

/*
						if (ImGui::SmallButton("StepperStep")){
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperStep\" -m \"") + std::to_string(step[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
						ImGui::SliderInt("Update Stepper Step", &(step[p.second.device_index]), -2000, 2000);
*/
					//MEAGAN 09/06
						ImGui::PushItemWidth(-1);
						
						ImGui::RadioButton("Reverse", &(step[p.second.device_index]), -1);
						ImGui::SameLine();
						ImGui::RadioButton("Stop", &(step[p.second.device_index]), 0);
						ImGui::SameLine();
						ImGui::RadioButton("Forward", &(step[p.second.device_index]), 1);

						// MEAGAN - Test this when you get home 10/31
						/*
						if (ImGui::SmallButton("Reverse")){
							step[p.second.device_index] = -1;
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperRun\" -m \"") + std::to_string(step[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
						ImGui::SameLine();
						if (ImGui::SmallButton("Stop")){
							step[p.second.device_index] = 0;
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperRun\" -m \"") + std::to_string(step[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
						ImGui::SameLine();
						if (ImGui::SmallButton("Forward")){
							step[p.second.device_index] = 1;
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperRun\" -m \"") + std::to_string(step[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
						*/

						//ImGui::SliderInt("##Stepper Run", &(step[p.second.device_index]), -2000, 2000);	
						ImGui::PopItemWidth();
						
						ImGui::Spacing();
						ImGui::SameLine(ImGui::GetColumnWidth() * (1.0/4.0));
						if (ImGui::SmallButton("Update StepperRun")){
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperRun\" -m \"") + std::to_string(step[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}	

/*
						if (ImGui::SmallButton("StepperSpeed")){
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperSpeed\" -m \"") + std::to_string(spd[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
						ImGui::SliderInt("Update Stepper Speed", &(spd[p.second.device_index]), 0, 20);
*/
					//MEAGAN 09/06
						ImGui::PushItemWidth(-1);
						ImGui::SliderInt("##Stepper Speed", &(spd[p.second.device_index]), 0, 20);
						ImGui::PopItemWidth();
						ImGui::Spacing();
						ImGui::SameLine(ImGui::GetColumnWidth() * (1.0/4.0));
						if (ImGui::SmallButton("Update StepperSpeed")){
							system((std::string("(sudo mosquitto_pub -d -t \"line1/" + p.second.device_name + "/out/stepperSpeed\" -m \"") + std::to_string(spd[p.second.device_index]) + "\" > /dev/null)&").c_str());
						}
					}//END PUBLISH COLLAPSE
					ImGui::Columns(1);
					ImGui::Unindent();
					}
					ImGui::End();
					
				}
			}
			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
		}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
