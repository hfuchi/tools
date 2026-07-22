// gcc -Wall -O2 -o wm26xft wm26xft.c -I/usr/include/freetype2 -lX11 -lXft -lfontconfig -lfreetype

//下記のコードをベースに、新しいアプリが起動した時に重ならないようにしてください。
//アプリがいっぱい起動して、配置場所がない場合には中央に配置してください。
// xtermが起動した時には期待したどおりに配置されますが、emacsは期待したどおりに配置されません。


//下記のコードをベースに、マウスポインタがアプリ(xtermやemacs)に入ると入力ポイントがアクティブになるようにしてください。アプリ自体が前面に来る必要はありません。

//下記のコードをベースに、アプリのタイトルバーをドラックして、左右のワークスペースに移動できるようにしてください。ドラックをはなすと移動が完了します。左右にドラックしている間はワークスペースを移動します。

//workspaceをEWMH拡張に対応してください。

// firefox で右クリックをするとメニューがカーソルから離れたところに出現します。firefox でメニューのファイルなどをクリックするとメニューがカーソルから離れたところに出現します。emacs(GTK)ではこの問題は起きません。下記のコードを修正してください。シェード時、アンシェード時にウィンドウのサイズが保たれるように注意してください。修正されたコード全体を表示してください。

//workspaceをEWMH拡張に対応してください。

//クローズボタンXの右上の線が欠けています。下記のコードを修正してください。修正されたコード全体を表示してください。


//どちらも違います。クローズボタンXの右上の線が欠けを指摘したところ、修正を行い右上の欠けはなくなりました。その修正では左下も補完しているようですが、まだ左下が線が欠けています。
// アプリゼロの場合、ショートカットキーが動作しない

// 起動したアプリのウィンドウのフォーカスが移るようにしてください。下記のコードを修正してください。修正されたコード全体を表示してください。

//KEY_k、KEY_m、KEY_n を定義しています。
//XK_k、XK_m, XK_n でショットカットを起動できるようにしていますが、
//else if で一つずつ実行しています。効率のよいコードに変更してください。

//フルスクリーン機能を追加してください。クローズボタンの横にフルスクリーンボタンを配置して、押下するとフルスクリーンになり、再度押下すると元に戻るようにてください。

// タイトルバーの文字をセンターに配置してください。下記のコードを修正してください。修正されたコード全体を表示してください。コメント部分はそのままにしてください。

//X11アプリケーション側でフルスクリーン状態になっても、実際にはフルスクリーンになりません。修正してください。たとえば、mplayer で F を押下してフルスクリーンにしますが、実際にはフルスクリーンになりません。

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DOUBLE_CLICK_THRESHOLD 300
#define TITLE_H 20
#define TITLE_BAR 0xa6a6a6
#define TITLE_CLOSE_BG 0xa6a6a6
#define TITLE_CLOSE_FG 0x000000
#define TITLE_FULL_FG 0x424242
#define BTN_SIZE 14
#define RESIZE_ZONE 20
#define NUM_WORKSPACES 4
#define FONT_NAME "VL Gothic:Regular:size=7"
#define TITLE_CENTER 

typedef struct {
    KeySym keysym;
    const char *cmd;
} Shortcut;

Shortcut shortcuts[] = {
    { XK_k, "/home/user/bin/terminal"},
    { XK_n, "/home/user/bin/FIREFOX" },
    { XK_m, "/home/user/bin/Emacs"   },
    { XK_r, "/home/user/bin/rec"   },
    { XK_s, "/home/user/bin/ss"   },        
};

typedef struct WinPair {
    Window client;
    Window frame;
    Window close_btn;
    Window full_btn; 
    int workspace;
    int is_dock;
    int is_sticky;
    int is_fullscreen; 
    int orig_x, orig_y, orig_w, orig_h; 
    struct WinPair *next;
} WinPair;

WinPair *window_head = NULL;
Atom atom_original_height;
Atom atom_net_wm_name;
Atom atom_utf8_string;
Atom atom_net_frame_extents;

Atom atom_net_number_of_desktops;
Atom atom_net_current_desktop;
Atom atom_net_wm_desktop;
Atom atom_wm_state;

// --- 追加: フルスクリーン関連のAtomをグローバルに ---
Atom atom_net_wm_state;
Atom atom_net_wm_state_fullscreen;

