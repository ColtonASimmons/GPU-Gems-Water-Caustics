#version 330 core 

layout(location=0) out vec4 color;

in vec3 worldPos;
in vec2 vtxc;

uniform float time;
uniform float waterHeight;
uniform vec2 waveOrigin;
uniform float lightDistance;

uniform sampler2D ground;
uniform sampler2D lightMap;

//Code from GPU Gems example translated into GLSL----------------------------------------------
// Changes:
// -Made it based on XZ plane instead of XY, since Y is usually vertical in OpenGL
// -Changed center of waves so I can test different starting positions

uniform float VTXSIZE;
// Amplitude
uniform float WAVESIZE;
// Frequency
uniform float FACTOR;
uniform float SPEED;
uniform float OCTAVES;

// This is a derivative of the wave function.
// It returns the d(wave)/dx and d(wave)/dy partial derivatives.
vec2 gradwave(float x, float z, float timer)
{
    float dYx = 0.0f;
    float dYz = 0.0f;
    float octaves = OCTAVES;
    float factor = FACTOR;

    float dx = x - waveOrigin.x;
    float dz = z - waveOrigin.y;
    float d = sqrt(dx * dx + dz * dz);
    do
    {
        dYx +=
            d * sin(timer * SPEED + (1.0 / factor) * dx * dz * WAVESIZE) * dz * WAVESIZE -
            factor * cos(timer * SPEED + (1.0 / factor) * dx * dz * WAVESIZE) * dx / d;
        dYz +=
            d * sin(timer * SPEED + (1.0 / factor) * dx * dz * WAVESIZE) * dx * WAVESIZE -
            factor * cos(timer * SPEED + (1.0 / factor) * dx * dz * WAVESIZE) * dz / d;
        factor = factor / 2;
        octaves--;
    } 
    while (octaves > 0);
    return vec2(2 * VTXSIZE * dYx, 2 * VTXSIZE * dYz);
}

//End of GPU Gems code-------------------------------------------------------------------------

//thought the GPU gems intercept code was confusing so I wrote a cleaner version
vec3 line_plane_intercept(vec3 rayOrigin, vec3 rayDir, float planeY)
{
    float distance = (planeY - rayOrigin.y) / rayDir.y;
    return rayOrigin + rayDir * distance;
}

void main()
{
    vec2 dxdz =  gradwave(worldPos.x, worldPos.z, time);
    vec3 lightDir = normalize(vec3(-dxdz.x, -lightDistance, -dxdz.y));
    vec3 intercept = line_plane_intercept(worldPos.xyz, lightDir, -waterHeight);

    color = texture(ground, vtxc);
    color += texture(lightMap, intercept.xz * waterHeight);
    color.a = 1.0;
}