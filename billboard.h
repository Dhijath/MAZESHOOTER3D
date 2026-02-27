/*==============================================================================

   ビルボード描画 [billboard.h]
                                                         Author : 51106
                                                         Date   : 2025/11/26
--------------------------------------------------------------------------------
==============================================================================*/
#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <DirectXMath.h>
#include <d3d11.h>

//==============================================================================
// 初期化／終了
//==============================================================================

// ビルボード機能の初期化（シェーダ／頂点バッファなどの準備）
void Billboard_Initialize();

// ビルボード機能の終了処理（リソース解放）
void Billboard_Finalize(void);

//==============================================================================
// 描画関数
//==============================================================================

// テクスチャ全体を使ったビルボード描画
// texID    : 使用するテクスチャ ID
// position : ワールド座標
// scale    : X,Y の拡大率
// pivot    : ピボット（原点オフセット） 0,0=左上 / 0.5,0.5=中央 など
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }
);

// テクスチャの一部分を切り出してビルボード描画
// tex_cut  : { x, y, w, h } （テクスチャ上の矩形・ピクセル単位）
// それ以外の引数は上と同じ
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT4& tex_cut,
    const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }  
);

// 色付きバージョン
void Billboard_Draw(
    int texID,
    const DirectX::XMFLOAT3& position,
    const DirectX::XMFLOAT2& scale,
    const DirectX::XMFLOAT4& color,
    const DirectX::XMFLOAT4& tex_cut,
    const DirectX::XMFLOAT2& pivot = { 0.0f, 0.0f }   
);




#endif // !BILLBOARD_H
