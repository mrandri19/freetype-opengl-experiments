#version 400

in vec2 ex_texCoords;
out vec4 gl_FragColor;

uniform sampler2D glyph_texture;

uniform vec4 fg_color;
uniform vec4 bg_color;

const float GAMMA = 1.43f;
const vec4 gamma = vec4(GAMMA, GAMMA, GAMMA, GAMMA);
const vec4 inv_gamma = 1.0f/gamma;

void main()
{
    vec4 alpha_map = texture(glyph_texture, ex_texCoords.xy);

    vec4 fg_color_linear = pow(fg_color, inv_gamma);
    vec4 bg_color_linear = pow(bg_color, inv_gamma);

    float r = alpha_map.r * fg_color_linear.r + (1.0 - alpha_map.r) * bg_color_linear.r;
    float g = alpha_map.g * fg_color_linear.g + (1.0 - alpha_map.g) * bg_color_linear.g;
    float b = alpha_map.b * fg_color_linear.b + (1.0 - alpha_map.b) * bg_color_linear.b;

    float brightness = 1;

    gl_FragColor = pow(vec4(r,g,b,brightness), gamma);
}
