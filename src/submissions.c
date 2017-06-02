#include "submissions.h"
#include "comments.h"
#include "login.h"
#include "net.h"
#include "parse.h"
#include "sys.h"
#include "utf8.h"

#include <stdlib.h>
#include <string.h>

void chan_destroy_submissions(struct chan *chan) {
    for (int i = 0; i < chan->nsubmissions; ++i) {
        free(chan->submissions[i].url);
        free(chan->submissions[i].title);
        free(chan->submissions[i].user);
        free(chan->submissions[i].comments);
        for (int j = 0; j < chan->submissions[i].nurls; ++j) {
            free(chan->submissions[i].urls[j]);
        }
        free(chan->submissions[i].urls);
    }
    free(chan->submissions);
    chan->submissions = NULL;
    chan->nsubmissions = 0;
}

void chan_update_submissions(struct chan *chan) {
    chan_destroy_submissions(chan);
    chan->submissions = malloc(30 * sizeof *chan->submissions);
    chan->nsubmissions = 30;

    char *data = http(chan->curl, "https://news.ycombinator.com/", NULL, 1);

    struct submission *submission = chan->submissions;
    while ((data = strstr(data, "<tr class='athing'"))) {
        data += strlen("<tr class='athing' id='");
        submission->id = atoi(data);

        char *auth = strstr(data, "auth=");
        submission->voted = 0;
        if (auth && auth < jumpch(data, '\n', 2)) {
            copyuntil(&submission->auth, auth + strlen("auth="), '&');
            if (!strncmp(jumpapos(auth, 2), "nosee", 5)) submission->voted = 1;
        } else submission->auth = NULL;

        data = strstr(data, "<td class=\"title\"><a href=\"") +
            strlen("<td class=\"title\"><a href=\"");
        copyuntil(&submission->url, data, '"');

        data = jumptag(data, 1);
        copyuntil(&submission->title, data, '<');

        // next line always starts with <span class="
        data = strchr(strchr(data, '\n'), '=') + 2;
        if (*data == 's') {
            // "score" i.e. it's a regular submission
            submission->job = 0;

            data = jumptag(data, 1);
            submission->score = atoi(data);

            data = jumptag(data, 2);
            // HN colors new users in green with <font></font>
            // take that into account
            int newuser = *data == '<';
            if (newuser) data = jumptag(data, 1);
            copyuntil(&submission->user, data, '<');

            data = jumptag(data, newuser ? 4 : 3);
        } else if (*data == 'a') {
            // "age" i.e. it's one of those job posts
            submission->job = 1;

            submission->score = 0;

            submission->user = malloc(1);
            submission->user[0] = '\0';

            data = jumptag(data, 2);
        } else exit(123); // TODO something better
        copyage(&submission->age, data);

        submission->comments = NULL;
        if (submission->job) {
            submission->ncomments = 0;
        } else {
            data = jumptag(strstr(data, "<a href=\"item?"), 1);
            submission->ncomments = atoi(data);
        }

        ++submission;
    }
}

// does NOT call wrefresh()!
void chan_redraw_submission(struct chan *chan, int i) {
    int normal_attr = i == chan->active_submission ? A_REVERSE : 0;
    wattrset(chan->main_win, normal_attr);
    struct submission submission = chan->submissions[i];

    int y, x;
    char *substr = NULL;
    int idx = 0, subidx = 0;
    wmove(chan->main_win, i, 0);
    do {
        if (substr) {
            int blen = bytelen(substr[subidx]);
            waddnstr(chan->main_win, substr + subidx, blen);
            if (!substr[subidx += blen]) {
                free(substr);
                substr = NULL;
                subidx = 0;
                wattron(chan->main_win, COLOR_PAIR(PAIR_WHITE));
                wattrset(chan->main_win, normal_attr);
            }
        } else {
            if (!chan->submission_fs[idx]) {
                waddch(chan->main_win, ' ');
            } else if (chan->submission_fs[idx] == '%') {
                switch (chan->submission_fs[++idx]) {
                    case 's':
                        substr = malloc(5);
                        if (submission.job) {
                            strcpy(substr, "    ");
                        } else {
                            snprintf(substr, 5, "%4d", submission.score);
                            if (submission.voted) {
                                wattron(chan->main_win, A_BOLD | COLOR_PAIR(PAIR_GREEN));
                            }
                        }
                        break;
                    case 'a':
                        substr = malloc(4);
                        snprintf(substr, 4, "%3s", submission.age);
                        break;
                    case 'c':
                        substr = malloc(4);
                        if (submission.job) {
                            strcpy(substr, "   ");
                        } else {
                            snprintf(substr, 4, "%3d", submission.ncomments);
                        }
                        break;
                    case 't':
                        substr = malloc(strlen(submission.title) + 1);
                        strcpy(substr, submission.title);
                        break;
                    case '%':
                        waddch(chan->main_win, '%');
                        break;
                    default:
                        waddch(chan->main_win, '%');
                        --idx;
                        break;
                }
                ++idx;
            } else {
                int blen = bytelen(chan->submission_fs[idx]);
                waddnstr(chan->main_win, chan->submission_fs + idx, blen);
                idx += blen;
            }
        }
        getyx(chan->main_win, y, x);
    } while (y == i);
}

void chan_draw_submissions(struct chan *chan) {
    wclear(chan->main_win);
    for (int i = 0; i < chan->nsubmissions; ++i) {
        chan_redraw_submission(chan, i);
    }
    wrefresh(chan->main_win);
}

#define ACTIVE chan->submissions[chan->active_submission]
int chan_submissions_key(struct chan *chan, int ch) {
    switch (ch) {
        case 'j':
            if (chan->active_submission < chan->nsubmissions - 1) {
                ++chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission - 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            return 1;
        case 'k':
            if (chan->active_submission > 0) {
                --chan->active_submission;
            }
            chan_redraw_submission(chan, chan->active_submission + 1);
            chan_redraw_submission(chan, chan->active_submission);
            wrefresh(chan->main_win);
            return 1;
        case 'l':
            chan_login_init(chan);
            return 1;
        case 'o':
            urlopen(ACTIVE.url);
            return 1;
        case 'r':
            chan_update_submissions(chan);
            chan_draw_submissions(chan);
            return 1;
        case 'u':
            if (chan->authenticated) {
                char *buf = malloc(100);
                sprintf(buf, "https://news.ycombinator.com/vote?id=%d&how=u%c&auth=%s",
                        ACTIVE.id, ACTIVE.voted ? 'n' : 'p', ACTIVE.auth);
                http(chan->curl, buf, NULL, 0);
                free(buf);

                ACTIVE.voted = 1 - ACTIVE.voted;
                ACTIVE.score += ACTIVE.voted ? 1 : -1;
                chan_redraw_submission(chan, chan->active_submission);
                wrefresh(chan->main_win);
            } else {
                wclear(chan->status_win);
                mvwaddstr(chan->status_win, 0, 0, "You must be authenticated to do that.");
                wrefresh(chan->status_win);
            }
            return 1;
        case '\n':
            chan->viewing = chan->submissions + chan->active_submission;
            if (!chan->viewing->comments) chan_update_comments(chan);
            chan_draw_comments(chan);
            return 1;
        default:
            return 0;
    }
}
