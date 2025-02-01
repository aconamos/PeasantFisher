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
    u64snowflake interaction_id;
    u64snowflake channel;
    u64snowflake victim;
    u64snowflake mod;
    u64unix_ms duration;
    char *reason;
    char *token;
};

void
abuse_succ(struct discord *client, struct discord_response *resp, const struct discord_guild_member *ret)
{
    struct abuse_info *abuse = resp->data;

    char *fuck;
    cog_asprintf(&fuck,
        "❗mod abuse alert❗\n\n<@!%ld> has been shot by <@!%ld> for \"%s\".\n\nThey will return <t:%ld:R>.",
        abuse->victim,
        abuse->mod,
        abuse->reason,
        time(NULL) + (abuse->duration / 1000)
    );

    discord_create_interaction_response(client, abuse->interaction_id, abuse->token, &(struct discord_interaction_response) {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data) {
            .content = fuck,
        }
    }, NULL);// TODO (probably won't do) make callback to edit and say "they have since returned"

    free(abuse->reason);
    free(abuse->token);
    free(abuse);
    free(fuck);
}

void 
abuse_fail(struct discord *client, struct discord_response *resp)
{
    struct abuse_info *abuse = resp->data;

    char *fuck;
    cog_asprintf(&fuck,
        "Everyone laugh at <@!%ld> who tried but miserably failed to abuse mod privileges on <@!%ld>.",
        abuse->mod,
        abuse->victim
    );

    discord_create_interaction_response(client, abuse->interaction_id, abuse->token, &(struct discord_interaction_response) {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data) {
            .content = fuck,
        }
    }, NULL);// TODO (probably won't do) make callback to edit and say "they have since returned"

    free(abuse->reason);
    free(abuse->token);
    free(abuse);
    free(fuck);
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

    if (time_offset > (uint64_t)60 * 60 * 24 * 28 * 1000) {
        discord_create_interaction_response(client, event->id, event->token, &(struct discord_interaction_response) {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data) {
                .content = "Discord doesn't let you timeout for over 28 days :/",
                .flags = DISCORD_MESSAGE_EPHEMERAL,
            },
        }, NULL);

        return;
    }

    struct abuse_info *info = malloc(sizeof(struct abuse_info));

    char *new_token = malloc(strlen(event->token) + 1);
    strcpy(new_token, event->token);

    char *new_reason = malloc(strlen(reason) + 1);
    strcpy(new_reason, reason);

    info->interaction_id = event->id;
    info->token = new_token;
    info->channel = event->channel_id;
    info->mod = event->member->user->id;
    info->duration = time_offset;
    info->victim = victim;
    info->reason = new_reason;

    // Try to modify user
    discord_modify_guild_member(client, GUILD_ID, victim, &(struct discord_modify_guild_member) {
        .communication_disabled_until = time(NULL) * 1000 + time_offset,
    }, &(struct discord_ret_guild_member) {
        .done = abuse_succ,
        .fail = abuse_fail,
        .data = info,
    });
}
#endif