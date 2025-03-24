
#include "datamgr.h"

typedef struct{
    uint16_t sensor_id;
    uint16_t room_id;
    double temp_avg;
    time_t last_modified;
    double temperatures[RUN_AVG_LENGTH];
    int nrOfTemp;
} my_element_t;


 
typedef struct {
    uint16_t sensor_id;
    double temp_avg;
    int nrOfTemp;
    double temperatures[RUN_AVG_LENGTH];
    long last_modified;
} sensor_t;

typedef struct {
    sensor_t sensors[MAX_SENSORS];
    int count;
} sensor_store_t;


dplist_t *list;

my_element_t *sensor;
 
dplist_t *sensor_list = NULL;  // Declare your dynamic list

uint16_t sensor_ids[MAX_SENSORS];
int sensor_count;
sensor_store_t store;


sensor_t* get_sensor_by_id(sensor_store_t *store, uint16_t sensor_id) {
    for (int i = 0; i < store->count; i++) {
        if (store->sensors[i].sensor_id == sensor_id) {
            return &store->sensors[i];
        }
    }
    return NULL;
}

void read_sensor_ids(FILE *sensor_map, uint16_t sensor_ids[], int *count) {
    *count = 0;
    uint16_t roo_id, sensor_id;

    // Read each line and extract the sensor ID
    while (fscanf(sensor_map, "%hu %hu", &roo_id, &sensor_id) == 2) {
        if (*count >= MAX_SENSORS) {
            break;
        }
        sensor_ids[(*count)++] = sensor_id;
    }
}
 

int update_sensor(sensor_store_t *store, uint16_t sensor_id, double new_temp, long timestamp) {
    sensor_t *sensor = get_sensor_by_id(store, sensor_id);

    // If sensor doesn't exist, create a new one
    if (sensor == NULL) {
        if (store->count >= MAX_SENSORS) {
            return -1;
        }

        // Initialize a new sensor
        sensor_t new_sensor = {0};
        new_sensor.sensor_id = sensor_id;
        new_sensor.temperatures[0] = new_temp;
        new_sensor.nrOfTemp = 1;
        new_sensor.temp_avg = new_temp;
        new_sensor.last_modified = timestamp;

        // Add the new sensor to the store
        store->sensors[store->count++] = new_sensor;
        return 0;
    }

    // Update the existing sensor
    if (sensor->nrOfTemp < RUN_AVG_LENGTH) {
        sensor->temperatures[sensor->nrOfTemp++] = new_temp;
    } else {
        for (int i = 1; i < RUN_AVG_LENGTH; i++) {
            sensor->temperatures[i - 1] = sensor->temperatures[i];
        }
        sensor->temperatures[RUN_AVG_LENGTH - 1] = new_temp;
    }

    // Recalculate the average temperature
    double sum = 0;
    for (int i = 0; i < sensor->nrOfTemp; i++) {
        sum += sensor->temperatures[i];
    }
    sensor->temp_avg = sum / sensor->nrOfTemp;

    // Update the last modified time
    sensor->last_modified = timestamp;

    return 0;
}

void datamgr_main(FILE *fp_sensor_map, sbuffer_t *buffer, int p[2], pthread_mutex_t buffer_lock, pthread_cond_t condition){
     // Read the sensor IDs from the file
    read_sensor_ids(fp_sensor_map, sensor_ids, &sensor_count);

    // Close the file
    fclose(fp_sensor_map);

// let's read data from buffer
    pthread_mutex_lock(&buffer_lock);
    sbuffer_node_t *node = malloc(sizeof(sbuffer_node_t));
    node = buffer->head;

    if (node == NULL) {  // means buffer is empty
        while (node == NULL){
            pthread_cond_wait(&condition, &buffer_lock); // wait for data
            // printf("\nwaiting for data-datamanager\n");
        }
    }
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&buffer_lock);
    print_content(list);  
}

 

// Function to handle sensor data (modified)
void datamgr_connection(uint16_t data_id, double value, long timestamp) {
    char msg[LOG_MESSAGE_SIZE];

    // Try to find the sensor with the given ID
    sensor_t *sensor = get_sensor_by_id(&store, data_id);
    
    if (sensor == NULL) {

        // Sensor with this ID doesn't exist, log it
        sprintf(msg, "Received sensor data with invalid sensor node %d.\n", data_id);
        write_into_log_pipe(msg);

        update_sensor(&store, data_id, value, timestamp);
        
    }

    // Process the sensor data (if the sensor exists)
    if (sensor != NULL) {

        // Handle temperature data
        if (sensor->nrOfTemp < RUN_AVG_LENGTH) {
            sensor->temperatures[sensor->nrOfTemp] = value;
            sensor->nrOfTemp++;
        }

        // If we have enough readings to compute the average
        if (sensor->nrOfTemp >= RUN_AVG_LENGTH) {
            double sum = 0;
            for (int i = 0; i < RUN_AVG_LENGTH; i++) {
                sum += sensor->temperatures[i];
            }
            double avg = sum / RUN_AVG_LENGTH;
            sensor->temp_avg = avg;

            // Check if the temperature is too hot or too cold
            if (avg < SET_MIN_TEMP) {
                sprintf(msg, "Sensor node %d reports it’s too cold (avg temp = %f).\n", data_id, avg);
                write_into_log_pipe(msg);
            }
            if (avg > SET_MAX_TEMP) {
                sprintf(msg, "Sensor node %d reports it’s too hot (avg temp = %f).\n", data_id, avg);
                write_into_log_pipe(msg);
            }

            // Update last modified timestamp
            sensor->last_modified = timestamp;

            // Shift the temperature readings
            for (int i = 0; i < RUN_AVG_LENGTH - 1; i++) {
                sensor->temperatures[i] = sensor->temperatures[i + 1];
            }
            sensor->temperatures[RUN_AVG_LENGTH - 1] = value;  // Insert the new temperature
        }
    }




}

void datamgr_free(){
    dpl_free(&list, true);
}