int current_workspace = 0;

static XftFont  *xft_font = NULL;
static XftColor  xft_fg;

int x_error_handler(Display *d, XErrorEvent *e) { return 0; }

int has_property(Display *display, Window w, Atom prop_atom, Atom target_atom) {
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *prop_data = NULL;
    int found = 0;

    if (XGetWindowProperty(display, w, prop_atom, 0, 1024, False, AnyPropertyType,
                           &type, &format, &nitems, &after, &prop_data) == Success && prop_data) {
        Atom *atoms = (Atom *)prop_data;
        for (unsigned long i = 0; i < nitems; i++) {
            if (atoms[i] == target_atom) {
                found = 1;
                break;
            }
        }
        XFree(prop_data);
    }
    return found;
}

void grab_key_robust(Display *display, Window root, KeySym keysym, unsigned int modifiers) {
    KeyCode code = XKeysymToKeycode(display, keysym);
    if (code == 0) return;

    unsigned int lock_mods[] = {0, Mod2Mask, LockMask, Mod2Mask | LockMask};

    for (int i = 0; i < 4; i++) {
        XGrabKey(display, code, modifiers | lock_mods[i], root,
                 False, GrabModeAsync, GrabModeAsync);
    }
}

void set_window_desktop(Display *display, Window client, long workspace) {
    XChangeProperty(display, client, atom_net_wm_desktop, XA_CARDINAL, 32, 
                    PropModeReplace, (unsigned char *)&workspace, 1);
}

int is_overlapping(Display *display, int x, int y, int w, int h) {
    WinPair *curr = window_head;
    while (curr) {
        if (!curr->is_dock && curr->frame != None && curr->workspace == current_workspace) {
            XWindowAttributes attr;
            if (XGetWindowAttributes(display, curr->frame, &attr)) {
                if (x < attr.x + attr.width && x + w > attr.x &&
                    y < attr.y + attr.height && y + h > attr.y) {
                    return 1;
                }
            }
        }
        curr = curr->next;
    }
    return 0;
}

int find_placement(Display *display, int win_w, int win_h, int *out_x, int *out_y) {
    int screen_w = DisplayWidth(display, DefaultScreen(display));
    int screen_h = DisplayHeight(display, DefaultScreen(display));
    int step = 10;

    for (int y = 0; y <= screen_h - win_h; y += step) {
        for (int x = 0; x <= screen_w - win_w; x += step) {
            if (!is_overlapping(display, x, y, win_w, win_h)) {
                *out_x = x;
                *out_y = y;
                return 1;
            }
        }
    }
    return 0;
}

static char *fetch_title(Display *display, Window w) {
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(display, w, atom_net_wm_name, 0, 1024, False,
                           atom_utf8_string, &type, &format,
                           &nitems, &after, &prop) == Success && prop) {
        char *name = strdup((char *)prop);
        XFree(prop);
        return name;
    }

    char *legacy = NULL;
    if (XFetchName(display, w, &legacy) && legacy) {
        char *name = strdup(legacy);
        XFree(legacy);
        return name;
    }

    return strdup("Untitled");
}

void draw_title(Display *display, WinPair *wp) {
    if (wp->is_dock || !xft_font) return;
    XClearWindow(display, wp->frame);

    char *name = fetch_title(display, wp->client);
    int len = strlen(name);

    XftDraw *draw = XftDrawCreate(
        display, wp->frame,
        DefaultVisual(display, DefaultScreen(display)),
        DefaultColormap(display, DefaultScreen(display)));

    XWindowAttributes frame_attr;
    XGetWindowAttributes(display, wp->frame, &frame_attr);

    XGlyphInfo extents;
    XftTextExtentsUtf8(display, xft_font, (FcChar8 *)name, len, &extents);

    int y = (TITLE_H + xft_font->ascent - xft_font->descent) / 2;
    int x = (frame_attr.width - extents.width) / 2;

    int min_x = BTN_SIZE * 2 + 12;
    if (x < min_x) x = min_x;

#ifdef TITLE_CENTER
    XftDrawStringUtf8(draw, &xft_fg, xft_font, x, y, (FcChar8 *)name, len);
#else
    XftDrawStringUtf8(draw, &xft_fg, xft_font, BTN_SIZE * 2 + 12, y, (FcChar8 *)name, len);
#endif	

    XftDrawDestroy(draw);
    free(name);
}

