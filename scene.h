/*==============================================================================

   シーン管理 [Scene.h]
														 Author : 51106
														 Date   : 2026/02/17
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SCENE_H
#define SCENE_H

void Scene_Initialize();

void Scene_Finalize();

void Scene_Update();

void Scene_Draw();

void Scene_Refresh();


enum Scene
{

	SCENE_TITLE,
	SCENE_GAME,
	SCENE_RESULT,
	SCENE_MAX

};

void Scene_Change(Scene Scene);

#endif // !SCENE_H

