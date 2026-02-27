/*==============================================================================

   メッシュフィールド描画 [MeshField.cpp]
														 Author : 51106
														 Date   : 2025/11/12
--------------------------------------------------------------------------------

   ・地面として使うメッシュフィールドを生成・描画するモジュール
   ・格子状の頂点データ＋インデックスを自前で作成
   ・2種類のテクスチャをレイヤー的に使用（Grass / stone）

==============================================================================*/

#include "Meshfield.h"
#include "direct3d.h"
#include "shader_field.h"
#include "texture.h"
#include "camera.h"
#include <ostream>
#include "debug_ostream.h"
#include "shader3d.h"

using namespace DirectX;

//====================================
// 3D頂点構造体
// position : 頂点座標
// normal   : 法線ベクトル
// color    : 頂点カラー
// uv       : テクスチャ座標
//====================================
struct Vertex3D
{
	XMFLOAT3 position; // 頂点座標
	XMFLOAT3 normal;   // 法線ベクトル
	XMFLOAT4 color;    // 頂点カラー
	XMFLOAT2 uv;       // UV座標
};


namespace
{
	// 1マスの大きさ（ワールド座標系での長さ）
	constexpr float FIELD_MESH_SIZE = 1.0f;

	// 横方向の分割数（四角形の数）
	constexpr int FIELD_MESH_H_COUNT = 50;

	// 縦方向の分割数（四角形の数）
	constexpr int FIELD_MESH_V_COUNT = 25;

	// 横方向の頂点数（分割数+1）
	constexpr int FIELD_MESH_H_VERTEX_COUNT = FIELD_MESH_H_COUNT + 1;

	// 縦方向の頂点数（分割数+1）
	constexpr int FIELD_MESH_V_VERTEX_COUNT = FIELD_MESH_V_COUNT + 1;

	// フィールド全体の頂点数
	constexpr int NUM_VERTEX = FIELD_MESH_H_VERTEX_COUNT * FIELD_MESH_V_VERTEX_COUNT;

	// フィールド全体のインデックス数
	// 1マス(四角形)につき三角形2枚 → 6インデックス
	constexpr int NUM_INDEX = FIELD_MESH_H_COUNT * FIELD_MESH_V_COUNT * 6;

	// 頂点データ配列
	Vertex3D g_MeshFieldVertex[NUM_VERTEX];

	// インデックスデータ配列
	unsigned short g_MeshFieldVertexIndex[NUM_INDEX];

	// 頂点バッファ
	ID3D11Buffer* g_pVertexBuffer = nullptr;

	// インデックスバッファ
	ID3D11Buffer* g_pIndexBuffer = nullptr;

	// 注意！初期化で外部から設定されるもの。Release不要。
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;

	// フィールド用テクスチャID
	// 0 : 草テクスチャ
	// 1 : 石テクスチャ
	int g_MeshFieldTexId0 = -1;
	int g_MeshFieldTexId1 = -1;

	// タイル用メッシュフィールド（1m×1m、細かい分割）
	constexpr int TILE_MESH_DIVISION = 8;  // 1辺の分割数
	constexpr int TILE_VERTEX_COUNT = (TILE_MESH_DIVISION + 1) * (TILE_MESH_DIVISION + 1);
	constexpr int TILE_INDEX_COUNT = TILE_MESH_DIVISION * TILE_MESH_DIVISION * 6;

	Vertex3D g_TileMeshVertex[TILE_VERTEX_COUNT];
	unsigned short g_TileMeshIndex[TILE_INDEX_COUNT];
	ID3D11Buffer* g_pTileVertexBuffer = nullptr;
	ID3D11Buffer* g_pTileIndexBuffer = nullptr;

	// 天井用メッシュフィールド（法線が下向き）
	Vertex3D g_CeilingMeshVertex[TILE_VERTEX_COUNT];
	unsigned short g_CeilingMeshIndex[TILE_INDEX_COUNT];
	ID3D11Buffer* g_pCeilingVertexBuffer = nullptr;
	ID3D11Buffer* g_pCeilingIndexBuffer = nullptr;
}

//====================================
// MeshField_Initialize
// メッシュフィールドの初期化
// ・頂点／インデックスデータ生成
// ・バッファ作成
// ・テクスチャ読み込み
// ・フィールドシェーダ初期化
//====================================

