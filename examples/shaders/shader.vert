#version 450

// layout(binding = 0) uniform CameraUBO {
//     mat4 view;
//     mat4 proj;
// } ubo;

layout(location = 0) in vec2 inPosition;

// layout(location = 1) in mat4 inModel; // mat4 takes locations 1,2,3,4
// layout(location = 5) in vec4 inTint;

layout(location = 0) out vec3 fragColor;

void main()
{
    // gl_Position = ubo.proj * ubo.view * inModel * vec4(inPosition, 0.0, 1.0);
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = vec3(1.0, 0.0, 1.0);
}