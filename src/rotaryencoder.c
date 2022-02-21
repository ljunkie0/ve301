/************************************************************
*
* Code for reading and debouncing from
*   https://github.com/joan2937/pigpio
* Added
*   - polling thread
*   - a second encoder
*   - support for hold event
*   - an event queue where the thread puts what it read
*
**************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>

#include "base.h"
#include "rotaryencoder.h"

#define encoderA 11
#define encoderB 10
#define pushButton 6
#define encoderA2 0
#define encoderB2 2
#define pushButton2 3

#define LEFT -1
#define RIGHT 1
#define PUSHED 2
#define HELD 3

#define HELD_SEC 0
#define HELD_NSEC 500000000

#define EVENT_QUEUE_LOCK 0

typedef void (*pi_renc_cb_t)(int);

struct _pi_renc_s;

typedef struct _pi_renc_s pi_renc_t;

/*
   This function releases the resources used by the decoder.
*/
void pi_renc_cancel(pi_renc_t *renc);

static int queued_events;
static int *event_queue;
static int event_pointer;

struct _pi_renc_s
{
   int id;
   int gpioLeft;
   int gpioRight;
   int gpioPush;
   pi_renc_cb_t callback;
   int levLeft;
   int levRight;
   int levPush;
   int lastGpio;
   timer_t *push_button_timer;
   int timerFired;
};

pi_renc_t * renc1;
pi_renc_t * renc2;

int pigpiod_id;

pthread_t *encoder_thread;

/*

             +---------+         +---------+      0
             |         |         |         |
   A         |         |         |         |
             |         |         |         |
   +---------+         +---------+         +----- 1

       +---------+         +---------+            0
       |         |         |         |
   B   |         |         |         |
       |         |         |         |
   ----+         +---------+         +---------+  1

*/

void print_binary(int binary) {
    int i;
    for (i = 4; i >= 0; i--) {
        int x = ((binary & (1<<i)) >> i);
        log_debug(ENCODER_CTX, "%d",x);
    }
    log_debug(ENCODER_CTX, "\n");
}

void print_event_queue() {
    if (event_pointer < queued_events) {
        int i;
        for (i = event_pointer; i < queued_events; i++) {
            log_debug(ENCODER_CTX, "%02d ", event_queue[i]);
        }
        log_debug(ENCODER_CTX, "\n");
    }
}

void enqueue_event(int event) {
    log_config(ENCODER_CTX, "enqueue_event(%d)\n", event);
    event_queue = realloc(event_queue,(queued_events+1)*sizeof(int));
    log_config(ENCODER_CTX, "enqueue_event: queued events: %d\n", queued_events);
    event_queue[queued_events++] = event;
}

static void push_button_timer_handler(union sigval sv) {
    pi_renc_t *renc = sv.sival_ptr;
    log_config(ENCODER_CTX, "pushButtonTimerHandler: renc->levPush = %d\n", renc->levPush);
    if (renc->levPush == 0) {
        if (renc->push_button_timer) {
            // Disarm timer
            struct itimerspec trigger;
            memset(&trigger, 0, sizeof(struct itimerspec));
            trigger.it_interval.tv_sec = 0;
            trigger.it_interval.tv_nsec = 0;
            trigger.it_value.tv_sec = 0;
            trigger.it_value.tv_nsec = 0;
            timer_settime(renc->push_button_timer, 0, &trigger, NULL);
            renc->timerFired = 1;
            (renc->callback)(HELD);
        }
    }
}

