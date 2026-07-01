#version 450

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform ModelPC {
    mat4 model;
    mat4 normal;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 viewDirVS;
layout(location = 1) out vec3 normalDirVS;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec3 lightDirVS;

void main()
{
    vec4 fragPosVS = ubo.view * pc.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * fragPosVS;
    viewDirVS = -fragPosVS.xyz;
    normalDirVS = (pc.normal * vec4(inNormal, 0.0)).xyz;
    uv = inUV;
    lightDirVS = (ubo.view * vec4(1,0,0,0)).xyz;
}