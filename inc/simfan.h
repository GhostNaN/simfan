#ifndef SIMFAN_H 
#define SIMFAN_H

extern int VERBOSE;
extern int INTERVAL;
extern int THRESHOLD;
extern char *CONFIG_FILE;
extern int KILLALL;

typedef struct fan_type {
    const char *name;
    char *pwm_file;
    int *pwm_steps;
    int  pwm_steps_count;
    int pwm_increment;
    int default_pwm_enable;
    int count;
    int curr_pwm;
    int target_pwm;
    char *temp_set;
} fan_type;

typedef struct temp_type {
    const char *name;
    char *temp_file;
    int *temp_steps;
    int  temp_steps_count;
    fan_type **assigned_fans;
    int assigned_fans_count;
    int count;
    int curr_temp;
} temp_type;

void config_set_globals();
fan_type  *config_get_fans();
temp_type  *config_get_temps(fan_type *fan_list);

void set_pwm_enable_mode(fan_type fan, int mode);
void read_curr_pwm(fan_type *fan_list);

void read_temps(temp_type *temp_list);
int temp_changed(temp_type *temp_list, int *past_temps);
void sort_temps(temp_type *temp_list, int *temp_indexes) ;
void set_temp_fans_target_pwm( temp_type temp);
void set_fans(fan_type *fan_list);
void max_fans(fan_type *fan_list);

#endif