void draw_close_btn(Display *display, Window button, int size) {
    GC gc = XCreateGC(display, button, 0, NULL);
    XSetForeground(display, gc, TITLE_CLOSE_FG);
    XSetLineAttributes(display, gc, 1, LineSolid, CapProjecting, JoinMiter);
    XDrawLine(display, button, gc, 4, 4, size - 5, size - 5);
    XDrawLine(display, button, gc, size - 5, 4, 4, size - 5);
    XFreeGC(display, gc);
}

void draw_full_btn(Display *display, Window button, int size) {
    GC gc = XCreateGC(display, button, 0, NULL);
    XSetForeground(display, gc, TITLE_FULL_FG);
    XSetLineAttributes(display, gc, 1, LineSolid, CapProjecting, JoinMiter);
    XDrawRectangle(display, button, gc, 3, 3, size - 7, size - 7);
    XFreeGC(display, gc);
}

WinPair* find_window(Window w) {
    WinPair *curr = window_head;
    while (curr) {
        if (curr->client == w || curr->frame == w || curr->close_btn == w || curr->full_btn == w) return curr;
        curr = curr->next;
    }
    return NULL;
}

void send_configure_notify(Display *display, WinPair *wp) {
    if (!wp || wp->is_dock || wp->frame == None || wp->client == None) return;

    XWindowAttributes client_attr;
    if (!XGetWindowAttributes(display, wp->client, &client_attr)) return;

    int root_x = 0, root_y = 0;
    Window child;
    XTranslateCoordinates(display, wp->frame, DefaultRootWindow(display),
                          0, TITLE_H, &root_x, &root_y, &child);

    XConfigureEvent ce;
    ce.type = ConfigureNotify;
    ce.display = display;
    ce.event = wp->client;
    ce.window = wp->client;
    
    ce.x = root_x;
    ce.y = root_y;
    ce.width = client_attr.width;
    ce.height = client_attr.height;
    ce.border_width = client_attr.border_width;
    ce.above = None;
    ce.override_redirect = False;

    XSendEvent(display, wp->client, False, StructureNotifyMask, (XEvent *)&ce);
}

// --- 追加: フルスクリーン切り替えの共通関数 ---
void set_fullscreen(Display *display, WinPair *wp, int state) {
    if (!wp || wp->is_dock) return;
    int screen_w = DisplayWidth(display, DefaultScreen(display));
    int screen_h = DisplayHeight(display, DefaultScreen(display));

    // state: 0=Remove, 1=Add, 2=Toggle
    int to_fullscreen = (state == 2) ? !wp->is_fullscreen : state;

    if (to_fullscreen && !wp->is_fullscreen) {
        XWindowAttributes attr;
        XGetWindowAttributes(display, wp->frame, &attr);
        wp->orig_x = attr.x;
        wp->orig_y = attr.y;
        wp->orig_w = attr.width;
        wp->orig_h = attr.height;

        wp->is_fullscreen = 1;
        // フレームを画面上部に押し出し、タイトルバーを画面外に隠す
        XMoveResizeWindow(display, wp->frame, 0, -TITLE_H, screen_w, screen_h + TITLE_H);
        XResizeWindow(display, wp->client, screen_w, screen_h);
        XRaiseWindow(display, wp->frame);

        // プロパティを更新
        XChangeProperty(display, wp->client, atom_net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&atom_net_wm_state_fullscreen, 1);
    } else if (!to_fullscreen && wp->is_fullscreen) {
        wp->is_fullscreen = 0;
        XMoveResizeWindow(display, wp->frame, wp->orig_x, wp->orig_y, wp->orig_w, wp->orig_h);
        XResizeWindow(display, wp->client, wp->orig_w, wp->orig_h - TITLE_H);
        
        // プロパティを削除
        XDeleteProperty(display, wp->client, atom_net_wm_state);
    }
    send_configure_notify(display, wp);
}

