/*==============================================================================

   深度 + 法線 エッジ検出ピクセルシェーダー [shader_pixel_edge.hlsl]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   - t0 : 法線テクスチャ（shader_pixel_normal.hlsl で書き出したもの）
   - t1 : 深度テクスチャ
   - 隣接ピクセルと法線差・深度差を比較してエッジを検出
   - エッジ部分をエッジカラーで出力、それ以外は透明

==============================================================================*/

//----------------------------------------------------------
// エッジ検出パラメータ
//----------------------------------------------------------
cbuffer EDGE_PARAM : register(b0)
{
    float  edgeDepthThreshold;  // 深度差の閾値（例: 0.001f）
    float  edgeNormalThreshold; // 法線差の閾値（例: 0.3f）
    float2 texelSize;           // 1ピクセルのUVサイズ（1/width, 1/height）
    float4 edgeColor;           // エッジの色（通常は黒 {0,0,0,1}）
}

Texture2D    normalTex   : register(t0); // 法線テクスチャ
Texture2D    depthTex    : register(t1); // 深度テクスチャ
SamplerState pointSampler : register(s0);

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv   : TEXCOORD0;
};

//----------------------------------------------------------
// 深度サンプル（0~1）
//----------------------------------------------------------
float SampleDepth(float2 uv)
{
    return depthTex.Sample(pointSampler, uv).r;
}

//----------------------------------------------------------
// 法線サンプル（0~1 -> -1~1 に戻す）
//----------------------------------------------------------
float3 SampleNormal(float2 uv)
{
    return normalTex.Sample(pointSampler, uv).rgb * 2.0f - 1.0f;
}

float4 main(PS_IN pi) : SV_TARGET
{
    float2 uv = pi.uv;

    // 隣接4ピクセルのUV
    float2 uvR = uv + float2( texelSize.x, 0.0f);
    float2 uvL = uv + float2(-texelSize.x, 0.0f);
    float2 uvU = uv + float2(0.0f,  texelSize.y);
    float2 uvD = uv + float2(0.0f, -texelSize.y);

    //----------------------------------------------------------
    // 深度エッジ検出
    //----------------------------------------------------------
    float d  = SampleDepth(uv);
    float dR = SampleDepth(uvR);
    float dL = SampleDepth(uvL);
    float dU = SampleDepth(uvU);
    float dD = SampleDepth(uvD);

    float depthDiff = abs(d - dR) + abs(d - dL) + abs(d - dU) + abs(d - dD);
    float depthEdge = step(edgeDepthThreshold, depthDiff);

    //----------------------------------------------------------
    // 法線エッジ検出
    //----------------------------------------------------------
    float3 n  = SampleNormal(uv);
    float3 nR = SampleNormal(uvR);
    float3 nL = SampleNormal(uvL);
    float3 nU = SampleNormal(uvU);
    float3 nD = SampleNormal(uvD);

    float normalDiff =
        length(n - nR) +
        length(n - nL) +
        length(n - nU) +
        length(n - nD);

    float normalEdge = step(edgeNormalThreshold, normalDiff);

    //----------------------------------------------------------
    // 深度 or 法線どちらかがエッジならエッジとして出力
    //----------------------------------------------------------
    float edge = saturate(depthEdge + normalEdge);

    // エッジ部分だけ色を出す（それ以外は透明）
    return float4(edgeColor.rgb, edge * edgeColor.a);
}
