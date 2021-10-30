#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* Defaults */
#define BARCLASS        "Bar"
#define BARCOMMAND      "$HOME/.config/lemonbar/lemonlaunch"
#define BORDER_WIDTH    1
#define BORDER_COLOUR   0xebdbb2
#define GAP_TOP         30
#define GAP_RIGHT       5
#define GAP_BOTTOM      5
#define GAP_LEFT        5

/* Structs */
typedef struct Key Key;
typedef void (*Events)(XEvent *event);

struct Key {
    unsigned int modifier;
    KeySym keysym;
    void (*function)(XEvent *event, char *command);
    char *command;
};

/* Function Declarations */
static int  barcheck(Window window);
static void barconfig(XEvent *event);
static void bardestroy(void);
static void barlaunch(void);
static void configure(XEvent *event);
static void destroy(XEvent *event, char *command);
static void enter(XEvent *event);
static void focus(XEvent *event, char *command);
static void fullscreen(XEvent *event, char *command);
static void grab(void);
static int  ignore(Display *display, XErrorEvent *event);
static void key(XEvent *event);
static void launch(XEvent *event, char *command);
static void loop(void);
static void map(XEvent *event);
static void quit(XEvent *event, char *command);
static void refresh(XEvent *event, char *command);
static void remap(XEvent *event);
static void scan(void);
static void setup(void);
static void size(void);
static void updatetitle(Window window);

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
static Window barwin;
static Atom netactivewindow, netsupported;

static Key keys[] = {
    /*  Modifiers,            Key,                        Function,         Arguments                   */
    {   Mod4Mask,             XK_t,                       launch,           "st -e tmux-default"       },
    {   Mod4Mask,             XK_w,                       launch,           "st -e setsid qutebrowser" }, /* TODO: Fix actual problem (ERROR 10: No Child Processes) */
    {   Mod4Mask,             XK_space,                   launch,           "dmenu_run -p 'RUN '"      },
    {   Mod4Mask,             XK_q,                       destroy,          0                          },
    {   Mod4Mask|ShiftMask,   XK_r,                       refresh,          0                          },
    {   Mod4Mask|ShiftMask,   XK_q,                       quit,             0                          },
    {   Mod4Mask|ShiftMask,   XK_x,                       launch,           "safe_shutdown"            },
    {   Mod4Mask,             XK_Tab,                     focus,            "next"                     },
    {   Mod4Mask|ShiftMask,   XK_Tab,                     focus,            "prev"                     },
    {   Mod4Mask,             XK_Return,                  fullscreen,       0                          },
    {   0,                    XF86XK_AudioMute,           launch,           "pamixer -t"               },
    {   0,                    XF86XK_AudioLowerVolume,    launch,           "pamixer -d 5"             },
    {   0,                    XF86XK_AudioRaiseVolume,    launch,           "pamixer -i 5"             },
    {   0,                    XF86XK_MonBrightnessDown,   launch,           "light -U 5"               },
    {   0,                    XF86XK_MonBrightnessUp,     launch,           "light -A 5"               },
};

static const Events events[LASTEvent] = {
    [ConfigureRequest] = configure,
    [EnterNotify] = enter,
    [KeyPress] = key,
    [MapRequest] = map,
};

/* Function Implementations */
int barcheck(Window window)
{
    char *class;
    XClassHint classhint = { NULL, NULL };

    XGetClassHint(display, window, &classhint);
    class = classhint.res_class;

    if (strcmp(class, BARCLASS) == 0) {
        return 0;
    } else {
        return 1;
    }

    if (classhint.res_class)
        XFree(classhint.res_class);
    if (classhint.res_name)
        XFree(classhint.res_name);
}

void barconfig(XEvent *event)
{
    barwin = event->xmaprequest.window;

    XMoveResizeWindow(display, barwin, 5, 5, 1268, 18);
}

void bardestroy(void)
{
    XSetCloseDownMode(display, DestroyAll);
    XKillClient(display, barwin);
}

