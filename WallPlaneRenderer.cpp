/*==============================================================================

   壁Plane描画 [WallPlaneRenderer.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/02
--------------------------------------------------------------------------------

   ■このファイルがやること
   ・TileWall が生成した WallPlane（論理的な壁面）を描画用メッシュに変換する
   ・1 WallPlane = Quad（4頂点 + 2三角形）
   ・すべての壁を 1 つの VertexBuffer / IndexBuffer にまとめて描画する
   ・描画は DrawIndexed を 1 回だけ呼ぶ

   ■設計方針
   ・頂点はワールド座標で確定させる
   ・World 行列による変換は行わない
   ・UV は頂点生成時に焼き込む

==============================================================================*/

#include "WallPlaneRenderer.h"
#include "direct3d.h"
#include "WallShader.h"

using namespace DirectX;

//==============================================================================
// コンストラクタ / デストラクタ
//==============================================================================

WallPlaneRenderer::WallPlaneRenderer()
{
}

WallPlaneRenderer::~WallPlaneRenderer()
{
    Release();
}

//==============================================================================
// GPUリソース解放
//==============================================================================

void WallPlaneRenderer::Release()
{
    if (m_ib)
    {
        m_ib->Release();
        m_ib = nullptr;
    }

    if (m_vb)
    {
        m_vb->Release();
        m_vb = nullptr;
    }

    m_indexCount = 0;
}

//==============================================================================
// ベクトル補助関数
//==============================================================================

static XMFLOAT3 Normalize3(const XMFLOAT3& v)
{
    XMVECTOR vv = XMLoadFloat3(&v);
    vv = XMVector3Normalize(vv);

    XMFLOAT3 o{};
    XMStoreFloat3(&o, vv);
    return o;
}

static XMFLOAT3 Cross3(const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMVECTOR va = XMLoadFloat3(&a);
    XMVECTOR vb = XMLoadFloat3(&b);
    XMVECTOR vc = XMVector3Cross(va, vb);

    XMFLOAT3 o{};
    XMStoreFloat3(&o, vc);
    return o;
}

//==============================================================================
// WallPlane → 描画用メッシュ構築
//==============================================================================

void WallPlaneRenderer::Build(const std::vector<WallPlane>& planes)
{
    Release();

    ID3D11Device* dev = Direct3D_GetDevice();
    if (!dev) return;

    std::vector<Vtx>      vtx;
    std::vector<uint32_t> idx;

    vtx.reserve(planes.size() * 4);
    idx.reserve(planes.size() * 6);

    const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };
    const XMFLOAT4 white = { 1, 1, 1, 1 };

    for (const WallPlane& p : planes)
    {
        // 法線（内側向き）
        XMFLOAT3 n = Normalize3(p.normal);

        // TileWall側で設定したtangentUをそのまま使用する
        XMFLOAT3 u = Normalize3(p.tangentU);

        // 縦方向ベクトル（常に +Y）
        XMFLOAT3 v = up;

        const float hx = p.width * 0.5f;
        const float hy = p.height * 0.5f;

        auto AddVertex = [&](float su, float sv, float uu, float vv)
            {
                Vtx vt{};

                vt.pos =
                {
                    p.center.x + u.x * (su * hx) + v.x * (sv * hy),
                    p.center.y + u.y * (su * hx) + v.y * (sv * hy),
                    p.center.z + u.z * (su * hx) + v.z * (sv * hy)
                };

                vt.nrm = n;
                vt.col = white;
                vt.uv = { uu, vv };

                vtx.push_back(vt);
            };

        const uint32_t base = static_cast<uint32_t>(vtx.size());

        const float u0 = 0.0f;
        const float u1 = p.uvRepeat.x;
        const float v0 = 0.0f;
        const float v1 = p.uvRepeat.y;

        // TL, TR, BR, BL
        AddVertex(-1, +1, u0, v0);
        AddVertex(+1, +1, u1, v0);
        AddVertex(+1, -1, u1, v1);
        AddVertex(-1, -1, u0, v1);

        // CCW順
        idx.push_back(base + 0);
        idx.push_back(base + 1);
        idx.push_back(base + 2);

        idx.push_back(base + 0);
        idx.push_back(base + 2);
        idx.push_back(base + 3);
    }

    if (vtx.empty() || idx.empty()) return;

    //--------------------------------------------------------------------------
    // VertexBuffer 作成
    //--------------------------------------------------------------------------
    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.ByteWidth = static_cast<UINT>(sizeof(Vtx) * vtx.size());

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = vtx.data();

        dev->CreateBuffer(&bd, &sd, &m_vb);
    }

    //--------------------------------------------------------------------------
    // IndexBuffer 作成
    //--------------------------------------------------------------------------
    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * idx.size());

        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = idx.data();

        dev->CreateBuffer(&bd, &sd, &m_ib);
    }

    m_indexCount = static_cast<UINT>(idx.size());
}

//==============================================================================
// テクスチャ設定
//==============================================================================

void WallPlaneRenderer::BindTexture_Adapter(ID3D11DeviceContext*, int texId)
{
    WallShader_SetTexture(texId);
}

//==============================================================================
// View / Projection 設定
//==============================================================================

void WallPlaneRenderer::SetViewProj_Adapter()
{
}

//==============================================================================
// 描画
//==============================================================================

void WallPlaneRenderer::Draw(int texId)
{
    if (!m_vb || !m_ib || m_indexCount == 0) return;

    ID3D11DeviceContext* ctx = Direct3D_GetContext();
    if (!ctx) return;

    WallShader_Begin();
    SetViewProj_Adapter();

    WallShader_SetColor({ 1, 1, 1, 1 });
    BindTexture_Adapter(ctx, texId);

    UINT stride = sizeof(Vtx);
    UINT offset = 0;

    ctx->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    ctx->IASetIndexBuffer(m_ib, DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    WallShader_SetWorldMatrix(XMMatrixIdentity());
    WallShader_SetUVRepeat({ 1.0f, 1.0f });

    ctx->DrawIndexed(m_indexCount, 0, 0);
}