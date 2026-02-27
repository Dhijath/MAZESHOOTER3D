/*==============================================================================

   当たり判定／デバッグ描画 [collision.cpp]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
==============================================================================*/

#include "collision.h"

#include <d3d11.h>
#include <DirectXMath.h>

#include "debug_ostream.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include <algorithm>
using namespace DirectX;


// デバッグ描画用の頂点数上限
static constexpr int NUM_VERTEX = 5000; // 頂点数の上限

// 　　　 extern宣言を削除して、実体定義に変更 　　　
// collision_obb.cpp から参照できるようにする

// デバッグ描画用の頂点バッファ
ID3D11Buffer* g_pVertexBuffer = nullptr;

// D3D デバイス／コンテキスト
ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;

// 白テクスチャの ID（線を描くときに使用）
int g_WhiteId = -1;



// デバッグ描画用頂点構造体
struct Vertex
{
    XMFLOAT3 position; // 頂点座標
    XMFLOAT4 color;    // 頂点カラー
    XMFLOAT2 uv;       // UV（白テクスチャなので 0 固定でOK）
};

// ========== 以降は元のコードと同じ ==========

//==============================================================================
// 2D：円と円の当たり判定
//==============================================================================
bool Collision_OverlapCircleCircle(const Circle& a, const Circle& b)
{
    float x1 = b.center.x - a.center.x;
    float y1 = b.center.y - a.center.y;

    return (a.radius + b.radius) * (a.radius + b.radius) > (x1 * x1 + y1 * y1);
}

//==============================================================================
// 2D：矩形と矩形の当たり判定（AABB）
//==============================================================================
bool Collision_OverlapCircleBox(const Box& a, const Box& b)
{
    float at = a.center.y - a.halfHeight;
    float ab = a.center.y + a.halfHeight;
    float al = a.center.x - a.halfWidth;
    float ar = a.center.x + a.halfWidth;
    float bt = b.center.y - b.halfHeight;
    float bb = b.center.y + b.halfHeight;
    float bl = b.center.x - b.halfWidth;
    float br = b.center.x + b.halfWidth;

    return al < br && ar > bl && at < bb && ab > bt;
}

//==============================================================================
// 3D：AABB 同士の重なり判定（bool だけ）
//==============================================================================
bool Collision_IsOverLapAABB(const AABB& a, const AABB& b)
{
    return a.min.x <= b.max.x
        && a.max.x >= b.min.x
        && a.min.y <= b.max.y
        && a.max.y >= b.min.y
        && a.min.z <= b.max.z
        && a.max.z >= b.min.z;
}

//==============================================================================
// 3D：AABB 同士の衝突判定
//==============================================================================
Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
    Hit hit{};
    hit.isHit = Collision_IsOverLapAABB(a, b);
    if (!hit.isHit) return hit;

    float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    DirectX::XMFLOAT3 aCenter = a.GetCenter();
    DirectX::XMFLOAT3 bCenter = b.GetCenter();
    DirectX::XMFLOAT3 normal = { 0, 0, 0 };

    if (xdepth <= ydepth && xdepth <= zdepth)
    {
        normal.x = (bCenter.x > aCenter.x) ? 1.0f : -1.0f;
    }
    else if (ydepth <= xdepth && ydepth <= zdepth)
    {
        normal.y = (bCenter.y > aCenter.y) ? 1.0f : -1.0f;
    }
    else
    {
        normal.z = (bCenter.z > aCenter.z) ? 1.0f : -1.0f;
    }

    hit.normal = normal;
    return hit;
}

//==============================================================================
// デバッグ描画 初期化
//==============================================================================
void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_WhiteId = Texture_Load(L"resource/texture/white.png");
    g_pDevice = pDevice;
    g_pContext = pContext;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);
}

//==============================================================================
// デバッグ描画 終了処理
//==============================================================================
void Collision_DebugFinalize()
{
    SAFE_RELEASE(g_pVertexBuffer);
}

