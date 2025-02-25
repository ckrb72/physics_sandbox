#pragma once
#include <vector>
#include <map>
#include <glm/glm.hpp>


struct Light
{
    uint32_t id;
    glm::vec3 position;
    glm::vec3 color;
};

/*
struct DirectionalLight: Light
{

};

struct SpotLight : Light
{

};

struct PointLight : Light
{

};
*/


// Just a wrapper for a render pass pretty much...
// References to all the objects you'd want to render but still need to hold the objects yourself

// Holds all objects we would want to render in a scene.
// Does not take ownership of objects added, just references them.
// It is up to the user to ensure that objects in a scene do not get freed before the scene.
class Scene
{
    private:
        std::vector<Light*> lights;
        //std::vector<Model*> models;
    
    public:

        Scene();
        ~Scene();

        /*
        uint32_t add(Model* m);
        uint32_t remove(Model* m);

        // Maybe don't include camera in scenes
        uint32_t add(Camera* c);
        uint32_t remove(Camera* c);
        */

        uint32_t add(Light* l);
        Light* remove(Light* l);

        /*
        void add_pass(RenderPass& pass);
        void remove_pass(RenderPass& pass);
        */
};