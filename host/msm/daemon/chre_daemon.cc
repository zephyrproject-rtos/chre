/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * The daemon that hosts CHRE on the SLPI via FastRPC.
 *
 * Several threads are required for this functionality:
 *   - Main thread: blocked waiting on SIGINT/SIGTERM, and requests graceful
 *     shutdown of CHRE when caught
 *   - Monitor thread: persistently blocked in a FastRPC call to the SLPI that
 *     only returns when CHRE exits or the SLPI crashes
 *     - TODO: see whether we can merge this with the RX thread
 *   - Reverse monitor thread: after initializing the SLPI-side monitor for this
 *     process, blocks on a condition variable. If this thread exits, CHRE on
 *     the SLPI side will be notified and shut down (this is only possible if
 *     this thread is not blocked in a FastRPC call).
 *     - TODO: confirm this and see whether we can merge this responsibility
 *       into the TX thread
 *   - Message to host (RX) thread: blocks in FastRPC call, waiting on incoming
 *     message from CHRE
 *   - Message to CHRE (TX) thread: blocks waiting on outbound queue, delivers
 *     messages to CHRE over FastRPC
 */

#include "chre/platform/slpi/fastrpc.h"
#include "chre_host/log.h"
#include "generated/chre_slpi.h"

#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef void *(thread_entry_point_f)(void *);

struct reverse_monitor_thread_data {
  pthread_t       thread;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
};

static void *chre_message_to_host_thread(void *arg);
static void *chre_monitor_thread(void *arg);
static void *chre_reverse_monitor_thread(void *arg);
static bool init_reverse_monitor(struct reverse_monitor_thread_data *data);
static bool set_signal_handler(void);
static bool start_thread(pthread_t *thread_handle,
                         thread_entry_point_f *thread_entry,
                         void *arg);
static void signal_handler(int sig);
static void wait_on_signal(void);

//! Set to true when we request a graceful shutdown of CHRE
static volatile bool chre_shutdown_requested = false;

// TODO: debug-only code
static void log_buffer(const uint8_t *buffer, size_t size) {
  char line[32];
  int offset = 0;
  char line_chars[32];
  int offset_chars = 0;

  LOGD("Dumping buffer of size %zu bytes", size);
  for (size_t i = 1; i <= size; ++i) {
    offset += snprintf(&line[offset], sizeof(line) - offset, "%02x ",
                       buffer[i - 1]);
    offset_chars += snprintf(
        &line_chars[offset_chars], sizeof(line_chars) - offset_chars,
        "%c", (isprint(buffer[i - 1])) ? buffer[i - 1] : '.');
    if ((i % 8) == 0) {
      LOGD("  %s\t%s", line, line_chars);
      offset = 0;
      offset_chars = 0;
    } else if ((i % 4) == 0) {
      offset += snprintf(&line[offset], sizeof(line) - offset, " ");
    }
  }

  if (offset > 0) {
    char tabs[8];
    char *pos = tabs;
    while (offset < 28) {
      *pos++ = '\t';
      offset += 8;
    }
    *pos = '\0';
    LOGD("  %s%s%s", line, tabs, line_chars);
  }
}

/**
 * Entry point for the thread that receives messages sent by CHRE.
 *
 * @return always returns NULL
 */
static void *chre_message_to_host_thread(void *arg) {
  // TODO: size this appropriately to handle encoded messages
  unsigned char messageBuffer[4096];
  unsigned int messageLen;
  int result = 0;
  (void) arg;

  while (!chre_shutdown_requested) {
    messageLen = 0;
    LOGD("Calling into chre_slpi_get_message_to_host");
    result = chre_slpi_get_message_to_host(
        messageBuffer, sizeof(messageBuffer), &messageLen);
    LOGV("Got message from CHRE with size %u (result %d)", messageLen, result);

    if (result == CHRE_FASTRPC_ERROR_SHUTTING_DOWN) {
      LOGD("CHRE shutting down, exiting CHRE->Host message thread");
      break;
    } else {
      // TODO: deliver message to clients
      log_buffer(messageBuffer, messageLen);
    }
  }

  LOGV("Message to host thread exited");
  return NULL;
}

/**
 * Entry point for the thread that blocks in a FastRPC call to monitor for
 * abnormal exit of CHRE or reboot of the SLPI.
 *
 * @return always returns NULL
 */
static void *chre_monitor_thread(void *arg) {
  (void) arg;
  int ret = chre_slpi_wait_on_thread_exit();
  if (!chre_shutdown_requested) {
    LOGE("Detected unexpected CHRE thread exit (%d)\n", ret);
    exit(EXIT_FAILURE);
  }

  LOGV("Monitor thread exited");
  return NULL;
}

/**
 * Entry point for the "reverse" monitor thread, which invokes a FastRPC method
 * to register a thread destructor, and blocks waiting on a condition variable.
 * This allows for the code running in the SLPI to detect abnormal shutdown of
 * the host-side binary and perform graceful cleanup.
 *
 * @return always returns NULL
 */
