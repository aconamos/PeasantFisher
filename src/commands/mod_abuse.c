#include "discord.h"
#include "log.h"

#include "../infra/arena.c"
#include "../project_vars.h"


#ifndef COMMAND_ABUSE
#define COMMAND_ABUSE

/**
 * Links against peasant_time, a small static Rust library to parse time using humantime.
 */
uint64_t 
rel_time(const char *c_char);

struct abuse_info {

};

void
abuse_succ(struct discord *client, struct discord_response *resp, const struct discord_guild_member *ret)
{
}

void 
abuse_fail(struct discord *client, struct discord_response *resp)
{
}

void
abuse_cb(struct discord *client, const struct discord_interaction *event)
{
    // Extract options
    u64snowflake victim;
    char *str_time = NULL, *reason = NULL;

    for (int i = 0; i < event->data->options->size; ++i) {
        char *name = event->data->options->array[i].name;
        char *value = event->data->options->array[i].value;

        if (0 == strcmp(name, "user")) {
            victim = strtol(value, NULL, 10);
        } else if (0 == strcmp(name, "time")) {
            str_time = value;
        } else if (0 == strcmp(name, "reason")) {
            reason = value;
        }

        if (reason == NULL) reason = "placeholder reason"; // TODO put this at initialization
    }

    log_debug("abuse_cb: parameters---\nuser: %ld\ntime: %s\nreason: %s", victim, str_time, reason);

    u64unix_ms time_offset = rel_time(str_time);
    log_debug("Acquired rel_time: %ld", time_offset);

    // Check for validity of parameters
    if (time_offset == 0) {
        discord_create_interaction_response(client, event->id, event->token, &(struct discord_interaction_response) {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data) {
                .content = "Couldn't parse time! Please input something like '1d', or '3h'.",
                .flags = DISCORD_MESSAGE_EPHEMERAL,
            },
        }, NULL);

        return;
    }

    if (time_offset > 60 * 60 * 24 * 28 * 1000) {
        discord_create_interaction_response(client, event->id, event->token, &(struct discord_interaction_response) {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data) {
                .content = "Discord doesn't let you timeout for over 28 days :/",
                .flags = DISCORD_MESSAGE_EPHEMERAL,
            },
        }, NULL);

        return;
    }

    // If the time is invalid, clamp to 28d (does discord API do this for us?)
    // skipping for now

    // Try to modify user
    discord_modify_guild_member(client, GUILD_ID, victim, &(struct discord_modify_guild_member) {
        .communication_disabled_until = time(NULL) * 1000 + time_offset,
    }, &(struct discord_ret_guild_member) {
        .done = abuse_succ,
        .fail = abuse_fail,
        .data = NULL,
    });

    // Two potential options:
    //  fail -> laugh at this user blah blah blah
    //  succ -> x was shot by y! reason: "reason"
}
#endif