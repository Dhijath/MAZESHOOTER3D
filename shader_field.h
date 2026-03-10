/*==============================================================================

   フィールド描画用シェーダ [Shader_field.h]
														 Author : 51106
														 Date   : 2025/11/12
--------------------------------------------------------------------------------

   ・地面などのフィールド専用シェーダ管理
   ・行列情報（ワールド／ビュー／射影）とカラーを設定
   ・シェーダ初期化・終了・描画開始を制御

==============================================================================*/

#pragma once
#ifndef SHADER_FIELD_H
#define SHADER_FIELD_H

#include <d3d11.h>
#include <DirectXMath.h>




//====================================
// 初期化／終了処理
//====================================

//------------------------------------
// Shader_field_Initialize
// フィールド描画用シェーダの初期化
// pDevice  : Direct3Dデバイス
// pContext : デバイスコンテキスト
//------------------------------------
bool Shader_field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

//------------------------------------
// Shader_field_Finalize
// フィールドシェーダの終了処理
//------------------------------------
void Shader_field_Finalize();

//====================================
// 行列設定関連
//====================================

//------------------------------------
// Shader_field_SetMatrix
// 行列を直接設定（汎用）
//------------------------------------
void Shader_field_SetMatrix(const DirectX::XMMATRIX& matrix);

//------------------------------------
// Shader_field_SetWorldMatrix
// ワールド行列を設定
//------------------------------------
void Shader_field_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

//------------------------------------
// Shader_field_SetViewMatrix
// ビュー行列を設定
//------------------------------------
void Shader_field_SetViewMatrix(const DirectX::XMMATRIX& matrix);

//------------------------------------
// Shader_field_SetProjectMatrix
// 射影（プロジェクション）行列を設定
//------------------------------------
void Shader_field_SetProjectMatrix(const DirectX::XMMATRIX& matrix);

//====================================
// カラー／描画関連
//====================================

//------------------------------------
// Shader_field_3D_SetColor
// フィールド全体に乗算するカラーを設定
//------------------------------------
void Shader_field_3D_SetColor(const DirectX::XMFLOAT4& color);

//------------------------------------
// Shader_field_Begin
// フィールドシェーダを描画パイプラインに設定
//------------------------------------
void Shader_field_Begin();

#endif // SHADER_FIELD_H