static void *chre_reverse_monitor_thread(void *arg) {
  struct reverse_monitor_thread_data *thread_data =
      (struct reverse_monitor_thread_data *) arg;

  int ret = chre_slpi_initialize_reverse_monitor();
  if (ret != CHRE_FASTRPC_SUCCESS) {
    LOGE("Failed to initialize reverse monitor on SLPI: %d", ret);
  } else {
    // Block here on the condition variable until the main thread notifies
    // us to exit
    pthread_mutex_lock(&thread_data->mutex);
    pthread_cond_wait(&thread_data->cond, &thread_data->mutex);
    pthread_mutex_unlock(&thread_data->mutex);
  }

  LOGV("Reverse monitor thread exited");
  return NULL;
}

/**
 * Initializes the data shared with the reverse monitor thread, and starts the
 * thread.
 *
 * @param data Pointer to structure containing the (uninitialized) condition
 *        variable and associated data passed to the reverse monitor thread
 *
 * @return true on success
 */
static bool init_reverse_monitor(struct reverse_monitor_thread_data *data) {
  bool success = false;
  int ret;

  if ((ret = pthread_mutex_init(&data->mutex, NULL)) != 0) {
    LOG_ERROR("Failed to initialize mutex", ret);
  } else if ((ret = pthread_cond_init(&data->cond, NULL)) != 0) {
    LOG_ERROR("Failed to initialize condition variable", ret);
  } else if (!start_thread(&data->thread, chre_reverse_monitor_thread, data)) {
    LOGE("Couldn't start reverse monitor thread");
  } else {
    success = true;
  }

  return success;
}

/**
 * Installs a signal handler to catch SIGINT/SIGTERM
 *
 * @return true on success
 */
static bool set_signal_handler(void) {
  struct sigaction sa;
  int ret;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;

  if ((ret = sigaction(SIGINT, &sa, NULL)) != 0) {
    LOG_ERROR("Couldn't set handler for SIGINT", ret);
  } else if ((ret = sigaction(SIGTERM, &sa, NULL)) != 0) {
    LOG_ERROR("Couldn't set handler for SIGTERM", ret);
  }

  return (ret == 0);
}

/**
 * Simple signal handler installed by set_signal_handler()
 */
static void signal_handler(int sig) {
  // Don't do anything here: this exists to allow SIGINT/SIGTERM to cause
  // sigwait() to return before terminating the process, which leads to
  // graceful shutdown
  LOGD("Caught signal %d", sig);
}

/**
 * Start a thread with default attributes, or log an error on failure
 *
 * @return bool true if the thread was successfully started
 */
static bool start_thread(pthread_t *thread_handle,
                         thread_entry_point_f *thread_entry,
                         void *arg) {
  int ret = pthread_create(thread_handle, NULL, thread_entry, arg);
  if (ret != 0) {
    LOG_ERROR("pthread_create failed", ret);
  }
  return (ret == 0);
}

/**
 * Installs the signal handler and blocks until a signal is caught
 */
static void wait_on_signal(void) {
  if (set_signal_handler()) {
    sigset_t signal_mask;

    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGTERM);

    int sig;
    int ret = sigwait(&signal_mask, &sig);
    if (ret != 0) {
      LOG_ERROR("Failed to wait on signal mask", ret);
    }
  }
}

int main() {
  int ret = -1;
  pthread_t monitor_thread;
  pthread_t msg_to_host_thread;
  struct reverse_monitor_thread_data reverse_monitor;

  if (!init_reverse_monitor(&reverse_monitor)) {
    LOGE("Couldn't initialize reverse monitor");
  } else if ((ret = chre_slpi_start_thread()) != CHRE_FASTRPC_SUCCESS) {
    LOGE("Failed to start CHRE on SLPI: %d", ret);
  } else {
    if (!start_thread(&monitor_thread, chre_monitor_thread, NULL)) {
      LOGE("Couldn't start monitor thread");
    } else if (!start_thread(&msg_to_host_thread, chre_message_to_host_thread,
                             NULL)) {
      LOGE("Couldn't start CHRE->Host message thread");
    } else {
      LOGI("CHRE on SLPI started");
      wait_on_signal();
    }

    chre_shutdown_requested = true;
    ret = chre_slpi_stop_thread();
    if (ret != CHRE_FASTRPC_SUCCESS) {
      LOGE("Failed to stop CHRE on SLPI: %d", ret);
    } else {
      // TODO: don't call pthread_join if the thread failed to start
      LOGV("Joining monitor thread");
      ret = pthread_join(monitor_thread, NULL);
      if (ret != 0) {
        LOG_ERROR("Join on monitor thread failed", ret);
      }

      LOGV("Joining reverse monitor thread");
      pthread_cond_signal(&reverse_monitor.cond);
      ret = pthread_join(reverse_monitor.thread, NULL);
      if (ret != 0) {
        LOG_ERROR("Join on reverse monitor thread failed", ret);
      }

      LOGV("Joining message to host thread");
      ret = pthread_join(msg_to_host_thread, NULL);
      if (ret != 0) {
        LOG_ERROR("Join on monitor thread failed", ret);
      }

      LOGI("Shutdown complete");
    }
  }

  return ret;
}

