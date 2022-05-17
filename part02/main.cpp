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

#define KEY_ESCAPE     9
#define KEY_SPACEBAR  65
#define KEY_UP       111
#define KEY_RIGHT    114
#define KEY_DOWN     116
#define KEY_LEFT     113

int main()
{
	Display *d = XOpenDisplay(NULL);
	if (d == NULL)
	{
		printf("Unable to open the display");
		return -1;
	}

	int s = DefaultScreen(d);

	Window w = XCreateSimpleWindow(d, RootWindow(d,s), 0, 0, 100, 100, 1,
                             BlackPixel(d,s), WhitePixel(d,s));

	XSelectInput(d, w, KeyPressMask);
	XMapWindow(d, w);


	XEvent e;
	bool done = false;

	while (!done)
	{
		if (XPending(d))
		{
			XNextEvent(d, &e);

			printf("EVENT: %d\n", e.type);

			if (e.type == KeyPress)
			{
				printf("KeyPress Event: %d\n", e.xkey.keycode);

				switch (e.xkey.keycode)
				{
					case KEY_UP       : printf("KEY_UP\n"); break;
					case KEY_RIGHT    : printf("KEY_RIGHT\n"); break;
					case KEY_DOWN     : printf("KEY_DOWN\n"); break;
					case KEY_LEFT     : printf("KEY_LEFT\n"); break;
					case KEY_SPACEBAR : printf("KEY_SPACEBAR\n"); break;
					case KEY_ESCAPE   : printf("KEY_ESCAPE\n"); done = true; break;
				}
			}
		}
	}

	printf("EXITING\n");

	XCloseDisplay(d);

	return 0;
}
