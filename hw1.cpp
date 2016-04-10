// Name: Alex Rinaldi
//cs335 Spring 2016 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
//
//. general animation framework
//. animation loop
//. object definition and movement
//. collision detection
//. mouse/keyboard interaction
//. object constructor
//. coding style
//. defined constants
//. use of static variables
//. dynamic memory allocation
//. simple opengl components
//. git
//
//elements we will add to program...
//. Game constructor
//. multiple particles
//. gravity
//. collision detection
//. more objects
//
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
extern "C" {
#include "fonts.h"
}

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define MAX_PARTICLES 800
#define GRAVITY 0.1

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;
static int savex = 0;
static int savey = 0;

//Structures

struct Vec {
    float x, y, z;
};

struct Shape {
    float width, height;
    float radius;
    float angle;
    Vec center;
    Vec color;
};

struct Particle {
    Shape s;
    Vec velocity;
};

struct Line {
    Vec endpoint[2];
};

struct Game {
    Shape box[5];
    Shape circle;
    Particle particle[MAX_PARTICLES];
    Line line;
    int n;
    bool bubbler;
};

const char step_text[5][255] = { 
    "Requirements",
    "Design",
    "Coding",
    "Testing",
    "Maintenance"
};


//Function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
int check_keys(XEvent *e, Game *game);
void movement(Game *game);
void render(Game *game);


void makeParticle(Game *game, int x, int y);

float radians_to_degrees(float rad)
{
    return rad * 180.0 / M_PI;
}

float degrees_to_radians(float deg)
{
    return deg * M_PI / 180.0;
}

void vec_add(const Vec &vec1, const Vec &vec2, Vec &out)
{
    out.x = vec1.x + vec2.x;
    out.y = vec1.y + vec2.y;
}

void vec_sub(const Vec &vec1, const Vec &vec2, Vec &out)
{
    out.x = vec2.x - vec1.x;
    out.y = vec2.y - vec1.y;
}

void vec_scale(Vec &vec, float scale, Vec &out)
{
    out.x = vec.x * scale;
    out.y = vec.y * scale;
}

float vec_magnitude(const Vec &vec)
{ 
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}

void vec_normalize(const Vec &vec, Vec &out)
{
    float magnitude = vec_magnitude(vec);
    out.x = vec.x / magnitude;
    out.y = vec.y / magnitude;
}

void vec_rotate(const Vec &vec, float angle, Vec &out)
{
    angle = degrees_to_radians(angle);
    out.x = vec.x * cos(angle) - vec.y * sin(angle);
    out.y = vec.x * sin(angle) + vec.y * cos(angle);
}

float vec_dot(const Vec &vec1, const Vec &vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

float vec_project(const Vec &vec1, const Vec &vec2)
{
    return vec_dot(vec1, vec2) / vec_magnitude(vec2);
}

float angle_between(const Vec &vec1, const Vec &vec2)
{
    return radians_to_degrees(acos(vec_dot(vec1, vec2) / (vec_magnitude(vec1) * vec_magnitude(vec2))));
}


int main(void)
{
    int done=0;
    srand(time(NULL));
    initXWindows();
    init_opengl();
    //declare game object
    Game game;
    game.n=0;
    game.bubbler = false;

    //declare a box shape
    for (int i = 0; i < 5; i++) {
        game.box[i].width = 100;
        game.box[i].height = 15;
        game.box[i].center.x = (i % 2) == 0 ? 280 : WINDOW_WIDTH - 280;
        game.box[i].center.y = 500 - i*80;
        game.box[i].angle = 0;
        game.box[i].color.x = 80;
        game.box[i].color.y = 0;
        game.box[i].color.z = 100;

    }


    game.circle.center.x = WINDOW_WIDTH / 2.0;
    game.circle.center.y = -100.0;
    game.circle.radius = 200.0;
    game.circle.color.x = 0;
    game.circle.color.y = 40;
    game.circle.color.z = 80;

    game.line.endpoint[0].x = 120 + 5 * 65;
    game.line.endpoint[0].y = 500 - 5 * 60;
    game.line.endpoint[1].x = 120 + 5 * 90;
    game.line.endpoint[1].y = 500 - 5 * 70;


    //game.box.angle = -30.0;

    //start animation
    while(!done) {
        while(XPending(dpy)) {
            XEvent e;
            XNextEvent(dpy, &e);
            check_mouse(&e, &game);
            done = check_keys(&e, &game);
        }
        if (game.bubbler) {
            for (int i = 0; i < 4; i++)
                makeParticle(&game, savex, WINDOW_HEIGHT - savey);

        }
        movement(&game);
        render(&game);
        glXSwapBuffers(dpy, win);
    }
    cleanupXWindows();
    return 0;
}

void set_title(void)
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "335 Lab1   LMB for particle");
}

