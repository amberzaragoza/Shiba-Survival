/*
 * Pong Framework with a basic Seven Segment Display (SSD)
 * Author: Randi A.
 * To Compile : g++ pong.cpp -Wall -lrt -lX11 -lGL -o pong
 */
#include <bitset>   //bit string library
#include <string.h> //memset
#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>

using namespace std;

class pWindow {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	pWindow() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		XSetWindowAttributes swa;
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			std::cout << "\n\tcannot connect to X server" << std::endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			std::cout << "\n\tno appropriate visual found\n" << std::endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		swa.colormap = cmap;
		swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
			PointerMotionMask | MotionNotify | ButtonPress | ButtonReleaseMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, 960, 720, 0,
				vi->depth, InputOutput, vi->visual,
				CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	~pWindow() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	void set_title() {
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "Pong Lab - Introduction to Basic Game Physics");
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
	bool getXPending() {
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
};

struct Global {
    char keys[65536];
    int p1Score, p2Score;
    Global() {
        memset(keys, 0, 65536);
        p1Score = p2Score = 0;
    }
} G;

/* Notice the G after the closing curly brace and the semicolon in the Global
 * struct? What we're doing is declaring an instance of the Global struct as
 * global. It would similar if we declared it as "Global G;" outside and before
 * main().
 */

struct LeftBumper {
    float position[2];
    float velocity[2]; //Needed to add drift
    
    LeftBumper() {
        position[0] = 27;
        position[1] = 360;
        velocity[0] = 0;
        velocity[1] = 0;
    }
    void setPos(float x, float y) {
        position[0] = x;
        position[1] = y;
    }
    void drawBumper() {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
            glVertex2f(0.0f, 63.0f);
            glVertex2f(9.0f, 63.0f);
            glVertex2f(9.0f, 0.0f);
            glVertex2f(0.0f, 0.0f);
        glEnd();
    }
} LeftBumper;

struct RightBumper {
    float position[2];
    float velocity[2]; //Needed to add drift
    
    RightBumper() {
        position[0] = 933;
        position[1] = 360;
        velocity[0] = 0;
        velocity[1] = 0;
    }
    void setPos(float x, float y) {
        position[0] = x;
        position[1] = y;
    }
    void drawBumper() {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
            glVertex2f(0.0f, 63.0f);
            glVertex2f(9.0f, 63.0f);
            glVertex2f(9.0f, 0.0f);
            glVertex2f(0.0f, 0.0f);
        glEnd();
    }
} rightBumper;

struct Ball {
    float pos[2];
    float vel[2];

    Ball() {
        srand(time(0));
        vel[0] = (rand() % 3) + 5;
        vel[1] = (rand() % 3) + 5;

        pos[0] = 480;
        pos[1] = 360;
    }
    void drawBall() {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
            glVertex2f(-5.0f, 5.0f);
            glVertex2f(5.0f, 5.0f);
            glVertex2f(5.0f, -5.0f);
            glVertex2f(-5.0f, -5.0f);
        glEnd();
    }
    void resetBall() {
        vel[0] = (rand() % 3) + 5;
        vel[1] = (rand() % 3) + 5;

        pos[0] = 480;
        pos[1] = 360;
    }
    
} ball;

struct Barrier {
    float endPoint1[2];
    float endPoint2[2];
    Barrier() {
        endPoint1[0] = endPoint2[0] = 480.0f;
        endPoint1[1] = 0.0f;
        endPoint2[1] = 720.0f;
    }
    void drawBarrier() {
        //quick and dirty way of drawing a dashed line, do not recommand
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(12.0f);
        glPushAttrib(GL_ENABLE_BIT); 
            glLineStipple(9, 0x9999);
            glEnable(GL_LINE_STIPPLE);
            glBegin(GL_LINES);
                glVertex2f(endPoint1[0], endPoint1[1]);
                glVertex2f(endPoint2[0], endPoint2[1]);
            glEnd();
        glPopAttrib();
    }
} barrier;

struct SSD { //Seven Segment Display (SSD)
    //abcdefg - #
    //1111110 - 0
    //0110000 - 1
    //1101101 - 2
    //1111001 - 3
    //0110011 - 4
    //1011011 - 5
    //0011111 - 6
    //1110000 - 7
    //1111111 - 8
    //1110011 - 9
    bitset<7> screen;

