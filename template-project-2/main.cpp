#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}


#define FULLSCREEN true
#define WINDOW_TITLE "Filip Jezierski 196333"
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define FPS_LIMIT 144 // set to -1 for unlimited FPS
#define FPS_COUNTER_INTERVAL 0.1
#define RAND_VAL_PRECISION 100

#define STRING_BUFFER_SIZE 128


#define HIGHSCORES_FILE "highscores.txt"
#define LEADERBOARD_LENGTH 20
#define LEADERBOARD_SCROLL_DELAY 0.02


//////////////////////////////////////////////////////////////////////////////////////
// GAMEPLAY CONSTANTS

#define CAR_SIZE { 14, 20 }
#define PLAYER_Y_POS 200

// the player is given SCORE_PER_DISTANCE points per each DISTANCE_TO_SCORE travelled
#define SCORE_PER_ENEMY_KILL 300
#define SCORE_PER_DISTANCE 50
#define SCORE_PENALTY_DURATION 3
#define DISTANCE_TO_SCORE SCREEN_HEIGHT

// value between 0 and 1
#define COLLISION_BOUNCE 1
#define COLLISION_KILL_SPEED 150
#define EXPLOSION_FRICTION 500

#define DEATH_ANIM_DURATION 0.3

#define INFINITE_LIVES_DURATION 10
#define POINTS_PER_LIFE 5000



// player stats
#define PLAYER_MAX_SPEED 800
#define PLAYER_START_POS SCREEN_HEIGHT + 20
#define PLAYER_START_SPEED 200
#define PLAYER_MIN_SPEED 400
#define PLAYER_MAX_SPEED_SIDES 300
#define PLAYER_ACCEL 800
#define PLAYER_ACCEL_SIDES 3000
#define PLAYER_IDLE_ACCEL_SIDES 2000

#define PLAYER_GUN_RANGE 150
#define PLAYER_FIRE_INTERVAL 0.07
#define MAX_BULLETS 16
#define BULLET_SIZE { 4, 10 }
#define BULLET_SPEED 500


// enemy stats
#define ENEMY_HP 4
#define ENEMY_TARGET_DISTANCE 40
#define ENEMY_MAX_SPEED 700
#define ENEMY_MIN_SPEED 300
#define ENEMY_MAX_SPEED_SIDES 300
#define ENEMY_ACCEL 400
#define ENEMY_ACCEL_SIDES 1000
#define ENEMY_BRAKING 150

// civilian stats
#define CIVILIAN_HP 2
#define CIVILIAN_SPEED 500
#define CIVILIAN_ACCEL 400
#define CIVILIAN_ACCEL_SIDES 1000


// NPC spawning

// this is a hard limit that cannot be exceeded
#define MAX_NPCS 16
#define NPC_SPAWN_TICK_INTERVAL 0.5

// NPCs further than this from the center of the screen are deleted
#define NPC_DELETE_DISTANCE SCREEN_HEIGHT

enum NPCType
{
	ENEMY,
	CIVILIAN,
};

enum UIAnchor
{
	CENTER,
	UPPER_LEFT,
	UPPER_RIGHT,
	LOWER_LEFT,
	LOWER_RIGHT,
	MIDDLE_LEFT,
	MIDDLE_RIGHT,
	UPPER_CENTER,
	LOWER_CENTER,
};



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
	double gametime;
	double delta;
	bool paused;

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
	bool saveScore;

	bool up;
	bool down;
	bool left;
	bool right;
	bool shoot;

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

// returns a random value in the range r1-r2
double RandRange(double r1, double r2)
{
	return r1 + (r2 - r1) * RandVal();
}


//////////////////////////////////////////////////////////////////////////////////////
// RENDERING

