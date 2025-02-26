#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <string>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <queue>
#include "ThreadPool.h"

#include <stack>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

double cam_radius = 5.0;
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    //glm::vec4 color;
    glm::vec2 tex_coords;
};

struct Texture
{
    uint32_t width, height, channels;
    uint32_t id = UINT32_MAX;
    std::string path;
};

// Note if it's laid out like this a model is essentially a collection of other models itself...
// This allows you to do cool things like wrap a collection of models in a model itself and then just do a draw_model call on that one model 
// to draw all of the models in that collection
struct Model
{
    std::vector<Vertex> vertices;   // Can probably throw this stuff out once we are done with it?
    std::vector<unsigned int> indices;  // Can probably throw this out once we are done with it?
    std::vector<Model> children;
    uint32_t vert_arr = UINT32_MAX, vert_buf = UINT32_MAX, indx_buf = UINT32_MAX;
};

const uint32_t WIN_WIDTH = 1920;
const uint32_t WIN_HEIGHT = 1080;
std::map<int, int> key_map;

static void draw_model(Model& m);
static void load_model_gpu(Model& m);
static void load_scene(const std::string& path);
static Texture load_texture(const std::string& path);
static Model load_model(Assimp::Importer& importer, const std::string& path);
static void keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static void window_resize_callback(GLFWwindow* window, int width, int height);
static uint32_t compile_shader(const std::string& vertex, const std::string& fragment);
static std::string read_file_text(const std::string& path);

double previous_time = 0;
bool cursor_locked = true;

std::mutex model_mutex;
std::queue<Model> models_to_process;

enum ParseState
{
    NONE,
    MESH,
    TEXTURE,
    LIGHT,
    CAMERA,
    OBJECT
};


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
    glfwSetKeyCallback(window, keypress_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "RENDER INIT: Failed to load graphics functions" << std::endl;
        return -1;
    }

    glfwSwapInterval(0);

    std::vector<glm::vec3> vertices = 
    {
        {-0.5, -0.5, 0.5},
        {0.5, -0.5, 0.5},
        {0.5, 0.5, 0.5},
        {-0.5, 0.5, 0.5},

        {-0.5, -0.5, -0.5},
        {0.5, -0.5, -0.5},
        {0.5, 0.5, -0.5},
        {-0.5, 0.5, -0.5}
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,

        5, 4, 7,
        7, 6, 5,

        3, 2, 6,
        6, 7, 3,

        1, 0, 4,
        4, 5, 1,

        4, 0, 3,
        3, 7, 4,

        1, 5, 6,
        6, 2, 1
    };

    uint32_t vertex_buffer, index_buffer, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vertex_buffer);
    glGenBuffers(1, &index_buffer);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
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
    model = glm::scale(model, glm::vec3(0.5));
    model = glm::translate(model, glm::vec3(2.0, 0.0, 0.0));
    previous_time = glfwGetTime();

    float delta_time = 0.0;

    double xpos_prev, ypos_prev;
    glfwGetCursorPos(window, &xpos_prev, &ypos_prev);

    std::mutex importer_mutex;
    std::stack<Assimp::Importer*> importer_stack;
    std::vector<Assimp::Importer> importers(4);

    for(Assimp::Importer& importer : importers)
    {
        importer_stack.push(&importer);
    }

    ThreadPool pool(4);

    pool.enqueue([&](){

        std::unique_lock<std::mutex> lock(importer_mutex);
        Assimp::Importer* importer = importer_stack.top();
        importer_stack.pop();
        lock.unlock();

        // Probably don't want to do all this copying so maybe generate a mesh here and then pass
        // a pointer to it as an argument so there is no need to copy it again
        Model model = load_model(*importer, "../assets/MarcusAurelius/MarcusAurelius.obj");

        lock.lock();
        importer_stack.push(importer);
        lock.unlock();

        std::unique_lock<std::mutex> model_lock(model_mutex);
        models_to_process.push(model);
        model_lock.unlock();
    });
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    bool model_loaded = false;

    Model loaded_model{};

    // Camera Parameters
    double sensitivity = 0.1;
    //double theta = -180.0;    For FPS Camera
    double theta = 0.0;
    double phi = 0.0;

    // For FPS Camera
    //glm::vec3 cam_pos = glm::vec3(0.0, 0.0, 5.0);
    //glm::vec3 cam_dir = glm::vec3(0.0, 0.0, -1.0);
    //glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_dir, glm::vec3(0.0, 1.0, 0.0));

    glm::vec3 cam_pos = glm::vec3(0.0, 0.0, cam_radius);
    glm::mat4 view = glm::lookAt(cam_pos, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

    double angle = 0.0;

    while(!glfwWindowShouldClose(window))
    {

        if(models_to_process.size() > 0 && model_loaded == false)
        {
            std::cout << "Models loaded: " << models_to_process.size() << std::endl;
            model_loaded = true;

            std::unique_lock<std::mutex> lock(model_mutex);
            Model m = models_to_process.front();
            models_to_process.pop();
            lock.unlock();
            
            load_model_gpu(m);
            loaded_model = m;
        }
        
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
        if (phi >= 89.0) phi = 89.0;
        if (phi <= -89.0) phi = -89.0;

        // FPS Camera
        /*cam_dir.x = sin(glm::radians(-theta)) * cos(glm::radians(phi));
        cam_dir.z = cos(glm::radians(-theta)) * cos(glm::radians(phi));
        cam_dir.y = -sin(glm::radians(phi));

        
        if (key_map[GLFW_KEY_W]) cam_pos += cam_dir * delta_time;
        if (key_map[GLFW_KEY_S]) cam_pos -= cam_dir * delta_time;
        if (key_map[GLFW_KEY_D]) cam_pos += glm::normalize(glm::cross(cam_dir, glm::vec3(0.0, 1.0, 0.0))) * delta_time;
        if (key_map[GLFW_KEY_A]) cam_pos -= glm::normalize(glm::cross(cam_dir, glm::vec3(0.0, 1.0, 0.0))) * delta_time;
        if (key_map[GLFW_KEY_SPACE]) cam_pos.y += 1.0 * delta_time;
        if (key_map[GLFW_KEY_LEFT_SHIFT]) cam_pos.y -= 1.0 * delta_time;
        if (key_map[GLFW_KEY_ESCAPE]) break;

        view = glm::lookAt(cam_pos, cam_pos + cam_dir, glm::vec3(0.0, 1.0, 0.0));*/

        // Orbit Camera

        cam_pos.x = cam_radius * sin(glm::radians(-theta)) * cos(glm::radians(phi));
        cam_pos.y = cam_radius * sin(glm::radians(phi));
        cam_pos.z = cam_radius * cos(glm::radians(-theta)) * cos(glm::radians(phi));

        if (key_map[GLFW_KEY_ESCAPE]) break;

        view = glm::lookAt(cam_pos, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.3, 0.3, 0.3, 1.0);

        glUseProgram(shader);
        glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));


        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(unsigned int), GL_UNSIGNED_INT, NULL);

        glm::mat4 model2(1.0);
        model2 = glm::rotate(model2, (float)glm::radians(angle), glm::vec3(0.0, 1.0, 0.0));

        angle += 10.0 * delta_time;

        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model2));
        if (model_loaded) draw_model(loaded_model);

        glfwSwapBuffers(window);
    }

    load_scene("../scenes/test.scene");

    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// TODO: Will definitely have to change this up to bind the uniforms it needs but is fine for
