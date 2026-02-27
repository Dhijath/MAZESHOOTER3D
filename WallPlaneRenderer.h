/*==============================================================================

   壁Plane描画 [WallPlaneRenderer.h]
                                                         Author : 51106
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------
==============================================================================*/

#ifndef WALL_PLANE_RENDERER_H
#define WALL_PLANE_RENDERER_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include "TileWall.h"

class WallPlaneRenderer
{
public:
    WallPlaneRenderer();
    ~WallPlaneRenderer();

    void Build(const std::vector<WallPlane>& planes);
    void Draw(int texId);

private:
    struct Vtx
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 nrm;
        DirectX::XMFLOAT4 col;
        DirectX::XMFLOAT2 uv;
    };

    ID3D11Buffer* m_vb = nullptr;
    ID3D11Buffer* m_ib = nullptr;
    UINT m_indexCount = 0;

private:
    void Release();

    // texId を PS(0) にバインド（WallShader経由）
    void BindTexture_Adapter(ID3D11DeviceContext* ctx, int texId);

    // 　要差し替え：View/Proj を WallShader に渡す（プロジェクト依存）
    void SetViewProj_Adapter();
};

#endif // WALL_PLANE_RENDERER_H