void MeshField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイス／コンテキストを保持
	g_pContext = pContext;
	g_pDevice = pDevice;

	//-------------------------
	// 頂点バッファの設定
	//-------------------------
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;                 // GPU専用（基本書き換えなし）
	bd.ByteWidth = sizeof(Vertex3D) * NUM_VERTEX;       // 頂点数分のサイズ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;            // 頂点バッファとして使う
	bd.CPUAccessFlags = 0;

	//-------------------------
	// 頂点配列の生成
	// 2重ループで格子状に頂点を配置
	//-------------------------
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; x++)
		{
			// 横 + 横の最大数 * 縦 → 1次元インデックスに変換
			int index = x + z * FIELD_MESH_H_VERTEX_COUNT;

			// ワールド座標
			g_MeshFieldVertex[index].position = {
				x * FIELD_MESH_SIZE,
				0.0f,                  // フィールドの高さは0固定
				z * FIELD_MESH_SIZE
			};

			// 頂点カラー（今は白で固定）
			g_MeshFieldVertex[index].color = { 1.0f,1.0f,1.0f,1.0f };

			constexpr float UV_TILE = 8.0f;

			g_MeshFieldVertex[index].uv = {
				x / UV_TILE,
				z / UV_TILE
			};


			// 法線ベクトル（Y+方向 → 上向き）
			g_MeshFieldVertex[index].normal = { 0.0f,1.0f,0.0f };
		}
	}

	//-------------------------
	// インデックスバッファの設定
	// 各マスを2つの三角形に分割
	//-------------------------
	int index;
	for (int z = 0; z < FIELD_MESH_V_COUNT; z++)
	{
		for (int x = 0; x < FIELD_MESH_H_COUNT; x++)
		{
			// このマス(四角形)のインデックス先頭位置
			index = (x + z * FIELD_MESH_H_COUNT) * 6;

			// 元々の分かりやすい書き方（コメントアウトされている方）
			// g_MeshFieldVertexIndex[index + 0] = x + 0 + (z + 0) * FIELD_MESH_H_VERTEX_COUNT;
			// g_MeshFieldVertexIndex[index + 1] = x + 1 + (z + 0) * FIELD_MESH_H_VERTEX_COUNT;
			// g_MeshFieldVertexIndex[index + 2] = x + 0 + (z + 1) * FIELD_MESH_H_VERTEX_COUNT;
			// g_MeshFieldVertexIndex[index + 3] = x + 1 + (z + 0) * FIELD_MESH_H_VERTEX_COUNT;
			// g_MeshFieldVertexIndex[index + 4] = x + 1 + (z + 1) * FIELD_MESH_H_VERTEX_COUNT;
			// g_MeshFieldVertexIndex[index + 5] = x + 0 + (z + 1) * FIELD_MESH_H_VERTEX_COUNT;

			// 少し書き方を工夫した版
			g_MeshFieldVertexIndex[index + 0] = x + (z + 0) * FIELD_MESH_H_VERTEX_COUNT;      // 左上
			g_MeshFieldVertexIndex[index + 1] = x + (z + 1) * FIELD_MESH_H_VERTEX_COUNT + 1;  // 右下
			g_MeshFieldVertexIndex[index + 2] = g_MeshFieldVertexIndex[index + 0] + 1;        // 右上

			g_MeshFieldVertexIndex[index + 3] = g_MeshFieldVertexIndex[index + 0];            // 左上
			g_MeshFieldVertexIndex[index + 4] = g_MeshFieldVertexIndex[index + 1] - 1;        // 左下
			g_MeshFieldVertexIndex[index + 5] = g_MeshFieldVertexIndex[index + 1];            // 右下

			index += 6;
		}
	}

	//-------------------------
	// 頂点バッファの生成
	//-------------------------
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_MeshFieldVertex; // 初期データとして頂点配列を渡す
	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	//-------------------------
	// インデックスバッファの生成
	//-------------------------
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	sd.pSysMem = g_MeshFieldVertexIndex;
	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

	//-------------------------
	// テクスチャ読み込み
	//-------------------------
	g_MeshFieldTexId0 = Texture_Load(L"resource/texture/glassground.png");
	g_MeshFieldTexId1 = Texture_Load(L"resource/texture/sandground.png");

	hal::dout << "MeshField tex0=" << g_MeshFieldTexId0
		<< " tex1=" << g_MeshFieldTexId1 << std::endl;

	Shader_field_Initialize(pDevice, pContext);

	//-------------------------
	// タイル用メッシュフィールドの生成（1m×1m）
	//-------------------------
	{
		const float tileSize = 1.0f;
		const float cellSize = tileSize / static_cast<float>(TILE_MESH_DIVISION);

		// 頂点生成
		for (int z = 0; z <= TILE_MESH_DIVISION; z++)
		{
			for (int x = 0; x <= TILE_MESH_DIVISION; x++)
			{
				const int idx = x + z * (TILE_MESH_DIVISION + 1);

				// 中心原点で-0.5～+0.5の範囲に配置
				g_TileMeshVertex[idx].position = {
					(x * cellSize) - tileSize * 0.5f,
					0.0f,
					(z * cellSize) - tileSize * 0.5f
				};

				g_TileMeshVertex[idx].color = { 1.0f, 1.0f, 1.0f, 1.0f };

				// UV座標（1タイル分）
				g_TileMeshVertex[idx].uv = {
					static_cast<float>(x) / static_cast<float>(TILE_MESH_DIVISION),
					static_cast<float>(z) / static_cast<float>(TILE_MESH_DIVISION)
				};

				g_TileMeshVertex[idx].normal = { 0.0f, 1.0f, 0.0f };
			}
		}

		// インデックス生成
		int idxPos = 0;
		for (int z = 0; z < TILE_MESH_DIVISION; z++)
		{
			for (int x = 0; x < TILE_MESH_DIVISION; x++)
			{
				const int base = x + z * (TILE_MESH_DIVISION + 1);

				g_TileMeshIndex[idxPos + 0] = base;
				g_TileMeshIndex[idxPos + 1] = base + (TILE_MESH_DIVISION + 1) + 1;
				g_TileMeshIndex[idxPos + 2] = base + 1;

				g_TileMeshIndex[idxPos + 3] = base;
				g_TileMeshIndex[idxPos + 4] = base + (TILE_MESH_DIVISION + 1);
				g_TileMeshIndex[idxPos + 5] = base + (TILE_MESH_DIVISION + 1) + 1;

				idxPos += 6;
			}
		}

		// 頂点バッファ作成
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(Vertex3D) * TILE_VERTEX_COUNT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		sd.pSysMem = g_TileMeshVertex;
		g_pDevice->CreateBuffer(&bd, &sd, &g_pTileVertexBuffer);

		// インデックスバッファ作成
		bd.ByteWidth = sizeof(unsigned short) * TILE_INDEX_COUNT;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

		sd.pSysMem = g_TileMeshIndex;
		g_pDevice->CreateBuffer(&bd, &sd, &g_pTileIndexBuffer);
	}


	//-------------------------
