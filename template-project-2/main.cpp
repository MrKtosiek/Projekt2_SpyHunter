#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}


#define FULLSCREEN false
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define FPS_COUNTER_INTERVAL 0.1
#define RAND_VAL_PRECISION 100

#define STRING_BUFFER_SIZE 128


//////////////////////////////////////////////////////////////////////////////////////
// GAMEPLAY CONSTANTS

#define MAX_ENEMIES 16
#define CAR_SIZE { 14, 20 }
#define PLAYER_Y_POS 240

// value between 0 and 1
#define COLLISION_BOUNCE 1

// player stats
#define PLAYER_MAX_SPEED 1000
#define PLAYER_MIN_SPEED 400
#define PLAYER_MAX_SPEED_SIDES 400
#define PLAYER_ACCEL 800
#define PLAYER_ACCEL_SIDES 2000

// enemy stats
#define ENEMY_TARGET_DISTANCE 50
#define ENEMY_MAX_SPEED 800
#define ENEMY_MIN_SPEED 300
#define ENEMY_MAX_SPEED_SIDES 400
#define ENEMY_ACCEL 400
#define ENEMY_ACCEL_SIDES 1000

// enemies further than this from the center of the screen (behind the player) are deleted
#define ENEMY_ACTIVE_DISTANCE SCREEN_HEIGHT * 3



// struct used to describe 2D positions, offsets & vectors
struct Vector2
{
	double x;
	double y;
};

struct Time
{
	long timeCounterCurrent, timeCounterPrevious;
	double time;
	double delta;

	// these variables are used for calculating the FPS
	int frames;
	double fps;
	double fpsTimer;
};

struct Input
{
	bool quit;
	bool pause;
	bool newGame;
	bool up;
	bool down;
	bool left;
	bool right;
	bool showDebug;
};



//////////////////////////////////////////////////////////////////////////////////////
// UTILITY

// returns the closes value to num that fits inside the r1-r2 range
double Clamp(double num, double r1, double r2)
{
	if (num < r1)
		return r1;
	if (num > r2)
		return r2;
	return num;
}

// Moves the value of num towards target by delta
void MoveTowards(double* num, double target, double delta)
{
	if (fabs(*num - target) <= delta)
		*num = target;
	if (*num > target)
		*num -= delta;
	if (*num < target)
		*num += delta;
}

// returns:
// 1 for positive numbers
// -1 for negative numbers
// 0 for 0
int Sign(double num)
{
	if (num > 0) return 1;
	if (num < 0) return -1;
	return 0;
}

// returns a random value in the range 0-1
double RandVal()
{
	return (double)(rand() % RAND_VAL_PRECISION) / RAND_VAL_PRECISION;
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

	int error = -1;

	if (FULLSCREEN)
		error = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, window, renderer);
	else
		error = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer);

	if (error != 0)
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
	SDL_Surface* sprite = NULL;
	
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

class Car : public GameObject
{
public:
	Vector2 speed = {};
};

class Player : public Car
{
public:
	int score = 0;
};

class Enemy : public Car
{
public:
	int health = 0;
};

struct GameData
{
	// game objects
	Player* player = NULL;
	int enemyCount = 0;
	Enemy* enemies[MAX_ENEMIES] = {};

	GameObject* background = NULL;
};





//////////////////////////////////////////////////////////////////////////////////////
// GAME MECHANICS

double CalculateMaxSideSpeed(double forwardSpeed, double maxForwardSpeed, double maxSideSpeed)
{
	return (forwardSpeed / maxForwardSpeed) * maxSideSpeed;
}

// accelerate & decelerate the player
void PlayerSteering(Player* player, Time time, Input* input)
{
	Vector2 steering = { 0,0 };

	if (input->up) steering.y--;
	if (input->down) steering.y++;
	if (input->left) steering.x--;
	if (input->right) steering.x++;

	player->speed.y += steering.y * time.delta * PLAYER_ACCEL;

	MoveTowards(&player->speed.x, 
		steering.x * CalculateMaxSideSpeed(-player->speed.y, PLAYER_MAX_SPEED, PLAYER_MAX_SPEED_SIDES),
		time.delta * PLAYER_ACCEL_SIDES);

	player->speed.y = Clamp(player->speed.y, -PLAYER_MAX_SPEED, -PLAYER_MIN_SPEED);

	player->position.x += player->speed.x * time.delta;

}

