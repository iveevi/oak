#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

layout (push_constant) uniform MVP {
        mat4 model;
        mat4 view;
        mat4 proj;
};

layout (location = 0) out vec3 frag_color;

void main()
{
        gl_Position = proj * view * model * vec4(position, 1.0);
        gl_Position.y = -gl_Position.y;
        gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
        frag_color = color;
}
