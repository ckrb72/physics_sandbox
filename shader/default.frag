#version 410 core
in vec3 f_norm;
out vec4 final_color;

void main()
{
    final_color = vec4(f_norm, 1.0);
    //final_color = vec4(0.6, 0.6, 0.8, 1.0);
}