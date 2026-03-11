/*==============================================================================

   アウトライン用頂点シェーダー [shader_vertex_outline.hlsl]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   ・バックフェース法によるアウトライン描画
   ・頂点を法線方向にワールド空間で膨張させる
   ・PSへは何も渡さなくてよいので最低限の出力のみ

==============================================================================*/
//=============================================================================
// アウトライン用 頂点シェーダー (shader_vertex_outline.hlsl)
// - b0 : World 行列
// - b1 : View  行列
// - b2 : Proj  行列
// - b3 : アウトライン太さ
//=============================================================================

cbuffer VS_CONSTANT_BUFFER_WORLD : register(b0)
{
    float4x4 world;
}
cbuffer VS_CONSTANT_BUFFER_VIEW : register(b1)
{
    float4x4 view;
}
cbuffer VS_CONSTANT_BUFFER_PROJ : register(b2)
{
    float4x4 proj;
}

//----------------------------------------------------------
// アウトラインパラメータ
//----------------------------------------------------------
cbuffer OUTLINE_PARAM : register(b3)
{
    float outlineWidth; // 膨張量（例: 0.02f）
    float3 outline_dummy;
}

struct VS_IN
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;

    // ワールド空間で法線方向に膨張
    float4 posW4  = mul(float4(vi.position, 1.0f), world);
    float3 nW     = normalize(mul(float4(vi.normal, 0.0f), world).xyz);

    posW4.xyz += nW * outlineWidth;

    // View → Proj
    float4 posV = mul(posW4, view);
    vo.posH     = mul(posV, proj);

    return vo;
}
