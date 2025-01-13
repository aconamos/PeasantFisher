#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "discord.h"
#include "log.h"

#define GUILD_ID 849505364764524565
#define APPLICATION_ID "1326422681955991554"
#define BOT_ID 1326422681955991554
#define YES_EMOJI "✅"
#define NO_EMOJI "❌"
#define VOTE_COUNT 2
#define YEET_SECONDS 90
#define YEET_DURATION 30
#define ACTIVE_YEETS_SIZE 20
#define MAX_YEETS 3


/**
 * TODO
 * - Reactions should trigger a query on the yeet
 *      - If the reactions meet quota, it triggers a function to yeet, and then deletes the yeet? (I think this works? Do we need to keep track after?)
 * - Max yeets feature
 *      - Use a discord timer to add one every hour
 *      - Create the message for being out of yeets
 */

struct yeet *active_yeets[ACTIVE_YEETS_SIZE]; // Just make an array that... should be big enough. If it crashes... oh well.

struct message_identifier {
    u64snowflake message;
    u64snowflake channel;
};

struct yeet {
    struct message_identifier m_id;
    u64snowflake author;
    u64snowflake victim;
    u64snowflake i_id;
    int y_reacts;
    int x_reacts;
    char *token;
};

struct yeet_with_users {
    struct yeet *yeet;
    char *msg;
    int backfire;
};

struct get_reactions_params {
    struct yeet *yeet;
    char *emoji;
};

/**
 * This method take a message ID snowflake and returns the index of the active_yeets array that corresponds to the snowflake,
 * if there are any. Returns -1 otherwise.
 */
int
get_yeet_idx_by_id(u64snowflake message_id)
{
    int ret = -1;
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) continue;
        if (active_yeets[i]->m_id.message == message_id) {
            ret = i;
        }
        break;
    }
    return ret;
}

/**
 * This method take an interaction ID snowflake and returns the index of the active_yeets array that corresponds to the snowflake,
 * if there are any. Returns -1 otherwise.
 */
int
get_yeet_idx_by_token(u64snowflake interaction_id) 
{
    int ret = -1;
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) continue;
        if (active_yeets[i]->i_id == interaction_id) {
            ret = i;
            break;
        }
    }
    return ret;
}

/**
 * This method returns the index of the first null pointer in the active_yeets array.
 */
int
get_first_null()
{
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) return i;
    }
    return -1; // God I hope we never reach this
}

void 
yeet_fail(struct discord *client, struct discord_response *resp)
{
    struct yeet_with_users *yww_data = resp->data;
    struct yeet *yeet = yww_data->yeet;
    discord_delete_message(client, yeet->m_id.channel, yeet->m_id.message, NULL, NULL);
    discord_create_message(client, yeet->m_id.channel, &(struct discord_create_message) {
        .content = "Yeet failed!",
    }, NULL);

    free(yww_data->msg);
    free(yww_data);
    yww_data = NULL;
}

void 
yeet_succ(struct discord *client, struct discord_response *resp, const struct discord_guild_member *ret)
{
    struct yeet_with_users *yww_data = resp->data;
    struct yeet *yeet = yww_data->yeet;
    discord_delete_message(client, yeet->m_id.channel, yeet->m_id.message, NULL, NULL);
    discord_create_message(client, yeet->m_id.channel, &(struct discord_create_message) {
        .content = yww_data->msg,
    }, NULL); // TODO: They will return in + callback
    // TODO: Should make the edit message a callback in case of failure or whatnot

    free(yww_data->msg);
    free(yww_data);
    yww_data = NULL;
    // TODO: Does the yeet ptr need to be freed at this point? I think we might be able to keep it.
}

