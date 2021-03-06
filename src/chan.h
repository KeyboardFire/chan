#ifndef __CHAN_H__
#define __CHAN_H__

#include <ncurses.h>
#include <curl/curl.h>

#define PAIR_RED     1
#define PAIR_GREEN   2
#define PAIR_YELLOW  3
#define PAIR_BLUE    4
#define PAIR_MAGENTA 5
#define PAIR_CYAN    6
#define PAIR_WHITE   7

#define PAIR_RED_BG     8
#define PAIR_GREEN_BG   9
#define PAIR_YELLOW_BG  10
#define PAIR_BLUE_BG    11
#define PAIR_MAGENTA_BG 12
#define PAIR_CYAN_BG    13
#define PAIR_WHITE_BG   14

#define URLNBUF_LEN 4
struct chan {
    CURL *curl;
    WINDOW *title_win;
    WINDOW *main_win;
    int main_lines;
    int main_cols;
    WINDOW *status_win;

    struct {
        struct sub *subs;
        int nsubs;
        int active;
        char *fmt_str;
        int page;
        enum {
            SUB_HOME = 0,
            SUB_NEW,
            SUB_SHOW,
            SUB_ASK,
            SUB_JOBS
        } mode;
    } sub;

    struct {
        struct sub *sub;
        char **buf;
        struct fmt **buf_fmt;
        int lines;
        int scroll;
        int active;
        int *offsets;
        char urlnbuf[URLNBUF_LEN];
    } com;

    struct {
        char sub_down;
        char sub_up;
        char sub_login;
        char sub_open_url;
        char sub_reload;
        char sub_upvote;
        char sub_view_comments;
        char sub_home;
        char sub_new;
        char sub_show;
        char sub_ask;
        char sub_jobs;
        char com_scroll_down;
        char com_scroll_up;
        char com_next;
        char com_prev;
        char com_next_at_depth;
        char com_prev_at_depth;
        char com_open_url;
        char com_back;
        char com_upvote;
    } keys;

    char *username;
    char *password;
    int authenticated;
};

#define FMT_USER 0
#define FMT_AGE  1
#define FMT_URL  2
#define FMT_UP   3
#define FMT_BAD  4
struct fmt {
    int type;
    int offset;
    int len;
    struct fmt *next;
};

struct sub {
    int id;
    char *auth;
    int voted;
    char *url;
    char *title;
    int job;
    int score;
    char *user;
    char *age;
    struct com *coms;
    int ncoms;
    char **urls;
    int nurls;
};

struct com {
    int id;
    int depth;
    char *auth;
    int voted;
    char *user;
    char *age;
    int badness;
    char *text;
};

struct chan *chan_init(int argc, char **argv);
void chan_main_loop(struct chan *chan);
void chan_destroy(struct chan *chan);

#endif
