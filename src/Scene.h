#pragma once
#include <vector>
#include <glm/glm.hpp>


struct Light
{
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


class Scene
{
    private:
        std::vector<Light*> lights;
        //std::vector<Model*> models;
    
    public:

        Scene();
        ~Scene();

        /*
        uint32_t add(Model& m);
        uint32_t remove(Model& m);

        // Maybe don't include camera in scenes
        uint32_t add(Camera& c);
        uint32_t remove(Camera& c);
        */

        uint32_t add(Light& l);
        Light& remove(Light& l);

        /*
        void add_pass(RenderPass& pass);
        void remove_pass(RenderPass& pass);
        */
};