#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "discord.h"
#include "log.h"

#define GUILD_ID 849505364764524565
#define BOT_ID 1326422681955991554
#define YES_EMOJI "✅"
#define NO_EMOJI "❌"


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
                                              event->token, &params, NULL);
    }

    // -- DATA COMMANDS BELOW THIS LINE --
    /* Return in case user input is missing for some reason */
    if (!event->data || !event->data->options) return;

    if (strcmp(event->data->name, "yeet") == 0) {
        log_info("Yeet called!");
        char* user_id = event->data->options->array[0].value;
        printf("USER: %s\n", user_id);
    }
}

void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    int emoji_id = event->emoji->id;
    char *name = event->emoji->name;
    
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