// 天井用メッシュフィールドの生成（法線下向き）
//-------------------------
	{
		const float tileSize = 1.0f;
		const float cellSize = tileSize / static_cast<float>(TILE_MESH_DIVISION);

		// 頂点生成（法線以外は床と同じ）
		for (int z = 0; z <= TILE_MESH_DIVISION; z++)
		{
			for (int x = 0; x <= TILE_MESH_DIVISION; x++)
			{
				const int idx = x + z * (TILE_MESH_DIVISION + 1);

				g_CeilingMeshVertex[idx].position = {
					(x * cellSize) - tileSize * 0.5f,
					0.0f,
					(z * cellSize) - tileSize * 0.5f
				};

				g_CeilingMeshVertex[idx].color = { 1.0f, 1.0f, 1.0f, 1.0f };

				g_CeilingMeshVertex[idx].uv = {
					static_cast<float>(x) / static_cast<float>(TILE_MESH_DIVISION),
					static_cast<float>(z) / static_cast<float>(TILE_MESH_DIVISION)
				};

				g_CeilingMeshVertex[idx].normal = { 0.0f, -1.0f, 0.0f };  // ← 下向き！
			}
		}

		// インデックス生成（巻き順を逆にする）
		int idxPos = 0;
		for (int z = 0; z < TILE_MESH_DIVISION; z++)
		{
			for (int x = 0; x < TILE_MESH_DIVISION; x++)
			{
				const int base = x + z * (TILE_MESH_DIVISION + 1);

				// 床と逆の巻き順
				g_CeilingMeshIndex[idxPos + 0] = base;
				g_CeilingMeshIndex[idxPos + 1] = base + 1;
				g_CeilingMeshIndex[idxPos + 2] = base + (TILE_MESH_DIVISION + 1) + 1;

				g_CeilingMeshIndex[idxPos + 3] = base;
				g_CeilingMeshIndex[idxPos + 4] = base + (TILE_MESH_DIVISION + 1) + 1;
				g_CeilingMeshIndex[idxPos + 5] = base + (TILE_MESH_DIVISION + 1);

				idxPos += 6;
			}
		}

		// バッファ作成
		bd.ByteWidth = sizeof(Vertex3D) * TILE_VERTEX_COUNT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		sd.pSysMem = g_CeilingMeshVertex;
		g_pDevice->CreateBuffer(&bd, &sd, &g_pCeilingVertexBuffer);

		bd.ByteWidth = sizeof(unsigned short) * TILE_INDEX_COUNT;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		sd.pSysMem = g_CeilingMeshIndex;
		g_pDevice->CreateBuffer(&bd, &sd, &g_pCeilingIndexBuffer);
	}


}

