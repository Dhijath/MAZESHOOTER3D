/*==============================================================================

   法線書き出し用ピクセルシェーダー [shader_pixel_normal.hlsl]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   - 法線をRGBに書き出す（-1~1 -> 0~1 に変換）
   - エッジ検出パスで法線差を計算するために使用
   - VS は shader_vertex_3d.cso を流用

==============================================================================*/

struct PS_IN
{
    float4 posH    : SV_POSITION;
    float3 posW    : TEXCOORD1;
    float3 normalW : TEXCOORD2;
    float4 color   : COLOR0;
    float2 uv      : TEXCOORD0;
};

float4 main(PS_IN pi) : SV_TARGET
{
    // 法線を -1~1 から 0~1 に変換してRGBに書き出す
    float3 n = normalize(pi.normalW);
    float3 col = n * 0.5f + 0.5f;
    return float4(col, 1.0f);
}
