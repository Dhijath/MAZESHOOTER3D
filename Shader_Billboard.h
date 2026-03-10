/*==============================================================================

   ビルボード用シェーダ [shader_billboard.h]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

// UV の拡大率＆オフセット情報
struct UVParameter
{
    DirectX::XMFLOAT2 scale;        // UV のスケール（幅・高さ）
    DirectX::XMFLOAT2 translation;  // UV のオフセット（左上座標など）
};

// ビルボードシェーダ初期化
bool Shader_Billboard_Initialize();

// ビルボードシェーダ終了処理
void Shader_Billboard_Finalize();

// 行列・色・UV などのパラメータ設定
void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetProjectMatrix(const DirectX::XMMATRIX& matrix);
void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color);
void Shader_Billboard_SetUVParameter(const UVParameter& parameter);

// シェーダを描画パイプラインにセット（描画前に呼ぶ）
void Shader_Billboard_Begin();