void cleanupXWindows(void) {
    //do not change
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void initXWindows(void) {
    //do not change
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        std::cout << "\n\tcannot connect to X server\n" << std::endl;
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if(vi == NULL) {
        std::cout << "\n\tno appropriate visual found\n" << std::endl;
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
            InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void makeParticle(Game *game, int x, int y) {
    if (game->n >= MAX_PARTICLES)
        return;
    //position of particle
    Particle *p = &game->particle[game->n];
    p->s.center.x = x;
    p->s.center.y = y;

    int random = rand() % 100 + 155;
    p->s.color.x = random / 2;
    p->s.color.y = random / 2;
    p->s.color.z = random;

    p->velocity.y = rand() % 3 - 1.5;
    p->velocity.x =  rand() % 3 - 1.5;
    game->n++;
}

void check_mouse(XEvent *e, Game *game)
{
    if (game) {}
    static int n = 0;

    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button was pressed
            //int y = WINDOW_HEIGHT - e->xbutton.y;
        }
        if (e->xbutton.button==3) {
            //Right button was pressed
            return;
        }
    }
    //Did the mouse move?
    if (savex != e->xbutton.x || savey != e->xbutton.y) {
        savex = e->xbutton.x;
        savey = e->xbutton.y;
        if (++n < 10)
            return;
        //int y = WINDOW_HEIGHT - e->xbutton.y;
        //makeParticle(game, e->xbutton.x, y);
    }
}

int check_keys(XEvent *e, Game *game)
{
    //Was there input from the keyboard?
    if (e->type == KeyPress) {
        int key = XLookupKeysym(&e->xkey, 0);
        if (key == XK_Escape) {
            return 1;
        }
        else if (key == XK_b) {
            game->bubbler = !game->bubbler;
        }
        //You may check other keys here.

    }
    return 0;
}

void movement(Game *game)
{
    Particle *p;
    Shape *s;
    Vec vert, horz;
    Vec centerToParticle;

    if (game->n <= 0)
        return;

    //box updates
    for (int i = 0; i < 5; i++) { 
        s = &game->box[i];
        //angle - return to resting position
        s->angle += ( s->angle >= 0 ? -0.1 : 0.1);

        //color
        if (s->color.x > 100) {
            s->color.x -= 2;
            s->color.z -= 2;
        }
    }

    //circle updates
    s = &game->circle;
    if (s->color.z > 100) {
        s->color.y -= 2;
        s->color.z -= 2;
    }

    //particle updates
    for (int i = 0; i < game->n; i++) {
        p = &game->particle[i];
        p->s.center.x += p->velocity.x;
        p->s.center.y += p->velocity.y;

        p->velocity.y -= 0.3;


        //check for collision with circle
        s = &game->circle;
        vec_sub(s->center, p->s.center, centerToParticle);
        float dist = vec_magnitude(centerToParticle);
        if (dist < s->radius) {
            //Vec z; z.x = 0; z.y = 0; z.z = 0;

            //particle position
            vec_normalize(centerToParticle, centerToParticle);
            vec_scale(centerToParticle, s->radius - dist, centerToParticle);
            vec_add(p->s.center, centerToParticle, p->s.center);

            //particle velocity
            vec_normalize(centerToParticle, centerToParticle);
            vec_scale(centerToParticle, 2.0, centerToParticle);
            vec_add(p->velocity, centerToParticle, p->velocity);

            //color
            if (s->color.z < 150) {
                s->color.z += 5;
                s->color.y += 5;
            }
        }

        //check for collision with boxes...
        for (int j = 0; j < 5; j++) {
            s = &game->box[j];
            //s->angle += ( s->angle >= 0 ? -0.1 : 0.1);

            vert.x = 0; vert.y = s->height;
            horz.y = 0; horz.x = s->width;

            vec_rotate(vert, s->angle, vert);
            vec_rotate(horz, s->angle, horz);


            vec_sub(s->center, p->s.center, centerToParticle);

            float projectX = vec_project(centerToParticle, horz);
            float projectY = vec_project(centerToParticle, vert);

            //collision
            if (abs(projectX) < s->width
                    && abs(projectY) < s->height) {

                Vec dV,dP;

                // adjust particle position
                vec_normalize(vert, dP);
                vec_scale(dP, s->height - projectY, dP);
                vec_add(p->s.center, dP, p->s.center);

                // adjust particle velocity
                vec_normalize(vert, dV);
                vec_scale(dV, 2, dV);
                vec_add(p->velocity, dV, p->velocity);


                // rotate box
                if (abs(s->angle) < 15) {
                    s->angle += (projectX * -0.001);

                }

                //adjust box color
                if (s->color.x < 200) {
                    s->color.x += 5;
                    s->color.z += 5;
                }
            }
        }

        /*
           if (p->s.center.y <= s->center.y + s->height
           && p->s.center.y >= s->center.y - s->height
           && p->s.center.x <= s->center.x + s->width
           && p->s.center.x >= s->center.x - s->width)
           {
           p->s.center.y = s->center.y + s->height;
           p->velocity.y *= -1 * 0.1;
           }
         */
        //check for off-screen
        if (p->s.center.y < 0.0) {
            game->particle[i] = game->particle[game->n - 1];
            game->n--;
        }
    }




}

/* draw circle borrowed from "bump" framework */
void drawCircle(float rad)
{
    int i;
    static int firsttime=1;
    static float verts[32][2];
    static int n=32;
    if (firsttime) {
        float ang=0.0;
        float inc = 3.14159 * 2.0 / (float)n;
        for (i=0; i<n; i++) {
            verts[i][0] = sin(ang);
            verts[i][1] = cos(ang);
            ang += inc;
        }
        firsttime=0;
    }
    glBegin(GL_TRIANGLE_FAN);
    for (i=0; i<n; i++) {
        glVertex2f(verts[i][0]*rad, verts[i][1]*rad);
    }
    glEnd();
}

void render(Game *game)
{
    float w, h;
    glClear(GL_COLOR_BUFFER_BIT);


    Rect r;


    //title text
    r.bot = WINDOW_HEIGHT - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 20, 0x00ffffff, "Waterfall Model");

    //draw box
    Shape *s;

    s = &game->circle;

    glPushMatrix();
    glColor3ub(s->color.x, s->color.y, s->color.z);
    glTranslatef(s->center.x, s->center.y, s->center.z);
    drawCircle(s->radius);
    glPopMatrix();


    for (int i = 0; i < 5; i++) {
        s = &game->box[i];
        glColor3ub(s->color.x,s->color.y,s->color.z);
        glPushMatrix();
        glTranslatef(s->center.x, s->center.y, s->center.z);

        w = s->width;
        h = s->height;
        glRotatef(s->angle, 0, 0, 1);    
        glBegin(GL_QUADS);
        glVertex2i(-w,-h);
        glVertex2i(-w, h);
        glVertex2i( w, h);
        glVertex2i( w,-h);
        glEnd();

        r.bot = -s->height / 2.0;
        r.left = 0;
        r.center = 1;
        ggprint16(&r, 20, 0x005c0c5f, step_text[i]);

        glPopMatrix();
    }
    //draw all particles here
    for (int i = 0; i < game->n; i++) {
        glPushMatrix();

        Particle *p = &game->particle[i];

        glColor3ub(p->s.color.x, p->s.color.y, p->s.color.z);
        Vec *c = &game->particle[i].s.center;
        w = 4;
        h = 4;
        glBegin(GL_QUADS);
        glVertex2i(c->x-w, c->y-h);
        glVertex2i(c->x-w, c->y+h);
        glVertex2i(c->x+w, c->y+h);
        glVertex2i(c->x+w, c->y-h);
        glEnd();
        glPopMatrix();

    }

}



