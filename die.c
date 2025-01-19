#include <time.h>
#include <stdarg.h>

#include "structs.c"


#ifndef DIE
#define DIE
#define FILENAME "crash.log"

/*
This code isn't too pretty, but it works!
*/


void 
die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);

    exit(1);
}

int
dumps_yeet_arr(struct yeet **yeets) 
{
    FILE *crash_log;
    crash_log = fopen(FILENAME, "w");

    if (crash_log == NULL) {
        fprintf(stderr, "Could not open crash.log! Super dying now! No information will be available!");
        return -1;
    }
    
    time_t now;
    time(&now);
    struct tm ts = *localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%Z %Y-%m-%d %H:%M:%S", &ts);
    char header[2000];
    sprintf(header, "----CRASH LOG [%s]----\n\n", buf);
    fputs(header, crash_log);

    char *yeet_msg;
    for (int i = 0; i < 20; i++) {
        struct yeet *o = *(yeets + i);
        if (o == NULL) {
            fputs("NULL YEET\n", crash_log);
            continue;
        }
        
        cog_asprintf(&yeet_msg,
            "YEET %d @ %p\n"
            "       MESSAGE_ID: %ld\n"
            "       CHANNEL_ID: %ld\n"
            "           AUTHOR: %ld\n"
            "           VICTIM: %ld\n"
            "   INTERACTION_ID: %ld\n"
            "Y_REACTS-X_REACTS: %d-%d\n"
            "INTERACTION_TOKEN: %s\n"
            "\n\n",
            i, (void*)o,
            o->m_id.message,
            o->m_id.channel,
            o->author,
            o->victim,
            o->i_id,
            o->y_reacts, o->x_reacts,
            o->token
        );
        fputs(yeet_msg, crash_log);
    }

    return 0;
}
#endif