// draw a text on surface screen, offset by (x, y) from the anchor
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, Vector2 offset, const char* text, SDL_Surface* charset, UIAnchor anchor)
{
	int x = offset.x;
	int y = offset.y;
	Vector2 size = { strlen(text) * 8, 8 };

	switch (anchor)
	{
	case CENTER:
		x += screen->w / 2 - size.x / 2;
		y += screen->h / 2 - size.y / 2;
		break;
	case UPPER_LEFT:
		break;
	case UPPER_RIGHT:
		x += screen->w - size.x;
		break;
	case LOWER_LEFT:
		y += screen->h - size.y;
		break;
	case LOWER_RIGHT:
		x += screen->w - size.x;
		y += screen->h - size.y;
		break;
	case MIDDLE_LEFT:
		y += screen->h / 2 - size.y / 2;
		break;
	case MIDDLE_RIGHT:
		x += screen->w - size.x;
		y += screen->h / 2 - size.y / 2;
		break;
	case UPPER_CENTER:
		x += screen->w / 2 - size.x / 2;
		break;
	case LOWER_CENTER:
		x += screen->w / 2 - size.x / 2;
		y += screen->h - size.y;
		break;
	default:
		break;
	}

	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text)
	{
		c = *text;
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

	SDL_SetWindowTitle(*window, WINDOW_TITLE);


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
	BMP_EXPLOSION_0,
	BMP_EXPLOSION_1,
	BMP_BULLET,
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
	error |= !LoadBitmap(&bmps[BMP_EXPLOSION_0], "./sprites/explosion_0.bmp");
	error |= !LoadBitmap(&bmps[BMP_EXPLOSION_1], "./sprites/explosion_1.bmp");
	error |= !LoadBitmap(&bmps[BMP_BULLET], "./sprites/bullet.bmp");
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
	bool visible = true;
	Vector2 position = {};
	Vector2 size = {};
	SDL_Surface* sprite = NULL;
	
	virtual void Draw(SDL_Surface* screen)
	{
		if (!visible) return;

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

	SDL_Surface* explosion0 = NULL;
	SDL_Surface* explosion1 = NULL;

	double deathTime = 0;
	bool IsDead()
	{
		return deathTime > 0;
	}

	// returns true if the animation has ended & the car can be deleted
	bool AnimateDeath(Time time)
	{
		if (deathTime > 0)
		{
			if (time.gametime >= deathTime + DEATH_ANIM_DURATION)
				return true; 
			else if (time.gametime >= deathTime + DEATH_ANIM_DURATION / 2)
				sprite = explosion1;
			else if (time.gametime >= deathTime)
				sprite = explosion0;
		}

		return false;
	}
};

class Player : public Car
{
public:
	double distanceCounter = 0;
	double scoringDistanceCounter = 0;
	double nextShootTime = 0;
	double scorePenalty = 0;
	int score = 0;
	int lifeScoreCounter = 0;
	int lives = 0;
};

class NPC : public Car
{
public:
	NPCType type = ENEMY;
	int health = 0;
};

struct GameData
{
	double gameOverTime = 0;

	// game objects
	Player* player = NULL;
	int npcCount = 0;
	NPC* npcs[MAX_NPCS] = {};
	GameObject* bullets[MAX_BULLETS] = {};

	GameObject* background = NULL;

	// NPC spawning
	double nextNPCSpawnTick = 0;
};





//////////////////////////////////////////////////////////////////////////////////////
// GAME MECHANICS


bool IsOnRoad(Vector2 pos)
{
	if (pos.x > SCREEN_WIDTH * 0.25 &&
		pos.x < SCREEN_WIDTH * 0.75)
		return true;

	return false;
}

double CalculateMaxSideSpeed(double forwardSpeed, double maxForwardSpeed, double maxSideSpeed)
{
	return (forwardSpeed / maxForwardSpeed) * maxSideSpeed;
}


void CreateNPC(GameData* gameData, SDL_Surface** bitmaps, Vector2 pos, NPCType type)
{
	if (gameData->npcCount >= MAX_NPCS)
	{
		printf("Couldn't create a new npc - max npc count reached\n");

		return;
	}

	NPC* npc = new NPC();
	npc->position = pos;
	npc->deathTime = 0;
	npc->size = CAR_SIZE;
	npc->explosion0 = bitmaps[BMP_EXPLOSION_0];
	npc->explosion1 = bitmaps[BMP_EXPLOSION_1];

	npc->type = type;
	switch (type)
	{
	case ENEMY:
		npc->speed = { 0, -ENEMY_MAX_SPEED };
		npc->sprite = bitmaps[BMP_ENEMY_CAR];
		npc->health = ENEMY_HP;
		break;
	case CIVILIAN:
		npc->speed = { 0, -CIVILIAN_SPEED };
		npc->sprite = bitmaps[BMP_CIVILIAN_CAR];
		npc->health = CIVILIAN_HP;
		break;
	default:
		break;
	}
	gameData->npcs[gameData->npcCount] = npc;
	gameData->npcCount++;
}
void DeleteNPC(GameData* gameData, int npcIndex)
{
	// if the deleted npc isn't the last one in the array
	// move the last one into it's place
	// like this:
	// ##D##L
	//     / 
	//    /  
	// ##L## 

	NPCType type = gameData->npcs[npcIndex]->type;
	delete gameData->npcs[npcIndex];

	gameData->npcCount--;
	if (npcIndex != gameData->npcCount)
	{
		gameData->npcs[npcIndex] = gameData->npcs[gameData->npcCount];
	}
}

void KillCar(Car* car, Time time)
{
	if (!car->IsDead())
	{
		car->deathTime = time.gametime;
	}
}
void DamageNPC(int damage, NPC* npc, Time time)
{
	npc->health -= damage;
	if (npc->health <= 0)
	{
		KillCar(npc, time);
	}
}

void PlayerSteering(Player* player, Time time, Input* input)
{
	// convert input into a direction vector
	Vector2 steering = { 0,0 };
	if (input->up) steering.y--;
	if (input->down) steering.y++;
	if (input->left) steering.x--;
	if (input->right) steering.x++;

	// accelerate / decelerate and steer the car
	if (steering.x == 0)
		MoveTowards(&player->speed.x, 0, time.delta * PLAYER_IDLE_ACCEL_SIDES);
	else
		MoveTowards(&player->speed.x, 
			steering.x * CalculateMaxSideSpeed(-player->speed.y, PLAYER_MAX_SPEED, PLAYER_MAX_SPEED_SIDES),
			time.delta * PLAYER_ACCEL_SIDES);

	player->speed.y += steering.y * time.delta * PLAYER_ACCEL;
	player->speed.y = Clamp(player->speed.y, -PLAYER_MAX_SPEED, -PLAYER_MIN_SPEED);

	// the player doesn't move along the y axis, the rest of the world does
	player->position.x += player->speed.x * time.delta;
	MoveTowards(&player->position.y, PLAYER_Y_POS, time.delta * PLAYER_START_SPEED);
}

void PlayerShoot(GameObject* bullets[MAX_BULLETS], Vector2 pos)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!bullets[i]->visible)
		{
			bullets[i]->visible = true;
			bullets[i]->position = pos;
			break;
		}
	}

	/*Player* player = gameData->player;
	for (int i = 0; i < gameData->npcCount; i++)
	{
		NPC* npc = gameData->npcs[i];
		if (npc->position.y < player->position.y &&
			npc->position.y > player->position.y - PLAYER_GUN_RANGE &&
			fabs(npc->position.x - player->position.x) <= player->size.x)
		{
			npc->health--;
			if (npc->health <= 0)
			{
				KillCar(npc, time);
			}
		}
	}*/
}
void PlayerShooting(GameData* gameData, Time time, Input* input)
{
	if (input->shoot && gameData->player->nextShootTime <= time.gametime)
	{
		gameData->player->nextShootTime = time.gametime + PLAYER_FIRE_INTERVAL;

		PlayerShoot(gameData->bullets, gameData->player->position);
	}
}

