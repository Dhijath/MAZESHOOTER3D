/*==============================================================================

   壁描画用ピクセルシェーダ（Lit） [shader_pixel_wall_lit.hlsl]
                                                         Author : 51106
                                                         Date   : 2026/02/17
--------------------------------------------------------------------------------

   ・壁専用 Lit PS（ライティング対応版）
   ・Ambient / Directional / Specular（Blinn-Phong）対応
   ・WallLightData 構造体と cbuffer のレイアウトを一致させる

==============================================================================*/
cbuffer PS_CONSTANT_BUFFER_DIFFUSE : register(b0)
{
    float4 diffuse_color;
}

cbuffer PS_CONSTANT_BUFFER_UV_REPEAT : register(b1)
{
    float2 uvRepeat;
    float2 padding;
}

cbuffer PS_CONSTANT_BUFFER_LIGHT : register(b2)
{
    float3 ambient;
    float padding1;
    float4 directional_direction;
    float4 directional_color;
    float3 camera_position;
    float specular_power;
    float4 specular_color;
}

struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 posW : TEXCOORD1;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD4;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

float4 main(PS_IN pi) : SV_TARGET
{
    float2 uv = pi.uv * uvRepeat;
    float4 texColor = tex.Sample(samplerState, uv);

    float3 N = normalize(pi.normal);

    float3 lightDir = normalize(-directional_direction.xyz);

    // Lambertディフューズ
    float NdotL = max(0.0f, dot(N, lightDir));
    float3 diffuse = directional_color.rgb * NdotL;

    // Blinn-Phongスペキュラ
    float3 viewDir = normalize(camera_position - pi.posW);
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(0.0f, dot(N, halfVec));
    float3 specular = specular_color.rgb * pow(NdotH, specular_power);

    // 最終カラー合成
    float3 lighting = ambient + diffuse + specular;
    float3 finalColor = texColor.rgb * pi.color.rgb * diffuse_color.rgb * lighting;
    float alpha = texColor.a * pi.color.a * diffuse_color.a;

    return float4(finalColor, alpha);
}