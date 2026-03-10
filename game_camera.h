
#ifndef Game_Camera_H
#define Game_Camera_H

#include <DirectXMath.h>

void Game_Camera_Initialize();
void Game_Camera_Finalize();
void Game_Camera_Update(double elapsd_time);

const DirectX::XMFLOAT4X4& Game_Camera_GetMatrix();
const DirectX::XMFLOAT4X4& Game_Camera_GetPerspectiveMatrix();

void Game_Camera_DebugDraw();

const DirectX::XMFLOAT3& Game_Camera_GetPosition();

#endif // !Game_Camera_H
