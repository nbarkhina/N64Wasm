#version 300 es

precision highp float;

//the texture coordinate that came 
//in from the vertex shader
in vec2 v_TexCoord;

//primary purpose of fragment shader is
//to spit out a color for every pixel
layout(location = 0) out vec4 color;

// uniform vec4 u_Color;
uniform sampler2D u_Texture;

//fragment shader gets called once per pixel
void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);
    color = texColor;
}