void switch_workspace(Display *display, int next_ws) {
    if (next_ws == current_workspace) return;
    WinPair *curr = window_head;
    while (curr) {
        if (!curr->is_sticky && !curr->is_dock) {
            if (curr->workspace == next_ws) {
                XMapWindow(display, curr->frame);
            } else if (curr->workspace == current_workspace) {
                XUnmapWindow(display, curr->frame);
            }
        } else {
            XRaiseWindow(display, curr->frame);
        }
        curr = curr->next;
    }
    current_workspace = next_ws;
    fprintf(stderr, "ws '%d' \n", current_workspace);

    long cur_desk = current_workspace;
    XChangeProperty(display, DefaultRootWindow(display), atom_net_current_desktop, 
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&cur_desk, 1);

    XSetInputFocus(display, DefaultRootWindow(display), RevertToPointerRoot, CurrentTime);
    XFlush(display);
}

static void init_xft(Display *display) {
    int screen = DefaultScreen(display);
    Visual *vis = DefaultVisual(display, screen);
    Colormap cm = DefaultColormap(display, screen);

    xft_font = XftFontOpenName(display, screen, FONT_NAME);
    if (!xft_font) xft_font = XftFontOpenName(display, screen, "fixed");
    if (!xft_font) exit(1);

    XRenderColor black = { 0x0000, 0x0000, 0x0000, 0xC000 };
    XftColorAllocValue(display, vis, cm, &black, &xft_fg);  
}

