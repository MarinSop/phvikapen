#version 440

layout(location = 0) in vec4 vertexColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(vertexColor.rgb * vertexColor.a, vertexColor.a);
}
