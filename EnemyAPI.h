/*==============================================================================

   エネミーAPI [EnemyAPI.h]
                                                         Author : 51106
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

   ・ゲーム側から使用するエネミー管理API
   ・Enemy クラスを直接触らせないための窓口

==============================================================================*/

#ifndef ENEMY_API_H
#define ENEMY_API_H

#include <DirectXMath.h>
#include "collision.h"

void Enemy_Initialize(const DirectX::XMFLOAT3& position);
void Enemy_Finalize();
void Enemy_Update(double elapsed_time);
void Enemy_Draw();

int  Enemy_Spawn(const DirectX::XMFLOAT3& position);
int  Enemy_GetCount();
AABB Enemy_GetAABBAt(int index);
const DirectX::XMFLOAT3& Enemy_GetPositionAt(int index);

#endif // ENEMY_API_H