// this first prototype
static void draw_model(Model& m)
{
    if(m.vert_arr != UINT32_MAX)
    {
        glBindVertexArray(m.vert_arr);
        glDrawElements(GL_TRIANGLES, m.indices.size(), GL_UNSIGNED_INT, NULL);
    }

    for(Model& m : m.children)
    {
        draw_model(m);
    }
}


static Texture load_texture(const std::string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if(!data)
    {
        std::cerr << "LOAD TEXTURE: failed to load texture <path: " << path << ">" << std::endl;
        return Texture();
    }

    uint32_t id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    uint32_t format;
    if (channels == 3) format = GL_RGB;
    else format = GL_RGBA;
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    Texture tex{};
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.id = id;
    tex.path = path;

    return tex;
}

static void load_model_gpu(Model& m)
{
    uint32_t vert_arr, vert_buf, indx_buf;

    // Only generate buffers if current Model actually has data
    // (sometimes assimp loads a node that doesn't have any data [usually the root node])
    if (m.vertices.size() > 0)
    {
        glGenVertexArrays(1, &vert_arr);
        glGenBuffers(1, &vert_buf);
        glGenBuffers(1, &indx_buf);
    
        glBindVertexArray(vert_arr);
        glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
        glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(Vertex), m.vertices.data(), GL_STATIC_DRAW);
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
    
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
    
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
        glEnableVertexAttribArray(2);
    
        /*
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(3);
        */
    
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indx_buf);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * sizeof(unsigned int), m.indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        m.vert_arr = vert_arr;
        m.vert_buf = vert_buf;
        m.indx_buf = indx_buf;
    }

    for(Model& m : m.children)
    {
        load_model_gpu(m);
    }
}

static void calculate_aabb(std::vector<glm::vec3>& vertices)
{
    glm::vec3 min = vertices[0];
    glm::vec3 max = vertices[0];
    for(glm::vec3& pos : vertices)
    {
        if (pos.x < min.x) min.x = pos.x;
        if (pos.y < min.y) min.y = pos.y;
        if (pos.z < min.z) min.z = pos.z;

        if (pos.x > max.x) max.x = pos.x;
        if (pos.y > max.y) max.y = pos.y;
        if (pos.z > max.z) max.z = pos.z;
    }
}