void MoveBackground(GameObject* background, double playerSpeed, Time time)
{
	background->position.y -= playerSpeed * (double)time.delta;
	if (background->position.y > SCREEN_HEIGHT)
		background->position.y = 0;
}

void UpdateEnemy(Enemy* enemy, Player* player, Time time)
{
	if (fabs(enemy->position.y - player->position.y) < ENEMY_TARGET_DISTANCE)
	{
		// match the players speed
		MoveTowards(&enemy->speed.y, player->speed.y - (enemy->position.y - player->position.y), time.delta * ENEMY_ACCEL);

		// try to push the player off the road
		MoveTowards(&enemy->speed.x, 
			Sign(player->position.x - enemy->position.x) * CalculateMaxSideSpeed(-enemy->speed.y, ENEMY_MAX_SPEED, ENEMY_MAX_SPEED_SIDES), 
			time.delta * ENEMY_ACCEL_SIDES);
	}
	else
	{
		// catch up or wait for the player
		if (enemy->position.y < player->position.y)
			MoveTowards(&enemy->speed.y, -ENEMY_MIN_SPEED, time.delta * ENEMY_ACCEL);
		else
			MoveTowards(&enemy->speed.y, -ENEMY_MAX_SPEED, time.delta * ENEMY_ACCEL);

		MoveTowards(&enemy->speed.x, 0, time.delta * ENEMY_ACCEL_SIDES);
	}

	enemy->speed.y = Clamp(enemy->speed.y, -ENEMY_MAX_SPEED, -ENEMY_MIN_SPEED);

	enemy->position.x += enemy->speed.x * time.delta;
	enemy->position.y += (enemy->speed.y - player->speed.y) * time.delta;
}



Vector2 CalculateOverlap(GameObject* go1, GameObject* go2)
{
	Vector2 overlap = {};
	overlap.x = (go1->size.x + go2->size.x) * 0.5 - fabs(go1->position.x - go2->position.x);
	overlap.y = (go1->size.y + go2->size.y) * 0.5 - fabs(go1->position.y - go2->position.y);
	return overlap;
}

void CheckCollision(Car* car1, Car* car2)
{
	Vector2 overlap = CalculateOverlap(car1, car2);
	if (overlap.x >= 0 && overlap.y >= 0)
	{
		double temp = car1->speed.x * COLLISION_BOUNCE;
		car1->speed.x = car2->speed.x * COLLISION_BOUNCE;
		car2->speed.x = temp;

		car1->position.x += (overlap.x + 1) * 0.5 * Sign(car1->position.x - car2->position.x);
		car2->position.x += (overlap.x + 1) * 0.5 * -Sign(car1->position.x - car2->position.x);
	}
}

void ResolveCollisions(GameData* gameData)
{
	for (int i = 0; i < gameData->enemyCount; i++)
	{
		CheckCollision(gameData->player, gameData->enemies[i]);

		for (int j = 0; j < gameData->enemyCount; j++)
		{
			CheckCollision(gameData->enemies[i], gameData->enemies[j]);
		}
	}
}




void CreateEnemy(GameData* gameData, SDL_Surface** bitmaps, Vector2 pos)
{
	if (gameData->enemyCount >= MAX_ENEMIES)
	{
		printf("Couldn't create a new enemy - max enemy count reached\n");

		return;
	}

	Enemy* enemy = new Enemy();
	enemy->position = pos;
	enemy->sprite = bitmaps[BMP_ENEMY_CAR];
	enemy->size = CAR_SIZE;
	enemy->speed = { 0, -PLAYER_MAX_SPEED / 2 };
	gameData->enemies[gameData->enemyCount] = enemy;
	gameData->enemyCount++;
}

