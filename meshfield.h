/*==============================================================================

   メッシュフィールド描画 [MeshField.h]
														 Author : 51106
														 Date   : 2025/11/12
--------------------------------------------------------------------------------

   ・地面や背景など、格子状のメッシュフィールドを描画するモジュール
   ・初期化・描画・終了処理を提供
   ・シェーダーやライト設定と組み合わせて使用する

==============================================================================*/
#pragma once
#ifndef MESHFIELD_H
#define MESHFIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

//====================================
// 初期化/終了処理
//====================================

//------------------------------------
// MeshField_Initialize
// メッシュフィールドの初期化処理
// pDevice  : Direct3Dデバイス
// pContext : デバイスコンテキスト
//------------------------------------
void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

//------------------------------------
// MeshField_Finalize
// メッシュフィールドの終了処理
// (使用したバッファの解放など)
//------------------------------------
void MeshField_Finalize(void);

//====================================
// 描画処理
//====================================

//------------------------------------
// MeshField_Draw
// メッシュフィールドの描画処理
// mtxW : ワールド行列(位置・回転・拡縮)
//------------------------------------
void MeshField_Draw(const DirectX::XMMATRIX& mtxW);

//------------------------------------
// MeshField_DrawTile
// タイル1個分のメッシュフィールド描画
// mtxW     : ワールド行列(位置・回転・拡縮)
// texID    : 使用するテクスチャID
// tileSize : タイルサイズ(メートル)
//------------------------------------
void MeshField_DrawTile(const DirectX::XMMATRIX& mtxW, int texID, float tileSize);

//------------------------------------
// MeshField_DrawTileCeiling
// タイル1個分の天井メッシュフィールド描画（法線下向き）
// mtxW     : ワールド行列(位置・回転・拡縮)
// texID    : 使用するテクスチャID
// tileSize : タイルサイズ(メートル)
//------------------------------------
void MeshField_DrawTileCeiling(const DirectX::XMMATRIX& mtxW, int texID, float tileSize);

#endif // MESHFIELD_H