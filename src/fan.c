#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <simfan.h>

void set_pwm_enable_mode(fan_type fan, int mode) {

    char *pwm_enable_file = calloc(strlen(fan.pwm_file) + strlen("_enable")+1,  sizeof(char));
    strcpy(pwm_enable_file, fan.pwm_file);
    strcat(pwm_enable_file, "_enable");

    FILE *file;
    if ((file = fopen(pwm_enable_file, "w")) == NULL) {
        perror(pwm_enable_file);
        exit(EXIT_FAILURE);
    }
    char pwm_enable_string[3];
    sprintf(pwm_enable_string, "%d", mode);
    if (fwrite(&pwm_enable_string, sizeof(pwm_enable_string), 1, file) == -1)  {
        perror(pwm_enable_file);
        exit(EXIT_FAILURE);
    }
    if (VERBOSE)
        printf("Fan:\"%s\" pwm_enable set to %i\n", fan.name, mode);
    fclose(file);
    free((void*) pwm_enable_file);
}

void read_curr_pwm(fan_type *fan_list) {
    FILE *file;

    for(int fan=0; fan < fan_list[0].count; fan++) {
        if  ((file = fopen(fan_list[fan].pwm_file, "r")) == NULL) {
            perror(fan_list[fan].pwm_file);
            exit(EXIT_FAILURE);
        }
        if (fscanf(file, "%d", &fan_list[fan].curr_pwm) == 0) {
            perror(fan_list[fan].pwm_file);
            exit(EXIT_FAILURE);
        }
    }
}

static void write_pwm (char *file_name, int pwm_num) {
    FILE *file;

    if  ((file = fopen(file_name, "w")) == NULL) {
        perror(file_name);
        exit(EXIT_FAILURE);
    }

    char pwm_string[3];
    sprintf(pwm_string, "%d", pwm_num);
    if (fwrite(&pwm_string, sizeof(pwm_string), 1, file) == -1)  {
        perror(file_name);
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

void set_temp_fans_target_pwm(temp_type temp, int *temp_indexes) {

    float rel_temp_pos, rel_fan_step;
    int pwm = 255;
    for (int fan=0; fan < temp.assigned_fans_count; fan++) {
        if (temp.assigned_fans[fan]->temp_set == NULL) {
            if (temp.curr_temp <= temp.temp_steps[0]) {
                pwm = temp.assigned_fans[fan]->pwm_steps[0];
            }
            if (temp.curr_temp >= temp.temp_steps[temp.temp_steps_count-1]) {
                pwm =  temp.assigned_fans[fan]->pwm_steps[temp.temp_steps_count-1];
            }
            else {
                for(int temp_step=temp.temp_steps_count-2; temp_step >= 0; temp_step--) { 
                    if (temp.curr_temp >= temp.temp_steps[temp_step]) {
                        //  Find where curr_temp sits between this_temp_step and next_temp_step as a float between 0.0-1.0
                        //  (curr_temp - this_temp_step) / (next_temp_step - this_temp_step)
                        rel_temp_pos = ( (float) temp.curr_temp - (float) temp.temp_steps[temp_step]) / ( (float) temp.temp_steps[temp_step+1] - (float) temp.temp_steps[temp_step]);
                        //  Apply rel_temp_pos value to find pwm value above this_pwm_step
                         // (next_pwm_step - this_pwm_step) * rel_temp_pos) + 0.5
                        rel_fan_step = (((temp.assigned_fans[fan]->pwm_steps[temp_step+1] - temp.assigned_fans[fan]->pwm_steps[temp_step]) * rel_temp_pos) + 0.5);
                        // Add on the rel_fan_step to this_pwm_step
                        pwm = temp.assigned_fans[fan]->pwm_steps[temp_step] + rel_fan_step ;
                        break;
                    }
                }
            }
            temp.assigned_fans[fan]->target_pwm = pwm;
            temp.assigned_fans[fan]->temp_set = (char*) temp.name;
        }
    }
}

void set_fans(fan_type *fan_list) {

    for (int fan=0; fan < fan_list[0].count; fan++) {
        if  (fan_list[fan].curr_pwm !=  fan_list[fan].target_pwm) {
            
            int pwm = fan_list[fan].target_pwm;
            if (fan_list[fan].curr_pwm < fan_list[fan].target_pwm) {
                pwm = fan_list[fan].curr_pwm + fan_list[fan].pwm_increment;
                if(pwm > fan_list[fan].target_pwm)
                    pwm = fan_list[fan].target_pwm;
            }
            else if (fan_list[fan].curr_pwm > fan_list[fan].target_pwm) {
                pwm = fan_list[fan].curr_pwm - fan_list[fan].pwm_increment;
                if(pwm < fan_list[fan].target_pwm)
                    pwm = fan_list[fan].target_pwm;
            }

            write_pwm(fan_list[fan].pwm_file, pwm);
            fan_list[fan].curr_pwm = pwm;

            if (VERBOSE)
                printf("Fan:\"%s\" set to PWM:\"%i\" because of Temp:\"%s\"\n", fan_list[fan].name, pwm, fan_list[fan].temp_set);
        }
    }

}