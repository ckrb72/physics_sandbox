#version 410 core

in vec3 f_norm;
in vec3 f_pos;
in vec2 f_tex;

out vec4 final_color;

uniform sampler2D diffuse0;

void main()
{
    //final_color = vec4(f_tex, 0.0, 1.0);
    final_color = texture(diffuse0, f_tex);
    //final_color = vec4(f_norm, 1.0);
}