void spawn(Display *display, const char *cmd) {
    if (fork() == 0) {
        if (display) close(ConnectionNumber(display));
        setsid();
        execl(cmd, cmd, NULL);
        exit(1);
    }
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) return 1;

    XSetErrorHandler(x_error_handler);
    Window root = DefaultRootWindow(display);

    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);

    XUngrabKey(display, AnyKey, AnyModifier, root);
    grab_key_robust(display, root, XK_F1,  0);
    grab_key_robust(display, root, XK_F2, 0);
    grab_key_robust(display, root, XK_k, Mod1Mask | ControlMask);
    grab_key_robust(display, root, XK_n, Mod1Mask | ControlMask);
    grab_key_robust(display, root, XK_m, Mod1Mask | ControlMask);
    
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    atom_original_height  = XInternAtom(display, "_MYWM_ORIGINAL_HEIGHT", False);
    atom_net_wm_name      = XInternAtom(display, "_NET_WM_NAME", False);
    atom_utf8_string      = XInternAtom(display, "UTF8_STRING", False);
    atom_net_frame_extents = XInternAtom(display, "_NET_FRAME_EXTENTS", False);

    // --- 変更: グローバル変数への代入 ---
    atom_net_wm_state             = XInternAtom(display, "_NET_WM_STATE", False);
    atom_net_wm_state_fullscreen  = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    Atom net_wm_state_sticky      = XInternAtom(display, "_NET_WM_STATE_STICKY", False);
    Atom net_wm_window_type       = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock  = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    Atom net_supported            = XInternAtom(display, "_NET_SUPPORTED", False);

    atom_net_number_of_desktops = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", False);
    atom_net_current_desktop    = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    atom_net_wm_desktop         = XInternAtom(display, "_NET_WM_DESKTOP", False);

    atom_wm_state = XInternAtom(display, "WM_STATE", False);

    Atom supported[] = {
        atom_net_wm_name,
        atom_net_wm_state, // 変更
        net_wm_state_sticky,
        net_wm_window_type,
        net_wm_window_type_dock,
        atom_net_frame_extents,
        atom_net_number_of_desktops,
        atom_net_current_desktop,    
        atom_net_wm_desktop,
        atom_net_wm_state_fullscreen // 追加
    };
    XChangeProperty(display, root, net_supported, XA_ATOM, 32, PropModeReplace, (unsigned char *)supported, 10);

    long num_desks = NUM_WORKSPACES;
    XChangeProperty(display, root, atom_net_number_of_desktops, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&num_desks, 1);
    long cur_desk = current_workspace;
    XChangeProperty(display, root, atom_net_current_desktop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&cur_desk, 1);

    init_xft(display);
    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);

    XEvent ev;
    Window grab_window = None;
    Window resize_window = None;
    int start_x = 0, start_y = 0, win_x = 0, win_y = 0, win_w = 0, win_h = 0;
    Time last_click_time = 0;
    Window last_click_window = None;
    int resize_client_is_viewable = 1, resize_client_h = 0;

    while (1) {
        XNextEvent(display, &ev);

        if (ev.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);
            if (keysym == XK_F1) switch_workspace(display, (current_workspace - 1 + NUM_WORKSPACES) % NUM_WORKSPACES);
            else if (keysym == XK_F2) switch_workspace(display, (current_workspace + 1) % NUM_WORKSPACES);
            for (int i = 0; i < sizeof(shortcuts)/sizeof(shortcuts[0]); i++) {
                if (keysym == shortcuts[i].keysym) { spawn(display, shortcuts[i].cmd); break; }
            }
        }
        else if (ev.type == Expose && ev.xexpose.count == 0) {
            WinPair *wp = find_window(ev.xexpose.window);
            if (wp && !wp->is_dock) {
                if (ev.xexpose.window == wp->close_btn) draw_close_btn(display, wp->close_btn, BTN_SIZE);
                else if (ev.xexpose.window == wp->full_btn) draw_full_btn(display, wp->full_btn, BTN_SIZE);
                else if (ev.xexpose.window == wp->frame) draw_title(display, wp);
            }
        }
        else if (ev.type == PropertyNotify && (ev.xproperty.atom == XA_WM_NAME || ev.xproperty.atom == atom_net_wm_name)) {
            WinPair *wp = find_window(ev.xproperty.window);
            if (wp && ev.xproperty.window == wp->client && !wp->is_dock) draw_title(display, wp);
        }
        else if (ev.type == DestroyNotify) {
            WinPair **curr = &window_head;
            while (*curr) {
                if ((*curr)->client == ev.xdestroywindow.window) {
                    WinPair *to_delete = *curr;
                    if (!to_delete->is_dock) XDestroyWindow(display, to_delete->frame);
                    *curr = to_delete->next;
                    free(to_delete);
                    XSetInputFocus(display, DefaultRootWindow(display), RevertToPointerRoot, CurrentTime);
                    break;
                }
                curr = &((*curr)->next);
            }
        }
        else if (ev.type == ConfigureRequest) {
            XConfigureRequestEvent *ce = &ev.xconfigurerequest;
            WinPair *wp = find_window(ce->window);

            if (wp && !wp->is_dock) {
				Window r, p, *c = NULL; unsigned int nc;
				if (XQueryTree(display, ce->window, &r, &p, &c, &nc) != 0) {
					if (c) XFree(c);
				}
				/*
                Window r, p, *c; unsigned int nc;
                XQueryTree(display, ce->window, &r, &p, &c, &nc);
                if (c) XFree(c);
				*/

                if (p != root && p != None) {
                    XWindowAttributes frame_attr;
                    XGetWindowAttributes(display, p, &frame_attr);

                    XMoveResizeWindow(display, p, frame_attr.x, frame_attr.y, ce->width, ce->height + TITLE_H);
                    XMoveResizeWindow(display, ce->window, 0, TITLE_H, ce->width, ce->height);
                    send_configure_notify(display, wp);
                } else {
                    XWindowChanges changes = {ce->x, ce->y, ce->width, ce->height, ce->border_width, ce->above, ce->detail};
                    XConfigureWindow(display, ce->window, ce->value_mask, &changes);
                }
            } else {
                XWindowChanges changes = {ce->x, ce->y, ce->width, ce->height, ce->border_width, ce->above, ce->detail};
                XConfigureWindow(display, ce->window, ce->value_mask, &changes);
            }
        }
        else if (ev.type == MapRequest) {
            Window client_w = ev.xmaprequest.window;
            XWindowAttributes attr;
            XGetWindowAttributes(display, client_w, &attr);

            if (attr.override_redirect) { XMapWindow(display, client_w); continue; }

			Window r, p, *c = NULL; unsigned int nc;
			if (XQueryTree(display, client_w, &r, &p, &c, &nc) != 0) {
				if (c) XFree(c);
			}
/*
            Window r, p, *c; unsigned int nc;
            XQueryTree(display, client_w, &r, &p, &c, &nc);
            if (c) XFree(c);
*/
            if (p != root) { XMapWindow(display, client_w); continue; }

            int is_dock   = has_property(display, client_w, XInternAtom(display, "_NET_WM_WINDOW_TYPE", False), XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False));
            int is_sticky = has_property(display, client_w, atom_net_wm_state, net_wm_state_sticky);

            int assigned_ws = current_workspace;
            Atom type; int format; unsigned long nitems, after; unsigned char *prop = NULL;
            if (XGetWindowProperty(display, client_w, atom_net_wm_desktop, 0, 1, False, XA_CARDINAL, &type, &format, &nitems, &after, &prop) == Success && prop) {
                long desk = *(long *)prop;
                if (desk == 0xFFFFFFFF) is_sticky = 1;
                else if (desk >= 0 && desk < NUM_WORKSPACES) assigned_ws = desk;
                XFree(prop);
            }

            int frame_w_size = attr.width;
            int frame_h_size = attr.height + TITLE_H;
            int place_x = attr.x, place_y = attr.y;

            if (!is_dock && !find_placement(display, frame_w_size, frame_h_size, &place_x, &place_y)) {
                int screen_w = DisplayWidth(display, DefaultScreen(display));
                int screen_h = DisplayHeight(display, DefaultScreen(display));
                place_x = (screen_w - frame_w_size) / 2;
                place_y = (screen_h - frame_h_size) / 2;
                if (place_x < 0) place_x = 0;
                if (place_y < 0) place_y = 0;
            }

            if (place_x == 0) place_x = 10;
            if (place_y == 0) place_y = 30;

            WinPair *new_win = (WinPair *)malloc(sizeof(WinPair));
            new_win->client = client_w; new_win->workspace = assigned_ws;
            new_win->is_dock = is_dock; new_win->is_sticky = is_sticky;
            new_win->is_fullscreen = 0; 
            new_win->next = window_head; window_head = new_win;

            set_window_desktop(display, client_w, is_sticky ? 0xFFFFFFFF : assigned_ws);
            long wm_state[] = { 1, None };
            XChangeProperty(display, client_w, atom_wm_state, atom_wm_state, 32, PropModeReplace, (unsigned char *)wm_state, 2);

            if (is_dock) {
                new_win->frame = client_w; new_win->close_btn = None; new_win->full_btn = None;
                XSelectInput(display, client_w, StructureNotifyMask | PropertyChangeMask);
                XMapWindow(display, client_w); XRaiseWindow(display, client_w);
            } else {
                Window frame_w = XCreateSimpleWindow(display, root, place_x, place_y, frame_w_size, frame_h_size, 0, 0, TITLE_BAR);
                Window close_btn = XCreateSimpleWindow(display, frame_w, 3, 3, BTN_SIZE, BTN_SIZE, 0, 0, TITLE_CLOSE_BG);
                Window full_btn = XCreateSimpleWindow(display, frame_w, 3 + BTN_SIZE + 3, 3, BTN_SIZE, BTN_SIZE, 0, 0, TITLE_CLOSE_BG);

                new_win->frame = frame_w; new_win->close_btn = close_btn; new_win->full_btn = full_btn;

                long extents[4] = {0, 0, TITLE_H, 0}; 
                XChangeProperty(display, client_w, atom_net_frame_extents, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)extents, 4);

                XSelectInput(display, client_w, StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
                XGrabButton(display, Button1, AnyModifier, client_w, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
                XSelectInput(display, frame_w, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | SubstructureRedirectMask | ExposureMask | EnterWindowMask);
                XSelectInput(display, close_btn, ButtonPressMask | ExposureMask);
                XSelectInput(display, full_btn, ButtonPressMask | ExposureMask); 
                
                XReparentWindow(display, client_w, frame_w, 0, TITLE_H);
                
                if (is_sticky || assigned_ws == current_workspace) XMapWindow(display, frame_w);
                XMapWindow(display, close_btn); XMapWindow(display, full_btn); XMapWindow(display, client_w);
                
                send_configure_notify(display, new_win);

                if (is_sticky || assigned_ws == current_workspace) {
                    XRaiseWindow(display, frame_w); XSetInputFocus(display, client_w, RevertToParent, CurrentTime);
                }
            }
            XFlush(display);
        }
        else if (ev.type == ButtonPress && ev.xbutton.button == 1) {
            Window clicked_w = ev.xbutton.window;
            Time current_time = ev.xbutton.time;

            WinPair *wp = find_window(clicked_w);
            if (wp) {
                XRaiseWindow(display, wp->frame);
                XSetInputFocus(display, wp->client, RevertToParent, current_time);

                if (clicked_w == wp->client) {
                    XAllowEvents(display, ReplayPointer, current_time);
                }
                else if (!wp->is_dock && clicked_w == wp->close_btn) {
                    XClientMessageEvent xev = {
                        .type = ClientMessage, .window = wp->client, .format = 32,
                        .message_type = XInternAtom(display, "WM_PROTOCOLS", True)
                    };
                    xev.data.l[0] = wm_delete_window;
                    XSendEvent(display, wp->client, False, NoEventMask, (XEvent *)&xev);
                }
                // --- 変更: 共通関数を使ってフルスクリーンを切り替える ---
                else if (!wp->is_dock && clicked_w == wp->full_btn) {
                    set_fullscreen(display, wp, 2); // 2: Toggle
                }
                else if (!wp->is_dock && clicked_w == wp->frame) {
                    if (!wp->is_fullscreen) {
                        XWindowAttributes attr;
                        XGetWindowAttributes(display, clicked_w, &attr);

                        if (ev.xbutton.x > attr.width - RESIZE_ZONE) {
                            resize_window = clicked_w;
                            start_x = ev.xbutton.x_root; start_y = ev.xbutton.y_root;
                            win_w = attr.width; win_h = attr.height; win_x = attr.x; win_y = attr.y;

                            XWindowAttributes c_attr;
                            if (XGetWindowAttributes(display, wp->client, &c_attr)) {
                                resize_client_is_viewable = (c_attr.map_state == IsViewable);
                                resize_client_h = c_attr.height;
                            } else {
                                resize_client_is_viewable = 1; resize_client_h = 0;
                            }

                        } else if (clicked_w == last_click_window && (current_time - last_click_time) < DOUBLE_CLICK_THRESHOLD) {
                            XWindowAttributes c_attr; XGetWindowAttributes(display, wp->client, &c_attr);

                            if (c_attr.map_state == IsViewable) {
                                long h = c_attr.height;
                                XChangeProperty(display, wp->client, atom_original_height, XA_INTEGER, 32, PropModeReplace, (unsigned char *)&h, 1);
                                XUnmapWindow(display, wp->client);
                                XSetInputFocus(display, DefaultRootWindow(display), RevertToPointerRoot, CurrentTime);
                                XWindowAttributes f_attr; XGetWindowAttributes(display, wp->frame, &f_attr);
                                XResizeWindow(display, wp->frame, f_attr.width, TITLE_H);
                            } else {
                                Atom type; int format; unsigned long nitems, after; unsigned char *prop = NULL;
                                long oh = 400;
                                if (XGetWindowProperty(display, wp->client, atom_original_height, 0, 1, False, XA_INTEGER, &type, &format, &nitems, &after, &prop) == Success && prop) { oh = *(long *)prop; XFree(prop); }

                                XWindowAttributes f_attr; XGetWindowAttributes(display, wp->frame, &f_attr);
                                XResizeWindow(display, wp->frame, f_attr.width, (int)oh + TITLE_H);
                                XResizeWindow(display, wp->client, f_attr.width, (int)oh);
                                XMapWindow(display, wp->client);

                                draw_title(display, wp);
                                if (wp->close_btn != None) draw_close_btn(display, wp->close_btn, BTN_SIZE);
                                if (wp->full_btn != None) draw_full_btn(display, wp->full_btn, BTN_SIZE);
                                send_configure_notify(display, wp);
                            }
                            last_click_time = 0;
                        } else {
                            last_click_time = current_time; last_click_window = clicked_w; grab_window = clicked_w;
                            start_x = ev.xbutton.x_root; start_y = ev.xbutton.y_root; win_x = attr.x; win_y = attr.y;
                        }
                    }
                }
            }
        }
        else if (ev.type == MotionNotify) {
            XEvent next_ev;
            while (XPending(display)) {
                XPeekEvent(display, &next_ev);
                if (next_ev.type == MotionNotify && next_ev.xmotion.window == ev.xmotion.window) XNextEvent(display, &ev);
                else break;
            }

            int screen_w = DisplayWidth(display, DefaultScreen(display));
            
            if (grab_window != None) {
                int root_x = ev.xmotion.x_root, root_y = ev.xmotion.y_root;
                int next_x = win_x + (root_x - start_x), next_y = win_y + (root_y - start_y);

                WinPair *wp = find_window(grab_window);
                if (wp) {
                    int edge_crossed = 0, new_ws = current_workspace, warp_x = root_x;

                    if (root_x <= 10) { new_ws = (current_workspace - 1 + NUM_WORKSPACES) % NUM_WORKSPACES; warp_x = screen_w - 10; edge_crossed = 1; } 
                    else if (root_x >= screen_w - 3) { new_ws = (current_workspace + 1) % NUM_WORKSPACES; warp_x = 10; edge_crossed = 1; }

                    if (edge_crossed) {
                        wp->workspace = new_ws;
                        set_window_desktop(display, wp->client, wp->is_sticky ? 0xFFFFFFFF : new_ws);
                        switch_workspace(display, new_ws);
                        XWarpPointer(display, None, root, 0, 0, 0, 0, warp_x, root_y);
                        next_x = warp_x - (start_x - win_x); start_x = warp_x; start_y = root_y; win_x = next_x; win_y = next_y;
                        XSync(display, False); XEvent junk; while (XCheckTypedEvent(display, MotionNotify, &junk));
                    }
                }
                XMoveWindow(display, grab_window, next_x, next_y);
            } else if (resize_window != None) {
                int new_w = (win_w + ev.xmotion.x_root - start_x > 50) ? win_w + ev.xmotion.x_root - start_x : 50;
                int new_h = (win_h + ev.xmotion.y_root - start_y > 50) ? win_h + ev.xmotion.y_root - start_y : 50;

                WinPair *wp = find_window(resize_window);
                if (wp) {
                    if (!resize_client_is_viewable) new_h = TITLE_H;
                    XResizeWindow(display, resize_window, new_w, new_h);
                    if (resize_client_is_viewable) XResizeWindow(display, wp->client, new_w, new_h - TITLE_H);
                    else XResizeWindow(display, wp->client, new_w, resize_client_h);
                }
            }
        }       
        else if (ev.type == ButtonRelease) {
            if (grab_window != None || resize_window != None) {
                WinPair *wp = find_window(grab_window != None ? grab_window : resize_window);
                if (wp) send_configure_notify(display, wp);
            }
            grab_window = None; resize_window = None;
        }
        else if (ev.type == EnterNotify) {
            if (ev.xcrossing.mode == NotifyNormal) {
                WinPair *wp = find_window(ev.xcrossing.window);
                if (wp && !wp->is_dock) XSetInputFocus(display, wp->client, RevertToParent, ev.xcrossing.time);
            }
        }
        else if (ev.type == ClientMessage) {
            if (ev.xclient.message_type == atom_net_current_desktop) {
                int next_ws = ev.xclient.data.l[0];
                if (next_ws >= 0 && next_ws < NUM_WORKSPACES) switch_workspace(display, next_ws);
            } else if (ev.xclient.message_type == atom_net_wm_desktop) {
                WinPair *wp = find_window(ev.xclient.window);
                if (wp && !wp->is_dock) {
                    long desk = ev.xclient.data.l[0];
                    if (desk == 0xFFFFFFFF) {
                        wp->is_sticky = 1; set_window_desktop(display, wp->client, 0xFFFFFFFF); XMapWindow(display, wp->frame); 
                    } else if (desk >= 0 && desk < NUM_WORKSPACES) {
                        wp->is_sticky = 0; wp->workspace = desk; set_window_desktop(display, wp->client, desk);
                        if (desk != current_workspace) XUnmapWindow(display, wp->frame); else XMapWindow(display, wp->frame);
                    }
                }
            }
            // --- 追加: アプリからのフルスクリーン要求の処理 (_NET_WM_STATE) ---
            else if (ev.xclient.message_type == atom_net_wm_state) {
                WinPair *wp = find_window(ev.xclient.window);
                if (wp && !wp->is_dock) {
                    long action = ev.xclient.data.l[0];
                    Atom prop1 = (Atom)ev.xclient.data.l[1];
                    Atom prop2 = (Atom)ev.xclient.data.l[2];

                    if (prop1 == atom_net_wm_state_fullscreen || prop2 == atom_net_wm_state_fullscreen) {
                        // action: 0 = Remove, 1 = Add, 2 = Toggle
                        set_fullscreen(display, wp, action);
                    }
                }
            }
        }
    } 
    return 0;
}