void AddScore(int points, Player* player, Time time)
{
	if (player->scorePenalty <= time.gametime)
	{
		player->score += points;

		player->lifeScoreCounter += points;
		if (player->lifeScoreCounter >= POINTS_PER_LIFE)
		{
			player->lives++;
			player->lifeScoreCounter -= POINTS_PER_LIFE;
		}
	}
}
void CountScorePerDistance(Player* player, Time time)
{
	// count the score based on distance
	player->scoringDistanceCounter -= player->speed.y * time.delta;
	if (player->scoringDistanceCounter >= DISTANCE_TO_SCORE)
	{
		AddScore(SCORE_PER_DISTANCE, player, time);
		player->scoringDistanceCounter = 0;
	}
}


void EnemyAI(NPC* npc, Player* player, Time time)
{
	if (fabs(npc->position.y - player->position.y) < ENEMY_TARGET_DISTANCE)
	{
		// match the players speed
		MoveTowards(&npc->speed.y, player->speed.y - (npc->position.y - player->position.y), time.delta * ENEMY_ACCEL);

		// try to push the player off the road
		MoveTowards(&npc->speed.x,
			Sign(player->position.x - npc->position.x) * CalculateMaxSideSpeed(-npc->speed.y, ENEMY_MAX_SPEED, ENEMY_MAX_SPEED_SIDES),
			time.delta * ENEMY_ACCEL_SIDES);
	}
	else
	{
		// catch up or wait for the player
		if (npc->position.y < player->position.y)
			MoveTowards(&npc->speed.y, player->speed.y + ENEMY_BRAKING, time.delta * ENEMY_ACCEL);
		else
			MoveTowards(&npc->speed.y, -ENEMY_MAX_SPEED, time.delta * ENEMY_ACCEL);

		MoveTowards(&npc->speed.x, 0, time.delta * ENEMY_ACCEL_SIDES);
	}


	npc->speed.y = Clamp(npc->speed.y, -ENEMY_MAX_SPEED, -ENEMY_MIN_SPEED);
}
void CivilianAI(NPC* npc, Time time)
{
	MoveTowards(&npc->speed.x, 0, time.delta * CIVILIAN_ACCEL_SIDES);
	MoveTowards(&npc->speed.y, -CIVILIAN_SPEED, time.delta * CIVILIAN_ACCEL);
}
void UpdateNPC(NPC* npc, Player* player, Time time)
{
	if (!npc->IsDead())
	{
		switch (npc->type)
		{
		case ENEMY:
			EnemyAI(npc, player, time);
			break;
		case CIVILIAN:
			CivilianAI(npc, time);
			break;
		default:
			break;
		}
	}
	else
	{
		MoveTowards(&npc->speed.x, 0, EXPLOSION_FRICTION * time.delta);
		MoveTowards(&npc->speed.y, 0, EXPLOSION_FRICTION * time.delta);
	}

	npc->position.x += npc->speed.x * time.delta;
	npc->position.y += (npc->speed.y - player->speed.y) * time.delta;
}


