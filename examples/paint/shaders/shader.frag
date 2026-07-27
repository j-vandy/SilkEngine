#version 450

layout(push_constant) uniform BrushPC {
    vec2 prevPosition;
    vec2 currPosition;
} brush;

layout(location = 0) out vec4 outColor;

const float RADIUS = 25.0;
const float EPSILON = 0.0001;

void main()
{
    // calculate dist to capsule
    vec2 prevToCurr = brush.currPosition - brush.prevPosition;
    vec2 prevToFrag = gl_FragCoord.xy - brush.prevPosition;

    float numerator = dot(prevToCurr, prevToFrag);
    float denominator = dot(prevToCurr, prevToCurr);

    float scalar = numerator / max(abs(denominator), EPSILON);

    float distToLine;
    if (scalar < 0.0)
    {
        distToLine = length(prevToFrag);
    }
    else if (scalar < 1.0)
    {
        vec2 closestPoint = (prevToCurr * scalar) + brush.prevPosition;
        distToLine = length(gl_FragCoord.xy - closestPoint);
    }
    else
    {
        distToLine = length(gl_FragCoord.xy - brush.currPosition);
    }

    if (abs(distToLine) > RADIUS)
    {
        discard;
    }

    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}