static void _cb(int gpio, int level, void *user) {

   pi_renc_t *renc = user;
   log_config(ENCODER_CTX, "_cb(gpio -> %d, level -> %d, renc -> %d)\n", gpio, level, renc->id);

   if (gpio == renc->gpioLeft || gpio == renc->gpioRight) {
       if (gpio == renc->gpioLeft) renc->levLeft = level; else renc->levRight = level;

       if (gpio != renc->lastGpio) /* debounce */
       {
          renc->lastGpio = gpio;

          if ((gpio == renc->gpioLeft) && (level == 1))
          {
             if (renc->levRight) (renc->callback)(RIGHT);
          }
          else if ((gpio == renc->gpioRight) && (level == 1))
          {
             if (renc->levLeft) (renc->callback)(LEFT);
          }
       }
    return;
   } else if (gpio == renc->gpioPush) {
        if (level != renc->levPush) {
            if (renc->levPush) { // level == 0 -> pushed

                renc->levPush = level;

                log_config(ENCODER_CTX, "pushbutton %d pressed. Creating timer\n", renc->gpioPush);
                timer_t *timerid = malloc(sizeof(timer_t));
                renc->push_button_timer = timerid;

                struct sigevent sev;
                struct itimerspec trigger;

                /* Set all `sev` and `trigger` memory to 0 */
                memset(&sev, 0, sizeof(struct sigevent));
                memset(&trigger, 0, sizeof(struct itimerspec));

                /*
                 * Set the notification method as SIGEV_THREAD:
                 *
                 * Upon timer expiration, `sigev_notify_function` (thread_handler()),
                 * will be invoked as if it were the start function of a new thread.
                 *
                 */
                sev.sigev_notify = SIGEV_THREAD;
                sev.sigev_notify_function = push_button_timer_handler;
                sev.sigev_value.sival_ptr = renc;

                /* Create the timer. In this example, CLOCK_REALTIME is used as the
                 * clock, meaning that we're using a system-wide real-time clock for
                 * this timer.
                 */
                if (timer_create(CLOCK_REALTIME, &sev, timerid) < 0) {
                    log_error(ENCODER_CTX, "Could not set timer: %d\n", errno);
                }

                /* Timer expiration will occur HELD_(N)SEC after being armed
                 * by timer_settime().
                 */
                trigger.it_interval.tv_sec = 0;
                trigger.it_interval.tv_nsec = 0;
                trigger.it_value.tv_sec = HELD_SEC;
                trigger.it_value.tv_nsec = HELD_NSEC;

                renc->timerFired = 0;

                timer_settime(*timerid, 0, &trigger, NULL);

            } else { // level == 1 -> released (ergo pushed)
                renc->levPush = level;
                log_config(ENCODER_CTX, "pushbutton %d released.", renc->gpioPush);
                if (renc->push_button_timer) {
                    log_config(ENCODER_CTX, " Deleting timer.");
                    timer_delete(renc->push_button_timer);
                    renc->push_button_timer = 0;
                }

                log_config(ENCODER_CTX, "\n");

                if (!renc->timerFired) {
                    (renc->callback)(PUSHED);
                }
            }
        }
   }

}

/*
   This function establishes a rotary encoder on gpioA and gpioB.

   When the encoder is turned the callback function is called.

   A pointer to a private data type is returned.  This should be passed
   to Pi_Renc_cancel if the rotary encoder is to be cancelled.
*/
pi_renc_t * pi_renc_new(int id, int gpioLeft, int gpioRight, int gpioPush, int gpioPower, pi_renc_cb_t callback)
{
   pi_renc_t *renc = malloc(sizeof(pi_renc_t));

    log_info(ENCODER_CTX, "pi_renc_new: renc=%p\n", renc);

    renc->id = id;
   renc->gpioLeft = gpioLeft;
   renc->gpioRight = gpioRight;
   renc->callback = callback;
   renc->push_button_timer = 0;
   renc->levLeft=digitalRead(renc->gpioLeft);
   renc->levRight=digitalRead(renc->gpioRight);
   renc->lastGpio = -1;
   renc->timerFired = 0;

   if (gpioPower > 0) {
    pinMode(gpioPower, OUTPUT);
    digitalWrite(gpioPower, 1);
   }

   pinMode(gpioLeft, INPUT);
   pinMode(gpioRight, INPUT);

   /* pull up is needed as encoder common is grounded */

   pullUpDnControl(gpioLeft, PUD_UP);
   pullUpDnControl(gpioRight, PUD_UP);

   /* monitor encoder level changes */

    if (gpioPush >= 0) {
        renc->gpioPush = gpioPush;
        renc->levPush=digitalRead(renc->gpioPush);
        pinMode(gpioPush, INPUT);
        pullUpDnControl(gpioPush, PUD_UP);
    } else {
        renc->gpioPush = -1;
    }

    return renc;

}