    SSD() {
        screen = bitset<7>("1111110"); // displaying 0
    }
    //update the display, score > 9 will print display "E" for error
    void updateDisplay(int score) {
        switch(score) {
            case 1:
                screen = bitset<7>("0110000");
                break;
            case 2:
                screen = bitset<7>("1101101");
                break;
            case 3:
                screen = bitset<7>("1111001");
                break;
            case 4:
                screen = bitset<7>("0110011");
                break;
            case 5:
                screen = bitset<7>("1011011");
                break;
            case 6:
                screen = bitset<7>("0011111");
                break;
            case 7:
                screen = bitset<7>("1110000");
                break;
            case 8:
                screen = bitset<7>("1111111");
                break;
            case 9:
                screen = bitset<7>("1110011");
                break;
            default:
                screen = bitset<7>("1001111");
                break;
        }
    }
    void renderSSD() {
        if (screen[6]) { //a
            glBegin(GL_POLYGON);
                glVertex2f(0.0f, 100.0f);
                glVertex2f(0.0f, 110.0f);
                glVertex2f(40.0f, 110.0f);
                glVertex2f(40.0f, 100.0f);
            glEnd();
        }
        if (screen[5]) { //b
            glBegin(GL_POLYGON);
                glVertex2f(30.0f, 60.0f);
                glVertex2f(30.0f, 100.0f);
                glVertex2f(40.0f, 100.0f);
                glVertex2f(40.0f, 60.0f);
            glEnd();
        }
        if (screen[4]) { //c
            glBegin(GL_POLYGON);
                glVertex2f(30.0f, 10.0f);
                glVertex2f(30.0f, 50.0f);
                glVertex2f(40.0f, 50.0f);
                glVertex2f(40.0f, 10.0f);
            glEnd();
        }
        if (screen[3]) { //d
            glBegin(GL_POLYGON);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(0.0f, 10.0f);
                glVertex2f(40.0f, 10.0f);
                glVertex2f(40.0f, 0.0f);
            glEnd();
        }
        if (screen[2]) { //e
            glBegin(GL_POLYGON);
                glVertex2f(0.0f, 10.0f);
                glVertex2f(0.0f, 50.0f);
                glVertex2f(10.0f, 50.0f);
                glVertex2f(10.0f, 10.0f);
            glEnd();
        }
        if (screen[1]) { //f
            glBegin(GL_POLYGON);
                glVertex2f(0.0f, 60.0f);
                glVertex2f(0.0f, 100.0f);
                glVertex2f(10.0f, 100.0f);
                glVertex2f(10.0f, 60.0f);
            glEnd();
        }

        if (screen[0]) { //g       
            glBegin(GL_POLYGON);
                glVertex2f(0.0f, 50.0f);
                glVertex2f(0.0f, 60.0f);
                glVertex2f(40.0f, 60.0f);
                glVertex2f(40.0f, 50.0f);
            glEnd();
        }
    }
} ssd1, ssd2;

void initialize_OpenGL();
void check_key(XEvent *e);
void physics();
void render();

int main() 
{
	//play window
	pWindow pWin; //window instance
    initialize_OpenGL();
	while(1) {
        while (pWin.getXPending()) {
            XEvent e = pWin.getXNextEvent();
            check_key(&e);
        }
        physics();
        render();
        pWin.swapBuffers();

	}
	return 0;
}

void initialize_OpenGL() {
    glViewport(0, 0, 960, 720);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, 960, 0, 720, -1, 1);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //background color
}

void check_key(XEvent *e) {
    int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
    // if key is relased set to 0
    if (e->type == KeyRelease) {
        G.keys[key] = 0;
    }
    // if key is pressed set to 1
    if (e->type == KeyPress) {
        G.keys[key] = 1;
    }
}

void physics() {
    //start the ball moving
    ball.pos[0] += ball.vel[0];
    ball.pos[1] += ball.vel[1];
    //keys w and s for left bumper
    if (G.keys[XK_w]) {
        LeftBumper.position[1] += 3;
    }
    if (G.keys[XK_s]) {
        LeftBumper.position[1] -= 3;
    }
    //keys o and l for right bumper
    if (G.keys[XK_o]) {
        rightBumper.position[1] += 3;
    }
    if (G.keys[XK_l]) {
        rightBumper.position[1] -= 3;
    }
    //collision with top and bottom of game window
    if ((ball.pos[1] > 720) || (ball.pos[1] < 0)) {
        ball.vel[1] *= -1.0f;
    }
    //p2 scores
    if (ball.pos[0] < 0) {
        ball.resetBall();
        G.p2Score++;
        ssd2.updateDisplay(G.p2Score);
    }
    //p1 scores
    if (ball.pos[0] > 960) {
        ball.resetBall();
        G.p1Score++;
        ssd1.updateDisplay(G.p1Score);
    }
    //collisions with bumpers, you need to make sure you reset the the ball's 
    //position to either left or right of a bumper, so it won't get stuck if it's going to fast
    if (    //left bumper
        ((ball.pos[1] <= (LeftBumper.position[1] + 63.0f)) && //top y
        (ball.pos[1] >= (LeftBumper.position[1])) && //bottom y
        (ball.pos[0] >= (LeftBumper.position[0])) && //left x
        (ball.pos[0] <= (LeftBumper.position[0] + 9.0f))) //right x
        || //right bumper
        ((ball.pos[1] <= (rightBumper.position[1] + 63.0f)) && //top y
        (ball.pos[1] >= (rightBumper.position[1])) && //bottom y
        (ball.pos[0] >= (rightBumper.position[0])) && //left x
        (ball.pos[0] <= (rightBumper.position[0] + 9.0f))) //right x
        ) {
        //negation of the ball's x-velocity
        ball.vel[0] *= -1.0f;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT); //clear buffer so we don't get any weird artifacts
    //draw left bumper
    glPushMatrix();
    glTranslated(LeftBumper.position[0], LeftBumper.position[1], 0);
    LeftBumper.drawBumper();
    glPopMatrix();
    //draw right bumper
    glPushMatrix();
    glTranslated(rightBumper.position[0], rightBumper.position[1], 0);
    rightBumper.drawBumper();
    glPopMatrix();
    //draw ball
    glPushMatrix();
    glTranslated(ball.pos[0], ball.pos[1], 0);
    ball.drawBall();
    glPopMatrix();
    //draw barrier
    glPushMatrix();
    barrier.drawBarrier();
    glPopMatrix();
    //draw p1 ssd
    glPushMatrix();
    glTranslated(280.0f, 600.0f, 0.0f);
    ssd1.renderSSD();
    glPopMatrix();
    //draw p2 ssd
    glPushMatrix();
    glTranslated(680.0f, 600.0f, 0.0f);
    ssd2.renderSSD();
    glPopMatrix();
}