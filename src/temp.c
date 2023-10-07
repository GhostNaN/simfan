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

// struct for sorting temps
typedef struct {
    float value;
    int index;
} temp_pair;

// Compare function for qsort (sorting in descending order)
int compare_temps(const void* a, const void* b) {
    float fa = ((temp_pair*)a)->value;
    float fb = ((temp_pair*)b)->value;

    if (fa < fb) return 1;
    if (fa > fb) return -1;
    return 0;
}

// Sorts the temps from highest to lowest relative temp
void sort_temps(temp_type *temp_list, int *temp_indexes) {

    int min_temp, max_temp;
    temp_pair rel_temp_list[temp_list[0].count];

    // Sort by temp as a float between the min and max temp values
    for(int temp=0; temp < temp_list[0].count; temp++) {
            min_temp = temp_list[temp].temp_steps[0];
            max_temp = temp_list[temp].temp_steps[temp_list[temp].temp_steps_count-1];
            rel_temp_list[temp].value = (float) (temp_list[temp].curr_temp - min_temp) / (float) (max_temp - min_temp);
            rel_temp_list[temp].index = temp;
    }
    qsort(rel_temp_list, temp_list[0].count, sizeof(temp_pair), compare_temps);

    for(int temp=0; temp < temp_list[0].count; temp++) {
        temp_indexes[temp] = rel_temp_list[temp].index;
    }
}