//==============================================================================
// デバッグ描画：2D 円（線）
//==============================================================================
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color)
{
    int vertexNum = static_cast<int>(circle.radius * XM_2PI + 1);

    Shader_Begin();

    D3D11_MAPPED_SUBRESOURCE msr;
    g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    Vertex* v = (Vertex*)msr.pData;

    const float rad = 2.0f * DirectX::XM_PI / vertexNum;

    for (int i = 0; i < vertexNum; ++i)
    {
        float angle = rad * i;
        v[i].position = {
            circle.center.x + circle.radius * cosf(angle),
            circle.center.y + circle.radius * sinf(angle),
            0.0f
        };
        v[i].color = color;
        v[i].uv = { 0.0f, 0.0f };
    }

    g_pContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    Shader_SetWorldMatrix(XMMatrixIdentity());

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    Set_Texture(g_WhiteId);

    g_pContext->Draw(vertexNum, 0);
}

//==============================================================================
// デバッグ描画：2D 矩形（線）
//==============================================================================
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color)
{
    Shader_Begin();

    D3D11_MAPPED_SUBRESOURCE msr;
    g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    Vertex* v = (Vertex*)msr.pData;

    v[0].position = { box.center.x - box.halfWidth, box.center.y - box.halfHeight, 0.0f };
    v[1].position = { box.center.x + box.halfWidth, box.center.y - box.halfHeight, 0.0f };
    v[2].position = { box.center.x + box.halfWidth, box.center.y + box.halfHeight, 0.0f };
    v[3].position = { box.center.x - box.halfWidth, box.center.y + box.halfHeight, 0.0f };

    for (int i = 0; i < 4; ++i)
    {
        v[i].color = color;
        v[i].uv = { 0.0f, 0.0f };
    }

    g_pContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    Shader_SetWorldMatrix(XMMatrixIdentity());

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    Set_Texture(g_WhiteId);

    g_pContext->Draw(4, 0);
}

//==============================================================================
// デバッグ描画：3D AABB（線で囲うボックス）
//==============================================================================
void Collision_DebugDraw(const AABB& aabb, const DirectX::XMFLOAT4& color)
{
    if (!g_pContext || !g_pVertexBuffer) return;

    DirectX::XMFLOAT3 corners[8] =
    {
        { aabb.min.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.max.y, aabb.max.z },
        { aabb.min.x, aabb.max.y, aabb.max.z }
    };

    Vertex v[24];

    auto set_line = [&](int v_index, int corner_a, int corner_b)
        {
            v[v_index].position = corners[corner_a];
            v[v_index].color = color;
            v[v_index].uv = { 0.0f, 0.0f };

            v[v_index + 1].position = corners[corner_b];
            v[v_index + 1].color = color;
            v[v_index + 1].uv = { 0.0f, 0.0f };
        };

    set_line(0, 0, 1);
    set_line(2, 1, 5);
    set_line(4, 5, 4);
    set_line(6, 4, 0);

    set_line(8, 3, 2);
    set_line(10, 2, 6);
    set_line(12, 6, 7);
    set_line(14, 7, 3);

    set_line(16, 0, 3);
    set_line(18, 1, 2);
    set_line(20, 5, 6);
    set_line(22, 4, 7);

    Shader_Begin();

    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if (FAILED(hr)) return;

    memcpy(msr.pData, v, sizeof(Vertex) * 24);

    g_pContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    Shader_SetWorldMatrix(DirectX::XMMatrixIdentity());

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    Set_Texture(g_WhiteId);

    g_pContext->Draw(24, 0);
}

//==============================================================================
// キャラが「上に乗っている面」の Y 座標を取得
//==============================================================================
bool Collision_GetSupportY(
    const AABB& actor,
    const AABB* colliders,
    int colliderCount,
    float* outSupportY)
{
    float bestY = -FLT_MAX;
    bool found = false;

    for (int i = 0; i < colliderCount; ++i)
    {
        const AABB& c = colliders[i];

        Hit hit = Collision_IsHitAABB(actor, c);
        if (!hit.isHit) continue;

        if (hit.normal.y == 1.0f)
        {
            float y = c.max.y;

            if (!found || y > bestY)
            {
                bestY = y;
                found = true;
            }
        }
    }

    if (found && outSupportY)
    {
        *outSupportY = bestY;
    }

    return found;
}