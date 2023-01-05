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


// struct used to describe 2D positions, offsets & vectors
struct Vector2
{
	float x;
	float y;
};

// a struct that holds all bitmaps used by sprites
struct Bitmaps
{
	SDL_Surface* charset;
	SDL_Surface* playerCar;
	SDL_Surface* enemyCar;
	SDL_Surface* civilianCar;
};

struct Input
{
	bool quit;
	bool up;
	bool down;
	bool left;
	bool right;
};


class GameObject
{
	Vector2 position = {};
	Vector2 size = {};
	SDL_Surface* surface;

public:
	virtual void Init()
	{

	}
	virtual void Update(double delta, Input* input)
	{

	}
};

struct GameData
{
	int gameObjectArrayCapacity;
	int gameObjectCount;
	GameObject** gameObjects;

	bool Init()
	{
		gameObjectArrayCapacity = 1;
		gameObjectCount = 0;
		gameObjects = (GameObject**)malloc(sizeof(GameObject*));
		if (gameObjects == NULL)
		{
			printf("GameData.Init(): Ran out of memory!\n");
			return false;
		}
		printf("GameData initialised\n");
		return true;
	}
};





//////////////////////////////////////////////////////////////////////////////////////
// DRAWING

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

// load all sprites
// returns true when successful
bool LoadAllBitmaps(Bitmaps* bmps)
{
	bool error = false;

	error |= !LoadBitmap(&bmps->charset, "./sprites/cs8x8.bmp");
	error |= !LoadBitmap(&bmps->playerCar, "./sprites/player_car.bmp");
	error |= !LoadBitmap(&bmps->enemyCar, "./sprites/enemy_car.bmp");
	error |= !LoadBitmap(&bmps->civilianCar, "./sprites/civilian_car.bmp");

	if (error)
		return false;

	SDL_SetColorKey(bmps->charset, true, 0x000000);
	return true;
}





//////////////////////////////////////////////////////////////////////////////////////
// MEMORY MANAGEMENT

void Instantiate(GameData* gameData, GameObject* gameObject)
{
	gameData->gameObjectCount++;
	if (gameData->gameObjectCount > gameData->gameObjectArrayCapacity)
	{
		gameData->gameObjectArrayCapacity *= 2;
		gameData->gameObjects = (GameObject**)realloc(gameData, sizeof(GameObject*) * gameData->gameObjectArrayCapacity);
		if (gameData->gameObjects == NULL)
		{
			printf("Ran out of memory while instantiating GameObject\n");
			exit(1);
		}
	}

	gameData->gameObjects[gameData->gameObjectCount - 1] = gameObject;
	printf("GameObject %d successfully instantiated\n", gameData->gameObjectCount - 1);
}

void FreeBitmaps(Bitmaps* bmps)
{
	SDL_FreeSurface(bmps->charset);
	SDL_FreeSurface(bmps->playerCar);
	SDL_FreeSurface(bmps->enemyCar);
	SDL_FreeSurface(bmps->civilianCar);
	free(bmps);
}





//////////////////////////////////////////////////////////////////////////////////////
// GAME MECHANICS

class Player : public GameObject
{
	Vector2 speed = {};

	virtual void Update(double delta, Input* input) override
	{
		
	}
};

class EnemyCar : public GameObject
{
	Vector2 speed = {};

	virtual void Update(double delta, Input* input) override
	{

	}
};

void GameInit(GameData* gameData)
{
	Instantiate(gameData, new Player);
}

void GameUpdate(double delta, GameData* gameData, Input* input)
{
	for (int i = 0; i < gameData->gameObjectCount; i++)
	{
		gameData->gameObjects[i]->Update(delta, input);
	}
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
// MAIN

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
	printf("printf output goes here:\n");

	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
	Input input = {};

	Bitmaps* bitmaps = (Bitmaps*)malloc(sizeof(Bitmaps));
	if (bitmaps == NULL)
		return 1;

	SDL_Event event;
	SDL_Surface* screen;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;


	GameData gameData;
	if (!gameData.Init())
		return 1;


	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// fullscreen mode
	//rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if (rc != 0)
	{
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Szablon do zdania drugiego 2017");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	SDL_ShowCursor(SDL_DISABLE);
	
	if (!LoadAllBitmaps(bitmaps))
	{
		FreeBitmaps(bitmaps);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}

	char text[128];
	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;

	GameInit(&gameData);

	while (!quit)
	{
		t2 = SDL_GetTicks();

		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		SDL_FillRect(screen, NULL, black);


		fpsTimer += delta;
		if (fpsTimer > 0.5)
		{
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		}


		GameUpdate(delta, &gameData, &input);



		// debug info
		//DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, red, blue);
		sprintf(text, "Elapsed time: %.1lfs, FPS: %.0lf ", worldTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, bitmaps->charset);
		sprintf(text, "Esc - exit, \030 - faster, \031 - slower");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, bitmaps->charset);




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
		frames++;
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
