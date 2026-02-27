/*==============================================================================

   3Dアニメーション描画用シェーダー [shader3d_anim.cpp]
                                                         Author : 51106
                                                         Date   : 2025/01/27
--------------------------------------------------------------------------------
   ・スケルタルアニメーション対応の3D描画シェーダー
   ・頂点シェーダーでボーン行列によるスキニング計算を実行
   ・ピクセルシェーダーは既存のshader_pixel_3d.hlslを使用

==============================================================================*/
#include "shader3d_anim.h"
#include "direct3d.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

//=============================================================================
// グローバル変数
//=============================================================================
namespace {
    // シェーダーオブジェクト
    ID3D11VertexShader* g_VertexShader = nullptr;
    ID3D11PixelShader* g_PixelShader = nullptr;
    ID3D11InputLayout* g_InputLayout = nullptr;

    // 定数バッファ
    ID3D11Buffer* g_WorldBuffer = nullptr;      // b0: ワールド行列
    ID3D11Buffer* g_ViewBuffer = nullptr;       // b1: ビュー行列
    ID3D11Buffer* g_ProjBuffer = nullptr;       // b2: プロジェクション行列
    ID3D11Buffer* g_BoneBuffer = nullptr;       // b3: ボーン行列配列
    ID3D11Buffer* g_DiffuseBuffer = nullptr;    // b0(PS): ディフューズ色

    // 現在の設定値
    DirectX::XMMATRIX g_WorldMatrix;
    DirectX::XMMATRIX g_ViewMatrix;
    DirectX::XMMATRIX g_ProjectionMatrix;
    DirectX::XMFLOAT4 g_DiffuseColor;
}

//=============================================================================
// 初期化
//=============================================================================
void Shader3dAnim_Initialize(void)
{
    ID3D11Device* device = Direct3D_GetDevice();
    HRESULT hr;

    //-------------------------------------------------------------------------
    // 頂点シェーダーのコンパイルと作成
    //-------------------------------------------------------------------------
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    hr = D3DCompileFromFile(
        L"shader_vertex_3d_anim.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &vsBlob,
        &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return;
    }

    hr = device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &g_VertexShader);

    if (FAILED(hr))
    {
        vsBlob->Release();
        return;
    }

    //-------------------------------------------------------------------------
    // 入力レイアウトの作成（ボーン情報を含む）
    //-------------------------------------------------------------------------
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &g_InputLayout);

    vsBlob->Release();

    if (FAILED(hr))
    {
        return;
    }

    //-------------------------------------------------------------------------
    // ピクセルシェーダーのコンパイルと作成（既存のものを使用）
    //-------------------------------------------------------------------------
    ID3DBlob* psBlob = nullptr;

    hr = D3DCompileFromFile(
        L"shader_pixel_3d.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &psBlob,
        &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return;
    }

    hr = device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &g_PixelShader);

    psBlob->Release();

    if (FAILED(hr))
    {
        return;
    }

    //-------------------------------------------------------------------------
    // 定数バッファの作成
    //-------------------------------------------------------------------------
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    // ワールド行列バッファ (VS b0)
    bd.ByteWidth = sizeof(DirectX::XMMATRIX);
    hr = device->CreateBuffer(&bd, nullptr, &g_WorldBuffer);
    if (FAILED(hr)) return;

    // ビュー行列バッファ (VS b1)
    bd.ByteWidth = sizeof(DirectX::XMMATRIX);
    hr = device->CreateBuffer(&bd, nullptr, &g_ViewBuffer);
    if (FAILED(hr)) return;

    // プロジェクション行列バッファ (VS b2)
    bd.ByteWidth = sizeof(DirectX::XMMATRIX);
    hr = device->CreateBuffer(&bd, nullptr, &g_ProjBuffer);
    if (FAILED(hr)) return;

    // ボーン行列バッファ (VS b3) - 256個の行列
    bd.ByteWidth = sizeof(DirectX::XMMATRIX) * 256;
    hr = device->CreateBuffer(&bd, nullptr, &g_BoneBuffer);
    if (FAILED(hr)) return;

    // ディフューズ色バッファ (PS b0)
    bd.ByteWidth = sizeof(DirectX::XMFLOAT4);
    hr = device->CreateBuffer(&bd, nullptr, &g_DiffuseBuffer);
    if (FAILED(hr)) return;

    //-------------------------------------------------------------------------
    // 初期値設定
    //-------------------------------------------------------------------------
    g_WorldMatrix = DirectX::XMMatrixIdentity();
    g_ViewMatrix = DirectX::XMMatrixIdentity();
    g_ProjectionMatrix = DirectX::XMMatrixIdentity();
    g_DiffuseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

