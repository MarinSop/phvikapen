#version 440

layout(location = 0) in vec2 textureCoordinate;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform Layer {
    vec4 flip;
};

layout(binding = 1) uniform sampler2D drawn;

void main()
{
    fragColor = texture(drawn, textureCoordinate);
}
