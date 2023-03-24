/*
 * Copyright (c) 2019-2023  High Performance Computing Center Stuttgart,
 *                          University of Stuttgart.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <papi.h>
#include <pthread.h>

#ifndef NDEBUG
#define PAPI_CHECK(PAPI_CMD, MSG)                                              \
  do {                                                                         \
    int retval = (PAPI_CMD);                                                   \
    if ((retval) != PAPI_OK) {                                                 \
      PAPI_perror(MSG);                                                        \
    }                                                                          \
  } while (0);
#else /* NDEBUG */
#define PAPI_CHECK(PAPI_CMD, MSG) (PAPI_CMD);
#endif /* NDEBUG */

static int eventSet = PAPI_NULL;

int MPI_Init(int *argc, char ***argv) {
  int ret = MPI_SUCCESS;

  char *eventnames_env = getenv("MPI_PAPI_COLLECTOR_EVENTS");
  char *eventnames = strdup(eventnames_env);

  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
    PAPI_perror("Error initializing PAPI library");
  }
  PAPI_CHECK(PAPI_thread_init(pthread_self) != PAPI_OK,
             "Error initializing PAPI thread support");

  /* adding events */
  PAPI_CHECK(PAPI_create_eventset(&eventSet), "Error to create PAPI event set");

  char *saveptr = NULL;
  char *eventName = strtok_r(eventnames, ",", &saveptr);
  while (eventName != NULL) {
    PAPI_CHECK(PAPI_add_named_event(eventSet, eventName),
               "Error adding PAPI event");
    eventName = strtok_r(NULL, ",", &saveptr);
  }
  free(eventnames);

  PAPI_CHECK(PAPI_start(eventSet), "Failed starting PAPI counters.");

  ret = PMPI_Init(argc, argv);
  return ret;
}

int MPI_Finalize() {
  int ret = MPI_SUCCESS;

  int num_events = PAPI_num_events(eventSet);
  if (num_events <= 0) {
    PAPI_perror("Failed determining the number of PAPI counters.");
  }
  long long *counters = (long long *)malloc(num_events * sizeof(long long));

  PAPI_stop(eventSet, counters);

  int world_rank = 0;
  int world_size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // MPI_Allreduce(MPI_IN_PLACE, &counters, num_events, MPI_LONG_LONG,
  // MPI_SUM, MPI_COMM_WORLD);
  long long *all_counters = NULL;
  int root = 0;
  if (world_rank == root) {
    all_counters =
        (long long *)malloc(world_size * num_events * sizeof(long long));
  }
  MPI_Gather(counters, num_events, MPI_LONG_LONG, all_counters, num_events,
             MPI_LONG_LONG, root, MPI_COMM_WORLD);
  free(counters);

  if (world_rank == 0) {
    FILE *fh = stdout;

    int events[num_events];
    PAPI_list_events(eventSet, events, &num_events);
    fprintf(
        fh,
        "\n\n---------- COUNTER SUMMARY accross MPI processes ----------\n");
    fprintf(fh, "%s\t", "rank");
    for (int i = 0; i < num_events; i++) {
      char eventName[PAPI_MAX_STR_LEN];
      PAPI_event_code_to_name(events[i], eventName);
      fprintf(fh, "%s\t", eventName);
    }
    fprintf(fh, "\n");

    for (int rank = 0; rank < world_size; rank++) {
      fprintf(fh, "%d\t", rank);
      for (int i = 0; i < num_events; i++) {
        int idx = rank * num_events + i;
        fprintf(fh, "%lld\t", all_counters[idx]);
      }
      fprintf(fh, "\n");
    }
    free(all_counters);
  }

  ret = PMPI_Finalize();
  return ret;
}
