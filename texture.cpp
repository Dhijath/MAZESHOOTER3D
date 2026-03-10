/*==============================================================================

   テクスチャ管理 [texture.cpp]
														 Author : 51106
														 Date   : 2025/11/12
--------------------------------------------------------------------------------

   ・WIC対応（DirectXTex）で画像を読み込み、SRVとして管理
   ・同一ファイルの多重読み込みはスキップ（既存インデックスを返す）
   ・幅/高さの取得、個別解放、全解放、PSへのセットを提供

   【所有権メモ】
	 g_pDevice / g_pContext : 外部で生成・破棄（ここでは Release しない）
	 pTexture(SRV)          : 本モジュールが確保/解放を担当

==============================================================================*/

#include "texture.h"
#include "direct3d.h"
#include <string>

#include "DirectXTex.h"
using namespace DirectX;

//==============================================================================
// 定数・内部型
//==============================================================================

static constexpr int MAX_TEXTURES = 256; // 最大テクスチャ数（固定長管理）

// テクスチャ1枚分の管理情報
struct Texture
{
	std::wstring                 filename;  // ファイル名（重複ロード抑止に使用）
	ID3D11ShaderResourceView* pTexture;  // SRV（PSから参照）
	unsigned int                 width;     // 幅(px)
	unsigned int                 height;    // 高さ(px)
};

namespace
{
	Texture                 g_Textures[MAX_TEXTURES]{};                  // 管理配列（index = texid）
	unsigned int            g_SetTextureIndex = static_cast<unsigned int>(-1); // 現在PSにセット済みのtexid
	ID3D11Device* g_pDevice = nullptr;                        // 外部所有（Release不要）
	ID3D11DeviceContext* g_pContext = nullptr;                        // 外部所有（Release不要）
}

//------------------------------------------------------------------------------
// 初期化：管理配列クリア＆デバイス/コンテキストの保存
//------------------------------------------------------------------------------
void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	for (Texture& t : g_Textures)
	{
		t.pTexture = nullptr; // SRV未割り当て
		// t.width  = 0;
		// t.height = 0;
	}
	g_SetTextureIndex = static_cast<unsigned int>(-1);

	// 所有権は呼び出し側（AddRef/Releaseしない）
	g_pDevice = pDevice;
	g_pContext = pContext;
}

//------------------------------------------------------------------------------
// 終了：全テクスチャ解放（デバイス/コンテキストは触らない）
//------------------------------------------------------------------------------
void Texture_Finalize(void)
{
	Texture_AllRelease();
}

//------------------------------------------------------------------------------
// 読み込み：WIC対応（PNG/JPG/BMP等）。重複ロードは既存IDを返す。
// 戻り値：成功=texid / 失敗=-1
//------------------------------------------------------------------------------
int Texture_Load(const wchar_t* pFilename)
{
	// 既に読み込んでいるファイルは同一IDを返す
	for (int i = 0; i < MAX_TEXTURES; ++i)
	{
		if (g_Textures[i].filename == pFilename)
		{
			return i;
		}
	}

	// 空いている管理領域を探す
	for (int i = 0; i < MAX_TEXTURES; ++i)
	{
		if (g_Textures[i].pTexture) continue; // 使用中はスキップ

		TexMetadata  metadata{};
		ScratchImage image{};

		// 画像読み込み（sRGB運用なら WIC_FLAGS_FORCE_SRGB 等の検討余地あり）
		HRESULT hr = LoadFromWICFile(pFilename, WIC_FLAGS_NONE, &metadata, image);
		if (FAILED(hr))
		{
			// タイトル=エラー文面 / 本文=ファイル名 の順で通知
			MessageBoxW(nullptr, L"テクスチャの読み込みに失敗しました", pFilename, MB_OK | MB_ICONERROR);
			return -1;
		}

		g_Textures[i].filename = pFilename;
		g_Textures[i].width = static_cast<unsigned int>(metadata.width);
		g_Textures[i].height = static_cast<unsigned int>(metadata.height);



		if (metadata.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
			metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// SRV生成
		hr = DirectX::CreateShaderResourceView(
			g_pDevice,
			image.GetImages(),
			image.GetImageCount(),
			metadata,
			&g_Textures[i].pTexture
		);

		// ※元コードの挙動を維持：ここで失敗チェックせず i を返す。
		// 実際の堅牢化では FAILED(hr) 時にメンバをロールバックし -1 を返すのが安全。
		return i;
	}

	// 空きなし
	return -1;
}

//------------------------------------------------------------------------------
// 全テクスチャ解放
//------------------------------------------------------------------------------
void Texture_AllRelease()
{
	for (Texture& t : g_Textures)
	{
		t.filename.clear();
		SAFE_RELEASE(t.pTexture); // SRV解放
		// t.width = t.height = 0; // 必要なら復帰
	}
}

//------------------------------------------------------------------------------
// 個別解放：範囲外は無視
//------------------------------------------------------------------------------
void Texture_Release(int texid)
{
	if (texid < 0 || texid >= MAX_TEXTURES) return;

	SAFE_RELEASE(g_Textures[texid].pTexture);
	g_Textures[texid].filename.clear();
	g_Textures[texid].width = 0;
	g_Textures[texid].height = 0;

	// 現在PSにバインド中のIDだったら未設定へ
	if (g_SetTextureIndex == static_cast<unsigned int>(texid))
	{
		g_SetTextureIndex = static_cast<unsigned int>(-1);
	}
}

//------------------------------------------------------------------------------
// PSへセット：slot はシェーダ側の tN スロット番号HH
//------------------------------------------------------------------------------
void Set_Texture(int texid, int slot)
{
	if (texid < 0 || texid >= MAX_TEXTURES)
	{
		// 無効IDの場合はスロットをnullでクリアしてテクスチャ汚染を防ぐ
		ID3D11ShaderResourceView* nullSRV = nullptr;
		g_pContext->PSSetShaderResources(slot, 1, &nullSRV);
		g_SetTextureIndex = static_cast<unsigned int>(-1);
		return;
	}

	g_SetTextureIndex = static_cast<unsigned int>(texid);
	g_pContext->PSSetShaderResources(slot, 1, &g_Textures[texid].pTexture);
}

//------------------------------------------------------------------------------
// 幅/高さの取得（無効IDは0）
//------------------------------------------------------------------------------
unsigned int Texture_Width(int texid)
{
	if (texid < 0) return 0;
	return g_Textures[texid].width;
}

unsigned int Texture_Height(int texid)
{
	if (texid < 0) return 0;
	return g_Textures[texid].height;
}