void MoveBackground(GameObject* background, double playerSpeed, Time time)
{
	background->position.y -= playerSpeed * (double)time.delta;
	if (background->position.y > SCREEN_HEIGHT)
		background->position.y = 0;
}



double GetRandomPosOnRoad()
{
	return RandRange(SCREEN_WIDTH * 0.3, SCREEN_WIDTH * 0.7);
}
void NPCSpawning(GameData* gameData, SDL_Surface** bitmaps, Time time)
{
	if (gameData->nextNPCSpawnTick <= time.gametime)
	{
		gameData->nextNPCSpawnTick = time.gametime + NPC_SPAWN_TICK_INTERVAL;

		if (RandVal() < (1.0 / (__max(gameData->npcCount, 1))))
		{
			NPCType type = ENEMY;
			if (gameData->npcCount >= 2 && rand() % 2)
			{
				 type = CIVILIAN;
			}

			CreateNPC(gameData, bitmaps, { GetRandomPosOnRoad() , -20 }, type);
		}
	}
}



Vector2 CalculateOverlap(GameObject* go1, GameObject* go2)
{
	Vector2 overlap = {};
	overlap.x = (go1->size.x + go2->size.x) * 0.5 - fabs(go1->position.x - go2->position.x);
	overlap.y = (go1->size.y + go2->size.y) * 0.5 - fabs(go1->position.y - go2->position.y);
	return overlap;
}
bool IsOverlapping(GameObject* go1, GameObject* go2)
{
	Vector2 overlap = CalculateOverlap(go1, go2);
	return overlap.x > 0 && overlap.y > 0;
}

