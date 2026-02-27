/*==============================================================================

   プレイヤー追従カメラ（旧） [Player_topCamera.cpp]
                                                         Author : 51106
                                                         Date   : 2025/12/19
--------------------------------------------------------------------------------

   ・固定追従（プレイヤー注視）カメラ
   ・プレイヤー位置に対し、後方＋上へオフセットして配置
   ・各 3D 系シェーダへ View / Proj を反映

==============================================================================*/

#include "Player_topCamera.h"                            // 旧トップカメラの宣言

#include <DirectXMath.h>                                 // DirectXMath（行列/ベクトル計算）
#include "shader3d.h"                                    // 3DシェーダへView/Projを渡す
#include "shader_field.h"                                // 地面用シェーダへView/Projを渡す
#include "shader_billboard.h"                            // ビルボード用シェーダへView/Projを渡す
#include "direct3d.h"                                    // バックバッファサイズ取得
#include "Player.h"                                      // プレイヤー位置/高さ取得
#include "WallShader.h"                                  // 壁用シェーダへView/Projを渡す

using namespace DirectX;                                 // DirectX名前空間

namespace                                                // ファイル内限定
{                                                        // ここから
    //==========================================================================
    // カメラ状態（Getter用）
    //==========================================================================
    XMFLOAT3  g_Front = { 0.0f, 0.0f, 1.0f };            // カメラ前方向（外部参照用）
    XMFLOAT3  g_Pos = { 0.0f, 0.0f, 0.0f };              // カメラ位置（外部参照用）
    XMFLOAT4X4 g_View{};                                 // View行列（外部参照用）
    XMFLOAT4X4 g_Proj{};                                 // Proj行列（外部参照用）

    //==========================================================================
    // 追従オフセット
    //==========================================================================
    float gHeight = 1.2f;                                // 上方向オフセット（プレイヤー縮小に合わせて近づける）
    float gBackZ = -2.2f;                                // 後方オフセット（-Z）（プレイヤー縮小に合わせて近づける）
}                                                        // namespace終わり

//==============================================================================
// 初期化
//==============================================================================
void Player_topCamera_Initialize()                       // 初期化
{                                                        // 開始
}                                                        // 終了

//==============================================================================
// 終了
//==============================================================================
void Player_topCamera_Finalize()                         // 終了
{                                                        // 開始
}                                                        // 終了

//==============================================================================
// 更新（固定追従）
//==============================================================================
void Player_topCamera_Update(double elapsed_time)        // 毎フレーム更新
{                                                        // 開始
    (void)elapsed_time;                                  // 経過時間はこのカメラでは未使用（固定追従のため）

    //==========================================================================
    // 1) プレイヤー位置取得（Yは無視して足元基準）
    //==========================================================================
    XMFLOAT3 pp = Player_GetPosition();                  // プレイヤー位置（足元基準想定）
    XMVECTOR p = XMVectorSet(pp.x, 0.0f, pp.z, 0.0f);    // 足元基準でXZのみ使用（Y=0固定）

    // ・足元注視だと小型モデルが画面下へ寄りやすく見失いやすい
    // ・Player_GetHeight() は当たり判定の高さ（モデルスケールと一致しない場合は要調整）
    const float targetY = pp.y + (Player_GetHeight() * 0.6f); // 注視点Y（胸付近）
    XMVECTOR targetPos = XMVectorSet(pp.x, targetY, pp.z, 0.0f); // 注視点（プレイヤー中心寄り）

    //==========================================================================
    // 2) 視点／注視点
    //==========================================================================
    XMVECTOR target = p;                                 // 注視点（初期：足元）
    XMVECTOR eye = p + XMVectorSet(0.0f, gHeight, gBackZ, 0.0f); // 視点（初期：足元+オフセット）


    // ・eye も targetPos 基準にして上下ズレを防ぐ
    target = targetPos;                                  // 注視点を胸付近へ変更
    eye = targetPos + XMVectorSet(0.0f, gHeight, gBackZ, 0.0f); // 視点も同じ基準で追従

    //==========================================================================
    // 3) カメラ前方向
    //==========================================================================
    XMVECTOR front = XMVector3Normalize(target - eye);   // 視点→注視点の正規化（前方向）
    XMStoreFloat3(&g_Front, front);                      // 前方向を保存（Getter用）
    XMStoreFloat3(&g_Pos, eye);                          // 位置を保存（Getter用）

    //==========================================================================
    // 4) View 行列作成＆適用
    //==========================================================================
    XMMATRIX view = XMMatrixLookAtLH(                    // 左手系LookAtでView行列作成
        eye,                                             // 視点
        target,                                          // 注視点
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)              // 上方向ベクトル
    );

    Shader3d_SetViewMatrix(view);                        // 3DシェーダへView設定
    Shader_field_SetViewMatrix(view);                    // フィールドシェーダへView設定
    Shader_Billboard_SetViewMatrix(view);                // ビルボードへView設定
    XMStoreFloat4x4(&g_View, view);                      // View行列を保存（Getter用）
    WallShader_SetViewMatrix(view);                      // 壁シェーダへView設定

    //==========================================================================
    // 5) Projection 行列作成＆適用
    //==========================================================================
    float aspect =                                       // アスペクト比 = 幅/高さ
        static_cast<float>(Direct3D_GetBackBufferWidth()) /
        static_cast<float>(Direct3D_GetBackBufferHeight());

    // 画角（FOV）を狭めてズーム寄りにする（プレイヤー縮小対策）
    const float fovDeg = 65.0f;                          // 縦FOV（度数法、人間に分かりやすい）
    const float fovY = XMConvertToRadians(fovDeg);       // 縦FOV（弧度法、行列計算用）

    XMMATRIX proj = XMMatrixPerspectiveFovLH(            // 左手系透視投影行列
        fovY,                                            // 縦画角（ラジアン）
        aspect,                                          // アスペクト比
        0.1f,                                            // 近クリップ
        1000.0f                                          // 遠クリップ
    );

    Shader3d_SetProjectMatrix(proj);                     // 3DシェーダへProj設定
    Shader_field_SetProjectMatrix(proj);                 // フィールドシェーダへProj設定
    Shader_Billboard_SetProjectMatrix(proj);             // ビルボードへProj設定
    XMStoreFloat4x4(&g_Proj, proj);                      // Proj行列を保存（Getter用）
    WallShader_SetProjectMatrix(proj);                   // 壁シェーダへProj設定
}                                                        // 終了

//==============================================================================
// Getter
//==============================================================================
const XMFLOAT3& Player_topCamera_GetFront() { return g_Front; } // 前方向を返す
const XMFLOAT3& Player_topCamera_GetPosition() { return g_Pos; } // 位置を返す
const XMFLOAT4X4& Player_topCamera_GetViewMatrix() { return g_View; } // View行列を返す
const XMFLOAT4X4& Player_topCamera_GetProjectionMatrix() { return g_Proj; } // Proj行列を返す