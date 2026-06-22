/*==============================================================================

   �V�[���h�V�X�e�� [shield.h]
                                                         Author : 51106
                                                         Date   : 2026/04/01
--------------------------------------------------------------------------------

   ���T�v
   �ELT �������Ńv���C���[����D10���ʑ̃V�[���h��W�J����
   �E�K�[�h���̓_���[�W�� 50% �y��
   �E��e���Ƀt���b�V�����o

   ���g�����iplayer.cpp ���j
     Shield_Initialize();
     Shield_Update(dt, ltPressed);          // ���t���[��
     Shield_Draw(playerCenterPos);          // �s�����`���ɌĂ�
     Shield_Finalize();

==============================================================================*/
#pragma once
#include <DirectXMath.h>

// �������iD3D ���\�[�X�����j
void  Shield_Initialize();

// �I�������iD3D ���\�[�X����j
void  Shield_Finalize();

// ���t���[���X�V
// �Eguarding : LT �� 0.5f �ȏ㉟����Ă��邩
void  Shield_Update(double dt, bool guarding);

// �`��i�s�����I�u�W�F�N�g�`���E�G�b�W�`���ɌĂԁj
// �Ecenter : �V�[���h�̒��S���W�i�v���C���[���ӂ�j
void  Shield_Draw(const DirectX::XMFLOAT3& center);

// �K�[�h����
bool  Shield_IsActive();

// �_���[�W��󂯂��Ƃ��ɌĂԁi�t���b�V�����o�j
void  Shield_NotifyHit();

// �_���[�W�y�����i0.5 = 50% �y���j
float Shield_GetDamageReduction();