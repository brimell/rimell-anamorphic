#include <metal_stdlib>
using namespace metal;

struct SolidUniforms {
    int renderX1;
    int renderY1;
    int renderX2;
    int renderY2;
    int outputX1;
    int outputY1;
    int outputRowFloats;
};

kernel void rimell_solid_colour(
    device float *output [[buffer(0)]],
    constant SolidUniforms &u [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    int x = int(gid.x) + u.renderX1;
    int y = int(gid.y) + u.renderY1;

    if (x >= u.renderX2 || y >= u.renderY2) {
        return;
    }

    int lx = x - u.outputX1;
    int ly = y - u.outputY1;
    int idx = ly * u.outputRowFloats + lx * 4;

    output[idx + 0] = 1.0;
    output[idx + 1] = 0.0;
    output[idx + 2] = 1.0;
    output[idx + 3] = 1.0;
}
