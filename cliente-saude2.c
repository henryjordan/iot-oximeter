#include "oc_api.h"

// bibliotecas do socket-server
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// funções do socket-server
void sendData( int sockfd, int x ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d\n", x );
  if ( (n = write( sockfd, buffer, strlen(buffer) ) ) < 0 )
    printf( "ERROR writing to socket\n");
  buffer[n] = '\0';
}

int getData( int sockfd ) {
  char buffer[32];
  int n;

  if ( (n = read(sockfd,buffer,31) ) < 0 )
    printf( "ERROR reading from socket\n");
  buffer[n] = '\0';
  return atoi( buffer );
}

// funcções do simple-client
static int
app_init(void)
{
  int ret = oc_init_platform("Apple", NULL, NULL);
  ret |= oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "1.0", "1.0",
                       NULL, NULL);
  return ret;
}

#define MAX_URI_LENGTH (30)
static char a_oximeter[MAX_URI_LENGTH];
static oc_server_handle_t oximeter_server;

static bool state;
static int BPM;
static oc_string_t name;
char buffer[256];
int n, newsockfd;

static void
get_oximeter(oc_client_response_t *data)
{
  PRINT("GET_oximeter:\n");
  oc_rep_t *rep = data->payload;
  //BPM = getData( newsockfd );
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      PRINT("%d\n", rep->value.boolean);
      state = rep->value.boolean;
      break;
    case INT:
      BPM = getData( newsockfd );
      PRINT("%d\n", rep->value.integer);
      BPM = rep->value.integer;
      sendData( newsockfd, BPM );
      break;
    case STRING:
      PRINT("%s\n", oc_string(rep->value.string));
      if (oc_string_len(name))
        oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      break;
    }
    rep = rep->next;
  }
}

static oc_discovery_flags_t
discovery(const char *di, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_server_handle_t *server,
          void *user_data)
{
  PRINT("FAZENDO DISCOVERY\n");
  (void)di;
  (void)user_data;
  (void)interfaces;
  int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;
// retirei um dos argumentos do if implementados no simpleclient para realizar o GET
  for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
    PRINT("DENTRO DO FOR\n");
    char *t = oc_string_array_get_item(types, i);
    PRINT("%d \n", strncmp(t, "core.oximeter", 13));
    if (strlen(t) == 13 && strncmp(t, "core.oximeter", 13) == 0) {
      memcpy(&oximeter_server, server, sizeof(oc_server_handle_t));

      strncpy(a_oximeter, uri, uri_len);
      a_oximeter[uri_len] = '\0';

      oc_do_get(a_oximeter, &oximeter_server, NULL, &get_oximeter, LOW_QOS, NULL);
      PRINT("Sent GET request\n");

      return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}

static void
issue_requests(void)
{
  oc_do_ip_discovery("core.oximeter", &discovery, NULL);
}

#include "port/oc_clock.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

pthread_mutex_t mutex;
pthread_cond_t cv;
struct timespec ts;

int quit = 0;

static void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mutex);
}

void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

int
main(void)
{
  int init;
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);

  int sockfd, portno = 44445, clilen;
  struct sockaddr_in serv_addr, cli_addr;

  printf( "using port #%d\n", portno );
    
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
      printf("ERROR opening socket\n");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons( portno );
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
  printf( "ERROR on binding\n" );
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  
  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .requests_entry = issue_requests };

  oc_clock_time_t next_event;

#ifdef OC_SECURITY
  oc_storage_config("./creds");
#endif /* OC_SECURITY */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

  while(quit != 1){
      printf( "waiting for new client...\n" );
      if ( ( newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen) ) < 0 )
         printf("ERROR on accept\n");
      printf( "opened new communication with client\n" );
  
      while (quit != 1) {
         next_event = oc_main_poll(); // onde ocorre o GET e OBSERVE
         pthread_mutex_lock(&mutex);
         if (next_event == 0) {
           pthread_cond_wait(&cv, &mutex);
         } else {
           ts.tv_sec = (next_event / OC_CLOCK_SECOND);
           ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
           pthread_cond_timedwait(&cv, &mutex, &ts);
         }
         pthread_mutex_unlock(&mutex);
      }
     close( newsockfd );
  }

  oc_main_shutdown();
  return 0;
}
