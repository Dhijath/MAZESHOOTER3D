/*==============================================================================

   テクスチャの管理 [texture.h]
														 Author : 51106
														 Date   : 2026/02/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef TEXTURE_H
#define TEXTURE_H
#include <d3d11.h>



void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Texture_Finalize(void);


// テクスチャの読み込み
//戻り値：管理番号。読み込めなかった場合は-1
int Texture_Load(const wchar_t* pFilename);

void Texture_AllRelease();

void Set_Texture(int texid, int slot = 0);

unsigned int Texture_Width(int texid);
unsigned int Texture_Height(int texid);

void Texture_Release(int texid);



#endif // !TEXTURE_H
