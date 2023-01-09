#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}


#define SCREEN_WIDTH	480
#define SCREEN_HEIGHT	320


#define STRING_BUFFER_SIZE 128



//////////////////////////////////////////////////////////////////////////////////////
// GAMEPLAY CONSTANTS

#define MAX_ENEMIES 16
#define CAR_SIZE {14, 20}
#define PLAYER_Y_POS 240

// player stats
#define PLAYER_MAX_SPEED 1000
#define PLAYER_MAX_SPEED_SIDES 400
#define PLAYER_ACCEL 1000
#define PLAYER_ACCEL_SIDES 3000




// struct used to describe 2D positions, offsets & vectors
struct Vector2
{
	float x;
	float y;
};

struct Time
{
	int t1, t2, frames;
	double time;
	double delta; 
	double fps;
	double fpsTimer;
};

struct Input
{
	bool quit;
	bool up;
	bool down;
	bool left;
	bool right;
};



//////////////////////////////////////////////////////////////////////////////////////
// UTILITY

// returns the closes value to num that fits inside the r1-r2 range
float Clamp(float num, float r1, float r2)
{
	if (num < r1)
		return r1;
	if (num > r2)
		return r2;
	return num;
}

// Moves the value of num towards target by delta
float MoveTowards(float num, float target, float delta)
{
	if (fabsf(num - target) <= delta)
		return target;
	if (num > target)
		return num - delta;
	if (num < target)
		return num + delta;
}




//////////////////////////////////////////////////////////////////////////////////////
// RENDERING

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset)
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text)
	{
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	}
}

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y)
{
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
}

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
	for (int i = 0; i < l; i++)
	{
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	}
}

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor)
{
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}





//////////////////////////////////////////////////////////////////////////////////////
// LOADING IMAGES

bool InitialiseSDL(SDL_Window** window, SDL_Renderer** renderer, SDL_Surface** screen, SDL_Texture** scrtex)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL_Init error: %s\n", SDL_GetError());
		return false;
	}

	// fullscreen mode
	//int rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, window, renderer);
	int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer);
	if (rc != 0)
	{
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(*window, "Filip Jezierski 196333");


	*screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	*scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	SDL_ShowCursor(SDL_DISABLE);

	return true;
}

// load a single sprite
// returns true when successful
bool LoadBitmap(SDL_Surface** surface, const char* filename)
{
	*surface = SDL_LoadBMP(filename);
	if (*surface == NULL)
	{
		printf("SDL_LoadBMP(%s) error: %s\n", filename, SDL_GetError());
		return false;
	}
	printf("SDL_LoadBMP(%s) bitmap loaded successfully\n", filename);
	return true;
}

// indexes of all bitmaps
// last element in the enum is used to get the number of other elements
enum BitmapData
{
	BMP_CHARSET,
	BMP_PLAYER_CAR,
	BMP_ENEMY_CAR,
	BMP_CIVILIAN_CAR,
	BMP_BACKGROUND,
	BMP_COUNT
};

// load all sprites
// returns true when successful
bool LoadAllBitmaps(SDL_Surface** bmps)
{
	bool error = false;

	error |= !LoadBitmap(&bmps[BMP_CHARSET], "./sprites/cs8x8.bmp");
	error |= !LoadBitmap(&bmps[BMP_PLAYER_CAR], "./sprites/player_car.bmp");
	error |= !LoadBitmap(&bmps[BMP_ENEMY_CAR], "./sprites/enemy_car.bmp");
	error |= !LoadBitmap(&bmps[BMP_CIVILIAN_CAR], "./sprites/civilian_car.bmp");
	error |= !LoadBitmap(&bmps[BMP_BACKGROUND], "./sprites/grass.bmp");

	if (error)
		return false;

	SDL_SetColorKey(bmps[BMP_CHARSET], true, 0x000000);
	return true;
}





//////////////////////////////////////////////////////////////////////////////////////
// MEMORY MANAGEMENT

void FreeBitmaps(SDL_Surface** bmps)
{
	for (int i = 0; i < BMP_COUNT; i++)
	{
		SDL_FreeSurface(bmps[i]);
	}
}





//////////////////////////////////////////////////////////////////////////////////////
// GAME OBJECTS

struct GameData;

class GameObject
{
public:
	Vector2 position = {};
	Vector2 size = {};
	SDL_Surface* sprite;
	
	virtual void Draw(SDL_Surface* screen)
	{
		if (sprite == NULL)
		{
			printf("Error while drawing GameObject: sprite is NULL\n");
			return;
		}
		DrawSurface(screen, sprite, position.x, position.y);
	}
};

class Player : public GameObject
{
public:
	Vector2 speed = {};
};

class Enemy : public GameObject
{
public:
	Vector2 speed = {};
};

struct GameData
{
	// game objects
	Player* player;
	int enemyCount;
	Enemy* enemies[MAX_ENEMIES];

	GameObject* background;
};





//////////////////////////////////////////////////////////////////////////////////////
// GAME MECHANICS

// accelerate & decelerate the player
void PlayerSteering(Player* player, Time time, Input* input)
{
	Vector2 steering = { 0,0 };

	if (input->up) steering.y--;
	if (input->down) steering.y++;
	if (input->left) steering.x--;
	if (input->right) steering.x++;

	player->speed.y += steering.y * time.delta * PLAYER_ACCEL;

	if (steering.x == 0)
		player->speed.x = MoveTowards(player->speed.x, 0, time.delta * PLAYER_ACCEL_SIDES);
	else
		player->speed.x += steering.x * time.delta * PLAYER_ACCEL_SIDES;

	player->speed.x = Clamp(player->speed.x, -PLAYER_MAX_SPEED_SIDES, PLAYER_MAX_SPEED_SIDES);
	player->speed.y = Clamp(player->speed.y, -PLAYER_MAX_SPEED, 0);

	player->position.x += player->speed.x * time.delta;

}