void CheckCollision(Car* car1, Car* car2, Time time)
{
	if (car1->IsDead() || car2->IsDead())
		return;

	Vector2 overlap = CalculateOverlap(car1, car2);
	if (overlap.x >= 0 && overlap.y >= 0)
	{
		if (fabs(car1->position.x - car2->position.x) >= (car1->size.x + car2->size.x) * 0.25)
		{
			// horizontal collision
			car1->position.x += (overlap.x + 1) * 0.5 * Sign(car1->position.x - car2->position.x);
			car2->position.x += (overlap.x + 1) * 0.5 * -Sign(car1->position.x - car2->position.x);

			double temp = car1->speed.x * COLLISION_BOUNCE;
			car1->speed.x = car2->speed.x * COLLISION_BOUNCE;
			car2->speed.x = temp;
		}
		else
		{
			// vertical collision
			car1->position.y += (overlap.y + 1) * 0.5 * Sign(car1->position.y - car2->position.y);
			car2->position.y += (overlap.y + 1) * 0.5 * -Sign(car1->position.y - car2->position.y);


			if (fabs(car1->speed.y - car2->speed.y) >= COLLISION_KILL_SPEED)
			{
				if (car1->position.y > car2->position.y)
					KillCar(car1, time);
				else
					KillCar(car2, time);

			}

			double temp = car1->speed.y * COLLISION_BOUNCE;
			car1->speed.y = car2->speed.y * COLLISION_BOUNCE;
			car2->speed.y = temp;
		}
	}
}
void ResolveCollisions(GameData* gameData, Time time)
{
	for (int i = 0; i < gameData->npcCount; i++)
	{
		CheckCollision(gameData->player, gameData->npcs[i], time);

		for (int j = 0; j < gameData->npcCount; j++)
		{
			CheckCollision(gameData->npcs[i], gameData->npcs[j], time);
		}
	}
}

bool IsGameOver(GameData* gameData)
{
	return gameData->gameOverTime != 0;
}
void GameOver(GameData* gameData, Time time)
{
	printf("GAME OVER!\n");
	gameData->gameOverTime = time.gametime;
	gameData->player->speed.y = 0;
}


void RespawnPlayer(GameData* gameData, SDL_Surface** bitmaps)
{
	if (gameData->player->lives > 0)
		gameData->player->lives--;

	gameData->player->deathTime = 0;
	gameData->player->sprite = bitmaps[BMP_PLAYER_CAR];
	gameData->player->position = { SCREEN_WIDTH / 2, PLAYER_START_POS };
	gameData->player->speed = {};
}


void UpdatePlayer(Time time, GameData* gameData, SDL_Surface** bitmaps, Input* input)
{
	if (!IsOnRoad(gameData->player->position))
		KillCar(gameData->player, time);

	if (!gameData->player->IsDead())
	{
		PlayerSteering(gameData->player, time, input);
		PlayerShooting(gameData, time, input);

		MoveBackground(gameData->background, gameData->player->speed.y, time);

		CountScorePerDistance(gameData->player, time);


	}
	else
	{
		gameData->player->speed.y = 0;
		if (gameData->player->lives > 0 || time.gametime < INFINITE_LIVES_DURATION)
		{
			if (gameData->player->AnimateDeath(time))
			{
				RespawnPlayer(gameData, bitmaps);
			}
		}
		else
		{
			if (gameData->player->AnimateDeath(time))
				gameData->player->visible = false;

			if (!IsGameOver(gameData))
				GameOver(gameData, time);
		}
	}
}
void UpdateNPCs(Time time, GameData* gameData)
{
	for (int i = 0; i < gameData->npcCount; i++)
	{
		UpdateNPC(gameData->npcs[i], gameData->player, time);

		if (!IsOnRoad(gameData->npcs[i]->position))
			KillCar(gameData->npcs[i], time);

		if (gameData->npcs[i]->AnimateDeath(time))
		{
			switch (gameData->npcs[i]->type)
			{
			case ENEMY:
				AddScore(SCORE_PER_ENEMY_KILL, gameData->player, time);
				break;
			case CIVILIAN:
				gameData->player->scorePenalty = time.gametime + SCORE_PENALTY_DURATION;
				break;
			default:
				break;
			}

			DeleteNPC(gameData, i);
		}
		else if (fabs(gameData->npcs[i]->position.y - SCREEN_HEIGHT / 2) >= NPC_DELETE_DISTANCE)
		{
			DeleteNPC(gameData, i);
		}
	}

}
void UpdateBullets(Time time, GameData* gameData)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!gameData->bullets[i]->visible) continue;

		gameData->bullets[i]->position.y -= time.delta * BULLET_SPEED;

		for (int j = 0; j < gameData->npcCount; j++)
		{
			if (!gameData->npcs[j]->IsDead() && IsOverlapping(gameData->bullets[i], gameData->npcs[j]))
			{
				DamageNPC(1, gameData->npcs[j], time);
				gameData->bullets[i]->visible = false;
				break;
			}
		}
		if (fabs(gameData->bullets[i]->position.y - gameData->player->position.y) > PLAYER_GUN_RANGE)
		{
			gameData->bullets[i]->visible = false;
		}
	}
}

