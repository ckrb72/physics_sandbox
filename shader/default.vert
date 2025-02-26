#version 410 core

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_norm;
layout(location = 2) in vec2 v_tex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 f_norm;
out vec2 f_tex;

void main()
{
    gl_Position = projection * view * model * vec4(v_pos, 1.0);
    f_norm = mat3(transpose(inverse(model))) * v_norm;  // This converts the normals to world space
    // If we wanted to have the normals in camera space we would do this instead
    //f_norm = mat3(transpose(inverse(view * model))) * v_norm;
    f_tex = v_tex;
}