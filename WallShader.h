/*==============================================================================

   壁専用シェーダ管理 [WallShader.h]
                                                         Author : 51106
                                                         Date   : 2026/02/16
--------------------------------------------------------------------------------

   ■このファイルがやること
   ・壁専用シェーダーの初期化／終了
   ・描画設定（行列・色・UV・テクスチャ）
   ・ライティング設定（環境光・平行光源・鏡面反射）

==============================================================================*/

#ifndef WALL_SHADER_H
#define WALL_SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

//==============================================================================
// 初期化／終了
//==============================================================================
bool WallShader_Initialize(ID3D11Device* dev, ID3D11DeviceContext* ctx);
void WallShader_Finalize();

//==============================================================================
// 描画設定
//==============================================================================
void WallShader_Begin();
void WallShader_SetWorldMatrix(const DirectX::XMMATRIX& mtxW);
void WallShader_SetViewMatrix(const DirectX::XMMATRIX& mtxV);
void WallShader_SetProjectMatrix(const DirectX::XMMATRIX& mtxP);
void WallShader_SetColor(const DirectX::XMFLOAT4& color);
void WallShader_SetUVRepeat(const DirectX::XMFLOAT2& uvRepeat);
void WallShader_SetTexture(int texID);

//==============================================================================
// ライティング設定
//==============================================================================
void WallShader_SetAmbient(const DirectX::XMFLOAT3& color);
void WallShader_SetDirectional(const DirectX::XMFLOAT4& direction, const DirectX::XMFLOAT4& color);
void WallShader_SetSpecular(const DirectX::XMFLOAT3& camera_position, float specular_power, const DirectX::XMFLOAT4& specular_color);

#endif // WALL_SHADER_H