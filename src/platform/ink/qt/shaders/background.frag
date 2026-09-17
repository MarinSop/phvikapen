#version 440

layout(location = 0) in vec2 pagePosition;

layout(location = 0) out vec4 fragColor;

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

float lineCoverage(float coordinate, float spacing, float pixelsPerUnit)
{
    float distanceInPixels = abs(coordinate - spacing * round(coordinate / spacing)) * pixelsPerUnit;
    float halfWidth = pattern.z * 0.5;
    return 1.0 - smoothstep(halfWidth - 0.5, halfWidth + 0.5, distanceInPixels);
}

void main()
{
    bool hasPaper = paper.z > 0.5;
    bool onPaper = !hasPaper
            || (pagePosition.x >= 0.0 && pagePosition.y >= 0.0 && pagePosition.x <= paper.x
                && pagePosition.y <= paper.y);
    if (!onPaper) {
        fragColor = vec4(deskColor.rgb * deskColor.a, deskColor.a);
        return;
    }

    float pixelsPerUnit = view.z * view.w;
    float spacing = pattern.y;
    float kind = pattern.x;
    float visibility = smoothstep(3.0, 8.0, spacing * pixelsPerUnit);
    float coverage = 0.0;
    if (kind > 0.5 && kind < 1.5) {
        if (!hasPaper || pagePosition.y >= pattern.w) {
            coverage = lineCoverage(pagePosition.y, spacing, pixelsPerUnit);
        }
    } else if (kind > 1.5 && kind < 2.5) {
        coverage = max(lineCoverage(pagePosition.x, spacing, pixelsPerUnit),
                       lineCoverage(pagePosition.y, spacing, pixelsPerUnit));
    } else if (kind > 2.5) {
        vec2 nearest = spacing * round(pagePosition / spacing);
        float distanceInPixels = length(pagePosition - nearest) * pixelsPerUnit;
        float radius = pattern.z;
        coverage = 1.0 - smoothstep(radius - 0.5, radius + 0.5, distanceInPixels);
    }

    vec4 color = mix(paperColor, lineColor, coverage * visibility);
    if (hasPaper && kind > 0.5 && kind < 1.5 && paper.w > 0.0) {
        float marginInPixels = abs(pagePosition.x - paper.w) * pixelsPerUnit;
        float halfWidth = pattern.z * 0.5;
        float margin = 1.0 - smoothstep(halfWidth - 0.5, halfWidth + 0.5, marginInPixels);
        color = mix(color, marginColor, margin);
    }
    fragColor = vec4(color.rgb * color.a, color.a);
}