// create all necessary GameObjects
void GameStart(GameData* gameData, SDL_Surface** bitmaps)
{
	gameData->npcCount = 0;

	GameObject* background = new GameObject();
	background->sprite = bitmaps[BMP_BACKGROUND];
	background->position = { SCREEN_WIDTH / 2, 0 };
	gameData->background = background;

	Player* player = new Player();
	player->sprite = bitmaps[BMP_PLAYER_CAR];
	player->lives = 0;
	player->deathTime = 0;
	player->explosion0 = bitmaps[BMP_EXPLOSION_0];
	player->explosion1 = bitmaps[BMP_EXPLOSION_1];
	player->position = { SCREEN_WIDTH / 2, PLAYER_START_POS };
	player->size = CAR_SIZE;
	gameData->player = player;

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		GameObject* bullet = new GameObject();
		bullet->visible = false;
		bullet->sprite = bitmaps[BMP_BULLET];
		bullet->size = BULLET_SIZE;
		gameData->bullets[i] = bullet;
	}
}

void GameUpdate(Time time, GameData* gameData, SDL_Surface** bitmaps, Input* input)
{
	UpdatePlayer(time, gameData, bitmaps, input);
	UpdateNPCs(time, gameData);
	UpdateBullets(time, gameData);

	if (!gameData->player->IsDead())
		NPCSpawning(gameData, bitmaps, time);

	ResolveCollisions(gameData, time);
}


