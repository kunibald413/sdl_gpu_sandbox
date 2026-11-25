/* 
    refernce:
    https://github.com/KdotJPG/Hex-Fractal-Region-Generator

    This shader code was created with the assistance of AI
*/

#define THREADS 8
#define VARIATIONS 9


cbuffer UniformBuffer : register(b0, space2)
{
    uint Seed;
    int Variety;
    int Steps;
    float Zoom;
    float2 Pan;
    float Time;
    float _Pad;
};

[[vk::image_format("rgba8")]]
RWTexture2D<float4> image : register(u0, space1);

static const float3 PALETTE[VARIATIONS] = {
    float3( 20,  40,  75) / 255.0f, // Deep Ocean Navy
    float3(247, 247, 242) / 255.0f, // Crisp Ivory
    float3(204, 121, 167) / 255.0f, // Soft Rose
    float3( 86, 180, 233) / 255.0f, // Sky Cyan
    float3(240, 228,  66) / 255.0f, // Warm Gold
    float3(213,  94,   0) / 255.0f, // Vibrant Coral Red
    float3(  0, 158, 115) / 255.0f, // Mint Teal
    float3(106,  61, 154) / 255.0f, // Royal Purple
    float3(140,  45,   4) / 255.0f, // Burnt Sienna
};

// PCG4D
uint HashPoint(uint seed, int step, int x, int y)
{
    uint4 v = uint4((uint)x, (uint)y, (uint)step, seed);
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;
    v ^= v >> 16u;
    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;
    return v.x;
}


int GetBaseCell(uint seed, int variety, int x, int y)
{
    return (int)(HashPoint(seed, 0, x, y) % (uint)variety);
}

int PickMidpoint(uint seed, int step, int x, int y, int valA, int valB)
{
    return ((HashPoint(seed, step, x, y) & 1u) != 0u) ? valB : valA;
}

void HandleSmoothWeight(inout int values[3], inout float scores[3], int val, float xsi, float ysi)
{
    float zsi = xsi - ysi;
    float weight = (1.0f - xsi * xsi) * (1.0f - ysi * ysi) * (1.0f - zsi * zsi);

    [unroll]
    for (int i = 0; i < 3; i++)
    {
        if (values[i] == -1 || values[i] == val)
        {
            values[i] = val;
            scores[i] += weight;
            break;
        }
    }
}

int HexFractalSmoothSample(float x, float y)
{
    float coordinateScale = (float)(1u << Steps) * 0.577350269f;
    x *= coordinateScale;
    y *= coordinateScale;

    float s = 0.366025403f * (x + y);
    float xs = x + s;
    float ys = y + s;

    float invStride = 1.0f / (float)(1u << Steps);
    float curXs = xs * invStride;
    float curYs = ys * invStride;

    int x0 = (int)floor(curXs);
    int y0 = (int)floor(curYs);

    int c00 = GetBaseCell(Seed, Variety, x0,     y0);
    int c10 = GetBaseCell(Seed, Variety, x0 + 1, y0);
    int c01 = GetBaseCell(Seed, Variety, x0,     y0 + 1);
    int c11 = GetBaseCell(Seed, Variety, x0 + 1, y0 + 1);

    for (int step = 1; step <= Steps; step++)
    {
        int mTop    = PickMidpoint(Seed, step, 2 * x0 + 1, 2 * y0,     c00, c10);
        int mBottom = PickMidpoint(Seed, step, 2 * x0 + 1, 2 * y0 + 2, c01, c11);
        int mLeft   = PickMidpoint(Seed, step, 2 * x0,     2 * y0 + 1, c00, c01);
        int mRight  = PickMidpoint(Seed, step, 2 * x0 + 2, 2 * y0 + 1, c10, c11);
        int mCenter = PickMidpoint(Seed, step, 2 * x0 + 1, 2 * y0 + 1, c00, c11);

        curXs *= 2.0f;
        curYs *= 2.0f;
        int nx0 = (int)floor(curXs);
        int ny0 = (int)floor(curYs);

        int qx = nx0 - 2 * x0;
        int qy = ny0 - 2 * y0;

        int nextC00, nextC10, nextC01, nextC11;
        if (qx == 0 && qy == 0) {
            nextC00 = c00;     nextC10 = mTop;
            nextC01 = mLeft;   nextC11 = mCenter;
        } else if (qx == 1 && qy == 0) {
            nextC00 = mTop;    nextC10 = c10;
            nextC01 = mCenter; nextC11 = mRight;
        } else if (qx == 0 && qy == 1) {
            nextC00 = mLeft;   nextC10 = mCenter;
            nextC01 = c01;     nextC11 = mBottom;
        } else {
            nextC00 = mCenter; nextC10 = mRight;
            nextC01 = mBottom; nextC11 = c11;
        }

        x0 = nx0; y0 = ny0;
        c00 = nextC00; c10 = nextC10;
        c01 = nextC01; c11 = nextC11;
    }

    float xsi = curXs - (float)x0;
    float ysi = curYs - (float)y0;
    bool upperTri = (ysi > xsi);

    int vals[3] = { c00, c11, upperTri ? c01 : c10 };
    int values[3] = { -1, -1, -1 };
    float scores[3] = { 0.0f, 0.0f, 0.0f };

    HandleSmoothWeight(values, scores, vals[0], xsi, ysi);
    HandleSmoothWeight(values, scores, vals[1], xsi - 1.0f, ysi - 1.0f);

    if (upperTri)
        HandleSmoothWeight(values, scores, vals[2], xsi, ysi - 1.0f);
    else
        HandleSmoothWeight(values, scores, vals[2], xsi - 1.0f, ysi);

    int bestVal = 0;
    float bestScore = -1.0f;

    [unroll]
    for (int i = 0; i < 3; i++)
    {
        if (scores[i] > bestScore)
        {
            bestVal = values[i];
            bestScore = scores[i];
        }
    }
    return bestVal;
}

[numthreads(THREADS, THREADS, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint width, height;
    image.GetDimensions(width, height);
    if (GlobalInvocationID.x >= width || GlobalInvocationID.y >= height)
    {
        return;
    }

    float nx = ((float)GlobalInvocationID.x / (float)width) * Zoom + Pan.x;
    float ny = ((float)GlobalInvocationID.y / (float)height) * Zoom + Pan.y;

    // animated whirl (unused, to use assing them to sample vars)
    float wx = nx + 0.10f * sin(ny * 1.2f + Time);
    float wy = ny + 0.10f * cos(nx * 1.2f + Time);

    float sample_x = nx;
    float sample_y = ny;

    int region = HexFractalSmoothSample(sample_x, sample_y);
    image[int2(GlobalInvocationID.xy)] = float4(PALETTE[region % VARIATIONS], 1.0f);
}