static void load_scene(const std::string& path)
{

    std::fstream file(path);
    if(!file.is_open())
    {
        std::cerr << "SCENE PARSE: Failed to open file <path: " << path << ">" << std::endl;
        return;
    }
   
    std::string line;
    ParseState state = NONE;

    // Note: std::getline() strips the retrieved line of '\n' so no need to take it into account
    while(std::getline(file, line))
    {
        // Don't care about empty lines
        if (line.compare("") == 0) continue;
        
        // Get rid of white spaces
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

        // In case I want to set everything to lowercase later on
        //for(char& c : line) c =std::tolower(c);

        // Don't care about comments
        if (line[0] == '#') continue;

        
        // Set up state
        if(line[0] == '<')
        {
            size_t start = line.find('<') + 1; // add 1 so we move past the starting delimiter
            size_t end = line.find('>');
            std::string substr = line.substr(start, end - start);

            if (substr.compare("Meshes") == 0) state = ParseState::MESH;
            else if (substr.compare("Textures") == 0) state = ParseState::TEXTURE;
            else
            {
                std::cerr << "SCENE PARSE: Section <" << substr << "> not valid" << std::endl;
                return;
            }

            continue;
        }

        if(line[0] == '[')
        {
            size_t start = line.find('[') + 1; // add 1 so we move past the starting delimiter
            size_t end = line.find(']');
            std::string substr = line.substr(start, end - start);

            if (substr.compare("Light") == 0) state = ParseState::LIGHT;
            else if (substr.compare("Camera") == 0) state = ParseState::CAMERA;
            else if (substr.compare("Object") == 0) state = ParseState::OBJECT;
            else
            {
                std::cerr << "SCENE PARSE: Object type [" << substr << "] not valid" << std::endl;
                return;
            }
            continue;
        }

        // Continue parsing...

        switch(state)
        {
            case ParseState::NONE:
            continue;
            break;

            case ParseState::MESH:

            break;

            case ParseState::TEXTURE:

            break;

            case ParseState::LIGHT:

            break;

            case ParseState::CAMERA:

            break;

            case ParseState::OBJECT:

            break;

            default: 
            continue;
            break; 
        }

        //std::cout << line << std::endl;
    }

    file.close();
}

static void save_scene(/*const Scene& s */ const std::string& path)
{
    /*
        Save scene to disk
    */
}

static void process_node(Model& model, aiNode* node, const aiScene* scene)
{
    for(int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        

        // Get the vertices
        for(int j = 0; j < mesh->mNumVertices; j++)
        {
            Vertex v;

            glm::vec3 temp;
            temp.x = mesh->mVertices[j].x;
            temp.y = mesh->mVertices[j].y;
            temp.z = mesh->mVertices[j].z;

            v.position = temp;

            temp.x = mesh->mNormals[j].x;
            temp.y = mesh->mNormals[j].y;
            temp.z = mesh->mNormals[j].z;

            v.normal = temp;

            /*if(mesh->mColors[0])
            {
                glm::vec4 temp;
                temp.x = mesh->mColors[0][j].r;
                temp.y = mesh->mColors[0][j].g;
                temp.z = mesh->mColors[0][j].b;
                temp.w = mesh->mColors[0][j].a;
            }*/

            if(mesh->mTextureCoords[0])
            {
                glm::vec2 temp;
                temp.x = mesh->mTextureCoords[0][j].x;
                temp.y = mesh->mTextureCoords[0][j].y;
                v.tex_coords = temp;
            }


            model.vertices.push_back(v);
        }

        // Get the indices
        // For each face get the indices in that face
        for(int j = 0; j < mesh->mNumFaces; j++)
        {
            aiFace* face = &mesh->mFaces[j];
            for(int k = 0; k <face->mNumIndices; k++)
            {
                model.indices.push_back(face->mIndices[k]);
            }
        }

        if(mesh->mMaterialIndex >= 0)
        {
            std::cout << "Found a material" << std::endl;
        }
    }

    for(int i = 0; i < node->mNumChildren; i++)
    {
        // Create new model
        Model child_model{};

        process_node(child_model, node->mChildren[i], scene);

        // Then push back the child_model as a child of the original model
        model.children.push_back(child_model);
    }
}

static Model load_model(Assimp::Importer& importer, const std::string& path)
{
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
    if(scene == nullptr)
    {
        std::cerr << "Could Not Load Model: " << path << std::endl;
        return Model();
    }

    Model model{};

    process_node(model, scene->mRootNode, scene);

    return model;
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cam_radius -= 0.2 * yoffset;
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

/*

class Scene
{
    // Maybe make this a map instead so we easily get objects
    std::vector<Mesh&> m_meshes;
    std::vector<Scene&> m_scenes;

    void add(Mesh& m);
    void add(Scene& s);
    void remove(Mesh& m);
    void remove(Scene& s);
}

int main()
{

    Renderer renderer;
    Scene main_scene;

    main_scene.add(meshes to add);

    main_scene.remove(any mesh we want);

    while(running)
    {
        renderer.render(scene, camera);
        renderer.render(other scene, camera);
        renderer.render(more stuff, camera2);
    }
}

*/