void UpdateInputs(Input* input, SDL_Event event)
{
	switch (event.type)
	{
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_ESCAPE) input->quit = true;
		else if (event.key.keysym.sym == SDLK_n) input->newGame = true;
		else if (event.key.keysym.sym == SDLK_UP) input->up = true;
		else if (event.key.keysym.sym == SDLK_DOWN) input->down = true;
		else if (event.key.keysym.sym == SDLK_LEFT) input->left = true;
		else if (event.key.keysym.sym == SDLK_RIGHT) input->right = true;
		else if (event.key.keysym.sym == SDLK_SPACE) input->shoot = true;
		else if (event.key.keysym.sym == SDLK_p) input->pause = true;
		else if (event.key.keysym.sym == SDLK_s) input->saveScore = true;
		else if (event.key.keysym.sym == SDLK_F3) input->showDebug = !input->showDebug; // toggled on keypress
		break;
	case SDL_KEYUP:
		if (event.key.keysym.sym == SDLK_UP) input->up = false;
		else if (event.key.keysym.sym == SDLK_DOWN) input->down = false;
		else if (event.key.keysym.sym == SDLK_LEFT) input->left = false;
		else if (event.key.keysym.sym == SDLK_RIGHT) input->right = false;
		else if (event.key.keysym.sym == SDLK_SPACE) input->shoot = false;
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
	if (!time->paused)
		time->gametime += time->delta;


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
// SAVING

struct Highscore
{
	int score;
	int time; // in miliseconds
};

enum LeaderboardSortMode
{
	SORT_BY_SCORE,
	SORT_BY_TIME
};

struct Leaderboard
{
	LeaderboardSortMode sortMode = SORT_BY_SCORE;
	int displayOffset = 0;
	double nextScrollTime = 0;
	int arrayCapacity = 0;
	int scoreCount = 0;
	Highscore* highscores = NULL;
};

void ScrollLeaderboard(Leaderboard* leaderboard, Input input, Time time)
{
	if ((input.up || input.down) &&
		time.time >= leaderboard->nextScrollTime)
	{
		leaderboard->nextScrollTime = time.time + LEADERBOARD_SCROLL_DELAY;

		if (input.up)
			leaderboard->displayOffset--;
		if (input.down)
			leaderboard->displayOffset++;


		if (leaderboard->displayOffset > leaderboard->scoreCount - LEADERBOARD_LENGTH)
			leaderboard->displayOffset = leaderboard->scoreCount - LEADERBOARD_LENGTH;

		if (leaderboard->displayOffset < 0)
			leaderboard->displayOffset = 0;
	}
}

void SortLeaderboard(Leaderboard* leaderboard)
{

}

bool AddScoreToLeaderboard(Leaderboard* leaderboard, Highscore score)
{
	leaderboard->scoreCount++;
	if (leaderboard->scoreCount > leaderboard->arrayCapacity)
	{
		leaderboard->arrayCapacity *= 2;
		leaderboard->highscores = (Highscore*)realloc(leaderboard->highscores, sizeof(Highscore) * leaderboard->arrayCapacity);
		if (leaderboard->highscores == NULL)
		{
			printf("Ran out of memory when saving the highscore!\n");
			return false;
		}
	}
	leaderboard->highscores[leaderboard->scoreCount - 1] = score;

	return true;
}

void WriteIntToFile(int value, FILE* file, char* stringBuffer, const char* label)
{
	itoa(value, stringBuffer, 10);
	fprintf(file, stringBuffer);
	fprintf(file, " #");
	fprintf(file, label);
	fprintf(file, "\n");
}

void SaveScore(Leaderboard* leaderboard, int score, double time)
{
	char stringBuffer[STRING_BUFFER_SIZE] = "";
	FILE* file = fopen("highscores.txt", "a");

	Highscore highscore = { score, (int)(time * 1000) };
	WriteIntToFile(highscore.score, file, stringBuffer, "score");
	WriteIntToFile(highscore.time, file, stringBuffer, "time");

	fclose(file);

	AddScoreToLeaderboard(leaderboard, highscore);
}

bool LoadLeaderboard(Leaderboard* leaderboard)
{
	leaderboard->arrayCapacity = 1;
	leaderboard->highscores = (Highscore*)malloc(sizeof(Highscore));

	char stringBuffer[STRING_BUFFER_SIZE] = "";
	FILE* file = fopen(HIGHSCORES_FILE, "r");

	while (fgets(stringBuffer, STRING_BUFFER_SIZE, file))
	{
		Highscore highscore = {};
		highscore.score = atoi(stringBuffer);
		fgets(stringBuffer, STRING_BUFFER_SIZE, file);
		highscore.time = atoi(stringBuffer);

		if (!AddScoreToLeaderboard(leaderboard, highscore))
			return false;
	}

	fclose(file);
	return true;
}




//////////////////////////////////////////////////////////////////////////////////////
// GAME VISUALS

void DrawGameObjects(SDL_Surface* screen, GameData* gameData)
{
	gameData->background->Draw(screen);

	gameData->player->Draw(screen);

	for (int i = 0; i < gameData->npcCount; i++)
	{
		gameData->npcs[i]->Draw(screen);
	}

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		gameData->bullets[i]->Draw(screen);
	}
}

