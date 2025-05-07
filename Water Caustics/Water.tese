#version 410 core

layout(quads, equal_spacing, ccw) in;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform float time;
uniform float waterHeight;
uniform vec2 waveOrigin;


//Code from GPU Gems example translated into GLSL----------------------------------------------
//Changes:
// -Made it based on XZ plane instead of XY, since Y is usually vertical in OpenGL
// -Changed center of waves so I can test different starting positions

uniform float VTXSIZE;
// Amplitude
uniform float WAVESIZE;
// Frequency
uniform float FACTOR;
uniform float SPEED;
uniform float OCTAVES;
// Example of the same wave function used in the vertex engine
float wave(float x, float z, float timer)
{
    float y = 0.0f;
    float octaves = OCTAVES;
    float factor = FACTOR;

    float dx = x - waveOrigin.x;
    float dz = z - waveOrigin.y;
    float d = sqrt(dx * dx + dz * dz);
    do
    {
        y -= factor * cos(timer * SPEED + (1 / factor) * x * z * WAVESIZE);
        factor = factor / 2.0;
        octaves--;
    } 
    while (octaves > 0);
    return 2 * VTXSIZE * d * y;
} 

//End of GPU Gems code-------------------------------------------------------------------------

vec4 InterpolateVec4(vec4 v0, vec4 v1, vec4 v2, vec4 v3)
{
    vec4 a = mix(v0, v1, gl_TessCoord.x);
    vec4 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

void main()
{
    vec4 loc = InterpolateVec4(gl_in[0].gl_Position,
                              gl_in[1].gl_Position,
                              gl_in[2].gl_Position,
                              gl_in[3].gl_Position);

    loc.y = waterHeight + wave(loc.x, loc.z, time);
    gl_Position = projectionMatrix * viewMatrix * loc;
}