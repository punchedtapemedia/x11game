/* 	x11game -- Demonstrates how to make a simple game in C++ using X11.

    Copyright (C) 2022 Punched Tape Media

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <X11/Xlib.h>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>

#define KEY_ESC 			9
#define KEY_RIGHT_ARROW 	114
#define KEY_LEFT_ARROW 		113
#define KEY_UP_ARROW 		111
#define KEY_DOWN_ARROW		116
#define KEY_LEFT_CTRL		37
#define KEY_RIGHT_CTRL  	105
#define KEY_LEFT_SHIFT  	50
#define KEY_RIGHT_SHIFT 	62
#define KEY_LEFT_ALT		64
#define KEY_RIGHT_ALT		108
#define KEY_SPACEBAR		65
#define KEY_NUM_LEFT_ARROW	83
#define KEY_NUM_RIGHT_ARROW	85
#define KEY_NUM_UP_ARROW	80
#define KEY_NUM_DOWN_ARROW	88

#define DIFFTM_MS(ST,EN) ((EN.tv_nsec - ST.tv_nsec)/1000000.0 + (EN.tv_sec - ST.tv_sec)*1000.0)

typedef struct {
	int x, y;
} Point;

typedef struct {
	int width, height;
} Size;

typedef struct {
	Point p;
	Size s;
} Rect;

bool isRectOverlapped(Point p1, Size s1, Point p2, Size s2)
{
	if (   ((p1.x          >= p2.x && p1.x          <= p2.x+s2.width) && (p1.y           >= p2.y && p1.y           < p2.y+s2.height ))
		|| ((p1.x          >= p2.x && p1.x          <= p2.x+s2.width) && (p1.y+s1.height >= p2.y && p1.y+s1.height < p2.y+s2.height ))
		|| ((p1.x+s1.width >= p2.x && p1.x+s1.width <= p2.x+s2.width) && (p1.y           >= p2.y && p1.y           < p2.y+s2.height ))
		|| ((p1.x+s1.width >= p2.x && p1.x+s1.width <= p2.x+s2.width) && (p1.y+s1.height >= p2.y && p1.y+s1.height < p2.y+s2.height )) )
		{
			return true;
		}
	return false;
}

class Player {
public:
	const Point INITIAL_POSITION = {50,50};
	const Size SIZE = {10, 10};
	Point position = INITIAL_POSITION;

	Player() {};
	~Player() {};
};

class Food {
public:
	const Size SIZE = {10, 10};
	const int COLOR[3] = {255,0,0};

	Point position = {0,0};
	bool eaten = false;

	Food() {};
	~Food() {};
};

class Ghost {
	struct timespec time_to_move;
	double delta_t_sec;

public:
	const Size SIZE = {10, 10};
	const int COLOR[3] = {0,0,255};

	Point position = {0,0};
	bool active = true;
	int speed = 4;

	Ghost() { delta_t_sec = 1.0 / (double)speed; clock_gettime(CLOCK_MONOTONIC, &time_to_move); };
	~Ghost() {};

	bool isTimeToMove();
	void updateTimeToMove();
};

void Ghost::updateTimeToMove()
{
	time_to_move.tv_nsec += delta_t_sec*1e9;
	while (time_to_move.tv_nsec > 1000000000L)
	{
		time_to_move.tv_nsec -= 1000000000L;
		time_to_move.tv_sec += 1;
	}
}

bool Ghost::isTimeToMove()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	printf("%s::%s::%d: dt=%lf ms\n", __FILE__, __FUNCTION__, __LINE__, DIFFTM_MS(now, time_to_move));
	if (DIFFTM_MS(now, time_to_move) < 0)
		return true;
	else
		return false;
}

class Game {
private:
	Display *display;
	int screen;
	Window window;
	int x11_fd;
	fd_set in_fds;
	XEvent e, ev;

	struct timeval tv;
	bool game_over = false;
	bool game_won = false;
	bool time_to_exit = false;
	bool redraw = false;
	std::vector<Food> food;
	std::vector<Ghost> ghosts;
	Player player;

	void initEventListener();
	void setupEventTimer();
	void waitForEvent();
	void createFood();
	void createGhosts();
	void drawSingleFood(Food &f);
	void drawAllFood();
	void drawSingleGhost(Ghost &g);
	void drawAllGhosts();
	void drawPlayer();
	void drawMessages();
	void handleEvents();
	void draw();
	void sendRedraw();
	void checkFoodEaten();
	void checkGhostContact();
	void updateGhosts();

public:
	Game();
	~Game();

	void run();
};

Game::Game()
{
	display = XOpenDisplay(NULL);
	if (display == NULL)
	{
		printf("%s::%s::%d: Unable to open display\n", __FILE__, __FUNCTION__, __LINE__);
		throw std::runtime_error("Failed to open display\n");
	}

	screen = DefaultScreen(display);
	printf("screen=%d\n", screen);

	srand(time(NULL));

	window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 500, 500, 1,
			BlackPixel(display,screen), WhitePixel(display,screen));

	XSelectInput(display, window, ExposureMask | KeyPressMask);
	XMapWindow(display, window);

	initEventListener();
	createFood();
	createGhosts();
}

Game::~Game()
{
	XCloseDisplay(display);
}

void Game::initEventListener()
{
	x11_fd = ConnectionNumber(display);
}

void Game::sendRedraw()
{
	if (redraw)
	{
		Window w_root;
		int x, y;
		unsigned int width, height, border_width, depth;
		XGetGeometry(display, window, &w_root, &x, &y, &width, &height, &border_width, &depth);
		XEvent ev;
		ev.xexpose.type = Expose;
		ev.xexpose.display = display;
		ev.xexpose.window = window;
		ev.xexpose.x = x;
		ev.xexpose.y = y;
		ev.xexpose.width = width;
		ev.xexpose.height = height;
		ev.xexpose.count = 0;
		XSendEvent(display,window,false, ExposureMask, &ev);
	}
}

void Game::drawSingleFood(Food &f)
{
	if (!f.eaten)
	{
		unsigned long col = (f.COLOR[0] << 16) + (f.COLOR[1] << 8) + f.COLOR[2];
		XSetForeground(display, DefaultGC(display,screen), col);
		XFillRectangle(display, window, DefaultGC(display,screen), f.position.x, f.position.y, f.SIZE.width, f.SIZE.height);
	}
}

void Game::drawSingleGhost(Ghost &g)
{
	if (g.active)
	{
		unsigned long col = (g.COLOR[0] << 16) + (g.COLOR[1] << 8) + g.COLOR[2];
		XSetForeground(display, DefaultGC(display,screen), col);
		XFillRectangle(display, window, DefaultGC(display,screen), g.position.x, g.position.y, g.SIZE.width, g.SIZE.height);
	}
}

void Game::drawAllFood()
{
	for (auto &f: food)
	{
		drawSingleFood(f);
	}
}

void Game::drawAllGhosts()
{
	for (auto &g: ghosts)
	{
		drawSingleGhost(g);
	}
}

void Game::updateGhosts()
{
	for (auto &g: ghosts)
	{
		if (g.active)
		{
			if (isRectOverlapped(player.position, player.SIZE, g.position, g.SIZE))
			{
				game_over = true;
				return;
			}

			if (g.isTimeToMove())
			{
				printf("Time to move Ghost\n");
				g.updateTimeToMove();

				switch (rand() % 4)
				{
				case 0: g.position.x += 10; break;
				case 1: g.position.x -= 10; break;
				case 2: g.position.y += 10; break;
				case 3: g.position.y -= 10; break;
				}

				//sendRedraw();
				redraw = true;
				printf("%s::%s::%d: Redraw=true\n", __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}
}

void Game::createFood()
{
	food.clear();
	food.resize(10);
	const int MAXX = 500, MAXY=500;

	for (auto &f:food)
	{
		f.position.x = (rand() % (MAXX/10) ) *10;
		f.position.y = (rand() % (MAXY/10) ) *10;
		f.eaten = false;
		//printf("%d,%d\n", f.x, f.y);
	}
}

void Game::createGhosts()
{
	ghosts.clear();
	ghosts.resize(10);
	const int MAXX = 500, MAXY=500;

	for (auto &g:ghosts)
	{
		g.position.x = (rand() % (MAXX/10) ) *10;
		g.position.y = (rand() % (MAXY/10) ) *10;
		g.active = true;
		g.speed = 1;
	}
}

void Game::setupEventTimer()
{
	FD_ZERO(&in_fds);
	FD_SET(x11_fd, &in_fds);

	tv.tv_sec = 0;
	tv.tv_usec = 100000L;
}

void Game::waitForEvent()
{
	setupEventTimer();

	int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);

	if (num_ready_fds > 0)
		printf("EVENT RECEIVED\n");
	else if (num_ready_fds == 0)
	{
		printf("Timer Fired!\n");
	}
	else
		printf("ERROR\n");
}

void Game::drawPlayer()
{
	unsigned long col = 0;
	XSetForeground(display, DefaultGC(display,screen), col);
	XFillRectangle(display,window,DefaultGC(display,screen),
			player.position.x, player.position.y, player.SIZE.width, player.SIZE.height);
}

void Game::drawMessages()
{
	if (game_over)
	{
		const char *game_over_win_msg = "GAME OVER!!! YOU WIN!!! PRESS SPACEBAR TO RESTART";
		const char *game_over_lose_msg = "GAME OVER!!! YOU LOSE!!! PRESS SPACEBAR TO RESTART";

		if (game_won)
			XDrawString(display,window,DefaultGC(display,screen), 10, 50, game_over_win_msg, strlen(game_over_win_msg));
		else
			XDrawString(display,window,DefaultGC(display,screen), 10, 50, game_over_lose_msg, strlen(game_over_lose_msg));
	}
}

void Game::draw()
{
	printf("Expose\n");
	XClearWindow(display, window);
	drawAllFood();
	drawAllGhosts();
	drawPlayer();
	drawMessages();
}

void Game::checkFoodEaten()
{
	bool all_eaten = true;
	for (auto &f: food)
	{
		if (!f.eaten)
		{
			if (isRectOverlapped(player.position, player.SIZE, f.position, f.SIZE))
			{
				f.eaten = true;
				//sendRedraw();
				redraw = true;
				printf("%s::%s::%d: Redraw=true\n", __FILE__, __FUNCTION__, __LINE__);
			}
			all_eaten = false;
		}
	}

	if (all_eaten)
	{
		game_over = true;
		game_won = true;
		//sendRedraw();
		redraw = true;
		printf("%s::%s::%d: Redraw=true\n", __FILE__, __FUNCTION__, __LINE__);
	}
}

void Game::handleEvents()
{
	while (XPending(display))
	{
		XNextEvent(display, &e);
		printf("Next Event Received: ");
		if (e.type == Expose)
			draw();

		if (e.type == KeyPress)
		{
			printf("Keypress\n");
			printf("Keycode = %u\n", e.xkey.keycode);

			switch (e.xkey.keycode)
			{
			case KEY_ESC : time_to_exit = true; break;
			case KEY_UP_ARROW : if (!game_over) {player.position.y-=10; redraw=true;} break;
			case KEY_DOWN_ARROW : if (!game_over) {player.position.y+=10; redraw=true;} break;
			case KEY_LEFT_ARROW : if (!game_over) {player.position.x-=10; redraw=true;} break;
			case KEY_RIGHT_ARROW : if (!game_over) {player.position.x+=10; redraw=true;} break;
			case KEY_SPACEBAR : if (game_over) {game_over = false; game_won = false; player.position = player.INITIAL_POSITION; createFood(); createGhosts(); redraw=true; } break;
			}

		}

		if (!game_over)
		{
			checkFoodEaten();

			Window w_root;
			int w_x, w_y;
			unsigned int w_width, w_height, w_border_width, w_depth;
			XGetGeometry(display, window, &w_root, &w_x, &w_y, &w_width, &w_height, &w_border_width, &w_depth);

			if ( (player.position.x < 0 || (player.position.x+player.SIZE.width) > w_width)
					|| (player.position.y < 0 || (player.position.y+player.SIZE.height) > w_height) )
			{
				game_over = true;
				//sendRedraw();
				redraw = true;
				printf("%s::%s::%d: Redraw=true\n", __FILE__, __FUNCTION__, __LINE__);
			}
		}
	}

	if (!game_over)
		updateGhosts();

}

void Game::run()
{
	while (!time_to_exit)
	{
		waitForEvent();
		handleEvents();
		sendRedraw();
	}
}

int main()
{
	Game g;

	g.run();

	return 0;
}
