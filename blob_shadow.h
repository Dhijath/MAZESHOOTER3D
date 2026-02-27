/*==============================================================================

   丸影（Blob Shadow）管理 [blob_shadow.h]
                                                         Author : 51106
                                                         Date   : 2025/12/17
--------------------------------------------------------------------------------

   ・MeshField に簡易的な丸影を落とすための定数バッファ管理
   ・PS register(b6) に渡す値を毎フレーム更新する
   ・ShadowMap 本実装の前段（見た目確認）として使う

==============================================================================*/
#pragma once
#include <DirectXMath.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace BlobShadow
{
    bool Initialize(ID3D11Device* device);
    void Finalize();

    // プレイヤー位置などを更新して PS(b6) にセット
    void SetToPixelShader(
        ID3D11DeviceContext* context,
        const DirectX::XMFLOAT3& centerW,
        float radius = 1.8f,
        float softness = 0.8f,
        float strength = 0.55f);

    bool IsReady();
}
