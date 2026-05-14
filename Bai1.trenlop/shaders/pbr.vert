#version 460 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoords;
layout(location=3) in vec3 aTangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;  // cho shadow mapping
    mat3 TBN;                // tangent space → normal map
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos         = worldPos.xyz;
    vs_out.TexCoords       = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    vs_out.TBN    = mat3(T, B, N);
    vs_out.Normal = N;

    gl_Position = projection * view * worldPos;
}
