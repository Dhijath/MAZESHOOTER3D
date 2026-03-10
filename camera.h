#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>

void Camera_Initialize();
void Camera_Finalize();
void Camera_Update(double elapsd_time);

const DirectX::XMFLOAT4X4& Camera_GetMatrix();
const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix();

void Camera_DebugDraw();

const DirectX::XMFLOAT3& Camera_GetPosition();

const DirectX::XMFLOAT3& Camera_GetFront();   





#endif // !CAMERA_H