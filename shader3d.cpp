/*==============================================================================

   3D描画用シェーダ実装 [Shader3d.cpp]
														 Author : 51106
														 Date   : 2026/02/15
--------------------------------------------------------------------------------

   ・VS/PS を .cso から読み込み、入力レイアウトと各種定数バッファを用意
   ・World / View / Projection 行列とカラーを更新して描画パイプラインへ設定
   ・Cube / Model などの3D描画で使用

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader3d.h"

namespace
{
    ID3D11VertexShader* g_pVertexShader = nullptr;
    ID3D11InputLayout* g_pInputLayout = nullptr;
    ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;
    ID3D11Buffer* g_pVSConstantBuffer1 = nullptr;
    ID3D11Buffer* g_pVSConstantBuffer2 = nullptr;
    ID3D11Buffer* g_pPSConstantBuffer0 = nullptr;
    ID3D11PixelShader* g_pPixelShader = nullptr;

    // 注意：ここは初期化時に外部から渡されるので、Release は不要
    ID3D11Device* g_pDevice = nullptr;
    ID3D11DeviceContext* g_pContext = nullptr;
}

bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    HRESULT hr; // DirectX 関数の戻り値（成功か失敗かを表す）

    // デバイスとデバイスコンテキストのチェック
    if (!pDevice || !pContext) {
        hal::dout << "Shader3d_Initialize() : 渡されたデバイスやコンテキストが不正です" << std::endl;
        return false;
    }

    // デバイスとデバイスコンテキストを保存しておく
    g_pDevice = pDevice;
    g_pContext = pContext;

    // 事前コンパイル済み頂点シェーダー(.cso)をバイナリとして読み込み
    std::ifstream ifs_vs("resource/shader/shader_vertex_3d.cso", std::ios::binary);

    if (!ifs_vs) {
        MessageBox(nullptr, "shader_vertex_3d.cso が見つかりません", "error", MB_OK);
        return false;
    }

    // ファイルサイズ取得
    ifs_vs.seekg(0, std::ios::end);
    std::streamsize filesize = ifs_vs.tellg();
    ifs_vs.seekg(0, std::ios::beg);

    // バイナリデータを格納するバッファを確保
    unsigned char* vsbinary_pointer = new unsigned char[filesize];

    // シェーダーバイナリ読み込み
    ifs_vs.read((char*)vsbinary_pointer, filesize);
    ifs_vs.close();

    // 頂点シェーダーを作成
    hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);
    if (FAILED(hr)) {
        hal::dout << "Shader3d_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
        delete[] vsbinary_pointer;
        return false;
    }

    // 頂点レイアウト定義（頂点バッファの中身の並びを GPU に教える）
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT num_elements = ARRAYSIZE(layout);

    // 入力レイアウト作成
    hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

    // バッファはもう不要なので解放
    delete[] vsbinary_pointer;

    if (FAILED(hr)) {
        hal::dout << "Shader3d_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
        return false;
    }

    // 頂点シェーダ用 定数バッファを作成（world / view / proj 行列用）
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);            // 行列 1個分のサイズ
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;    // 定数バッファとして使う

    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // World 行列用
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // View 行列用
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // Projection 行列用

    // ピクセルシェーダー(.cso)読み込み
    std::ifstream ifs_ps("resource/shader/shader_pixel_3d.cso", std::ios::binary);
    if (!ifs_ps) {
        MessageBox(nullptr, "shader_pixel_3d.cso が見つかりません", "error", MB_OK);
        return false;
    }

    ifs_ps.seekg(0, std::ios::end);
    filesize = ifs_ps.tellg();
    ifs_ps.seekg(0, std::ios::beg);

    unsigned char* psbinary_pointer = new unsigned char[filesize];
    ifs_ps.read((char*)psbinary_pointer, filesize);
    ifs_ps.close();

    // ピクセルシェーダーを作成
    hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

    // バッファ解放
    delete[] psbinary_pointer;

    if (FAILED(hr)) {
        hal::dout << "Shader3d_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
        return false;
    }

    // ピクセルシェーダ用 定数バッファ（色など用）作成
    buffer_desc.ByteWidth = sizeof(XMFLOAT4); // float4 1個分のサイズ
    g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);

    return true;
}

void Shader3d_Finalize()
{
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVSConstantBuffer2);
    SAFE_RELEASE(g_pPSConstantBuffer0);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pVertexShader);
}

// （旧）汎用行列セット関数。今は World 専用などを使う想定
void Shader3d_SetMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    // ワールド行列用の定数バッファを書き換え
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    // ビュー行列用の定数バッファを書き換え
    g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetProjectMatrix(const DirectX::XMMATRIX& matrix)
{
    XMFLOAT4X4 transpose;
    XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

    // プロジェクション行列用の定数バッファを書き換え
    g_pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetColor(const XMFLOAT4& color)
{
    // ピクセルシェーダ用の定数バッファに色をセット
    g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader3d_Begin()
{
    // 頂点シェーダーとピクセルシェーダーを描画パイプラインにセット
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // 入力レイアウトをセット
    g_pContext->IASetInputLayout(g_pInputLayout);

    // VS 用定数バッファをバインド
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
    g_pContext->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
    g_pContext->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);

    // PS 用定数バッファをバインド（色）
    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    // サンプラーステートを設定したい時はここで PSSetSamplers する
    // g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
}