//=============================================================================
// 終了処理
//=============================================================================
void Shader3dAnim_Finalize(void)
{
    // 定数バッファの解放
    if (g_DiffuseBuffer)
    {
        g_DiffuseBuffer->Release();
        g_DiffuseBuffer = nullptr;
    }

    if (g_BoneBuffer)
    {
        g_BoneBuffer->Release();
        g_BoneBuffer = nullptr;
    }

    if (g_ProjBuffer)
    {
        g_ProjBuffer->Release();
        g_ProjBuffer = nullptr;
    }

    if (g_ViewBuffer)
    {
        g_ViewBuffer->Release();
        g_ViewBuffer = nullptr;
    }

    if (g_WorldBuffer)
    {
        g_WorldBuffer->Release();
        g_WorldBuffer = nullptr;
    }

    // シェーダーオブジェクトの解放
    if (g_InputLayout)
    {
        g_InputLayout->Release();
        g_InputLayout = nullptr;
    }

    if (g_PixelShader)
    {
        g_PixelShader->Release();
        g_PixelShader = nullptr;
    }

    if (g_VertexShader)
    {
        g_VertexShader->Release();
        g_VertexShader = nullptr;
    }
}

//=============================================================================
// 描画開始
//=============================================================================
void Shader3dAnim_Begin(void)
{
    ID3D11DeviceContext* context = Direct3D_GetContext();

    // シェーダーをパイプラインにセット
    context->VSSetShader(g_VertexShader, nullptr, 0);
    context->PSSetShader(g_PixelShader, nullptr, 0);
    context->IASetInputLayout(g_InputLayout);

    // 定数バッファを各スロットにセット
    context->VSSetConstantBuffers(0, 1, &g_WorldBuffer);
    context->VSSetConstantBuffers(1, 1, &g_ViewBuffer);
    context->VSSetConstantBuffers(2, 1, &g_ProjBuffer);
    context->VSSetConstantBuffers(3, 1, &g_BoneBuffer);

    context->PSSetConstantBuffers(0, 1, &g_DiffuseBuffer);
}

//=============================================================================
// ワールド行列設定
//=============================================================================
void Shader3dAnim_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    g_WorldMatrix = matrix;

    // 転置してGPUに送る（DirectXMathは行優先、HLSLは列優先）
    DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(matrix);

    ID3D11DeviceContext* context = Direct3D_GetContext();
    context->UpdateSubresource(g_WorldBuffer, 0, nullptr, &transposed, 0, 0);
}

//=============================================================================
// ビュー行列設定
//=============================================================================
void Shader3dAnim_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
    g_ViewMatrix = matrix;

    DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(matrix);

    ID3D11DeviceContext* context = Direct3D_GetContext();
    context->UpdateSubresource(g_ViewBuffer, 0, nullptr, &transposed, 0, 0);
}

//=============================================================================
// プロジェクション行列設定
//=============================================================================
void Shader3dAnim_SetProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
    g_ProjectionMatrix = matrix;

    DirectX::XMMATRIX transposed = DirectX::XMMatrixTranspose(matrix);

    ID3D11DeviceContext* context = Direct3D_GetContext();
    context->UpdateSubresource(g_ProjBuffer, 0, nullptr, &transposed, 0, 0);
}

//=============================================================================
// ボーン行列設定
//=============================================================================
void Shader3dAnim_SetBoneMatrices(const DirectX::XMMATRIX* matrices, unsigned int count)
{
    if (count > 256)
    {
        count = 256;  // 最大256個に制限
    }

    // ボーン行列を転置して送る
    DirectX::XMMATRIX transposedBones[256];
    for (unsigned int i = 0; i < count; i++)
    {
        transposedBones[i] = DirectX::XMMatrixTranspose(matrices[i]);
    }

    // 残りは単位行列で埋める
    for (unsigned int i = count; i < 256; i++)
    {
        transposedBones[i] = DirectX::XMMatrixIdentity();
    }

    ID3D11DeviceContext* context = Direct3D_GetContext();
    context->UpdateSubresource(g_BoneBuffer, 0, nullptr, transposedBones, 0, 0);
}

//=============================================================================
// ディフューズ色設定
//=============================================================================
void Shader3dAnim_SetColor(const DirectX::XMFLOAT4& color)
{
    g_DiffuseColor = color;

    ID3D11DeviceContext* context = Direct3D_GetContext();
    context->UpdateSubresource(g_DiffuseBuffer, 0, nullptr, &color, 0, 0);
}