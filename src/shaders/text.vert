#version 330

layout (location=0) in vec4 in_vertex;
layout (location=1) in ivec2 in_texture_ids;

uniform mat4 projection;

out vec2 ex_texCoords;
flat out ivec2 ex_texture_ids;

void main() {
    gl_Position = projection * vec4(in_vertex.xy, 0.0, 1.0);
    ex_texCoords = in_vertex.zw;
    ex_texture_ids = in_texture_ids;
}
