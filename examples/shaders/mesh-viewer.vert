#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(push_constant) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 color;
    vec3 light_direction;
};

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_light_direction;

void main()
{
    gl_Position = proj * view * model * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

    mat3 mv = mat3(view * model);

    out_color = color;
    out_normal = normalize(mv * normal);
    out_light_direction = light_direction;
}
