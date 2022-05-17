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

#define KEY_ESCAPE     9
#define KEY_SPACEBAR  65
#define KEY_UP       111
#define KEY_RIGHT    114
#define KEY_DOWN     116
#define KEY_LEFT     113

namespace mygame {

class GameDisplay {
public:
	GameDisplay();
	~GameDisplay();

	Display *getDisplay();

	void drawRect(unsigned long col, int x, int y, int width, int height);

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

	window_ = XCreateSimpleWindow(display_, RootWindow(display_,screen_), 0, 0, 100, 100, 1,
                             BlackPixel(display_,screen_), WhitePixel(display_,screen_));

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

void GameDisplay::drawRect(unsigned long col, int x, int y, int width, int height)
{
	XSetForeground(display_, DefaultGC(display_,screen_), col);
	XFillRectangle(display_, window_, DefaultGC(display_,screen_), x,y, width, height);
}

class Game {
public:
	Game();

	void run();

private:
	GameDisplay gamedisplay_;
	XEvent event_;
	bool is_running_ = true;

	bool getEvent();
	void handleEvent();
};

Game::Game()
{
}

void Game::run()
{
	while (is_running_)
	{
		if (getEvent())
		{
			handleEvent();
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

void Game::handleEvent()
{
	if (event_.type == Expose)
	{
		gamedisplay_.drawRect(0x6091ab, 10,10, 20,40);
	}

	if (event_.type == KeyPress)
	{
		printf("KeyPress Event: %d\n", event_.xkey.keycode);

		switch (event_.xkey.keycode)
		{
			case KEY_UP       : printf("KEY_UP\n"); break;
			case KEY_RIGHT    : printf("KEY_RIGHT\n"); break;
			case KEY_DOWN     : printf("KEY_DOWN\n"); break;
			case KEY_LEFT     : printf("KEY_LEFT\n"); break;
			case KEY_SPACEBAR : printf("KEY_SPACEBAR\n"); break;
			case KEY_ESCAPE   : printf("KEY_ESCAPE\n");
							    is_running_ = false; break;
		}
	}
}

}

int main()
{
	mygame::Game g;

	g.run();

	return 0;
}
