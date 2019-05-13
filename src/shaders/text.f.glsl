#version 400

in vec2 ex_texCoords;
out vec4 gl_FragColor;

uniform sampler2D glyph_texture;

uniform vec4 fg_color;
uniform vec4 bg_color;

const float GAMMA = 2.2f;
const vec4 gamma = vec4(GAMMA, GAMMA, GAMMA, GAMMA);
const vec4 inv_gamma = 1.0f/gamma;

void main()
{
    vec4 texel = texture(glyph_texture, ex_texCoords.xy);

    vec4 fg_color_linear = pow(fg_color, inv_gamma);
    vec4 bg_color_linear = pow(bg_color, inv_gamma);

    float r = texel.r * fg_color_linear.r + (1.0 - texel.r) * bg_color_linear.r;
    float g = texel.g * fg_color_linear.g + (1.0 - texel.g) * bg_color_linear.g;
    float b = texel.b * fg_color_linear.b + (1.0 - texel.b) * bg_color_linear.b;

    float brightness = 1;

    gl_FragColor = pow(vec4(r,g,b,brightness), gamma);
}
