#version 450

layout(location = 0) in vec3 viewDirVS;
layout(location = 1) in vec3 normalDirVS;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 lightDirVS;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
    vec3 N = normalize(normalDirVS);
    vec3 L = normalize(lightDirVS);

    float diffuse = max(dot(N,L), 0.0);

    vec3 V = normalize(viewDirVS);
    vec3 H = normalize(L + V);
    float shininess = 20.0;
    float spec = 0.0;
    if (diffuse > 0.0)
        spec = pow(max(dot(N, H), 0.0), shininess);

    vec3 texColor = texture(texSampler, uv).xyz; 

    float ambient = 0.01;
    vec3 color = texColor * (diffuse + ambient) + vec3(spec);
    outColor = vec4(color, 1.0);
}