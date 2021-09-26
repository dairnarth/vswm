#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

/* Defaults */
#define BORDER_WIDTH    1
#define BORDER_COLOUR   0xebdbb2
#define GAP_TOP         28
#define GAP_RIGHT       5
#define GAP_BOTTOM      5
#define GAP_LEFT        5

typedef struct Key Key;
typedef void (*Events)(XEvent *event);

struct Key {
    unsigned int modifier;
    KeySym keysym;
    void (*function)(XEvent *event, char *command);
    char *command;
};

/* Function Declarations */
static void size(void);
static void grab(void);
static void scan(void);
static void loop(void);

static void enter(XEvent *event);
static void configure(XEvent *event);
static void key(XEvent *event);
static void map(XEvent *event);

static void focus(XEvent *event, char *command);
static void launch(XEvent *event, char *command);
static void destroy(XEvent *event, char *command);
static void refresh(XEvent *event, char *command);
static void quit(XEvent *event, char *command);
static void fullscreen(XEvent *event, char *command);
static void remap(XEvent *event);

static int ignore(Display *display, XErrorEvent *event);

/* Variables */
static int screen, width, height;
static int running = 1;
static int isfullscreen = 0;
static int border_width = BORDER_WIDTH;
static int gap_top    = GAP_TOP;
static int gap_right  = GAP_RIGHT;
static int gap_bottom = GAP_BOTTOM;
static int gap_left   = GAP_LEFT;
static Display *display;
static Window root;

static Key keys[] = {
    /*  Modifiers,            Key,                        Function,         Arguments                   */
    {   Mod4Mask,             XK_t,                       launch,           "st -e tmux-default"        },
    {   Mod4Mask,             XK_w,                       launch,           "st -e setsid qutebrowser"  }, /* TODO: Fix actual problem (ERROR 10: No Child Processes) */
    {   Mod4Mask,             XK_space,                   launch,           "dmenu_run"                 },
    {   Mod4Mask,             XK_q,                       destroy,          0                           },
    {   Mod4Mask|ShiftMask,   XK_r,                       refresh,          0                           },
    {   Mod4Mask|ShiftMask,   XK_q,                       quit,             0                           },
    {   Mod4Mask|ShiftMask,   XK_x,                       launch,           "safe_shutdown"             },
    {   Mod4Mask,             XK_Tab,                     focus,            "next"                      },
    {   Mod4Mask|ShiftMask,   XK_Tab,                     focus,            "prev"                      },
    {   Mod4Mask,             XK_Return,                  fullscreen,       0                           },
    {   0,                    XF86XK_AudioMute,           launch,           "pamixer -t"                },
    {   0,                    XF86XK_AudioLowerVolume,    launch,           "pamixer -d 5"              },
    {   0,                    XF86XK_AudioRaiseVolume,    launch,           "pamixer -i 5"              },
    {   0,                    XF86XK_MonBrightnessDown,   launch,           "light -U 5"                },
    {   0,                    XF86XK_MonBrightnessUp,     launch,           "light -A 5"                },
};

static const Events events[LASTEvent] = {
    [ConfigureRequest] = configure,
    [EnterNotify] = enter,
    [KeyPress] = key,
    [MapRequest] = map,
};

void size(void)
{
    width = XDisplayWidth(display, screen) - \
            (gap_right + gap_left + (border_width * 2));
    height = XDisplayHeight(display, screen) - \
            (gap_top + gap_bottom + (border_width * 2));
}

void grab(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
        XGrabKey(display, XKeysymToKeycode(display, keys[i].keysym),
            keys[i].modifier, root, True, GrabModeAsync, GrabModeAsync);
}

void scan(void)
{
    unsigned int i, n;
    Window r, p, *c;

    if (XQueryTree(display, root, &r, &p, &c, &n)) {
        for (i = 0; i < n; i++)
            XMoveResizeWindow(display, c[i], gap_left, gap_top, width, height);

        if (c)
            XFree(c);
    }
}

void loop(void)
{
    XEvent event;

    while (running && !XNextEvent(display, &event))
        if (events[event.type])
            events[event.type](&event);
}

void enter(XEvent *event)
{
    Window window = event->xcrossing.window;

    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XRaiseWindow(display, window);
}

void configure(XEvent *event)
{
    XConfigureRequestEvent *request = &event->xconfigurerequest;
    XWindowChanges changes = {
        .x = request->x,
        .y = request->y,
        .width = request->width,
        .height = request->height,
        .border_width = request->border_width,
        .sibling = request->above,
        .stack_mode = request->detail,
    };

    XConfigureWindow(display, request->window, request->value_mask, &changes);
}

void key(XEvent *event)
{
    unsigned int i;
    KeySym keysym = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);

    for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
        if (keysym == keys[i].keysym && keys[i].modifier == event->xkey.state)
            keys[i].function(event, keys[i].command);
}

void map(XEvent *event)
{
    Window window = event->xmaprequest.window;
    XWindowChanges changes = { .border_width = border_width };

    XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
    XSetWindowBorder(display, window, BORDER_COLOUR);
    XConfigureWindow(display, window, CWBorderWidth, &changes);
    XMoveResizeWindow(display, window, gap_left, gap_top, width, height);
    XMapWindow(display, window);
}

void focus(XEvent *event, char *command)
{
    (void)event;
    int next = command[0] == 'n';

    XCirculateSubwindows(display, root, next ? RaiseLowest : LowerHighest);
    remap(event);
}

void launch(XEvent *event, char *command)
{
    (void)event;

    if (fork() == 0) {
        if (display)
            close(XConnectionNumber(display));

        setsid();
        execl("/bin/bash", "bash", "-c", command, 0);

        exit(1);
    }
}

void destroy(XEvent *event, char *command)
{
    (void)command;

    XSetCloseDownMode(display, DestroyAll);
    XKillClient(display, event->xkey.subwindow);
}

void refresh(XEvent *event, char *command)
{
    (void)event;
    (void)command;

    size();
    scan();
}

void quit(XEvent *event, char *command)
{
    (void)event;
    (void)command;

    running = 0;
}

void fullscreen(XEvent *event, char *command)
{
    (void)command;

    if (isfullscreen == 0) {
        border_width = 0;
        gap_top      = 0;
        gap_right    = 0;
        gap_bottom   = 0;
        gap_left     = 0;
        isfullscreen = 1;
    } else if (isfullscreen == 1) {
        border_width = BORDER_WIDTH;
        gap_top      = GAP_TOP;
        gap_right    = GAP_RIGHT;
        gap_bottom   = GAP_BOTTOM;
        gap_left     = GAP_LEFT;
        isfullscreen = 0;
    }
    remap(event);
}

void remap(XEvent *event)
{
    XMapRequestEvent *request = &event->xmaprequest;
    int revert;

    XGetInputFocus(display, &request->window, &revert);
    refresh(NULL, NULL);
    map(event);
}

int ignore(Display *display, XErrorEvent *event)
{
    (void)display;
    (void)event;

    return 0;
}

int main(void)
{
    if (!(display = XOpenDisplay(0)))
        exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(ignore);

    screen = XDefaultScreen(display);
    root = XDefaultRootWindow(display);

    XSelectInput(display, root, SubstructureRedirectMask);
    XDefineCursor(display, root, XCreateFontCursor(display, 68));

    size();
    grab();
    scan();
    loop();
}
