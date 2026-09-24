#version 440

layout(location = 0) in vec2 corner;

layout(location = 0) out vec2 textureCoordinate;

layout(std140, binding = 0) uniform Media {
    mat4 projection;
    vec4 area;
    vec4 spin;
};

void main()
{
    textureCoordinate = corner;
    vec2 middle = area.xy + (area.zw * 0.5);
    vec2 fromMiddle = (corner - vec2(0.5)) * area.zw;
    vec2 turned = vec2(fromMiddle.x * spin.x - fromMiddle.y * spin.y,
                       fromMiddle.x * spin.y + fromMiddle.y * spin.x);
    gl_Position = projection * vec4(middle + turned, 0.0, 1.0);
}
