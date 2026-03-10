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

//==============================================================================
// ミサイル爆発エリアダメージ
//
// ■役割
// ・中心から radius 以内にいる生存エネミーすべてに damage を与える
//
// ■引数
// ・center : 爆発中心座標
// ・radius : 爆発半径
// ・damage : 爆発ダメージ量
//==============================================================================
void Enemy_ApplyExplosionDamage(const DirectX::XMFLOAT3& center, float radius, int damage);

#endif // ENEMY_API_H
