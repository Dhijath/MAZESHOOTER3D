#ifndef EFFECT_H
#define EFFECT_H

#include <DirectXMath.h>

void Effect_Initialize();
void Effect_Finalize();

// アセット解放なしで全エフェクトをクリア（ルーム遷移時用）
void Effect_ClearAll();

void Effect_Update(double elapsed_time);
void Effect_Draw();
void Effect_Create(const DirectX::XMFLOAT2& position);

#endif // !EFFECT_H

