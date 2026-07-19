#version 450

layout(push_constant) uniform BrushPC {
    vec2 position;
} brush;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = vec3(0.0, 0.0, 0.0);

    float fragToBrushDist = length(brush.position - gl_FragCoord.xy);
    if (fragToBrushDist < 25)
    {
        color = vec3(1.0, 1.0, 1.0);
    }

    outColor = vec4(color, 1.0);
}