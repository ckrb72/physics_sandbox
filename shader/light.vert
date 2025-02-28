#version 410 core
layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_norm;
layout(location = 2) in vec2 v_tex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 transpose_inverse_model;

out vec3 f_pos;
out vec3 f_norm;
out vec2 f_tex;

void main()
{
    gl_Position = projection * view * model * vec4(v_pos, 1.0);
    f_pos = vec3(model * vec4(v_pos, 1.0));
    f_norm = mat3(transpose_inverse_model) * v_norm;
    f_tex = v_tex;
}