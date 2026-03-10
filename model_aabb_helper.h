/*==============================================================================
   モデルAABB自動設定ヘルパー [model_aabb_helper.h]
                                                         Author : 51106
                                                         Date   : 2026/01/16
--------------------------------------------------------------------------------
   ■役割
   ・モデルの実際のサイズからAABB寸法を自動取得
   ・Enemy / Player の当たり判定を自動調整

   ■使用例
   // Enemy::Initialize() 内で
   m_pModel = ModelLoad("resource/Models/Bullet.fbx", 0.5f);
   ModelSize size = ModelGetSize(m_pModel, 0.5f);

   // メンバ変数に保存（80%に縮小して余裕を持たせる）
   m_HalfWidth = size.halfWidth * 0.8f;
   m_HalfDepth = size.halfDepth * 0.8f;
   m_Height = size.height * 0.9f;
==============================================================================*/

#pragma once
#include "model.h"
#include <DirectXMath.h>

//==============================================================================
// モデルサイズ情報
//==============================================================================
struct ModelSize
{
    float width;      // X方向の幅（スケール適用済み）
    float height;     // Y方向の高さ
    float depth;      // Z方向の奥行き

    float halfWidth;  // 半幅（当たり判定用）
    float halfDepth;  // 半奥行き
};

//==============================================================================
// モデルからサイズを自動取得
// 
// ■引数
// ・model : ModelLoad で読み込んだモデル
// ・scale : ModelLoad 時のスケール値（例：0.5f）
// 
// ■戻り値
// ・モデルのワールドサイズ（スケール適用済み）
// 
// ■注意
// ・ModelLoad時に aiProcess_GenBoundingBoxes が必要
//   （既に model.cpp で指定済み）
//==============================================================================
inline ModelSize ModelGetSize(MODEL* model, float scale)
{
    if (!model || !model->AiScene || model->AiScene->mNumMeshes == 0)
    {
        // デフォルト値（1m立方体）
        return { 1.0f, 1.0f, 1.0f, 0.5f, 0.5f };
    }

    // 全メッシュのAABBを統合
    aiVector3D min = model->AiScene->mMeshes[0]->mAABB.mMin;
    aiVector3D max = model->AiScene->mMeshes[0]->mAABB.mMax;

    for (unsigned int m = 1; m < model->AiScene->mNumMeshes; m++)
    {
        aiMesh* mesh = model->AiScene->mMeshes[m];

        if (min.x > mesh->mAABB.mMin.x) min.x = mesh->mAABB.mMin.x;
        if (min.y > mesh->mAABB.mMin.y) min.y = mesh->mAABB.mMin.y;
        if (min.z > mesh->mAABB.mMin.z) min.z = mesh->mAABB.mMin.z;

        if (max.x < mesh->mAABB.mMax.x) max.x = mesh->mAABB.mMax.x;
        if (max.y < mesh->mAABB.mMax.y) max.y = mesh->mAABB.mMax.y;
        if (max.z < mesh->mAABB.mMax.z) max.z = mesh->mAABB.mMax.z;
    }

    // スケール適用
    ModelSize size;
    size.width = (max.x - min.x) * scale;
    size.height = (max.y - min.y) * scale;
    size.depth = (max.z - min.z) * scale;

    size.halfWidth = size.width * 0.5f;
    size.halfDepth = size.depth * 0.5f;

    return size;
}

//==============================================================================
// デバッグ用：モデルサイズをコンソール出力
//==============================================================================
#include <iostream>

inline void ModelPrintSize(MODEL* model, float scale, const char* name)
{
    ModelSize size = ModelGetSize(model, scale);

    std::cout << "=== Model Size: " << name << " ===" << std::endl;
    std::cout << "  Width  (X): " << size.width << " m" << std::endl;
    std::cout << "  Height (Y): " << size.height << " m" << std::endl;
    std::cout << "  Depth  (Z): " << size.depth << " m" << std::endl;
    std::cout << "  HalfWidth : " << size.halfWidth << " m" << std::endl;
    std::cout << "  HalfDepth : " << size.halfDepth << " m" << std::endl;
    std::cout << "================================" << std::endl;
}