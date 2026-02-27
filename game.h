#ifndef GAME_H
#define GAME_H

void Game_Initialize();

void Game_Finalize();

void Game_Update(double elapsed_time);

void Game_Draw();

void MiniMap_Render3D();

void MiniMap_Draw2D();


#endif // !GAME_H

