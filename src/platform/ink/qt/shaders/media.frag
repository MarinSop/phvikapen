#version 440

layout(location = 0) in vec2 textureCoordinate;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform Media {
    mat4 projection;
    vec4 paper;
};

layout(binding = 1) uniform sampler2D page;

void main()
{
    vec4 color = texture(page, textureCoordinate);
    fragColor = vec4(color.rgb * color.a, color.a);
}
