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
    vec4 sheets[16];
    vec4 rulings[16];
};

float lineCoverage(float coordinate, float spacing, float pixelsPerUnit, float width)
{
    float distanceInPixels = abs(coordinate - spacing * round(coordinate / spacing)) * pixelsPerUnit;
    float halfWidth = width * 0.5;
    return 1.0 - smoothstep(halfWidth - 0.5, halfWidth + 0.5, distanceInPixels);
}

vec4 sheetColor(vec2 local, vec4 ruling, float pixelsPerUnit)
{
    float kind = ruling.x;
    float spacing = ruling.y;
    float width = ruling.z;
    bool margins = ruling.w > 0.5;
    float visibility = smoothstep(3.0, 8.0, spacing * pixelsPerUnit);
    float coverage = 0.0;
    if (kind > 0.5 && kind < 1.5) {
        if (!margins || local.y >= pattern.w) {
            coverage = lineCoverage(local.y, spacing, pixelsPerUnit, width);
        }
    } else if (kind > 1.5 && kind < 2.5) {
        coverage = max(lineCoverage(local.x, spacing, pixelsPerUnit, width),
                       lineCoverage(local.y, spacing, pixelsPerUnit, width));
    } else if (kind > 2.5) {
        vec2 nearest = spacing * round(local / spacing);
        float distanceInPixels = length(local - nearest) * pixelsPerUnit;
        coverage = 1.0 - smoothstep(width - 0.5, width + 0.5, distanceInPixels);
    }

    vec4 color = mix(paperColor, lineColor, coverage * visibility);
    if (margins && kind > 0.5 && kind < 1.5 && paper.w > 0.0) {
        float marginInPixels = abs(local.x - paper.w) * pixelsPerUnit;
        float halfWidth = width * 0.5;
        float margin = 1.0 - smoothstep(halfWidth - 0.5, halfWidth + 0.5, marginInPixels);
        color = mix(color, marginColor, margin);
    }
    return color;
}

void main()
{
    float pixelsPerUnit = view.z * view.w;
    int count = int(size.z + 0.5);
    if (count == 0) {
        vec4 color = sheetColor(pagePosition, vec4(pattern.xyz, 0.0), pixelsPerUnit);
        fragColor = vec4(color.rgb * color.a, color.a);
        return;
    }

    for (int i = 0; i < count; ++i) {
        vec2 local = pagePosition - sheets[i].xy;
        if (local.x >= 0.0 && local.y >= 0.0 && local.x <= sheets[i].z && local.y <= sheets[i].w) {
            vec4 color = sheetColor(local, rulings[i], pixelsPerUnit);
            fragColor = vec4(color.rgb * color.a, color.a);
            return;
        }
    }

    fragColor = vec4(deskColor.rgb * deskColor.a, deskColor.a);
}