void
get_users_done(struct discord *client, struct discord_response *resp, const struct discord_users *ret) 
{
    struct get_reactions_params *params = resp->data;
    char *emoji = params->emoji;
    struct yeet *yeet = params->yeet;
    int is_backfire = 0;
    u64snowflake target;

    if (strcmp(emoji, YES_EMOJI) == 0) {
        yeet->y_reacts = ret->size;
        is_backfire = ret->size;
    }
    else if (strcmp(emoji, NO_EMOJI) == 0) {
        yeet->x_reacts = ret->size;
        is_backfire = -ret->size;
    }

    if (abs(is_backfire) < VOTE_COUNT) return;
    if (is_backfire > 0) target = yeet->victim;
    else target = yeet->author;
    // TODO: There's some duplicated logic above here that could use a trim
    
    log_debug("TARGET BEING YEETED: %ld", target);
    log_debug("WAS BACKFIRE? %d", is_backfire);
    log_debug("USERS SIZE: %ld", ret->size);
    
    const char *yeet_msg_fmt = "User <@!%ld> yeeted in %f seconds!\nBrought to you by %s";
    char *_fstr = malloc(25);
    char *yeet_msg = malloc(2000); // TODO get an exact size
    char list[2000] = ""; // I wish I could use a variable size here, but it gives me an error.

    for (int i = 0; i < ret->size; i++) {
        cog_asprintf(&_fstr, "<@!%ld> ", ret->array[i].id);
        strcat(list, _fstr);
    }

    cog_asprintf(&yeet_msg, yeet_msg_fmt, yeet->victim, -6.9f, list);

    // free(list);
    free(_fstr);

    // For some reason, we gotta use heap memory here. 
    struct yeet_with_users *data = malloc(sizeof(struct yeet_with_users));
    data->msg = yeet_msg;
    data->yeet = yeet;

    discord_modify_guild_member(client, GUILD_ID, yeet->victim, &(struct discord_modify_guild_member) {
        .communication_disabled_until = (u64unix_ms)((time(NULL) + YEET_DURATION)* 1000), // This needs an extra *1000 because for some reason discord's timestamps have extra precision. /shrug
    }, &(struct discord_ret_guild_member) {
        .done = yeet_succ,
        .fail = yeet_fail,
        .data = data,
    });

    free(resp->data);
    
    // Saving this for later
    // discord_edit_followup_message(client, BOT_ID, yeet->token, yeet->m_id.message, &(struct discord_edit_followup_message) {
    //     .content = "User Yeeted",
    //     // TODO: Create another followup, not just edit
    // }, NULL);
}

/**
 * This callback serves to delete a message and, if necessary, remove the corresponding yeet
 * from the list of active yeets. It is called from a discord timer.
 */
void
murder_message(struct discord *client, struct discord_timer *timer)
{
    // Delete the message
    struct message_identifier *data = (timer->data);
    // TODO: Only make this call if the yeet hasn't passed
    discord_delete_message(client, data->channel, data->message, NULL, NULL);

    // At this point, the yeet could either still be pending decision or decided. 
    // If decided, the pointer will be null and removed from the active_yeets array.
    // We will use the message id to search and if the search doesn't return -1,
    // we free the pointer there and set it to NULL.
    int idx = get_yeet_idx_by_id(data->message);
    if (idx != -1) {
        free(active_yeets[idx]->token);
        free(active_yeets[idx]);
        active_yeets[idx] = NULL;
        // TODO: Add one more to yeet count
    }
    free(data);
}

/**
 * This callback serves to finish constructing yeet data and start the timer to delete it.
 * It is the last call to construct the yeet following the initial reaction from the bot.
 */
void 
init_react_cb_done_get(struct discord *client, struct discord_response *resp, const struct discord_message *ret) 
{
    discord_create_reaction(client, ret->channel_id, ret->id, 0, YES_EMOJI, NULL);
    discord_create_reaction(client, ret->channel_id, ret->id, 0, NO_EMOJI, NULL);
    // Let it be known, that here lies the very first call to malloc I have ever made.
    // A monumental step in my CS career. I can only look back on my days of Java and think, "Wow. What a baby."
    // Though I realize I am far from the adult; I am still but a toddler. Nonetheless, I manage memory.
    // What a day.
    // This message_identifier is a little redudant because it's only going to be used for 
    // deleting the message, but it's not a problem I care to fix.
    struct message_identifier *p_message_identifier = malloc(sizeof(struct message_identifier));
    p_message_identifier->channel = ret->channel_id;
    p_message_identifier->message = ret->id;
    log_debug("Timestamp of the yeet message: %ld", ret->timestamp);
    log_debug("Time from sys: %ld", time(NULL));

    log_debug("Interaction ID (CB): %ld", ret->interaction->id);
    int idx = get_yeet_idx_by_token(ret->interaction->id);
    struct yeet *yeet = active_yeets[idx];
    yeet->m_id.channel = ret->channel_id;
    yeet->m_id.message = ret->id;
    // TODO: Send the actual yeet* over to the timer callback to gather it all in one place
    // Hopefully that will also allow me to make use of concord's built in cleanup functionality
    discord_timer(client, murder_message, NULL, p_message_identifier, YEET_SECONDS * 1000);
}

/**
 * This callback serves to call the other, more useful callback.
 * I'm not sure why it's required; I copied it from somewhere.
 */
void 
init_react_cb_done(struct discord *client, struct discord_response *resp, const struct discord_interaction_response *ret) 
{
    const struct discord_interaction *event = resp->keep;

    discord_get_original_interaction_response(client, BOT_ID, event->token, &(struct discord_ret_message) {
        .done = init_react_cb_done_get,
    });
}

/**
 * This serves to build the application commands necessary for the bot to function.
 */
