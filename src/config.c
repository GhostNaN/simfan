#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>

#include <libconfig.h>

#include <simfan.h>

static char *find_real_file_path(const char *path) {
    glob_t gstruct;
    int glob_err;

    if ((glob_err = glob(path, GLOB_ERR, NULL, &gstruct))) {
        fprintf(stderr,"Glob could not find file:\"%s\"\n", path);
        exit(EXIT_FAILURE);
    }

    char *real_path  = calloc(strlen(gstruct.gl_pathv[0])+1, sizeof(char));
    strcpy(real_path, gstruct.gl_pathv[0]);

    globfree(&gstruct);
    return real_path;
} 

static config_t get_config() {
    config_t cfg;
    config_init(&cfg);

    // Find config file
    const char *configFileLocations[] = {
        CONFIG_FILE,
        "./simfan.conf",
        "../simfan.conf",
        "/etc/simfan.conf"
    };

    for (int i=0; i < 4; i++) {
        if(config_read_file(&cfg, configFileLocations[i]) == CONFIG_TRUE)
            break;
        else {
            config_error_t err_type = config_error_type(&cfg);
            if (err_type == CONFIG_ERR_PARSE) {
                // config_error_line() and config_error_text() doesn't work
                fprintf(stderr, "Could not parse:\"%s\"\n"
                                         "Please make sure your config is correctly set\n"
                                         "Refer to this: https://hyperrealm.github.io/libconfig/libconfig_manual.html#Configuration-Files\n", configFileLocations[i]);
                exit(EXIT_FAILURE);
            }
            else if (err_type == CONFIG_ERR_FILE_IO && i == 3) {
                perror("No config file found!");
                fprintf(stderr, "Please check simfan.conf for suitable locations\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return cfg;
}

void  config_set_globals() {
    config_t cfg = get_config();

    int interval;
    if(config_lookup_int(&cfg, "interval", &interval)) {
        INTERVAL = interval;
        if (VERBOSE)
            printf ("Interval:\"%i\"\n", INTERVAL);
    }
    else {
        printf ("Missing interval number!\n"
                    "Using default of:\"%i\"\n", INTERVAL);
    }

    int threshold;
    if(config_lookup_int(&cfg, "threshold", &threshold)) {
        THRESHOLD = threshold;
        if (VERBOSE)
            printf ("Threshold:\"%i\"\n", THRESHOLD);
    }
    else {
        printf ("Missing threshold number!\n"
                    "Using default of:\"%i\"\n", THRESHOLD);
    }
}

fan_type  *config_get_fans() {
    config_t cfg = get_config();
    config_setting_t *setting;

    setting = config_lookup(&cfg, "fans");
    int fan_count = config_setting_length(setting);
    fan_type *fan_list = calloc(fan_count, sizeof(fan_type));
    fan_list[0].count = fan_count;

    for(int fan=0; fan < fan_count; fan++) {
        config_setting_t *fan_settings = config_setting_get_elem(setting, fan);

        config_setting_lookup_string(fan_settings, "name", &fan_list[fan].name);

        const char *pwm_file_string;
        config_setting_lookup_string(fan_settings, "pwm_file", &pwm_file_string);
        fan_list[fan].pwm_file = find_real_file_path(pwm_file_string);
        free((void*) pwm_file_string);


        config_setting_t *pwm_steps_setting = config_setting_lookup(fan_settings, "pwm_steps");
        fan_list[fan].pwm_steps_count = config_setting_length(pwm_steps_setting);
        if (fan_list[fan].pwm_steps_count < 2) {
            fprintf(stderr, "Fan:\"%s\" had less than 2 pwm_steps\n", fan_list[fan].name);
            fprintf(stderr, "A MINIUMUM of 2 pwm_steps is required\n");
            exit(EXIT_FAILURE);
        } 
        fan_list[fan].pwm_steps = calloc(fan_list[fan].pwm_steps_count, sizeof(int));
        int past_pwm_step = -1;
        for (int j=0; j < fan_list[fan].pwm_steps_count; j++) {
            int pwm_step = config_setting_get_int_elem(pwm_steps_setting, j);

            if (0 > pwm_step ||pwm_step > 255) {
                fprintf(stderr, "Fan:\"%s\" pwm_steps had a value of \"%i\" \n", fan_list[fan].name, pwm_step);
                fprintf(stderr, "Only values from 0-255 can be set for pwm_steps\n");
                exit(EXIT_FAILURE);
            }
            if (past_pwm_step > pwm_step) {
                fprintf(stderr, "Fan:\"%s\" pwm_steps contained values of \"%i,%i\"\n", fan_list[fan].name, past_pwm_step, pwm_step);
                fprintf(stderr, "pwm_steps MUST be ordered from low to high\n");
                exit(EXIT_FAILURE);
            }
            past_pwm_step = pwm_step;

            fan_list[fan].pwm_steps[j] = pwm_step;
        }     

        config_setting_lookup_int(fan_settings, "pwm_increment_speed", &fan_list[fan].pwm_increment);
        if (fan_list[fan].pwm_increment <= 0) {
            fprintf(stderr, "Fan:\"%s\" had a pwm_increment value of \"%i\"\n", fan_list[fan].name, fan_list[fan].pwm_increment);
            fprintf(stderr, "pwm_increment values MUST be greater than 0\n");
            exit(EXIT_FAILURE);
        }
    }

    return fan_list;
}

temp_type  *config_get_temps(fan_type *fan_list) {
    config_t cfg = get_config();
    config_setting_t *setting;

    setting = config_lookup(&cfg, "temp_sensors");
    int temp_count = config_setting_length(setting);
    temp_type *temp_list = calloc(temp_count, sizeof(temp_type));
    temp_list[0].count = temp_count;

    for(int temp=0; temp < temp_count; temp++) {
        config_setting_t *temp_settings = config_setting_get_elem(setting, temp);

        config_setting_lookup_string(temp_settings, "name", &temp_list[temp].name);

        const char *temp_file;
        config_setting_lookup_string(temp_settings, "temp_file", &temp_file);
        temp_list[temp].temp_file = find_real_file_path(temp_file);
        free((void*) temp_file);

        config_setting_t *temp_steps_setting = config_setting_lookup(temp_settings, "temp_steps");
        temp_list[temp].temp_steps_count = config_setting_length(temp_steps_setting);
        if (temp_list[temp].temp_steps_count < 2) {
            fprintf(stderr, "Temp:\"%s\" had less than 2 temp_steps\n", temp_list[temp].name);
            fprintf(stderr, "A MINIUMUM of 2 temp_steps is required\n");
            exit(EXIT_FAILURE);
        }
        temp_list[temp].temp_steps = calloc(temp_list[temp].temp_steps_count, sizeof(int));
        int past_temp_step_value = -256;
        for (int temp_step=0; temp_step < temp_list[temp].temp_steps_count; temp_step++) {
            int temp_step_value = config_setting_get_int_elem(temp_steps_setting, temp_step);
            // If array is not ordered from low to high
            if (past_temp_step_value > temp_step_value) {
                fprintf(stderr, "Temp:\"%s\" temp_steps contained values of \"%i,%i\"\n", temp_list[temp].name, past_temp_step_value, temp_step_value);
                fprintf(stderr, "temp_steps MUST be ordered from low to high\n");
                exit(EXIT_FAILURE);
            }
            past_temp_step_value = temp_step_value;
            temp_list[temp].temp_steps[temp_step] = temp_step_value;
        }

        config_setting_t *assigned_fans_setting = config_setting_lookup(temp_settings, "assigned_fans");
        temp_list[temp].assigned_fans_count = config_setting_length(assigned_fans_setting);
        temp_list[temp].assigned_fans = calloc(temp_list[temp].assigned_fans_count+1, sizeof(fan_type));
        
        for (int assigned_fan=0; assigned_fan < temp_list[temp].assigned_fans_count; assigned_fan++) {
            
            int assigned_fan_found = 0;
            const char * assigned_fan_name = config_setting_get_string_elem(assigned_fans_setting, assigned_fan);
            for(int fan=0; fan < fan_list[0].count; fan++) {
                if (fan_list[fan].pwm_steps_count != temp_list[temp].temp_steps_count) {
                    fprintf(stderr, "Fan:\"%s\" had %i values in pwm_steps and Temp:\"%s\" had %i values in temp_steps \n", 
                            fan_list[fan].name, fan_list[fan].pwm_steps_count, temp_list[temp].name, temp_list[temp].temp_steps_count);
                    fprintf(stderr, "Both MUST have the same number of steps\n");
                    exit(EXIT_FAILURE);
                }

                if (strcmp(assigned_fan_name, fan_list[fan].name) == 0) {
                    temp_list[temp].assigned_fans[assigned_fan] = &fan_list[fan];
                    assigned_fan_found = 1;
                }
            }
            if (!assigned_fan_found) {
                fprintf(stderr, "Temp:\"%s\" could not find Fan:\"%s\"\n", temp_list[temp].name, assigned_fan_name);
                exit(EXIT_FAILURE);
            }
        }
    }

    return temp_list;
}