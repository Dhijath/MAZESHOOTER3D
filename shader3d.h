/*==============================================================================

   3D描画用シェーダ管理 [Shader3D.h]
                                                         Author : 51106
                                                         Date   : 2026/02/15
--------------------------------------------------------------------------------

   ・3Dモデル描画用シェーダの初期化／終了
   ・行列（World / View / Projection）の設定
   ・頂点カラーに掛ける色の設定
   ・描画開始時のバインド（Begin）

==============================================================================*/
/*==============================================================================

   シェーダー管理（3D描画用） [Shader3d_.h]
                                                         Author : 51106
                                                         Date   : 2026/02/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef Shader3d_H
#define Shader3d_H

#include <d3d11.h>
#include <DirectXMath.h>

// 3D シェーダーの初期化（デバイス、コンテキストを渡す）
bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

// 3D シェーダーの解放処理
void Shader3d_Finalize();

// （汎用）行列を VS 定数バッファ 0 に設定
void Shader3d_SetMatrix(const DirectX::XMMATRIX& matrix);

// ワールド行列を VS 定数バッファ 0 に設定
void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

// ビュー行列を VS 定数バッファ 1 に設定
void Shader3d_SetViewMatrix(const DirectX::XMMATRIX& matrix);

// プロジェクション行列を VS 定数バッファ 2 に設定
void Shader3d_SetProjectMatrix(const DirectX::XMMATRIX& matrix);

// ピクセルシェーダで使う色（float4）を設定
void Shader3d_SetColor(const DirectX::XMFLOAT4& color);

// 描画開始前に呼んで、シェーダーや定数バッファをパイプラインにセット
void Shader3d_Begin();



#endif

