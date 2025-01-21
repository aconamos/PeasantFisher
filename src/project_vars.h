#ifndef PEASANTFISHER_CONFIG_VARS
#define PEASANTFISHER_CONFIG_VARS
    #define YES_EMOJI "✅"
    #define NO_EMOJI "❌"
    #define ACTIVE_YEETS_SIZE 20
    #define BOT_ID 1326422681955991554

    #ifdef DEBUG
        // Debug-specific variables make the bot much faster to debug
        #define GUILD_ID 849505364764524565
        #define VOTE_COUNT 2
        #define YEET_VOTING_SECONDS 20
        #define YEET_DURATION 30
        #define MAX_YEETS 2
        #define YEET_REGEN_S 20
    #else
        // Production variables go here
        #define GUILD_ID 849505364764524565
        #define VOTE_COUNT 3
        #define YEET_VOTING_SECONDS 90
        #define YEET_DURATION 300
        #define MAX_YEETS 3
        #define YEET_REGEN_S 3600
    #endif
#endif