void barlaunch(void)
{
    XEvent *event = { NULL };

    launch(event, BARCOMMAND);
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

void destroy(XEvent *event, char *command)
{
    (void)command;

    XSetCloseDownMode(display, DestroyAll);
    XKillClient(display, event->xkey.subwindow);
}

void enter(XEvent *event)
{
    Window window = event->xcrossing.window;

    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XRaiseWindow(display, window);
    remap(event);
    updatetitle(window);
}

void focus(XEvent *event, char *command)
{
    (void)event;
    int next = command[0] == 'n';

    XCirculateSubwindows(display, root, next ? RaiseLowest : LowerHighest);
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
        bardestroy();
    } else if (isfullscreen == 1) {
        border_width = BORDER_WIDTH;
        gap_top      = GAP_TOP;
        gap_right    = GAP_RIGHT;
        gap_bottom   = GAP_BOTTOM;
        gap_left     = GAP_LEFT;
        isfullscreen = 0;
        barlaunch();
    }
    remap(event);
}

void grab(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
        XGrabKey(display, XKeysymToKeycode(display, keys[i].keysym),
            keys[i].modifier, root, True, GrabModeAsync, GrabModeAsync);
}

int ignore(Display *display, XErrorEvent *event)
{
    (void)display;
    (void)event;

    return 0;
}

void key(XEvent *event)
{
    unsigned int i;
    KeySym keysym = XkbKeycodeToKeysym(display, event->xkey.keycode, 0, 0);

    for (i = 0; i < sizeof(keys) / sizeof(struct Key); i++)
        if (keysym == keys[i].keysym && keys[i].modifier == event->xkey.state)
            keys[i].function(event, keys[i].command);
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

void loop(void)
{
    XEvent event;

    while (running && !XNextEvent(display, &event))
        if (events[event.type])
            events[event.type](&event);
}

void map(XEvent *event)
{
    Window window = event->xmaprequest.window;
    XWindowChanges changes = { .border_width = border_width };

    XSelectInput(display, window, StructureNotifyMask | EnterWindowMask);
    XSetWindowBorder(display, window, BORDER_COLOUR);
    XConfigureWindow(display, window, CWBorderWidth, &changes);
    barcheck(window) ?
        XMoveResizeWindow(display, window, gap_left, gap_top, width, height) :
        barconfig(event);
    XMapWindow(display, window);
}

void quit(XEvent *event, char *command)
{
    (void)event;
    (void)command;

    running = 0;
}

void refresh(XEvent *event, char *command)
{
    (void)event;
    (void)command;

    size();
    scan();
}

void remap(XEvent *event)
{
    XMapRequestEvent *request = &event->xmaprequest;
    int revert;

    XGetInputFocus(display, &request->window, &revert);
    refresh(NULL, NULL);
    map(event);
}

void scan(void)
{
    unsigned int i, n;
    Window r, p, *c;

    if (XQueryTree(display, root, &r, &p, &c, &n)) {
        for (i = 0; i < n; i++)
            if (c[i] != barwin)
                XMoveResizeWindow(display, c[i], gap_left, gap_top, width, height);
        if (c)
            XFree(c);
    }
}

void setup(void)
{
    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(ignore);

    screen = XDefaultScreen(display);
    root = XDefaultRootWindow(display);

    XSelectInput(display, root, SubstructureRedirectMask);
    XDefineCursor(display, root, XCreateFontCursor(display, 68));

    netsupported    = XInternAtom(display, "_NET_SUPPORTED",     False);
    netactivewindow = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    XChangeProperty(display, root, netsupported, XA_ATOM, 32, PropModeReplace,
            (unsigned char *) &netactivewindow, 1);

    size();
    grab();
    scan();
    barlaunch();
}

void size(void)
{
    width = XDisplayWidth(display, screen) - \
            (gap_right + gap_left + (border_width * 2));
    height = XDisplayHeight(display, screen) - \
            (gap_top + gap_bottom + (border_width * 2));
}

void updatetitle(Window window)
{
    XEvent event;

    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = netactivewindow;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = CurrentTime;
    event.xclient.data.l[2] = window;

    XSendEvent(display, root, False,
        (SubstructureNotifyMask|SubstructureRedirectMask), &event);
    XChangeProperty(display, root, netactivewindow, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *) &window, 1);
}

int main(void)
{
    if (!(display = XOpenDisplay(0)))
        exit(1);

    setup();
    loop();
}
