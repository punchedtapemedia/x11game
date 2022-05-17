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
	void redraw();

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
	int x_=10;
	int y_=10;
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
		gamedisplay_.drawRect(0x6091ab, x_,y_, 10,10);
	}

	if (event_.type == KeyPress)
	{
		printf("KeyPress Event: %d\n", event_.xkey.keycode);

		switch (event_.xkey.keycode)
		{
			case KEY_UP       : printf("KEY_UP\n");    y_ -= 2; gamedisplay_.redraw(); break;
			case KEY_DOWN     : printf("KEY_DOWN\n");  y_ += 2; gamedisplay_.redraw(); break;
			case KEY_LEFT     : printf("KEY_LEFT\n");  x_ -= 2; gamedisplay_.redraw(); break;
			case KEY_RIGHT    : printf("KEY_RIGHT\n"); x_ += 2; gamedisplay_.redraw(); break;

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
