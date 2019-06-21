#version 330

in vec2 ex_texCoords;
flat in ivec2 ex_texture_ids;

uniform sampler2DArray monochromatic_texture_array;
uniform sampler2DArray colored_texture_array;
uniform vec4 fg_color_sRGB;

// Dual source blending
// https://www.khronos.org/opengl/wiki/Blending#Dual_Source_Blending
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_blend_func_extended.txt
// Since OpenGL 3.3
layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 colorMask;

void main()
{
    vec4 alpha_map;

    // If it's colored
    if(ex_texture_ids.y==1) {
        alpha_map = texture(colored_texture_array, vec3(ex_texCoords, ex_texture_ids.x));
        color = alpha_map;
    } else {
        alpha_map = texture(monochromatic_texture_array, vec3(ex_texCoords, ex_texture_ids.x));
        color = fg_color_sRGB;
    }

    colorMask = fg_color_sRGB.a*alpha_map;
}
