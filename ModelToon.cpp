/*==============================================================================

   トゥーンモデル描画 [ModelToon.cpp]
                                                         Author : 51106
                                                         Date   : 2026/03/11
--------------------------------------------------------------------------------
   ・ModelDraw() を 2 パスに拡張したトゥーン版
     1パス目 : ShaderToon_BeginOutline() → アウトライン（裏面膨張）
     2パス目 : ShaderToon_Begin()        → トゥーン本体
   ・テクスチャ / マテリアルカラーの取り回しは ModelDraw() と完全に同じ

==============================================================================*/
#include "ModelToon.h"
#include "ShaderToon.h"
#include "texture.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;

// model.cpp 内の白テクスチャ ID を参照するための extern
// （model.cpp の g_TextureWhite は namespace 内 static なので、
//   ここでは Texture_Load で再取得する方式にする）
namespace
{
    // 白テクスチャ（ModelToon 用。初回 ModelDrawToon 呼び出し時にロード）
    int g_ToonTextureWhite = -1;
}

//==============================================================================
// 内部ヘルパー：メッシュのテクスチャを PS スロット 0 にセット
// （model.cpp の ModelDraw と同じロジック）
//==============================================================================
static void BindMeshTexture(MODEL* model, unsigned int m)
{
    aiString texName;
    aiMaterial* aimaterial =
        model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];
    aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texName);

    bool textureSet = false;

    if (texName.length != 0)
    {
        auto it = model->Texture.find(texName.C_Str());
        if (it != model->Texture.end() && it->second)
        {
            ID3D11ShaderResourceView* srv = it->second;
            Direct3D_GetContext()->PSSetShaderResources(0, 1, &srv);
            textureSet = true;
        }
    }

    if (!textureSet)
    {
        // 白テクスチャ（未ロードなら初回ここで取得）
        if (g_ToonTextureWhite < 0)
            g_ToonTextureWhite = Texture_Load(L"resource/texture/white.png");

        Set_Texture(g_ToonTextureWhite);
    }
}

//==============================================================================
// ModelDrawToon
//==============================================================================
void ModelDrawToon(MODEL* model, const XMMATRIX& mtxWorld)
{
    // モデル未読込ガード
    if (!model || !model->AiScene || model->AiScene->mNumMeshes == 0)
        return;

    auto* ctx = Direct3D_GetContext();
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //==========================================================================
    // 1パス目：アウトライン
    //   CullFront + 法線膨張 VS + 黒べた PS
    //==========================================================================
    ShaderToon_BeginOutline();
    ShaderToon_SetWorldMatrix(mtxWorld);

    for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];

        if (mesh->mNumFaces == 0)           continue;
        if (!model->VertexBuffer[m])        continue;
        if (!model->IndexBuffer[m])         continue;

        // アウトラインパスではテクスチャ不要だが
        // IASetVertexBuffers / IASetIndexBuffer は必要
        UINT stride = sizeof(float) * (3 + 3 + 4 + 2); // Vertex3D のサイズ
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        ctx->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        ctx->DrawIndexed(mesh->mNumFaces * 3, 0, 0);
    }

    //==========================================================================
    // 2パス目：トゥーン本体
    //   CullBack + 通常 VS + トゥーン PS
    //==========================================================================
    ShaderToon_Begin();
    ShaderToon_SetWorldMatrix(mtxWorld);

    for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];

        if (mesh->mNumFaces == 0)           continue;
        if (!model->VertexBuffer[m])        continue;
        if (!model->IndexBuffer[m])         continue;

        // テクスチャ設定（model.cpp と同じロジック）
        BindMeshTexture(model, m);

        // マテリアルカラー設定（model.cpp と同じロジック）
        {
            aiMaterial* aimaterial =
                model->AiScene->mMaterials[mesh->mMaterialIndex];
            aiColor3D diffuse;
            aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            ShaderToon_SetColor(XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, 1.0f));
        }

        // VB / IB セット＆描画
        UINT stride = sizeof(float) * (3 + 3 + 4 + 2); // Vertex3D のサイズ
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);
        ctx->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

        ctx->DrawIndexed(mesh->mNumFaces * 3, 0, 0);
    }
}