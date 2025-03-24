#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "connmgr.h"
#include "sbuffer.h"
#include "sensor_db.h"
#include "datamgr.h"

// Global variables
pthread_mutex_t buffer_lock; // Mutex for synchronizing access to buffer
pthread_cond_t empty_buffer; // Buffer empty_buffer
sbuffer_t *buffer;           // Shared buffer
char pipe_buffer[500000000]; // Shared pipe between child and parent
int p[2];                    // Pipe
int seqNum = 1;              // Sequence number in log file
int port;                    // Port number parsed from command line arguments given to connection manager
int MAX_CONN;

// Function prototypes
void *connection_manager_func(void *arg);
void *data_manager_func(void *arg);
void *storage_manager_func(void *arg);

int main(int argc, char *argv[])
{

  // Parse port number from command line arguments
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s port\n", argv[1]);
    exit(EXIT_FAILURE);
  }
  port = atoi(argv[1]);
  MAX_CONN = atoi(argv[2]);

  // Initialize buffer & mutex
  // int a = sbuffer_init(&buffer, buffer_lock);
  int a = sbuffer_init(&buffer, &buffer_lock); // Pass address of buffer_lock

  if (a != SBUFFER_SUCCESS)
  {
    printf("buffer init failed\n");
    return 1;
  }

  pthread_t connection_manager;
  pthread_t data_manager;
  pthread_t storage_manager;

  // Check pipe for child & parent process
  if (pipe(p) < 0)
  {
    printf("failed to create pipe");
    exit(1);
  }

  // Create child process
  pid_t pid = fork();

  if (pid < 0)
  {

    printf("failed to fork");
    exit(2);
  }

  // WE ARE IN CHILD PROCESS
  if (pid == 0)
  {

    FILE *gateway_log = fopen("gateway.log", "w"); // Open log file for writing
    if (gateway_log == NULL)
    {
      perror("Error opening log file");
      exit(1);
    }
    close(p[1]); // Close writing end of pipe before reading
    char buffer[1024];
    char leftover[1024] = {0}; // Store incomplete data from previous reads
    ssize_t bytes_read;
    while ((bytes_read = read(p[0], buffer, sizeof(buffer) - 1)) > 0)
    {
      buffer[bytes_read] = '\0'; // Null-terminate buffer
      // Combine leftover data with the new buffer
      char combined[2048];
      snprintf(combined, sizeof(combined), "%s%s", leftover, buffer);
      while ((bytes_read = read(p[0], buffer, sizeof(buffer) - 1)) > 0)
      {
        buffer[bytes_read] = '\0';
        char *event = strtok(buffer, "\n");
        while (event != NULL)
        {
          time_t mytime = time(NULL);
          char *time_str = ctime(&mytime);
          time_str[strlen(time_str) - 1] = '\0';
          fprintf(gateway_log, "%d %s %s\n", seqNum, time_str, event);
          fflush(gateway_log);
          seqNum++;
          event = strtok(NULL, "\n");
        }
      }
      // Save leftover data (incomplete message) for the next read
      strncpy(leftover, combined + strlen(combined) - bytes_read, sizeof(leftover) - 1);
    }
    close(p[0]);         // Close reading end of pipe after loop
    fclose(gateway_log); // Close log file
    exit(3);
  }
  // WE ARE IN PARENT PROCESS
  else
  {
    // Create threads
    pthread_create(&connection_manager, NULL, connection_manager_func, NULL);
    pthread_create(&data_manager, NULL, data_manager_func, NULL);
    pthread_create(&storage_manager, NULL, storage_manager_func, NULL);

    // Wait for threads to finish
    pthread_join(connection_manager, NULL);
    pthread_join(data_manager, NULL);
    pthread_join(storage_manager, NULL);

    pthread_mutex_destroy(&buffer_lock);
    pthread_cond_destroy(&empty_buffer);
    sbuffer_free(&buffer);
    pthread_exit(NULL);
  }

  return 0;
}

void *connection_manager_func(void *arg)
{
  // Start server & write to buffer
  connectionmgr_main(port, MAX_CONN, buffer, buffer_lock, empty_buffer);
  pthread_exit(NULL);
  return NULL;
}

void *data_manager_func(void *arg)
{
  FILE *sensor_map = fopen("room_sensor.map", "r");
  datamgr_main(sensor_map, buffer, p, buffer_lock, empty_buffer);
  pthread_exit(NULL);
  return NULL;
}

void *storage_manager_func(void *arg)
{
  pthread_mutex_lock(&buffer_lock);
  // Make and open csv file for write
  // FILE *csv = open_db("data.csv", false);
  sbuffer_node_t *node;
  sensor_data_t data;

  while (1)
  {

    if (buffer == NULL)
    {
      pthread_mutex_unlock(&buffer_lock);
      return NULL;
    }
    node = buffer->head;
    if (node == NULL)
    { // means buffer is empty
      while (node == NULL)
      {
        pthread_cond_wait(&empty_buffer, &buffer_lock); // wait for data
        // printf("\nwaiting for data-storagemanager\n");
      }
    }

    pthread_cond_signal(&empty_buffer);

    while (node != NULL)
    {
      data = node->data;
      if (node->read_by_storagemgr == 1)
      {
        if (node->read_by_datamgr == 1)
          sbuffer_remove(buffer, &data, buffer_lock); // remove node if read by all threads
      }
      else
      {
        // insert_sensor(csv, data.id, data.value, data.ts);
        node->read_by_storagemgr = 1;
      }
      node = node->next;
    }
  }
  pthread_mutex_unlock(&buffer_lock);
  pthread_exit(NULL);
  return NULL;
}
void write_data(uint16_t id, double value, long int timestamp)
{
  FILE *csv = open_db("data.csv", true);
  insert_sensor(csv, id, value, timestamp);
  close_db(csv);
}

void write_into_log_pipe(char *msg)
{
  // write(p[1], msg, strlen(msg));

  char buffer[LOG_MESSAGE_SIZE + 1];
  snprintf(buffer, sizeof(buffer), "%s\n", msg);  
  write(p[1], buffer, strlen(buffer));           
}
