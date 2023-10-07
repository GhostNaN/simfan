#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <simfan.h>

int VERBOSE = 0;
int INTERVAL = 1;
int THRESHOLD = 1;
char *CONFIG_FILE = NULL;
int MAX_FANS = 0;
int KILLALL = 0;

static void handle_signal(int signum) {
    (void) signum;
    printf("Quitting simfan..\n");
    KILLALL = 1;
}

static void parse_command_line(int argc, char **argv) {

    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"config", required_argument, NULL, 'c'},
        {"max", no_argument, NULL, 'x'},
        {0, 0, 0, 0}
    };

    const char *usage =
        "Usage: simfan [options]\n"
        "\n"
        "Options:\n"
        "--help      -h          Displays this help message\n"
        "--verbose   -v          Be more verbose\n"
        "--config    -c FILE     Specifies config file to use\n"
        "--max    -x           Send all the fans to full blast\n"
        ;

    int opt;
    while((opt = getopt_long(argc, argv, "hvc:x", long_options, NULL)) != -1) {

        switch (opt) {
        case 'h':
            printf( "%s", usage);
            exit(EXIT_SUCCESS);
        case 'v':
            VERBOSE = 1;
            break;
        case 'c':
            CONFIG_FILE = strdup(optarg);
            break;
        case 'x':
            MAX_FANS = 1;
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTERM, handle_signal);

    parse_command_line(argc, argv);

    config_set_globals();
    fan_type *fan_list = config_get_fans();
    temp_type *temp_list = config_get_temps(fan_list);
    printf("Config passed, Starting simfan...\n");

    // Enable manual pwm mode
    for(int fan=0; fan < fan_list[0].count; fan++)
        set_pwm_enable_mode(fan_list[fan], 1);

    read_curr_pwm(fan_list);
    
    int temp_indexes[temp_list[0].count];
    int past_temps[temp_list[0].count];
    // Set all to 0 to trigger a temp_changed
    memset(temp_indexes, 0, sizeof(temp_indexes));
    memset(past_temps, 0, sizeof(past_temps));

    if (MAX_FANS) {
        printf("Setting all fans to to max pwm of 255\n");
        max_fans(fan_list);
        while(!KILLALL)
            sleep(1);
    }
    while(!KILLALL) {
        read_temps(temp_list);
        if (temp_changed(temp_list, past_temps)) {
            if (VERBOSE)
                printf("\n");
            if (temp_list[0].count > 1) // No need to sort array size of 1
                sort_temps(temp_list, temp_indexes);

            for (int fan=0; fan < fan_list[0].count; fan++)
                fan_list[fan].temp_set = NULL;
            
            for (int index=0; index < temp_list[0].count; index++)
                set_temp_fans_target_pwm(temp_list[temp_indexes[index]]);
        }

        for (int i=0; i < INTERVAL; i++) {
            set_fans(fan_list);
            sleep(1);
            if (KILLALL)
                break;
        }

        if (VERBOSE)
            printf("\n\n");
    }
    // Return pwm control to default
    for(int fan=0; fan < fan_list[0].count; fan++)
        set_pwm_enable_mode(fan_list[fan], fan_list[fan].default_pwm_enable);

    free((void*) fan_list);
    free((void*) temp_list);
    free(CONFIG_FILE);
    
    return EXIT_SUCCESS;
}
