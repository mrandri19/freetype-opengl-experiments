#version 330

in vec2 ex_texCoords;

uniform sampler2DArray glyph_texture_array;
uniform vec4 fg_color_sRGB;
uniform int colored;
uniform int index;

// Dual source blending
// https://www.khronos.org/opengl/wiki/Blending#Dual_Source_Blending
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_blend_func_extended.txt
// Since OpenGL 3.3
layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 colorMask;

// https://stackoverflow.com/questions/48491340/use-rgb-texture-as-alpha-values-subpixel-font-rendering-in-opengl
// TODO: understand WHY it works, and if this is an actual solution, then write a blog post
void main()
{
    vec4 alpha_map = texture(glyph_texture_array, vec3(ex_texCoords, index));

    if(colored==1) {
        color = alpha_map;
    } else {
        color = fg_color_sRGB;
    }

    colorMask = fg_color_sRGB.a*alpha_map;
}
