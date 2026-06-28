#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
    // outColor = vec4(fragColor, 1.0) * max(dot(normal, vec3(1,0,0)), 0.0);
    float lighting = max(dot(normal, vec3(1,0,0)), 0.005);
    outColor = texture(texSampler, uv) * lighting;
}