#version 440

layout(location = 0) in vec2 corner;

layout(location = 0) out vec2 textureCoordinate;

layout(std140, binding = 0) uniform Layer {
    vec4 flip;
};

void main()
{
    textureCoordinate = vec2(corner.x, mix(corner.y, 1.0 - corner.y, flip.x));
    gl_Position = vec4((corner * 2.0) - 1.0, 0.0, 1.0);
}
