/*==============================================================================

   プレイヤー追従カメラ（旧） [Player_topCamera.h]
                                                         Author : 51106
                                                         Date   : 2025/12/19
--------------------------------------------------------------------------------

   ・旧カメラ（固定追従）：プレイヤーを注視し、後方上から追従
   ・ビュー行列／射影行列の更新を担当
   ・Player_Camera（切替ハブ）から呼ばれる想定

==============================================================================*/

#ifndef PLAYER_TOPCAMERA_H
#define PLAYER_TOPCAMERA_H

#include <DirectXMath.h>

//==============================================================================
// 初期化／終了
//==============================================================================
void Player_topCamera_Initialize();
void Player_topCamera_Finalize();

//==============================================================================
// 更新
//==============================================================================
void Player_topCamera_Update(double elapsed_time);

//==============================================================================
// Getter
//==============================================================================
const DirectX::XMFLOAT3& Player_topCamera_GetFront();
const DirectX::XMFLOAT3& Player_topCamera_GetPosition();
const DirectX::XMFLOAT4X4& Player_topCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& Player_topCamera_GetProjectionMatrix();

#endif // PLAYER_TOPCAMERA_H