void MoveBackground(GameObject* background, float playerSpeed, Time time)
{
	background->position.y -= playerSpeed * (float)time.delta;
	if (background->position.y > SCREEN_HEIGHT)
		background->position.y = 0;
}





void CreateEnemy(GameData* gameData, Vector2 pos)
{
	if (gameData->enemyCount >= MAX_ENEMIES)
	{
		printf("Couldn't create a new enemy - max enemy count reached\n");

		return;
	}

	Enemy* enemy = new Enemy();
	enemy->position = pos;
	gameData->enemies[gameData->enemyCount] = enemy;
	gameData->enemyCount++;
}

void DeleteEnemy(GameData* gameData, Enemy* enemy)
{
	gameData->enemyCount--;

	// if the deleted enemy isn't the last one in the array
	// move the last object into it's place
	// like this:
	// ##D##L
	//     / 
	//    /  
	// ##L## 
	int index = &enemy - gameData->enemies;
	if (index + 2 != gameData->enemyCount)
	{
		gameData->enemies[index] = gameData->enemies[gameData->enemyCount];
	}
	delete enemy;

	printf("Enemy %d destroyed successfully\n", index);
}


// create all necessary GameObjects
void GameStart(GameData* gameData, SDL_Surface** bitmaps)
{
	Player* player = new Player();
	player->sprite = bitmaps[BMP_PLAYER_CAR];
	player->position = {SCREEN_WIDTH / 2, PLAYER_Y_POS};
	player->size = CAR_SIZE;
	gameData->player = player;

	GameObject* background = new GameObject();
	background->sprite = bitmaps[BMP_BACKGROUND];
	background->position = { SCREEN_WIDTH / 2, 0 };
	gameData->background = background;
}

void GameUpdate(Time time, GameData* gameData, Input* input)
{
	PlayerSteering(gameData->player, time, input);

	MoveBackground(gameData->background, gameData->player->speed.y, time);
}

void UpdateInputs(Input* input, SDL_Event event)
{
	switch (event.type)
	{
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_ESCAPE) input->quit = true;
		else if (event.key.keysym.sym == SDLK_UP) input->up = true;
		else if (event.key.keysym.sym == SDLK_DOWN) input->down = true;
		else if (event.key.keysym.sym == SDLK_LEFT) input->left = true;
		else if (event.key.keysym.sym == SDLK_RIGHT) input->right = true;
		break;
	case SDL_KEYUP:
		if (event.key.keysym.sym == SDLK_UP) input->up = false;
		else if (event.key.keysym.sym == SDLK_DOWN) input->down = false;
		else if (event.key.keysym.sym == SDLK_LEFT) input->left = false;
		else if (event.key.keysym.sym == SDLK_RIGHT) input->right = false;
		break;
	case SDL_QUIT:
		input->quit = true;
		break;
	}
}





//////////////////////////////////////////////////////////////////////////////////////
// GAME VISUALS

void DrawGameObjects(SDL_Surface* screen, GameData* gameData)
{
	gameData->background->Draw(screen);

	gameData->player->Draw(screen);

	for (int i = 0; i < gameData->enemyCount; i++)
	{
		gameData->enemies[i]->Draw(screen);
	}
}

void DrawDebugInfo(SDL_Surface* screen, Time* time, SDL_Surface* charset, char* stringBuffer)
{
	//DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, blue);
	sprintf(stringBuffer, "Elapsed time: %.1lfs, FPS: %.0lf ", time->time, time->fps);
	DrawString(screen, screen->w / 2 - strlen(stringBuffer) * 8 / 2, 10, stringBuffer, charset);
	sprintf(stringBuffer, "Esc - exit, \030 - faster, \031 - slower");
	DrawString(screen, screen->w / 2 - strlen(stringBuffer) * 8 / 2, 26, stringBuffer, charset);

}





//////////////////////////////////////////////////////////////////////////////////////
// MAIN

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
	printf("printf output goes here:\n");

	char stringBuffer[STRING_BUFFER_SIZE] = {};
	int quit = 0;
	Time time = {};
	Input input = {};

	SDL_Surface* bitmaps[BMP_COUNT];


	SDL_Event event;
	SDL_Surface* screen = NULL;
	SDL_Texture* scrtex = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	if (!InitialiseSDL(&window, &renderer, &screen, &scrtex))
		return 1;

	if (!LoadAllBitmaps(bitmaps))
	{
		FreeBitmaps(bitmaps);
		return 1;
	}

	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	time.t1 = SDL_GetTicks();


	GameData gameData;

	GameStart(&gameData, bitmaps);

	while (!quit)
	{
		time.t2 = SDL_GetTicks();

		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		time.delta = (time.t2 - time.t1) * 0.001;
		time.t1 = time.t2;

		time.time += time.delta;

		SDL_FillRect(screen, NULL, black);


		time.fpsTimer += time.delta;
		if (time.fpsTimer > 0.5)
		{
			time.fps = time.frames * 2;
			time.frames = 0;
			time.fpsTimer -= 0.5;
		}


		GameUpdate(time, &gameData, &input);

		DrawGameObjects(screen, &gameData);

		DrawDebugInfo(screen, &time, bitmaps[BMP_CHARSET], stringBuffer);



		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		// SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// handling of events (if there were any)
		while (SDL_PollEvent(&event))
		{
			UpdateInputs(&input, event);
		}

		if (input.quit)
		{
			quit = 1;
		}
		time.frames++;
	}

	// freeing all surfaces
	FreeBitmaps(bitmaps);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}
