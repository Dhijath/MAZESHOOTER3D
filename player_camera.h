/*==============================================================================

   プレイヤーカメラ制御（切替） [Player_Camera.h]
                                                         Author : 51106
                                                         Date   : 2025/12/19
--------------------------------------------------------------------------------

   ・旧（固定追従）と新（マウス操作）を切り替えるハブ
   ・ビュー行列／射影行列の更新を担当
   ・F1 等でモード切替（実装は cpp 側）

==============================================================================*/

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include <DirectXMath.h>

//==============================================================================
// カメラモード（C形式 enum：スコープ演算子は付けない）
//==============================================================================
enum PlayerCamera_Mode
{
    PLAYER_CAMERA_MODE_FIXED_FOLLOW = 0,   // 旧：固定追従（プレイヤー注視）
    PLAYER_CAMERA_MODE_MOUSE_FREE,         // 新：マウス操作（前方視点）
};

//==============================================================================
// 初期化／終了
//==============================================================================
void Player_Camera_Initialize();
void Player_Camera_Finalize();

//==============================================================================
// 更新
//==============================================================================
void Player_Camera_Update(double elapsed_time);

//==============================================================================
// Getter
//==============================================================================
const DirectX::XMFLOAT3& Player_Camera_GetFront();
const DirectX::XMFLOAT3& Player_Camera_GetPosition();
const DirectX::XMFLOAT4X4& Player_Camera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Player_Camera_GetProjectionMatrix();

//==============================================================================
// ミニマップ用（上から見下ろし）View/Proj を各シェーダへセット
//==============================================================================
void Player_Camera_SetMiniMapTopDown(const DirectX::XMFLOAT3& center, float height, float range);

//==============================================================================
// 本編の View/Proj（保存済み）を各シェーダへ再セット
//==============================================================================
void Player_Camera_ApplyMainViewProj();

//==============================================================================
// モード切替API
//==============================================================================
PlayerCamera_Mode Player_Camera_GetMode();
void Player_Camera_SetMode(PlayerCamera_Mode mode);
void Player_Camera_ToggleMode();

bool Player_Camera_IsMouseLeftTrigger();
bool Player_Camera_IsMouseLeftPressed();



#endif // PLAYER_CAMERA_H

