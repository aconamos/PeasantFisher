#include "discord.h"
#include "log.h"

#include "../infra/arena.c"
#include "../project_vars.h"


#ifndef COMMAND_ABUSE
#define COMMAND_ABUSE

/**
 * Links against peasant_time, a small static Rust library to parse time.
 */
uint64_t 
rel_time(const char *c_char);

void
abuse_cb(struct discord *client, const struct discord_interaction *event)
{
    // Extract options
    u64snowflake victim;
    char *time = NULL, *reason = NULL;

    for (int i = 0; i < event->data->options->size; ++i) {
        char *name = event->data->options->array[i].name;
        char *value = event->data->options->array[i].value;

        if (0 == strcmp(name, "user")) {
            victim = strtol(value, NULL, 10);
        } else if (0 == strcmp(name, "time")) {
            time = value;
        } else if (0 == strcmp(name, "reason")) {
            reason = value;
        }

        if (reason == NULL) reason = "placeholder reason"; // TODO put this at initialization
    }

    log_debug("abuse_cb: parameters---\nuser: %ld\ntime: %s\nreason: %s", victim, time, reason);

    u64unix_ms time_offset = rel_time(time);
    log_debug("Acquired rel_time: %ld", time_offset);


    // If the time is invalid, clamp to 28d (does discord API do this for us?)
    // skipping for now

    // Try to modify user
    // Two potential options:
    //  fail -> laugh at this user blah blah blah
    //  succ -> x was shot by y! reason: "reason"
}
#endif