void DeleteEnemy(GameData* gameData, int enemyIndex)
{
	// if the deleted enemy isn't the last one in the array
	// move the last one into it's place
	// like this:
	// ##D##L
	//     / 
	//    /  
	// ##L## 

	delete gameData->enemies[enemyIndex];

	gameData->enemyCount--;
	if (enemyIndex != gameData->enemyCount)
	{
		gameData->enemies[enemyIndex] = gameData->enemies[gameData->enemyCount];
	}

	printf("Enemy %d deleted\n", enemyIndex);

}


// create all necessary GameObjects
void GameStart(GameData* gameData, SDL_Surface** bitmaps)
{
	gameData->enemyCount = 0;

	GameObject* background = new GameObject();
	background->sprite = bitmaps[BMP_BACKGROUND];
	background->position = { SCREEN_WIDTH / 2, 0 };
	gameData->background = background;

	Player* player = new Player();
	player->sprite = bitmaps[BMP_PLAYER_CAR];
	player->position = { SCREEN_WIDTH / 2, PLAYER_Y_POS };
	player->size = CAR_SIZE;
	gameData->player = player;

	for (int i = 0; i < 1600; i++)
	{
		CreateEnemy(gameData, bitmaps, { (double)SCREEN_WIDTH / 2 + (rand() % 100), (double)SCREEN_HEIGHT / 2 + (rand() % 100) });
	}
}

void GameUpdate(Time time, GameData* gameData, Input* input)
{
	PlayerSteering(gameData->player, time, input);

	MoveBackground(gameData->background, gameData->player->speed.y, time);

	for (int i = 0; i < gameData->enemyCount; i++)
	{
		UpdateEnemy(gameData->enemies[i], gameData->player, time);
		if (gameData->enemies[i]->position.y >= 2 * SCREEN_HEIGHT)
		{
			DeleteEnemy(gameData, i);
		}
	}

	ResolveCollisions(gameData);
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
		else if (event.key.keysym.sym == SDLK_F3) input->showDebug = !input->showDebug; // debug info is toggled on keypress
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



void MeasureTime(Time* time)
{
	time->timeCounterCurrent = SDL_GetPerformanceCounter();
	time->delta = ((double)(time->timeCounterCurrent - time->timeCounterPrevious) / (double)SDL_GetPerformanceFrequency());
	time->timeCounterPrevious = time->timeCounterCurrent;

	time->time += time->delta;


	// measure the FPS
	time->fpsTimer += time->delta;
	if (time->fpsTimer > FPS_COUNTER_INTERVAL)
	{
		time->fps = time->frames / FPS_COUNTER_INTERVAL;
		time->frames = 0;
		time->fpsTimer = 0;
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

void DrawUI(SDL_Surface* screen, GameData* gameData, Time time, SDL_Surface* charset, char* stringBuffer)
{
	//DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, blue);
	sprintf(stringBuffer, "Score: %d", gameData->player->score);
	DrawString(screen, screen->w / 2 - strlen(stringBuffer) * 8 / 2, 10, stringBuffer, charset);
}

void DrawDebugInfo(SDL_Surface* screen, GameData* gameData, Time time, SDL_Surface* charset, char* stringBuffer)
{
	//DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, blue);
	sprintf(stringBuffer, "FPS: %.0lf ", time.fps);
	DrawString(screen, SCREEN_WIDTH / 2 - strlen(stringBuffer) * 8 / 2, 20, stringBuffer, charset);
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

	// this loop is repeated when the player starts a new game
	while (!quit)
	{
		GameData gameData;
		GameStart(&gameData, bitmaps);

		// reset the tick counter so that the time delta 
		// in the first frame doesn't take into account time spent loading the game
		time.timeCounterPrevious = SDL_GetPerformanceCounter();

		// GAMEPLAY LOOP
		// this loop is repeated every frame
		while (!quit)
		{
			MeasureTime(&time);

			GameUpdate(time, &gameData, &input);

			DrawGameObjects(screen, &gameData);

			DrawUI(screen, &gameData, time, bitmaps[BMP_CHARSET], stringBuffer);

			if (input.showDebug)
				DrawDebugInfo(screen, &gameData, time, bitmaps[BMP_CHARSET], stringBuffer);



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
