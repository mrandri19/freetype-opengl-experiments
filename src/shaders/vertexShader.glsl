#version 400

in vec2 position;
in vec3 inColor;

out vec3 aColor;

void main()
{
    gl_Position = vec4(position.x,position.y,0.0,1.0);
    aColor = inColor;
}
