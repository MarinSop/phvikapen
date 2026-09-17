#version 440

layout(location = 0) in vec2 corner;

layout(location = 0) out vec2 pagePosition;

layout(std140, binding = 0) uniform Background {
    mat4 projection;
    vec4 view;
    vec4 size;
    vec4 paper;
    vec4 pattern;
    vec4 deskColor;
    vec4 paperColor;
    vec4 lineColor;
    vec4 marginColor;
};

void main()
{
    vec2 viewPosition = corner * size.xy;
    pagePosition = view.xy + viewPosition / view.z;
    gl_Position = projection * vec4(viewPosition, 0.0, 1.0);
}
