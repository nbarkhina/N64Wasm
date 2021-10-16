#version 300 es

precision highp float;

//position and texture coordinates
//come in from the application as inputs
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;

//texture coordinate is just passthrough
//to the fragment shader
out vec2 v_TexCoord;

//vertex shader gets called once per vertex
void main()
{
    //gl_Position is a built in property
    gl_Position = position;
    v_TexCoord = texCoord;
}