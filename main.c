#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "discord.h"
#include "log.h"

#define GUILD_ID 849505364764524565
#define APPLICATION_ID "1326422681955991554"
#define BOT_ID 1326422681955991554
#define YES_EMOJI "✅"
#define NO_EMOJI "❌"
#define VOTE_COUNT 5
#define YEET_SECONDS 90
#define ACTIVE_YEETS_SIZE 20


/**
 * TODO
 * - Reactions should trigger a query on the yeet
 *      - If the reactions meet quota, it triggers a function to yeet, and then deletes the yeet? (I think this works? Do we need to keep track after?)
 */

struct yeet *active_yeets[ACTIVE_YEETS_SIZE]; // Just make an array that... should be big enough. If it crashes... oh well.

struct message_identifier {
    u64snowflake message;
    u64snowflake channel;
};

struct yeet {
    struct message_identifier m_id;
    char *author;
    char *token;
    u64snowflake i_id;
    u64snowflake victim;
};

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

int
get_first_null()
{
    for (int i = 0; i < ACTIVE_YEETS_SIZE; i++) {
        if (active_yeets[i] == NULL) return i;
    }
    return -1; // God I hope we never reach this
}

void
murder_message(struct discord *client, struct discord_timer *timer)
{
    // Delete the message
    struct message_identifier *data = (timer->data);
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

    log_debug("Interaction ID (CB): %ld", ret->interaction->id);
    int idx = get_yeet_idx_by_token(ret->interaction->id);
    struct yeet *yeet = active_yeets[idx];
    yeet->m_id.channel = ret->channel_id;
    yeet->m_id.message = ret->id;
    discord_timer(client, murder_message, NULL, p_message_identifier, YEET_SECONDS * 1000);
}

void 
init_react_cb_done(struct discord *client, struct discord_response *resp, const struct discord_interaction_response *ret) 
{
    const struct discord_interaction *event = resp->keep;

    discord_get_original_interaction_response(client, BOT_ID, event->token, &(struct discord_ret_message) {
        .done = init_react_cb_done_get,
    });
}

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
        char* author_id = event->data->options->array[0].value;

        // Format message
        char* yeet_message;
        asprintf(&yeet_message, 
            "Do you want to yeet <@!%s>? (%d %s's needed)\n"
            "Or, vote %s to yeet the author: <@!%ld>\n"
            "\n"
            "Otherwise, this will be deleted <t:%ld:R>\n"
        , author_id, VOTE_COUNT, YES_EMOJI, NO_EMOJI, event->member->user->id, time(NULL) + YEET_SECONDS);
        
        // Build yeet
        int free_yeet_idx = get_first_null(); // This doesn't check for the -1 case. That's because if we reach it, we're fucked anyways. Crashing is the best behavior then.
        struct yeet *free_yeet = malloc(sizeof(struct yeet));
        active_yeets[free_yeet_idx] = free_yeet;

        log_debug("%ld", event->id);
        free_yeet->i_id = event->id;
        free_yeet->victim = event->member->user->id;
        free_yeet->author = author_id; //TODO we need to copy this because it's a char* for whatever reason even when it could just be a regular snowflake ID 
        log_debug("Pre strcpy");
        free_yeet->token = malloc(strlen(event->token));
        strcpy(free_yeet->token, event->token);
        log_debug("Post strcpy");
        log_debug("free_yeet token: %s", event->token);

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

void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    if (event->member->user->id == BOT_ID) return;

    char *name = event->emoji->name;
    u64snowflake m_id = event->message_id;
    
    int idx = get_yeet_idx_by_id(m_id);
    if (idx == -1) return;
    struct yeet *yeet = active_yeets[idx];
    
    char *str;
    asprintf(&str, "EMOJI REACTED: %s", name);
    log_info(str);
    free(str);

    if (strcmp(name, YES_EMOJI) == 0) {
        log_debug("YES!");
    }
    else if (strcmp(name, NO_EMOJI) == 0) {
        log_debug("NO!");
    }

    log_debug("TOKEN %s / MESSAGE_ID %ld", yeet->token, yeet->m_id.message);
    discord_edit_followup_message(client, BOT_ID, yeet->token, yeet->m_id.message, &(struct discord_edit_followup_message) {
        .content = "Message fuckin EDITED!!!!",
    }, NULL);
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