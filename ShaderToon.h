/*==============================================================================

   トゥーンシェーダー管理 [ShaderToon.h]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   ・トゥーン描画 + アウトライン描画をまとめて管理
   ・Shader3d と同じ定数バッファ構成を使うので
     Light / Camera 系の CB はそのまま流用できる

==============================================================================*/
#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

//==============================================================================
// 初期化 / 終了
//==============================================================================

/// <summary>
/// トゥーンシェーダーを初期化する
/// Shader3d_Initialize() の後に呼ぶこと
/// </summary>
bool ShaderToon_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

/// <summary>
/// トゥーンシェーダーを解放する
/// </summary>
void ShaderToon_Finalize();

//==============================================================================
// パラメータ設定
//==============================================================================

/// <summary>
/// トゥーン階調パラメータを設定する
/// </summary>
/// <param name="steps">階調数（2〜4 推奨）</param>
/// <param name="specularSize">ハイライト閾値（0.0〜1.0）</param>
/// <param name="rimPower">リムライト強度（0.0 で無効）</param>
/// <param name="rimThreshold">リムライト閾値（0.0〜1.0）</param>
void ShaderToon_SetToonParam(int steps, float specularSize, float rimPower, float rimThreshold);

/// <summary>
/// アウトライン太さを設定する（ワールド単位）
/// </summary>
void ShaderToon_SetOutlineWidth(float width);

/// <summary>
/// アウトライン色を設定する（デフォルト黒）
/// </summary>
void ShaderToon_SetOutlineColor(const DirectX::XMFLOAT4& color);

//==============================================================================
// ワールド行列 / カラー設定（Shader3d と同じインターフェース）
//==============================================================================
void ShaderToon_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void ShaderToon_SetViewMatrix(const DirectX::XMMATRIX& matrix);
void ShaderToon_SetProjectMatrix(const DirectX::XMMATRIX& matrix);
void ShaderToon_SetColor(const DirectX::XMFLOAT4& color);

//==============================================================================
// 描画開始
//==============================================================================

/// <summary>
/// トゥーン PS + 通常 VS をパイプラインにセットする
/// ModelDraw() の直前に呼ぶ
/// </summary>
void ShaderToon_Begin();

/// <summary>
/// アウトライン描画パスをパイプラインにセットする
/// 1パス目（アウトライン）→ ShaderToon_BeginOutline()
/// 2パス目（トゥーン本体）→ ShaderToon_Begin()
/// の順に呼ぶ
/// </summary>
void ShaderToon_BeginOutline();