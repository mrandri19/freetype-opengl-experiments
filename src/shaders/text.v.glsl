#version 400

in vec4 in_vertex;

uniform mat4 projection;
uniform vec3 bg_color;

out vec2 ex_texCoords;

void main() {
    gl_Position = projection * vec4(in_vertex.xy, 0.0, 1.0);
    ex_texCoords = in_vertex.zw;
}
