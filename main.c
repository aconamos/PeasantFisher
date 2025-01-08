#include <string.h>
#include <assert.h>
#include <discord.h>

#define GUILD_ID 849505364764524565
#define BOT_ID 1326422681955991554
#define YES_EMOJI "✅"
#define NO_EMOJI "❌"

void 
on_ready(struct discord *client, const struct discord_ready *event) 
{
    struct discord_create_guild_application_command params = {
        .name = "ping",
        .description = "Ping command!"
    };
    discord_create_guild_application_command(client, event->application->id,
                                             GUILD_ID, &params, NULL);

    struct discord_create_guild_application_command yeet_params = {
        .name = "yeet",
        .description = "Yeet someone!"
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
}

void 
on_react(struct discord *client, const struct discord_message_reaction_add *event) 
{
    int emoji_id = event->emoji->id;
    char *name = event->emoji->name;
    printf("EMOJI REACTED: %d, %s", emoji_id, name);

    if (strcmp(name, YES_EMOJI) == 0) {
        printf("YES!");
    }
    else if (strcmp(name, NO_EMOJI) == 0) {
        printf("NO!");
    }
    printf("\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    const char *config_file;
    if (argc > 1)
        config_file = argv[1];
    else
        config_file = "./config.json";
    
    ccord_global_init();
    struct discord *client = discord_config_init(config_file);
    assert(NULL != client && "Couldn't initialize client");

    discord_set_on_ready(client, &on_ready);
    discord_set_on_interaction_create(client, &on_interaction);
    discord_set_on_message_reaction_add(client, &on_react);
    discord_run(client);

    fgetc(stdin);

    discord_cleanup(client);
    ccord_global_cleanup();
}