int read_pin(int gpio, int previousLevel, pi_renc_t *renc) {
    int level = digitalRead(gpio);
    if (previousLevel != level) {
        _cb(gpio,level,renc);
        return 1;
    }
    return 0;
}

int read_encoder(pi_renc_t *renc) {
    int ret = read_pin(renc->gpioLeft,renc->levLeft,renc);
    ret += read_pin(renc->gpioRight,renc->levRight,renc);
    if (renc->gpioPush >= 0) {
        ret += read_pin(renc->gpioPush,renc->levPush,renc);
    }
    if (ret) return 1;
    return 0;
}

int read_encoders(pi_renc_t *renc1, pi_renc_t *renc2) {
    int ret = read_encoder(renc1);
    ret += read_encoder(renc2);
    return ret;
}

void encoder_loop(pi_renc_t *renc1, pi_renc_t *renc2) {
    while (1) {
        int ret = read_encoders(renc1, renc2);
        while (ret) {
            ret = read_encoders(renc1,renc2);
        }
        delay(2);
    }
}

void pi_renc_cancel(pi_renc_t *renc)
{
   if (renc) {

      if (renc->push_button_timer) {
        timer_delete(*(renc->push_button_timer));
      }

      log_debug(ENCODER_CTX, "free(renc -> %p)\n", renc);
      free(renc);
   }
}

int next_event() {
    int next_event;
    next_event = 0;
    if (event_queue) {
        print_event_queue();
        if (event_pointer < queued_events) {
            next_event = event_queue[event_pointer];
        } else {
            log_config(ENCODER_CTX, "free(event_queue -> %p)\n", event_queue);
            free(event_queue);
            event_queue = NULL;
            queued_events = 0;
            event_pointer = -1;
        }
        event_pointer++;
    }
    return next_event;
}

void callback1(int way) {
    log_config(ENCODER_CTX, "callback1: way=%d\n", way);
    switch(way) {
        case RIGHT:
            enqueue_event(BUTTON_A_TURNED_RIGHT);
            break;
        case LEFT:
            enqueue_event(BUTTON_A_TURNED_LEFT);
            break;
        case PUSHED:
            enqueue_event(BUTTON_A_PRESSED);
            break;
        case HELD:
            enqueue_event(BUTTON_A_HOLD);
            break;
    }
}

void callback2(int way) {
    log_config(ENCODER_CTX, "callback2: way=%d\n", way);
    switch(way) {
        case RIGHT:
            enqueue_event(BUTTON_B_TURNED_RIGHT);
            break;
        case LEFT:
            enqueue_event(BUTTON_B_TURNED_LEFT);
            break;
        case PUSHED:
            enqueue_event(BUTTON_B_PRESSED);
            break;
        case HELD:
            enqueue_event(BUTTON_B_HOLD);
            break;
    }
}

PI_THREAD (encoderThread) {
    wiringPiSetupGpio();
    log_info(ENCODER_CTX, "WiringPi initialised\n");

    renc1 = pi_renc_new(0, 8, 7, 25, 24, callback1);
    renc2 = pi_renc_new(1, 9, 11, 10, -1, callback2);

    log_info(ENCODER_CTX, "Encoder setup finished\n");

    encoder_loop(renc1,renc2);

    return NULL;

}

void setup_encoder() {

    int ret = piThreadCreate(encoderThread);
    if (ret) {
        log_error(ENCODER_CTX, "Could not create encoder thread\n");
    }

}

