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
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <chrono>

#define KEY_ESCAPE     9
#define KEY_SPACEBAR  65
#define KEY_UP       111
#define KEY_RIGHT    114
#define KEY_DOWN     116
#define KEY_LEFT     113

namespace mygame {

class Time {
public:
    Time()
    {
        start_ = std::chrono::high_resolution_clock::now();
    }

    long time()
    {
        std::chrono::duration<long, std::nano> elap = std::chrono::high_resolution_clock::now() - start_;
        return elap.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

struct Point {
	int x, y;
};

struct Size {
	int width, height;
};

struct Rect {
	int x, y;
	int width, height;

	inline Point tl() const
	{
		return {std::min(x,x+width), std::min(y, y+height)};
	}
	inline Point br() const
	{
		return {std::max(x,x+width), std::max(y, y+height)};
	}
	inline Point tr() const
	{
		return {std::max(x,x+width), std::min(y, y+height)};
	}
	inline Point bl() const
	{
		return {std::min(x,x+width), std::max(y, y+height)};
	}
};

inline bool pointInRect(const Point &p, const Rect &r)
{
    return (   p.x >= r.tl().x && p.x <= r.br().x
            && p.y >= r.tl().y && p.y <= r.br().y);
}

inline bool inRange(int i, int min_i, int max_i)
{
    return (i >= min_i && i <= max_i);
}


bool rectangleIntersect(const Rect &r1, const Rect &r2)
{
    // Check 1 -- Any corner inside rect
    if ((pointInRect(r1.tl(), r2) || pointInRect(r1.br(), r2))
        || (pointInRect(r1.tr(), r2) || pointInRect(r1.bl(), r2)))
        return true;

    // Check 2 -- Overlapped, but all points outside
    //     +---+
    //  +--+---+----+
    //  |  |   |    |
    //  +--+---+----+
    //     +---+
	if ( (inRange(r1.tl().x, r2.tl().x, r2.br().x) || inRange(r1.br().x, r2.tl().x, r2.br().x))
		   && r1.tl().y < r2.tl().y && r1.br().y > r2.br().y
        || (inRange(r1.tl().y, r2.tl().y, r2.br().y) || inRange(r1.br().y, r2.tl().y, r2.br().y))
           && r1.tl().x < r2.tl().x && r1.br().x > r2.br().x )
		return true;

	return false;
}

class GameDisplay {
public:
	const int DEFAULT_WIDTH = 800;
	const int DEFAULT_HEIGHT = 600;
	GameDisplay();
	~GameDisplay();

	Display *getDisplay();

	void drawRect(unsigned long col, int x, int y, int width, int height) const;
	void redraw();
	Rect getGeometry();
    void drawText(int x, int y, const std::string &str) const;

private:
	Display *display_;
	int screen_;
	Window window_;
};

GameDisplay::GameDisplay()
{
	display_ = XOpenDisplay(NULL);
	if (display_ == NULL)
	{
		throw std::runtime_error("Unable to open the display");
	}

	screen_ = DefaultScreen(display_);

	window_ = XCreateSimpleWindow(display_, RootWindow(display_,screen_), 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, 1, 
                             BlackPixel(display_,screen_), 0x363d4d); //WhitePixel(display_,screen_));

	XSelectInput(display_, window_, KeyPressMask | ExposureMask );
	XMapWindow(display_, window_);
}

GameDisplay::~GameDisplay()
{
	XCloseDisplay(display_);
}

Display *GameDisplay::getDisplay()
{
	return display_;
}

void GameDisplay::drawRect(unsigned long col, int x, int y, int width, int height) const
{
	XSetForeground(display_, DefaultGC(display_,screen_), col);
	XFillRectangle(display_, window_, DefaultGC(display_,screen_), x,y, width, height);
}

void GameDisplay::redraw()
{
	XClearWindow(display_, window_);

	Window root_wind;
	int x, y;
	unsigned int width, height, border_width, depth;
	XGetGeometry(display_, window_, &root_wind, &x, &y, &width,
					  &height, &border_width, &depth);
					  
	XEvent ev;
	ev.xexpose.type = Expose;
	ev.xexpose.display = display_;
	ev.xexpose.window = window_;
	ev.xexpose.x = x;
	ev.xexpose.y = y;
	ev.xexpose.width = width;
	ev.xexpose.height = height;
	ev.xexpose.count = 0;
	
	XSendEvent(display_, window_, false, ExposureMask, &ev);
}

Rect GameDisplay::getGeometry()
{
	Window root_wind;
	int x, y;
	unsigned int width, height, border_width, depth;
	XGetGeometry(display_, window_, &root_wind, &x, &y, &width,
					  &height, &border_width, &depth);
					  
	Rect r;
  
	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;
  
	return r;
}

void GameDisplay::drawText(int x, int y, const std::string &str) const
{
    XDrawString(display_, window_, DefaultGC(display_, screen_), x, y, str.c_str(), str.size());
}


struct Character {
	unsigned long color = 0x6091ab;
	Point position {10, 10};
	Size size {10, 10};

	Character(unsigned long new_col, Point new_pos, Size new_sz)
	: color(new_col), position(new_pos), size(new_sz) {};
	
	Rect bounds() const
	{
		return {position.x, position.y, size.width, size.height};
	}
};

struct Player : public Character {
	Player() : Character(0x6091ab, {10,10}, {10,10}) {};
};

struct Food : public Character {
	Food() : Character(0xe0f731, {100, 100}, {10, 10}) {};
};

struct Ghost : public Character {
	Ghost() : Character(0xff0000, {100, 100}, {10, 10})
    {
        time_at_last_move_ns_ = time_.time();
    };

    void move()
    {
        int direction = std::rand() % 4;
        const int MOVE_DIST = 10;

        switch (direction)
        {
            case 0 : position.y -= MOVE_DIST; break;
            case 1 : position.y += MOVE_DIST; break;
            case 2 : position.x -= MOVE_DIST; break;
            case 3 : position.x += MOVE_DIST; break;
        }

        time_at_last_move_ns_ = time_.time();
    }

    bool isTimeToMove()
    {
        return ((time_.time() - time_at_last_move_ns_) >= move_time_ns_);
    }

    long time_at_last_move_ns_;
    long move_time_ns_ {250'000'000};
    Time time_;
};

class Game {
public:
	Game();

	void run();

private:
	GameDisplay gamedisplay_;
	XEvent event_;
	bool is_running_ = true;
    bool game_over = false;
    bool game_won = false;
	Player player_;
	std::vector<Food> food_;
	std::vector<Ghost> ghosts_;

	bool getEvent();
    void updateGhosts();
	void handleEvent();
    void resetGame();
	bool isPlayerWithinBounds();
	void drawPlayer();
	void draw();
	void createFood();
	void drawAllFood();
	void createGhosts();
	void drawAllGhosts();
    void drawMessage();
	void update();
	void drawCharacter(const Character &obj) const;
};

Game::Game()
{
	std::srand(std::time(nullptr));
	createFood();
	createGhosts();
}

void Game::run()
{
	while (is_running_)
	{
        if (!game_over)
            updateGhosts();

        if (getEvent())
		{
			handleEvent();
			if (!game_over && !isPlayerWithinBounds())
			{
				printf("PLAYER OUT OF BOUNDS -- GAME OVER!! -- YOU LOSE!!\n");
                game_over = true;
                game_won = false;
			}
		}
	}
}

bool Game::getEvent()
{
	if (XPending(gamedisplay_.getDisplay()))
	{
		XNextEvent(gamedisplay_.getDisplay(), &event_);
		printf("EVENT: %d\n", event_.type);
		return true;
	}

	return false;
}

void Game::drawPlayer()
{
	drawCharacter(player_);
}

void Game::draw()
{
	drawAllFood();
	drawAllGhosts();
	drawPlayer();
    drawMessage();
}

void Game::createFood()
{
	food_.clear();
	food_.resize(10);
	const int MAXX = 800;
	const int MAXY = 600;

	for (auto &f: food_)
	{
		f.position.x = (std::rand() % MAXX)/10*10;
		f.position.y = (std::rand() % MAXY)/10*10;
	}
}

void Game::drawAllFood()
{
	for (auto &f: food_)
	{
		drawCharacter(f);
	}
}

void Game::createGhosts()
{
	ghosts_.clear();
	ghosts_.resize(10);
	const int MAXX = 800;
	const int MAXY = 600;

	for (auto &g: ghosts_)
	{
		g.position.x = (std::rand() % MAXX)/10*10;
		g.position.y = (std::rand() % MAXY)/10*10;
	}
}

void Game::drawAllGhosts()
{
	for (auto &g: ghosts_)
	{
		drawCharacter(g);
	}
}

void Game::drawMessage()
{
    if (!game_over)
        return;

    if (game_won)
        gamedisplay_.drawText(100, 100, "YOU WIN!!  PRESS SPACEBAR TO RESTART...");
    else
        gamedisplay_.drawText(100, 100, "YOU LOSE!! PRESS SPACEBAR TO RESTART...");
}

void Game::update()
{
  	auto iter = std::find_if(food_.begin(), food_.end(), [&](const Food &f){
		return rectangleIntersect(player_.bounds(), f.bounds());
	});

	if (iter != food_.end())
	{
		food_.erase(iter);
	}

	if (food_.empty())
	{
		game_over = true;
        game_won = true;
	}
  	
	auto iter_ghosts = std::find_if(ghosts_.begin(), ghosts_.end(), [&](const Ghost &g){
		return rectangleIntersect(player_.bounds(), g.bounds());
	});

	if (iter_ghosts != ghosts_.end())
	{
        game_over = true;
		game_won = false;
		std::cout << "YOU LOSE!!\n";
	}
}

void Game::drawCharacter(const Character &obj) const
{
	gamedisplay_.drawRect(obj.color, 
		obj.position.x,
		obj.position.y,
		obj.size.width,
		obj.size.height);
}

void Game::updateGhosts()
{
    bool ghost_moved = false;
    for (auto &g: ghosts_)
    {
        if (g.isTimeToMove()) {
            g.move();
            ghost_moved = true;
        }
    }

    if (ghost_moved)
        gamedisplay_.redraw();
}

void Game::handleEvent()
{
	if (event_.type == Expose)
	{
		draw();
	}

	if (event_.type == KeyPress)
	{
		printf("KeyPress Event: %d\n", event_.xkey.keycode);

		switch (event_.xkey.keycode)
		{
			case KEY_UP       : printf("KEY_UP\n");    if (!game_over) { player_.position.y -= 10; gamedisplay_.redraw(); } break;
			case KEY_DOWN     : printf("KEY_DOWN\n");  if (!game_over) { player_.position.y += 10; gamedisplay_.redraw(); } break;
			case KEY_LEFT     : printf("KEY_LEFT\n");  if (!game_over) { player_.position.x -= 10; gamedisplay_.redraw(); } break;
			case KEY_RIGHT    : printf("KEY_RIGHT\n"); if (!game_over) { player_.position.x += 10; gamedisplay_.redraw(); } break;
			
			case KEY_SPACEBAR : printf("KEY_SPACEBAR\n"); if (game_over) resetGame(); break;

			case KEY_ESCAPE   : printf("KEY_ESCAPE\n"); 
							    is_running_ = false; break;
		}
		update();
	}
}

void Game::resetGame()
{
    player_.position = {10, 10};
    createFood();
    createGhosts();
    game_won = false;
    game_over = false;
}

bool Game::isPlayerWithinBounds()
{
	Rect w = gamedisplay_.getGeometry();
	
	if (   player_.position.x < 0 || player_.position.x > w.width 
		|| player_.position.y < 0 || player_.position.y > w.height)
	{
		return false;
	}
	
	return true;
}

}

int main()
{
	mygame::Game g;

	g.run();

	return 0;
}
