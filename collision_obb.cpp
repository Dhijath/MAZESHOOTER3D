/*==============================================================================

   OBB�����蔻�� [collision_obb.cpp]
                                                         Author : 51106
                                                         Date   : 2026/01/17
--------------------------------------------------------------------------------
==============================================================================*/

#include "collision_obb.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <algorithm>
#include <cfloat>

using namespace DirectX;

// �f�o�b�O�`��p�icollision.cpp �Œ�`����Ă����̂�Q�Ɓj
extern ID3D11Buffer* g_pVertexBuffer;
extern ID3D11Device* g_pDevice;
extern ID3D11DeviceContext* g_pContext;
extern int g_WhiteId;

namespace
{
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
        XMFLOAT2 uv;
    };
}

//==============================================================================
// �ʒu�E�T�C�Y�E�����ifront�j���琶��
//==============================================================================
OBB OBB::CreateFromFront(
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT3& halfExtents,
    const DirectX::XMFLOAT3& front)
{
    OBB obb;
    obb.center = position;
    obb.halfExtents = halfExtents;

    // Y���͏�ɏ����
    obb.axisY = { 0.0f, 1.0f, 0.0f };

    // front �𐳋K������ Z���ɂ���
    XMVECTOR vFront = XMLoadFloat3(&front);
    vFront = XMVector3Normalize(vFront);
    XMStoreFloat3(&obb.axisZ, vFront);

    // X�� = Y �~ Z�i�E��n�j
    XMVECTOR vY = XMLoadFloat3(&obb.axisY);
    XMVECTOR vZ = XMLoadFloat3(&obb.axisZ);
    XMVECTOR vX = XMVector3Cross(vY, vZ);
    vX = XMVector3Normalize(vX);
    XMStoreFloat3(&obb.axisX, vX);

    return obb;
}

//==============================================================================
// AABB���琶���i��]�Ȃ��j
//==============================================================================
OBB OBB::CreateFromAABB(const AABB& aabb)
{
    OBB obb;

    // ���S���W
    obb.center.x = (aabb.min.x + aabb.max.x) * 0.5f;
    obb.center.y = (aabb.min.y + aabb.max.y) * 0.5f;
    obb.center.z = (aabb.min.z + aabb.max.z) * 0.5f;

    // ���T�C�Y
    obb.halfExtents.x = (aabb.max.x - aabb.min.x) * 0.5f;
    obb.halfExtents.y = (aabb.max.y - aabb.min.y) * 0.5f;
    obb.halfExtents.z = (aabb.max.z - aabb.min.z) * 0.5f;

    // ���͕W����XYZ
    obb.axisX = { 1.0f, 0.0f, 0.0f };
    obb.axisY = { 0.0f, 1.0f, 0.0f };
    obb.axisZ = { 0.0f, 0.0f, 1.0f };

    return obb;
}

//==============================================================================
// ����w���p�[�F���鎲�ւ̓��e����v�Z
//
// �����
// �EOBB��C�ӂ̎��ɓ��e�����Ƃ��́u���a�v��Ԃ�
//
// ������
// �Eobb  : ���e�Ώۂ�OBB
// �Eaxis : ���e���i���K���ς݁j
//
// ���߂�l
// �E���e���a�i��ɐ��̒l�j
//==============================================================================
static float ProjectOBBOntoAxis(const OBB& obb, const XMVECTOR& axis)
{
    XMVECTOR vX = XMLoadFloat3(&obb.axisX);
    XMVECTOR vY = XMLoadFloat3(&obb.axisY);
    XMVECTOR vZ = XMLoadFloat3(&obb.axisZ);

    float projX = fabsf(XMVectorGetX(XMVector3Dot(vX, axis))) * obb.halfExtents.x;
    float projY = fabsf(XMVectorGetX(XMVector3Dot(vY, axis))) * obb.halfExtents.y;
    float projZ = fabsf(XMVectorGetX(XMVector3Dot(vZ, axis))) * obb.halfExtents.z;

    return projX + projY + projZ;
}

//==============================================================================
// OBB���m�̏d�Ȃ蔻��i�������藝�j
//==============================================================================
bool Collision_IsOverlapOBB(const OBB& a, const OBB& b)
{
    // ���S�ԃx�N�g��
    XMVECTOR vCenterA = XMLoadFloat3(&a.center);
    XMVECTOR vCenterB = XMLoadFloat3(&b.center);
    XMVECTOR vDist = vCenterB - vCenterA;

    // Y����]�݂̂Ȃ̂ŁA���莲��4�{�i�eOBB��X/Z���j
    XMVECTOR axes[5] =
    {
        XMLoadFloat3(&a.axisX),
        XMLoadFloat3(&a.axisY),  // Y-axis check (needed for height separation)
        XMLoadFloat3(&a.axisZ),
        XMLoadFloat3(&b.axisX),
        XMLoadFloat3(&b.axisZ)
    };

    for (int i = 0; i < 5; ++i)
    {
        XMVECTOR axis = XMVector3Normalize(axes[i]);

        // ���S�ԋ����̓��e
        float distProj = fabsf(XMVectorGetX(XMVector3Dot(vDist, axis)));

        // �eOBB�̓��e���a
        float radiusA = ProjectOBBOntoAxis(a, axis);
        float radiusB = ProjectOBBOntoAxis(b, axis);

        // �������������������Փ�
        if (distProj > radiusA + radiusB)
            return false;
    }

    // ���ׂĂ̎��ŏd�Ȃ��Ă��� �� �Փ�
    return true;
}

