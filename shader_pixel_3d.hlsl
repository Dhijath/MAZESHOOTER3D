/*==============================================================================

   3D描画用ピクセルシェーダー [shader_pixel_3d.hlsl]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------

   ・従来の Ambient + Directional + Specular + PointLight に加えて
     Blob Shadow（丸影）を合成
   ・Blob Shadow は PS register(b6) で制御
     ※ C++側で BlobShadow::SetToPixelShader() により b6 をセットする

==============================================================================*/
//=============================================================================
// 3D 用 ピクセルシェーダー (shader_pixel_3d.hlsl)
// - b0 : diffuse_color
// - b1 : ambient_color
// - b2 : directional_vector / directional_color
// - b3 : eye_posW / specular_power / specular_color
// - b4 : Point_light[]
// - b6 : Blob Shadow
//=============================================================================

cbuffer PS_CONSTANT_BUFFER_DIFFUSE : register(b0)
{
    float4 diffuse_color;
}

cbuffer PS_CONSTANT_BUFFER_AMBIENT : register(b1)
{
    float4 ambient_color;
}

cbuffer PS_CONSTANT_BUFFER_DIRECTIONAL : register(b2)
{
cbuffer PS_CONSTANT_BUFFER_TOON : register(b5)
{
    int toon_enable;
    float toon_shade_steps;
    float toon_min_brightness;
    float toon_padding;
}

    float4 directional_vector; // xyz = ライト方向
    float4 directional_color = { 1, 1, 1, 1 };
}

cbuffer PS_CONSTANT_BUFFER_SPECULAR : register(b3)
{
    float3 eye_posW;
    float specular_power = 30.0f;
    float4 specular_color = { 0.1f, 0.01f, 0.1f, 1.0f };
}

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_CONSTANT_BUFFER_POINT : register(b4)
{
    PointLight Point_light[4];
    int Point_light_count;
    float3 point_light_dummy;
};

//----------------------------------------------------------
// Blob Shadow（丸影）用定数（MeshField と同じ）
// PS register(b6)
//----------------------------------------------------------
cbuffer BLOB_SHADOW : register(b6)
{
    float3 blobCenterW; // 影中心（プレイヤー位置など）
    float blobRadius; // 半径（m）

    float blobSoftness; // ぼかし幅（m）
    float blobStrength; // 濃さ（0..1）
    float2 pad;
}

struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 posW : TEXCOORD1; // VSと一致
    float3 normalW : TEXCOORD2;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

float BlobShadowFactor(float3 posW)
{
    // XZ 平面距離で丸影
    float2 d = posW.xz - blobCenterW.xz;
    float dist = length(d);

    if (toon_enable != 0)
    {
        float steps = max(toon_shade_steps, 1.0f);
        dl = floor(dl * steps) / steps;
        dl = max(dl, toon_min_brightness);
    }

    // dist <= radius で影、境界は softness でフェード
    float t = saturate((dist - blobRadius) / max(blobSoftness, 0.0001f));
    float inside = 1.0f - t; // 1:中心, 0:外
    inside = inside * inside; // ちょい自然に

    // 影係数：中心ほど暗い（1-strength）
    return lerp(1.0f - blobStrength, 1.0f, 1.0f - inside);
}

float4 main(PS_IN pi) : SV_TARGET
{
    // --- 材質色（テクスチャ × 頂点カラー × ディフューズ色）
    float4 texSample = tex.Sample(samplerState, pi.uv);
    float3 material_color = texSample.rgb * pi.color.rgb * diffuse_color.rgb;

    // --- 法線＆視線
    float3 N = normalize(pi.normalW);
    float3 toEye = normalize(eye_posW - pi.posW);

    // --- 平行光ディフューズ
    float3 Ld = -normalize(directional_vector.xyz);
    float dl = dot(Ld, N + 1.0f) * 0.5f;
    dl = max(dl, 0.0f);

    float3 diffuse = material_color * directional_color.rgb * dl;

    // --- 環境光
    float3 ambient = material_color * ambient_color.rgb;

    // --- 平行光スペキュラ
    float3 r = reflect(-Ld, N);
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * t;

    // --- アルファ
    float alpha = texSample.a * pi.color.a * diffuse_color.a;

    float3 color = ambient + diffuse + specular;

    // --- 点光源
    for (int i = 0; i < Point_light_count; i++)
    {
        PointLight pl = Point_light[i];

        float3 lightToPixel = pi.posW - pl.posW;
        float distance = length(lightToPixel);

        float A = max(1.0f - distance / pl.range, 0.0f);
        A = A * A;

        float3 Lp = -normalize(lightToPixel);
        float point_light_dl = max(dot(Lp, N), 0.0f);

        color += material_color * pl.color.rgb * A * point_light_dl;

        float3 point_light_r = reflect(normalize(lightToPixel), N);
        float point_light_t = pow(max(dot(point_light_r, toEye), 0.0f), specular_power);

        color += pl.color.rgb * point_light_t * A;
    }

    // --- 丸影を合成（受ける側）
    // ※ 影を受けたくない物体がある場合は、C++側で strength=0 にするか
    //  その描画の直前だけ b6 を外す運用にしてください
    float shadow = BlobShadowFactor(pi.posW);
    color *= shadow;

    return float4(color, alpha);
}
