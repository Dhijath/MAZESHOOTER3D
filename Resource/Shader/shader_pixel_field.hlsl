/*==============================================================================

   フィールド用ピクセルシェーダー [shader_pixel_field.hlsl]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------

   ・地面テクスチャを blend(R/G) で混ぜる
   ・Blob Shadow（丸影）を簡易的に合成
     - プレイヤー位置（blobCenterW）からのXZ距離で影を作る
     - ShadowMap本実装の前段として、見た目確認に最適

==============================================================================*/

struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 posW : TEXCOORD1; //  ワールド座標
    float3 normalW : TEXCOORD2;
    float4 blend : COLOR0; // R/G をブレンド係数に使用
    float2 uv : TEXCOORD0;
};

Texture2D tex0 : register(t0); // 草
Texture2D tex1 : register(t1); // 土
SamplerState samp : register(s0);

//----------------------------------------------------------
// Blob Shadow（丸影）用定数
//  ※ C++側で PS b6 にセットしてください
//----------------------------------------------------------
cbuffer BLOB_SHADOW : register(b6)
{
    float3 blobCenterW; // プレイヤー位置（ワールド）
    float blobRadius; // 半径（m）

    float blobSoftness; // ぼかし幅（m）
    float blobStrength; // 濃さ（0..1）
    float2 pad;
};

float BlobShadowFactor(float3 posW)
{
    // XZ 平面距離で丸影
    float2 d = posW.xz - blobCenterW.xz;
    float dist = length(d);

    // dist <= radius で影。境界は softness でフェード
    float t = saturate((dist - blobRadius) / max(blobSoftness, 0.0001f));
    float inside = 1.0f - t; // 1:中心, 0:外
    inside = inside * inside; // 少し自然に（好み）

    // 影係数：中心ほど暗い（1-strength）へ
    return lerp(1.0f - blobStrength, 1.0f, 1.0f - inside);
}

float4 main(PS_IN pi) : SV_TARGET
{
    float4 c0 = tex0.Sample(samp, pi.uv); // 草
    float4 c1 = tex1.Sample(samp, pi.uv); // 土

    // blend（R/G）で混合（今のロジック維持）
    float r = pi.blend.r;
    float g = pi.blend.g;

    float4 tex_color = c0 * g + c1 * r;

    // 丸影を掛ける
    float shadow = BlobShadowFactor(pi.posW);
    tex_color.rgb *= shadow;

    return tex_color;
}
