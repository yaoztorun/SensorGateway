#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#include "config.h"
#include "lib/dplist.h"
#include "sbuffer.h"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 22
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 10
#error SET_MIN_TEMP not set
#endif

#define MAX_TEMP_READINGS 5
#define MAX_SENSORS 25

/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition, ...)    do {                       \
                      if (condition) {                              \
                        printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
                        exit(EXIT_FAILURE);                         \
                      }                                             \
                    } while(0)

/**
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them. 
 *  When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
 *  \param fp_sensor_map file pointer to the map file
 *  \param fp_sensor_data file pointer to the binary data file
 */
void datamgr_main(FILE *fp_sensor_map, sbuffer_t *buffer, int p[2], pthread_mutex_t buffer_lock, pthread_cond_t condition);

/**
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free();

/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
//uint16_t datamgr_get_room_id(sensor_id_t sensor_id);

/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
//sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);

/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
//time_t datamgr_get_last_modified(sensor_id_t sensor_id);

/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
//int datamgr_get_total_sensors();


#endif  //DATAMGR_H_
