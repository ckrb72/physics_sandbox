#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <string>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

const uint32_t WIN_WIDTH = 1920;
const uint32_t WIN_HEIGHT = 1080;

std::map<int, int> key_map;

static void keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
static void window_resize_callback(GLFWwindow* window, int width, int height);
static uint32_t compile_shader(const std::string& vertex, const std::string& fragment);
static std::string read_file_text(const std::string& path);

double previous_time = 0;

int main()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Physics Sandbox", NULL, NULL);

    if(!window)
    {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }

    glfwSetWindowSizeCallback(window, window_resize_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, keypress_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to load graphics functions" << std::endl;
        return -1;
    }

    glfwSwapInterval(0);


    float vertices[] = 
    {
        -0.5, -0.5, 0.0,
        0.5, -0.5, 0.0,
        0.5, 0.5, 0.0,
        -0.5, 0.5, 0.0
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    uint32_t vertex_buffer, index_buffer, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vertex_buffer);
    glGenBuffers(1, &index_buffer);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    uint32_t shader = compile_shader("../shader/default.vert", "../shader/default.frag");
    uint32_t model_loc = glGetUniformLocation(shader, "model");
    uint32_t view_loc = glGetUniformLocation(shader, "view");
    uint32_t projection_loc = glGetUniformLocation(shader, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0), (double)WIN_WIDTH / (double)WIN_HEIGHT, 0.1, 100.0);
    glm::mat4 model = glm::mat4(1.0);
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0, 1.0, 0.0));

    glm::vec3 cam_pos = glm::vec3(0.0, 0.0, 5.0);
    glm::vec3 cam_dir = glm::vec3(0.0, 0.0, -1.0);
    glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_dir, glm::vec3(0.0, 1.0, 0.0));

    previous_time = glfwGetTime();

    float delta_time = 0.0;

    double xpos_prev, ypos_prev;
    glfwGetCursorPos(window, &xpos_prev, &ypos_prev);

    double sensitivity = 0.1;
    double theta = -180.0;
    double phi = 0.0;

    while(!glfwWindowShouldClose(window))
    {
        // Update Time
        double current_time = glfwGetTime();
        delta_time = (current_time - previous_time);
        previous_time = current_time;

        // Handle Events
        glfwPollEvents();

        // Update Cursor
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        double cursor_dx = xpos - xpos_prev;
        double cursor_dy = ypos - ypos_prev;
        xpos_prev = xpos;
        ypos_prev = ypos;

        // Update Camera
        theta += cursor_dx * sensitivity;
        phi += cursor_dy * sensitivity;
        if (phi >= 180.0) phi = 179.0;
        if (phi <= -180.0) phi = 179.0;

        cam_dir.x = sin(glm::radians(-theta)) * cos(glm::radians(phi));
        cam_dir.z = cos(glm::radians(-theta)) * cos(glm::radians(phi));
        cam_dir.y = -sin(glm::radians(phi));

        
        if (key_map[GLFW_KEY_W]) cam_pos += cam_dir * delta_time;
        if (key_map[GLFW_KEY_S]) cam_pos -= cam_dir * delta_time;
        if (key_map[GLFW_KEY_D]) cam_pos += glm::normalize(glm::cross(cam_dir, glm::vec3(0.0, 1.0, 0.0))) * delta_time;
        if (key_map[GLFW_KEY_A]) cam_pos -= glm::normalize(glm::cross(cam_dir, glm::vec3(0.0, 1.0, 0.0))) * delta_time;
        if (key_map[GLFW_KEY_SPACE]) cam_pos.y += 1.0 * delta_time;
        if (key_map[GLFW_KEY_LEFT_SHIFT]) cam_pos.y -= 1.0 * delta_time;


        view = glm::lookAt(cam_pos, cam_pos + cam_dir, glm::vec3(0.0, 1.0, 0.0));

        std::cout << "Theta: " << theta << " Phi: " << phi << std::endl;

        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.3, 0.3, 0.3, 1.0);

        glUseProgram(shader);
        glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));


        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(unsigned int), GL_UNSIGNED_INT, NULL);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

static void keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        key_map[key] = 1;
    }
    else if (action == GLFW_RELEASE)
    {
        key_map[key] = 0;
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    //std::cout << "X: " << xpos << "Y: " << ypos << std::endl;
}

static void window_resize_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

static uint32_t compile_shader(const std::string& vertex, const std::string& fragment)
{
    int success;
    char log[512];
    
    std::string vertex_source = read_file_text(vertex);
    std::string fragment_source = read_file_text(fragment);

    const char* const vertex_src = vertex_source.c_str();

    uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_src, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, nullptr, log);
        std::cerr << "Vertex: " << std::endl;
        std::cerr << log << std::endl;
        glDeleteShader(vertex_shader);
        return false;
    }

    const char* const fragment_src = fragment_source.c_str();

    uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, nullptr, log);
        std::cerr << "Fragment: " << std::endl;
        std::cerr << log << std::endl;
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return false;
    }


    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, log);

        std::cerr << "Linker: " << std::endl;
        std::cerr << log << std::endl;


        glDetachShader(program, vertex_shader);
        glDetachShader(program, fragment_shader);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(program);

        return false;
    }

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

static std::string read_file_text(const std::string& path)
{
    std::ifstream file;
    file.open(path, std::ios::in);

    if(!file.is_open())
    {
        std::cout << "failed to open file" << std::endl;

        return "";
    }

    std::string str;
    std::string line;

    while(std::getline(file, line))
    {
        str += line + "\n";
    }

    file.close();

    return str;
}