/*==============================================================================

   スコア管理 [Score.h]
														 Author : 51106
														 Date   : 2026/02/17
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef SCORE_H
#define SCORE_H

void Score_Initialize(float x, float y, int digit);

void Score_Finalize();

void Score_Draw();

unsigned int Score_GetScore();

void Score_Addscore(int score);

void Score_Reset();



#endif
