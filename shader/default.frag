#version 410 core

in vec3 f_norm;
in vec2 f_tex;

out vec4 final_color;

uniform sampler2D tex;

void main()
{
    //final_color = texture(tex, f_tex);
    final_color = vec4(f_tex, 0.0, 1.0);
    //final_color = vec4(f_norm, 1.0);
    //final_color = vec4(0.6, 0.6, 0.8, 1.0);
}