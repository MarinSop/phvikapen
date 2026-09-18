#version 440

layout(location = 0) in vec2 corner;

layout(location = 0) out vec2 textureCoordinate;

layout(std140, binding = 0) uniform Media {
    mat4 projection;
    vec4 area;
};

void main()
{
    textureCoordinate = corner;
    gl_Position = projection * vec4(area.xy + (corner * area.zw), 0.0, 1.0);
}
