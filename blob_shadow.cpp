/*==============================================================================

   丸影（Blob Shadow）管理 [blob_shadow.cpp]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------

==============================================================================*/
#include "blob_shadow.h"
#include <d3d11.h>

namespace
{
    struct BlobShadowCB
    {
        DirectX::XMFLOAT3 centerW;
        float radius;

        float softness;
        float strength;
        float pad[2]; // 16バイト境界
    };

    ID3D11Buffer* g_pCB = nullptr;

    void SafeRelease(IUnknown*& p)
    {
        if (p) { p->Release(); p = nullptr; }
    }
}

namespace BlobShadow
{
    bool Initialize(ID3D11Device* device)
    {
        if (!device) return false;
        if (g_pCB) return true;

        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = sizeof(BlobShadowCB);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        if (FAILED(device->CreateBuffer(&bd, nullptr, &g_pCB)))
            return false;

        return true;
    }

    void Finalize()
    {
        SafeRelease((IUnknown*&)g_pCB);
    }

    bool IsReady()
    {
        return g_pCB != nullptr;
    }

    void SetToPixelShader(
        ID3D11DeviceContext* context,
        const DirectX::XMFLOAT3& centerW,
        float radius,
        float softness,
        float strength)
    {
        if (!context || !g_pCB) return;

        BlobShadowCB cb{};
        cb.centerW = centerW;
        cb.radius = radius;
        cb.softness = softness;
        cb.strength = strength;

        context->UpdateSubresource(g_pCB, 0, nullptr, &cb, 0, 0);

        // PS register(b6)
        ID3D11Buffer* buf = g_pCB;
        context->PSSetConstantBuffers(6, 1, &buf);
    }
}