void DrawLeaderboard(SDL_Surface* screen, Leaderboard leaderboard, SDL_Surface* charset, char* stringBuffer)
{
	DrawString(screen, { 5,-90 }, "Highscores:", charset, MIDDLE_LEFT);
	if (leaderboard.sortMode == SORT_BY_SCORE)
		DrawString(screen, { 5,-78 }, "(Sorted by score)", charset, MIDDLE_LEFT);
	else
		DrawString(screen, { 5,-78 }, "(Sorted by time)", charset, MIDDLE_LEFT);
	for (int i = 0; 
		i < LEADERBOARD_LENGTH &&
		i + leaderboard.displayOffset < leaderboard.scoreCount;
		i++)
	{
		int index = i + leaderboard.displayOffset;
		sprintf(stringBuffer, "%3d.%7.2f %6.0d", index + 1, leaderboard.highscores[index].time * 0.001, leaderboard.highscores[index].score);
		DrawString(screen, { 2,(double)(-65 + i * 10) }, stringBuffer, charset, MIDDLE_LEFT);
	}
}
void DrawUI(SDL_Surface* screen, GameData* gameData, Time time, Leaderboard leaderboard, SDL_Surface* charset, char* stringBuffer)
{
	DrawString(screen, { 0,10 }, WINDOW_TITLE, charset, UPPER_CENTER);
	DrawString(screen, { -5,-5 }, "ABCDEFIJKLM", charset, LOWER_RIGHT);


	if (!IsGameOver(gameData))
	{
		sprintf(stringBuffer, "Time: %.2f Score: %d", time.gametime, gameData->player->score);
		DrawString(screen, { 0,30 }, stringBuffer, charset, UPPER_CENTER);

		if (time.gametime >= INFINITE_LIVES_DURATION)
		{
			sprintf(stringBuffer, "Lives: %d", gameData->player->lives);
			DrawString(screen, { 0,50 }, stringBuffer, charset, UPPER_CENTER);
		}
		else
			DrawString(screen, { 0,50 }, "Lives: INF", charset, UPPER_CENTER);
		
		if (gameData->player->scorePenalty > time.gametime)
			DrawString(screen, { 0,-20 }, "No points!", charset, LOWER_CENTER);
	}
	else
	{
		DrawString(screen, { 0,-40 }, "GAME OVER", charset, CENTER);

		sprintf(stringBuffer, "Score: %d", gameData->player->score);
		DrawString(screen, { 0,-20 }, stringBuffer, charset, CENTER);

		sprintf(stringBuffer, "Time: %.2f", gameData->gameOverTime);
		DrawString(screen, { 0,0 }, stringBuffer, charset, CENTER);


		DrawString(screen, { 0,-40 }, "N - new game  ", charset, LOWER_CENTER);
		DrawString(screen, { 0,-25 }, "S - save score", charset, LOWER_CENTER);
	}

	if (time.paused || IsGameOver(gameData))
	{
		DrawLeaderboard(screen, leaderboard, charset, stringBuffer);
	}
}

void DrawDebugInfo(SDL_Surface* screen, GameData* gameData, Time time, SDL_Surface* charset, char* stringBuffer)
{
	//DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, blue);
	sprintf(stringBuffer, "FPS: %.0lf ", time.fps);
	DrawString(screen, { 0,10 }, stringBuffer, charset, UPPER_RIGHT);
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

	Leaderboard leaderboard;
	if (!LoadLeaderboard(&leaderboard))
	{
		printf("Couldn't load the leaderboard!\n");
		return 1;
	}

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
		Time time = {};
		Input input = {};
		bool scoreSaved = false; // this is to prevent saving the score multiple times

		GameData gameData;
		GameStart(&gameData, bitmaps);

		// reset the tick counter so that the time delta 
		// in the first frame doesn't take into account time spent loading the game
		time.timeCounterPrevious = SDL_GetPerformanceCounter();

		// GAMEPLAY LOOP
		// this loop is repeated every frame
		while (!quit && !input.newGame)
		{
			MeasureTime(&time);

			if (!time.paused)
				GameUpdate(time, &gameData, bitmaps, &input);

			DrawGameObjects(screen, &gameData);
			DrawUI(screen, &gameData, time, leaderboard, bitmaps[BMP_CHARSET], stringBuffer);

			if (input.showDebug)
				DrawDebugInfo(screen, &gameData, time, bitmaps[BMP_CHARSET], stringBuffer);

			if (time.paused || IsGameOver(&gameData))
				ScrollLeaderboard(&leaderboard, input, time);


			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			// SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			// handling of events (if there were any)
			input.pause = false;
			input.saveScore = false;
			while (SDL_PollEvent(&event))
			{
				UpdateInputs(&input, event);
			}

			if (!scoreSaved && IsGameOver(&gameData) && input.saveScore)
			{
				SaveScore(&leaderboard, gameData.player->score, gameData.gameOverTime);
				scoreSaved = true;
			}

			if (input.pause)
				time.paused = !time.paused;

			if (input.quit)
				quit = 1;

			// limit the FPS
			if (FPS_LIMIT > 0)
			{
				SDL_Delay(__max(1000.0 / (FPS_LIMIT) - time.delta, 0));
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
