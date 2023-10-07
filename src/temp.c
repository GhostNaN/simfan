#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <simfan.h>

void read_temps(temp_type *temp_list) {
    FILE *file;
    for(int temp=0; temp < temp_list[0].count; temp++) {
        if  ((file = fopen(temp_list[temp].temp_file, "r")) == NULL) {
            perror(temp_list[temp].temp_file);
            exit(EXIT_FAILURE);
        }
        int curr_temp;
        if (fscanf(file, "%d", &curr_temp) == 0) {
            perror(temp_list[temp].temp_file);
            exit(EXIT_FAILURE);
        }
        // Round temp
        temp_list[temp].curr_temp = ((float) curr_temp / 1000) + 0.5;
        fclose(file);

        if (VERBOSE)
            printf("Temp:\"%s\" is at %i°C\n", temp_list[temp].name, temp_list[temp].curr_temp);
    }
}

int temp_changed(temp_type *temp_list, int *past_temps) {

    int temp_changed = 0;
    for(int temp=0; temp < temp_list[0].count; temp++) {   

            if ((temp_list[temp].curr_temp < (past_temps[temp] - THRESHOLD)) || (temp_list[temp].curr_temp > (past_temps[temp] + THRESHOLD))) {
                if (VERBOSE)
                    printf("Temp:\"%s\" when from %i°C to %i°C\n", temp_list[temp].name, past_temps[temp], temp_list[temp].curr_temp);
                past_temps[temp] = temp_list[temp].curr_temp;
                temp_changed = 1;
            }
    }
    return temp_changed;
}

// Sorts the temps from highest to lowest relative temp
void sort_temps(temp_type *temp_list, int *temp_indexes) {

    float rel_temp;
    float top_temps[temp_list[0].count];
    int min_temp, max_temp;

    for(int index=0; index < temp_list[0].count; index++) {
        for(int temp=0; temp < temp_list[0].count; temp++) {
            
            min_temp = temp_list[temp].temp_steps[0];
            max_temp = temp_list[temp].temp_steps[temp_list[temp].temp_steps_count-1];
            rel_temp = (float) (temp_list[temp].curr_temp - min_temp) / (float) (max_temp - min_temp);

            // Highest relative temp and not the same a previous highest temp
            if (index==0) {
                if (rel_temp >= top_temps[index] || temp==0) {
                    top_temps[index] = rel_temp;
                    temp_indexes[index] = temp;
                }
            }
            else {
                if ((rel_temp >= top_temps[index] && rel_temp < top_temps[index-1]) || temp==0) {
                    top_temps[index] = rel_temp;
                    temp_indexes[index] = temp;
                }
            }
        }
    }
}