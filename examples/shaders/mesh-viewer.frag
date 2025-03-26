#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_light_direction;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_color, 1.0) * max(dot(in_normal, in_light_direction), 0.0);
}
