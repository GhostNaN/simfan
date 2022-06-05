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
        {0, 0, 0, 0}
    };

    const char *usage =
        "Usage: simfan [options]\n"
        "\n"
        "Options:\n"
        "--help           -h            Displays this help message\n"
        "--verbose    -v             Be more verbose\n"
        "--config       -c FILE     Specifies config file to use\n"
        ;

    char opt;
    while((opt = getopt_long(argc, argv, "hvc:", long_options, NULL)) != -1) {

        switch (opt) {
        case 'h':
            fprintf(stdout, "%s", usage);
            exit(EXIT_SUCCESS);
        case 'v':
            VERBOSE = 1;
            break;
        case 'c':
            CONFIG_FILE = strdup(optarg);
            break;
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

    // Enable manual pwm mode
    set_pwm_enable_mode(fan_list, '1');
    read_curr_pwm(fan_list);
    
    int temp_indexes[temp_list[0].count];
    int past_temps[temp_list[0].count];
    memset(temp_indexes, 0, sizeof(temp_indexes));
    memset(past_temps, 0, sizeof(past_temps));

    while(!KILLALL) {
        read_temps(temp_list);
        if (temp_changed(temp_list, past_temps)) {
            if (temp_list[0].count > 1) // No need to sort array size of 1
                sort_temps(temp_list, temp_indexes);

            for (int fan=0; fan < fan_list[0].count; fan++)
                fan_list[fan].temp_set = NULL;
            
            for (int index=0; index < temp_list[0].count; index++) {
                set_temp_fans_target_pwm(temp_list[temp_indexes[index]], temp_indexes);
            }
        }

        for (int i=0; i < INTERVAL; i++) {
            set_fans(fan_list);
            sleep(1);
        }

        if (VERBOSE)
            printf("\n");
    }
    // Return pwm control to bios
    set_pwm_enable_mode(fan_list, '5');

    free((void*) fan_list);
    free((void*) temp_list);
    
    return EXIT_SUCCESS;
}
