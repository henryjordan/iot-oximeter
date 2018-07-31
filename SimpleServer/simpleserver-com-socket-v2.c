/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "oc_api.h"

// bibliotecas do socketclient
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

// funções do socketclient
void error(char *msg) {
    perror(msg);
    exit(0);
}

void sendData( int sockfd, int x ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d\n", x );
  if ( (n = write( sockfd, buffer, strlen(buffer) ) ) < 0 )
      error( const_cast<char *>( "ERROR writing to socket") );
  buffer[n] = '\0';
}

int getData( int sockfd ) {
  char buffer[32];
  int n;

  if ( (n = read(sockfd,buffer,31) ) < 0 )
       error( const_cast<char *>( "ERROR reading from socket") );
  buffer[n] = '\0';
  return atoi( buffer );
}

// código do simpleserver
static bool state = false;
int bpm;
oc_string_t name;

static int
app_init(void)
{
  int ret = oc_init_platform("Intel", NULL, NULL);
  ret |=
    oc_add_device("/oic/d", "oic.d.oximeter", "Oximeter", "1.0", "1.0", NULL, NULL);
  oc_new_string(&name, "Oxímetro", 12);
  return ret;
}

static void
get_oximeter(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  int sockfd, portno = 51717, n;
  char serverIp[] = "192.168.25.115";
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];

  (void)user_data;
  // ++power; atualização dos dados

  PRINT("GET_oximeter:\n");
  oc_rep_start_root_object();
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
  case OC_IF_RW:
    oc_rep_set_boolean(root, state, state);
    oc_rep_set_int(root, power, power);
    oc_rep_set_text_string(root, name, oc_string(name));
    break;
  default:
    break;
  }
  oc_rep_end_root_object();
  oc_send_response(request, OC_STATUS_OK);
}

/*static void
post_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  PRINT("POST_light:\n");
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: %s ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      state = rep->value.boolean;
      PRINT("value: %d\n", state);
      break;
    case INT:
      power = rep->value.integer;
      PRINT("value: %d\n", power);
      break;
    case STRING:
      oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      oc_send_response(request, OC_STATUS_BAD_REQUEST);
      return;
      break;
    }
    rep = rep->next;
  }
  oc_send_response(request, OC_STATUS_CHANGED);
}

static void
put_light(oc_request_t *request, oc_interface_mask_t interface,
           void *user_data)
{
  (void)interface;
  (void)user_data;
  post_light(request, interface, user_data);
}*/

static void
register_resources(void)
{
  oc_resource_t *res = oc_new_resource("/a/oximeter", 2, 0);
  oc_resource_bind_resource_type(res, "core.oximeter");
  oc_resource_bind_resource_type(res, "core.oxygen");
  oc_resource_bind_resource_type(res, "core.BPM");
  oc_resource_bind_resource_interface(res, OC_IF_RW);
  oc_resource_set_default_interface(res, OC_IF_RW);
  oc_resource_set_discoverable(res, true);
  oc_resource_set_periodic_observable(res, 1);
  oc_resource_set_request_handler(res, OC_GET, get_oximeter, NULL);
  oc_add_resource(res);
}


#if defined(CONFIG_MICROKERNEL) || defined(CONFIG_NANOKERNEL) /* Zephyr */

#include <sections.h>
#include <string.h>
#include <zephyr.h>

static struct nano_sem block;

static void
signal_event_loop(void)
{
  nano_sem_give(&block);
}

void
main(void)
{
  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .register_resources =
                                         register_resources };

  nano_sem_init(&block);

  if (oc_main_init(&handler) < 0)
    return;

  oc_clock_time_t next_event;

  while (true) {
    next_event = oc_main_poll();
    if (next_event == 0)
      next_event = TICKS_UNLIMITED;
    else
      next_event -= oc_clock_time();
    nano_task_sem_take(&block, next_event);
  }

  oc_main_shutdown();
}

#elif defined(__linux__) /* Linux */
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

  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .register_resources =
                                         register_resources };

  oc_clock_time_t next_event;

#ifdef OC_SECURITY
  oc_storage_config("./creds");
#endif /* OC_SECURITY */

  init = oc_main_init(&handler);
  if (init < 0)
    return init;

  while (quit != 1) {
    next_event = oc_main_poll();
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

  oc_main_shutdown();
  return 0;
}
#endif /* __linux__ */
