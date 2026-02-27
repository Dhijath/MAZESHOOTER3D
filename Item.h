/*==============================================================================

   Item [Item.h]
                                                         Author : 51106
                                                         Date   : 2026/02/18
--------------------------------------------------------------------------------
==============================================================================*/

#ifndef ITEM_H
#define ITEM_H

#include <DirectXMath.h>

enum class ItemType
{
    HP_HEAL,      // HP回復
    ENERGY_HEAL,  // エネルギー回復
    ATK_UP,       // 攻撃力アップ
    SPEED_UP,     // スピードアップ
};

class Item
{
public:
    void Initialize(ItemType type, const DirectX::XMFLOAT3& position);
    void Update();
    void Draw() const;

    bool     IsAlive()   const;
    ItemType GetType()   const;
    const DirectX::XMFLOAT3& GetPosition() const;

    void Pickup();

private:
    ItemType           m_Type = ItemType::HP_HEAL;
    DirectX::XMFLOAT3 m_Position = {};
    bool               m_IsAlive = false;
};

// SE解放（ItemManager_Finalize から呼び出す）
void Item_UnloadSE();
#endif // ITEM_H