void 
on_ready(struct discord *client, const struct discord_ready *event) 
{
    // This ping command is probably going to stay a while for ensuring slash commands work.
    struct discord_create_guild_application_command params = {
        .name = "ping",
        .description = "Ping command!"
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &params, NULL);

    struct discord_application_command_option options[] = {
        {
            .type = DISCORD_APPLICATION_OPTION_USER,
            .name = "user",
            .description = "Who to yeet",
            .required = true
        },
    };

    struct discord_create_guild_application_command yeet_params = {
        .name = "yeet",
        .description = "Yeet someone!",
        .options = 
            &(struct discord_application_command_options) {
                .size = sizeof(options) / sizeof *options,
                .array = options,
            },
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &yeet_params, NULL);
}

void 
on_interaction(struct discord *client, const struct discord_interaction *event) 
{
    if (event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
        return; /* return if interaction isn't a slash command */

    if (strcmp(event->data->name, "ping") == 0) {
          struct discord_interaction_response params = {
                .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
                .data = &(struct discord_interaction_callback_data){
                      .content = "pong"
                }
          };
          discord_create_interaction_response(client, event->id,
                                              event->token, &params, &(struct discord_ret_interaction_response) {
                                                .keep = event,
                                                .done = init_react_cb_done,
                                             });
    }

    // -- DATA COMMANDS BELOW THIS LINE --
    /* Return in case user input is missing for some reason */
    if (!event->data || !event->data->options) return;

    if (strcmp(event->data->name, "yeet") == 0) {
        log_info("Yeet called!");
        u64snowflake victim_id = strtoul(event->data->options->array[0].value, NULL, 10);

        // Format message
        char* yeet_message;
        cog_asprintf(&yeet_message, 
            "Do you want to yeet <@!%ld>? (%d %s's needed)\n"
            "Or, vote %s to yeet the author: <@!%ld>\n"
            "\n"
            "Otherwise, this will be deleted <t:%ld:R>\n"
        , victim_id, VOTE_COUNT, YES_EMOJI, NO_EMOJI, event->member->user->id, time(NULL) + YEET_SECONDS);
        
        // Build yeet
        int free_yeet_idx = get_first_null(); // This doesn't check for the -1 case. That's because if we reach it, we're fucked anyways. Crashing is the best behavior then.
        struct yeet *free_yeet = malloc(sizeof(struct yeet));
        active_yeets[free_yeet_idx] = free_yeet;

        free_yeet->i_id = event->id;
        free_yeet->author = event->member->user->id;
        free_yeet->victim = victim_id;
        free_yeet->token = malloc(strlen(event->token));
        strcpy(free_yeet->token, event->token);

        struct discord_interaction_response params = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = yeet_message,
            }
        };
        discord_create_interaction_response(client, event->id,
                                              event->token, &params, &(struct discord_ret_interaction_response) {
                                                .keep = event,
                                                .done = init_react_cb_done,
                                             });
        free(yeet_message);
    }
}

/**
 * This is the callback for when any message is reacted to. If the message reacted to has a corresponding yeet in the array,
 * it make a couple calls to the discord API to count reactions and then react accordingly.
 */
void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    if (event->member->user->id == BOT_ID) return;

    char *name = event->emoji->name;
    u64snowflake m_id = event->message_id;
    u64snowflake c_id = event->channel_id;
    
    // Check to make sure we're not doing yeet things on a non-yeet message
    int idx = get_yeet_idx_by_id(m_id);
    if (idx == -1) return;

    struct yeet *yeet = active_yeets[idx];
    
    char *str;
    cog_asprintf(&str, "EMOJI REACTED: %s", name);
    log_info(str);
    free(str);

    log_debug("TOKEN %s / MESSAGE_ID %ld", yeet->token, yeet->m_id.message);

    struct get_reactions_params *yes_params = malloc(sizeof(struct get_reactions_params));
    struct get_reactions_params *no_params = malloc(sizeof(struct get_reactions_params));
    yes_params->yeet = yeet;
    no_params->yeet = yeet;
    yes_params->emoji = YES_EMOJI;
    no_params->emoji = NO_EMOJI;

    discord_get_reactions(client, c_id, m_id, 0, YES_EMOJI, NULL, &(struct discord_ret_users) {
        .done = get_users_done,
        .keep = event,
        .data = yes_params,
    });
    discord_get_reactions(client, c_id, m_id, 0, NO_EMOJI, NULL, &(struct discord_ret_users) {
        .done = get_users_done,
        .keep = event,
        .data = no_params,
    });

}

int main(int argc, char* argv[]) {
    // Load config file
    const char *config_file;
    if (argc > 1)
        config_file = argv[1];
    else
        config_file = "./config.json";
    
    // Initialize the client using some magic function calls
    ccord_global_init();
    struct discord *client = discord_config_init(config_file);
    assert(NULL != client && "Couldn't initialize client");

    // Register events (I actually love the function reference style C uses)
    discord_set_on_ready(client, &on_ready);
    discord_set_on_interaction_create(client, &on_interaction);
    discord_set_on_message_reaction_add(client, &on_react);

    // Run and wait for input?    
    discord_run(client);
    fgetc(stdin);

    discord_cleanup(client);
    ccord_global_cleanup();
}