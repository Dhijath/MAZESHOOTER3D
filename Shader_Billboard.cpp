/*==============================================================================

   ビルボード用シェーダ [shader_billboard.cpp]
                                                         Author : 51106
                                                         Date   : 2026/02/15
--------------------------------------------------------------------------------

==============================================================================*/

#include "shader_billboard.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "Sampler.h"

// 匿名名前空間：この cpp 内だけで使うシェーダ関連のグローバル
namespace
{
    // VS / PS 本体
    ID3D11VertexShader* g_pVertexShader = nullptr;
    ID3D11PixelShader* g_pPixelShader = nullptr;

    // 頂点レイアウト
    ID3D11InputLayout* g_pInputLayout = nullptr;

    // VS 用定数バッファ
    ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // World 行列
    ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // View  行列
    ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // Proj  行列
    ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // UVParameter
    ID3D11Buffer* g_pVSConstantBuffer4 = nullptr; // 色用

    // PS 用定数バッファ
    ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // カラー
}

//==============================================================================
// 初期化：シェーダ本体、入力レイアウト、定数バッファを作成
//==============================================================================
bool Shader_Billboard_Initialize()
{
    HRESULT hr; // DirectX 関数の戻り値

    // ----------------------------------------
    // 頂点シェーダ（事前コンパイル済み .cso）読み込み
    // ----------------------------------------
    std::ifstream ifs_vs("resource/shader/shader_vertex_billboard.cso", std::ios::binary);

    if (!ifs_vs)
    {
        MessageBox(nullptr, "shader_vertex_billboard.csoが見つかりません", "error", MB_OK);
        return false;
    }

    // ファイルサイズを取得
    ifs_vs.seekg(0, std::ios::end);
    std::streamsize filesize = ifs_vs.tellg();
    ifs_vs.seekg(0, std::ios::beg);

    // バイナリデータを格納するためのバッファを確保
    unsigned char* vsbinary_pointer = new unsigned char[filesize];

    // バイナリデータを読み込む
    ifs_vs.read(reinterpret_cast<char*>(vsbinary_pointer), filesize);
    ifs_vs.close();

    // 頂点シェーダーの作成
    hr = Direct3D_GetDevice()->CreateVertexShader(
        vsbinary_pointer,
        filesize,
        nullptr,
        &g_pVertexShader
    );

    if (FAILED(hr))
    {
        hal::dout << "Shader_Billboard_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
        delete[] vsbinary_pointer;
        return false;
    }

    // ----------------------------------------
    // 頂点レイアウトの定義（POSITION / COLOR / TEXCOORD）
    // ----------------------------------------
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT num_elements = ARRAYSIZE(layout);

    // 頂点レイアウトの作成
    hr = Direct3D_GetDevice()->CreateInputLayout(
        layout,
        num_elements,
        vsbinary_pointer,
        filesize,
        &g_pInputLayout
    );

    // VS バイナリはもう不要なので開放
    delete[] vsbinary_pointer;

    if (FAILED(hr))
    {
        hal::dout << "Shader_Billboard_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
        return false;
    }

    // ----------------------------------------
    // VS 用定数バッファ（World / View / Proj / UVParameter）
    // ----------------------------------------
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = 0;

    // 行列 1 つ分ずつ
    buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // World
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // View
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // Proj

    // UVParameter 用
    buffer_desc.ByteWidth = sizeof(UVParameter);
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer3);


    // VS用の色バッファ
    buffer_desc.ByteWidth = sizeof(XMFLOAT4);
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer4);

    // ----------------------------------------
    // ピクセルシェーダー読み込み（事前コンパイル済み .cso）
    // ----------------------------------------
    std::ifstream ifs_ps("resource/shader/shader_pixel_billboard.cso", std::ios::binary);
    if (!ifs_ps)
    {
        MessageBox(nullptr, "shader_pixel_billboard.csoが見つかりません", "error", MB_OK);
        return false;
    }

    ifs_ps.seekg(0, std::ios::end);
    filesize = ifs_ps.tellg();
    ifs_ps.seekg(0, std::ios::beg);

    unsigned char* psbinary_pointer = new unsigned char[filesize];
    ifs_ps.read(reinterpret_cast<char*>(psbinary_pointer), filesize);
    ifs_ps.close();

    // ピクセルシェーダーの作成
    hr = Direct3D_GetDevice()->CreatePixelShader(
        psbinary_pointer,
        filesize,
        nullptr,
        &g_pPixelShader
    );

    delete[] psbinary_pointer;

    if (FAILED(hr))
    {
        hal::dout << "Shader_Billboard_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
        return false;
    }

    // PS 用定数バッファ（色用）
    buffer_desc.ByteWidth = sizeof(XMFLOAT4);
    Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);

    return true;
}

//==============================================================================
// 終了処理：シェーダや定数バッファ等の解放
//==============================================================================
void Shader_Billboard_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);

    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVSConstantBuffer2);
    SAFE_RELEASE(g_pVSConstantBuffer3);
    SAFE_RELEASE(g_pVSConstantBuffer4);
    SAFE_RELEASE(g_pPSConstantBuffer0);

    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}

//==============================================================================
// World 行列設定
// シェーダに渡す前に転置してから定数バッファへ
//==============================================================================
void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    Direct3D_GetContext()->UpdateSubresource(
        g_pVSConstantBuffer0,
        0,
        nullptr,
        &transpose,
        0,
        0
    );
}

//==============================================================================
// View 行列設定
//==============================================================================
void Shader_Billboard_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    Direct3D_GetContext()->UpdateSubresource(
        g_pVSConstantBuffer1,
        0,
        nullptr,
        &transpose,
        0,
        0
    );
}

//==============================================================================
// Projection 行列設定
//==============================================================================
void Shader_Billboard_SetProjectMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    Direct3D_GetContext()->UpdateSubresource(
        g_pVSConstantBuffer2,
        0,
        nullptr,
        &transpose,
        0,
        0
    );
}

//==============================================================================
// ピクセルシェーダー側の色設定
//==============================================================================
void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color)
{
    // VS用バッファに送る（b4）
    Direct3D_GetContext()->UpdateSubresource(
        g_pVSConstantBuffer4, 
        0,
        nullptr,
        &color,
        0,
        0
    );
}

//==============================================================================
// UVParameter 設定（VS 側）
//==============================================================================
void Shader_Billboard_SetUVParameter(const UVParameter& parameter)
{
    Direct3D_GetContext()->UpdateSubresource(
        g_pVSConstantBuffer3,
        0,
        nullptr,
        &parameter,
        0,
        0
    );
}

//==============================================================================
// 描画開始前に呼び出し：シェーダ／レイアウト／定数バッファをセット
//==============================================================================
void Shader_Billboard_Begin()
{
    Direct3D_GetContext()->VSSetShader(g_pVertexShader, nullptr, 0);
    Direct3D_GetContext()->PSSetShader(g_pPixelShader, nullptr, 0);

    Direct3D_GetContext()->IASetInputLayout(g_pInputLayout);

    // VS 定数バッファを設定（b0〜b4）
    Direct3D_GetContext()->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
    Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
    Direct3D_GetContext()->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);
    Direct3D_GetContext()->VSSetConstantBuffers(4, 1, &g_pVSConstantBuffer4);  

    // PS 定数バッファを設定（b0）
    Direct3D_GetContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);
}