//====================================
// MeshField_Finalize
// フィールドの終了処理
// ・シェーダ解放
// ・頂点／インデックスバッファ解放
//====================================
void MeshField_Finalize()
{
	Shader_field_Finalize();
	SAFE_RELEASE(g_pTileIndexBuffer);
	SAFE_RELEASE(g_pTileVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pCeilingIndexBuffer);
	SAFE_RELEASE(g_pCeilingVertexBuffer);
}

//====================================
// MeshField_Draw
// フィールド描画
// mtxW : 本来ワールド行列を受け取るが
//        現状は内部で平行移動を指定している
//====================================

void MeshField_Draw(const DirectX::XMMATRIX& mtxW)
{
	Shader_field_Begin();

	// 草＆石テクスチャをバインド（関数名は実際のやつに合わせて）
	Set_Texture(g_MeshFieldTexId0, 0); // tex0 : register(t0)
	//Set_Texture(g_MeshFieldTexId1, 1); // tex1 : register(t1)

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	float offset_x = FIELD_MESH_H_COUNT * FIELD_MESH_SIZE * 0.5f;
	float offset_z = FIELD_MESH_V_COUNT * FIELD_MESH_SIZE * 0.5f;
	Shader_field_SetWorldMatrix(mtxW * XMMatrixTranslation(-offset_x, 0.0f, -offset_z));


	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// テクスチャ色をそのまま使いたいなら白に
	//Shader_field_3D_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

//====================================
// MeshField_DrawTile
// タイル1個分のメッシュフィールド描画
// mtxW     : ワールド行列（位置・回転・拡縮）
// texID    : 使用するテクスチャID
// tileSize : タイルサイズ（メートル）
//====================================
void MeshField_DrawTile(const DirectX::XMMATRIX& mtxW, int texID, float tileSize)
{
	// Shader_field_Begin() を呼ばない（呼び出し側で Shader3d_Begin() 済みを想定）

	Set_Texture(texID, 0);

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pTileVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pTileIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	const XMMATRIX scale = XMMatrixScaling(tileSize, 1.0f, tileSize);

	// Shader_field_SetWorldMatrix ではなく Shader3d_SetWorldMatrix を使う
	Shader3d_SetWorldMatrix(scale * mtxW);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pContext->DrawIndexed(TILE_INDEX_COUNT, 0, 0);
}

void MeshField_DrawTileCeiling(const DirectX::XMMATRIX& mtxW, int texID, float tileSize)
{
	Set_Texture(texID, 0);

	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pCeilingVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pCeilingIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	const XMMATRIX scale = XMMatrixScaling(tileSize, 1.0f, tileSize);
	Shader3d_SetWorldMatrix(scale * mtxW);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pContext->DrawIndexed(TILE_INDEX_COUNT, 0, 0);
}
