#pragma once
#include <string>
#include <vector>
#include "ThreadPool.h"
#include "Scene.h"

struct Texture{

};


struct Model{

};

class AssetManager
{
    private:
        ThreadPool pool;
        std::vector<Texture> textures;
        std::vector<Model> models;

    public:
        AssetManager(uint8_t thd_count);
        ~AssetManager();

        int32_t load_texture(const std::string& path);
        int32_t load_model(const std::string& path);
        Model& get_model(int32_t id) const;
        Texture& get_texture(int32_t id) const;
        Scene load_scene(const std::string& path);
        int32_t save_scene(const Scene& scene);
};