//==============================================================================
// OBB��AABB�̏d�Ȃ蔻��
//==============================================================================
bool Collision_IsOverlapOBB_AABB(const OBB& obb, const AABB& aabb)
{
    // AABB��OBB�ɕϊ����Ĕ���
    OBB obbFromAABB = OBB::CreateFromAABB(aabb);
    return Collision_IsOverlapOBB(obb, obbFromAABB);
}

//==============================================================================
// OBB���m�̏Փ˔���i�����o�������t���j
//==============================================================================
Hit Collision_IsHitOBB(const OBB& a, const OBB& b)
{
    Hit hit{};
    hit.isHit = false;

    XMVECTOR vCenterA = XMLoadFloat3(&a.center);
    XMVECTOR vCenterB = XMLoadFloat3(&b.center);
    XMVECTOR vDist = vCenterB - vCenterA;

    XMVECTOR axes[5] =
    {
        XMLoadFloat3(&a.axisX),
        XMLoadFloat3(&a.axisY),  // Y-axis check (needed for height separation)
        XMLoadFloat3(&a.axisZ),
        XMLoadFloat3(&b.axisX),
        XMLoadFloat3(&b.axisZ)
    };

    float minPenetration = FLT_MAX;
    XMVECTOR bestAxis = XMVectorZero();

    for (int i = 0; i < 5; ++i)
    {
        XMVECTOR axis = XMVector3Normalize(axes[i]);

        float distProj = XMVectorGetX(XMVector3Dot(vDist, axis));
        float radiusA = ProjectOBBOntoAxis(a, axis);
        float radiusB = ProjectOBBOntoAxis(b, axis);

        float penetration = (radiusA + radiusB) - fabsf(distProj);

        // �������������������Փ�
        if (penetration < 0.0f)
            return hit;

        // �ŏ��߂荞�ݎ���L�^
        if (penetration < minPenetration)
        {
            minPenetration = penetration;
            bestAxis = axis;

            // b����a������o�������ɏC��
            if (distProj < 0.0f)
                bestAxis = -bestAxis;
        }
    }

    // �Փ˂���
    hit.isHit = true;
    XMStoreFloat3(&hit.normal, XMVector3Normalize(bestAxis));

    return hit;
}

//==============================================================================
// OBB��AABB�̏Փ˔���i�����o�������t���j
//==============================================================================
Hit Collision_IsHitOBB_AABB(const OBB& obb, const AABB& aabb)
{
    // AABB��OBB�ɕϊ����Ĕ���
    OBB obbFromAABB = OBB::CreateFromAABB(aabb);
    return Collision_IsHitOBB(obb, obbFromAABB);
}

//==============================================================================
// �f�o�b�O�`��FOBB�i���ň͂��j
//==============================================================================
void Collision_DebugDraw(const OBB& obb, const DirectX::XMFLOAT4& color)
{
    // ���������`�F�b�N
    if (!g_pContext || !g_pVertexBuffer) return;

    XMVECTOR vCenter = XMLoadFloat3(&obb.center);
    XMVECTOR vX = XMLoadFloat3(&obb.axisX) * obb.halfExtents.x;
    XMVECTOR vY = XMLoadFloat3(&obb.axisY) * obb.halfExtents.y;
    XMVECTOR vZ = XMLoadFloat3(&obb.axisZ) * obb.halfExtents.z;

    // 8���_��v�Z
    XMFLOAT3 corners[8];
    XMStoreFloat3(&corners[0], vCenter - vX - vY - vZ); // 0: Bottom-Back-Left
    XMStoreFloat3(&corners[1], vCenter + vX - vY - vZ); // 1: Bottom-Back-Right
    XMStoreFloat3(&corners[2], vCenter + vX + vY - vZ); // 2: Top-Back-Right
    XMStoreFloat3(&corners[3], vCenter - vX + vY - vZ); // 3: Top-Back-Left
    XMStoreFloat3(&corners[4], vCenter - vX - vY + vZ); // 4: Bottom-Front-Left
    XMStoreFloat3(&corners[5], vCenter + vX - vY + vZ); // 5: Bottom-Front-Right
    XMStoreFloat3(&corners[6], vCenter + vX + vY + vZ); // 6: Top-Front-Right
    XMStoreFloat3(&corners[7], vCenter - vX + vY + vZ); // 7: Top-Front-Left

    // 12�{�̐��i24���_�j
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

    // ��ʁi4�{�j
    set_line(0, 0, 1);
    set_line(2, 1, 5);
    set_line(4, 5, 4);
    set_line(6, 4, 0);

    // ��ʁi4�{�j
    set_line(8, 3, 2);
    set_line(10, 2, 6);
    set_line(12, 6, 7);
    set_line(14, 7, 3);

    // ������4��
    set_line(16, 0, 3);
    set_line(18, 1, 2);
    set_line(20, 5, 6);
    set_line(22, 4, 7);

    // D3D�`�揈��
    Shader_Begin();

    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    if (FAILED(hr)) return;

    memcpy(msr.pData, v, sizeof(Vertex) * 24);
    g_pContext->Unmap(g_pVertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    Shader_SetWorldMatrix(XMMatrixIdentity());

    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    Set_Texture(g_WhiteId);

    g_pContext->Draw(24, 0);
}
