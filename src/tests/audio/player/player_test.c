/*
 * VE301
 *
 * Standalone tests for player thread, event, and queue behavior.
 */

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../../../audio/player.h"
#include "../../../base/util.h"
#include "../../test.h"

struct test_state {
    pthread_mutex_t mutex;
    int init_calls;
    int cleanup_calls;
    int start_action_calls;
    int stop_action_calls;
    int action_order[8];
    int action_order_len;
    pthread_t init_thread;
    pthread_t start_action_thread;
    pthread_t stop_action_thread;
    const char *last_song;
};

static struct test_state g_state = {
    PTHREAD_MUTEX_INITIALIZER,
    0,
    0,
    0,
    0,
    {0},
    0,
    0,
    0,
    0,
    NULL
};

static void state_reset(void) {
    int i;

    pthread_mutex_lock(&g_state.mutex);
    g_state.init_calls = 0;
    g_state.cleanup_calls = 0;
    g_state.start_action_calls = 0;
    g_state.stop_action_calls = 0;
    g_state.action_order_len = 0;
    for (i = 0; i < (int) (sizeof(g_state.action_order) / sizeof(g_state.action_order[0])); ++i) {
        g_state.action_order[i] = 0;
    }
    g_state.init_thread = 0;
    g_state.start_action_thread = 0;
    g_state.stop_action_thread = 0;
    g_state.last_song = NULL;
    pthread_mutex_unlock(&g_state.mutex);
}

static void drain_player_events(void) {
    player_event *event;

    event = player_next_event();
    while (event) {
        player_event_free(event);
        event = player_next_event();
    }
}

static int expect_next_event(player *expected_player, player_status expected_status) {
    player_event *event = player_next_event();

    ASSERT_MSG(event != NULL, "expected queued player event");
    ASSERT_MSG(event->player == expected_player, "event should reference the expected player");
    ASSERT_MSG(event->status == expected_status, "event should have the expected status");
    player_event_free(event);
    return 1;
}

static int assert_no_pending_events(void) {
    player_event *event = player_next_event();

    ASSERT_MSG(event == NULL, "did not expect any more player events");
    return 1;
}

static int test_init(void) {
    pthread_mutex_lock(&g_state.mutex);
    g_state.init_calls++;
    g_state.init_thread = pthread_self();
    pthread_mutex_unlock(&g_state.mutex);
    return 1;
}

static int test_cleanup(void) {
    pthread_mutex_lock(&g_state.mutex);
    g_state.cleanup_calls++;
    pthread_mutex_unlock(&g_state.mutex);
    return 1;
}

static void test_playback_start(void *data) {
    pthread_mutex_lock(&g_state.mutex);
    g_state.start_action_calls++;
    g_state.start_action_thread = pthread_self();
    g_state.last_song = (const char *) data;
    pthread_mutex_unlock(&g_state.mutex);
}

static void test_playback_stop(void *data) {
    (void) data;
    pthread_mutex_lock(&g_state.mutex);
    g_state.stop_action_calls++;
    g_state.stop_action_thread = pthread_self();
    pthread_mutex_unlock(&g_state.mutex);
}

static void queue_record_start(void *data) {
    pthread_mutex_lock(&g_state.mutex);
    g_state.start_action_calls++;
    g_state.start_action_thread = pthread_self();
    if (g_state.action_order_len < (int) (sizeof(g_state.action_order) / sizeof(g_state.action_order[0]))) {
        g_state.action_order[g_state.action_order_len++] = (int) (intptr_t) data;
    }
    pthread_mutex_unlock(&g_state.mutex);
}

static void queue_record_stop(void *data) {
    (void) data;
    pthread_mutex_lock(&g_state.mutex);
    g_state.stop_action_calls++;
    g_state.stop_action_thread = pthread_self();
    if (g_state.action_order_len < (int) (sizeof(g_state.action_order) / sizeof(g_state.action_order[0]))) {
        g_state.action_order[g_state.action_order_len++] = 0;
    }
    pthread_mutex_unlock(&g_state.mutex);
}

static int wait_for_counter(int *counter, int expected, int timeout_ms) {
    long long start = current_time_millis();

    while (current_time_millis() - start < timeout_ms) {
        int value;

        pthread_mutex_lock(&g_state.mutex);
        value = *counter;
        pthread_mutex_unlock(&g_state.mutex);

        if (value >= expected) {
            return 1;
        }

        sched_yield();
        {
            struct timespec sleep_ns = {0, 1000000};
            nanosleep(&sleep_ns, NULL);
        }
    }

    return 0;
}

TEST(player_start_run_stop, "starts player thread, runs queued actions, and stops cleanly") {
    const char *song = "station://test";
    player *p;
    pthread_t main_thread = pthread_self();
    int cleanup_calls;
    const char *last_song;
    pthread_t init_thread;
    pthread_t start_action_thread;
    pthread_t stop_action_thread;

    state_reset();

    p = player_new("test-player",
                   "icon",
                   "label",
                   1,
                   &test_init,
                   NULL,
                   &test_cleanup,
                   &test_playback_start,
                   &test_playback_stop);
    ASSERT_MSG(p != NULL, "player_new returned NULL");

    ASSERT_MSG(player_start(p) == 0, "player_start should succeed");
    ASSERT_MSG(wait_for_counter(&g_state.init_calls, 1, 1000), "init function was not called");
    ASSERT_MSG(player_thread_running(p) == 1, "thread should report running after start");

    ASSERT_MSG(player_playback_start(p, (void *) song) == 1, "player_playback_start should queue action");
    ASSERT_MSG(player_playback_stop(p) == 1, "player_playback_stop should queue action");
    ASSERT_MSG(wait_for_counter(&g_state.start_action_calls, 1, 1000), "start action was not executed");
    ASSERT_MSG(wait_for_counter(&g_state.stop_action_calls, 1, 1000), "stop action was not executed");

    pthread_mutex_lock(&g_state.mutex);
    cleanup_calls = g_state.cleanup_calls;
    last_song = g_state.last_song;
    init_thread = g_state.init_thread;
    start_action_thread = g_state.start_action_thread;
    stop_action_thread = g_state.stop_action_thread;
    pthread_mutex_unlock(&g_state.mutex);

    ASSERT_MSG(cleanup_calls == 0, "cleanup should not run before stop");
    ASSERT_MSG(last_song == song, "queued playback data was not forwarded");
    ASSERT_MSG(!pthread_equal(init_thread, main_thread), "init should run on worker thread");
    ASSERT_MSG(pthread_equal(init_thread, start_action_thread), "start action should run on worker thread");
    ASSERT_MSG(pthread_equal(init_thread, stop_action_thread), "stop action should run on worker thread");

    ASSERT_MSG(player_stop(p) == 1, "player_stop should succeed");
    ASSERT_MSG(player_thread_running(p) == 0, "thread should report stopped after stop");

    pthread_mutex_lock(&g_state.mutex);
    cleanup_calls = g_state.cleanup_calls;
    pthread_mutex_unlock(&g_state.mutex);
    ASSERT_MSG(cleanup_calls == 1, "cleanup should run exactly once on stop");

    player_free(p);
    return 1;
}

TEST(player_emits_events, "emits activation, metadata, playback, and cover events in order") {
    player *p;

    state_reset();
    drain_player_events();

    p = player_new("event-player", "icon", "label", 1, NULL, NULL, NULL, NULL, NULL);
    ASSERT_MSG(p != NULL, "player_new returned NULL");

    player_set_active(p, 1);
    player_set_album(p, "album");
    player_set_artist(p, "artist");
    player_set_title(p, "title");
    player_set_playback_status(p, PLAYER_PLAYBACK_PLAYING);
    player_set_cover_image_path(p, "/tmp/cover.png");

    ASSERT_MSG(expect_next_event(p, PLAYER_ACTIVATED), "expected activation event");
    ASSERT_MSG(expect_next_event(p, PLAYER_PLAYBACK_STOPPED), "expected stopped event after activation");
    ASSERT_MSG(expect_next_event(p, PLAYER_METADATA_CHANGED), "expected album metadata event");
    ASSERT_MSG(expect_next_event(p, PLAYER_METADATA_CHANGED), "expected artist metadata event");
    ASSERT_MSG(expect_next_event(p, PLAYER_METADATA_CHANGED), "expected title metadata event");
    ASSERT_MSG(expect_next_event(p, PLAYER_PLAYBACK_PLAYING), "expected playback status event");
    ASSERT_MSG(expect_next_event(p, PLAYER_COVER_CHANGED), "expected cover change event");
    ASSERT_MSG(assert_no_pending_events(), "did not expect extra events after initial update sequence");

    player_set_album(p, "album");
    player_set_artist(p, "artist");
    player_set_title(p, "title");
    player_set_playback_status(p, PLAYER_PLAYBACK_PLAYING);
    player_set_cover_image_path(p, "/tmp/cover.png");
    ASSERT_MSG(assert_no_pending_events(), "setting unchanged values should not emit events");

    player_free(p);
    return 1;
}

TEST(player_queues_actions_fifo, "runs queued playback actions in FIFO order") {
    player *p;
    int action_order_len;
    int action_order[3];
    pthread_t init_thread;
    pthread_t start_action_thread;
    pthread_t stop_action_thread;

    state_reset();

    p = player_new("queue-player",
                   "icon",
                   "label",
                   1,
                   &test_init,
                   NULL,
                   &test_cleanup,
                   &queue_record_start,
                   &queue_record_stop);
    ASSERT_MSG(p != NULL, "player_new returned NULL");

    ASSERT_MSG(player_start(p) == 0, "player_start should succeed");
    ASSERT_MSG(wait_for_counter(&g_state.init_calls, 1, 1000), "init function was not called");

    ASSERT_MSG(player_playback_start(p, (void *) (intptr_t) 1) == 1, "first queued action should succeed");
    ASSERT_MSG(player_playback_stop(p) == 1, "queued stop action should succeed");
    ASSERT_MSG(player_playback_start(p, (void *) (intptr_t) 2) == 1, "second queued action should succeed");
    ASSERT_MSG(wait_for_counter(&g_state.action_order_len, 3, 1000), "queued actions were not executed");

    pthread_mutex_lock(&g_state.mutex);
    action_order_len = g_state.action_order_len;
    action_order[0] = g_state.action_order[0];
    action_order[1] = g_state.action_order[1];
    action_order[2] = g_state.action_order[2];
    init_thread = g_state.init_thread;
    start_action_thread = g_state.start_action_thread;
    stop_action_thread = g_state.stop_action_thread;
    pthread_mutex_unlock(&g_state.mutex);

    ASSERT_MSG(action_order_len == 3, "expected three recorded actions");
    ASSERT_MSG(action_order[0] == 1, "first queued action should execute first");
    ASSERT_MSG(action_order[1] == 0, "stop action should execute second");
    ASSERT_MSG(action_order[2] == 2, "second queued start action should execute third");
    ASSERT_MSG(pthread_equal(init_thread, start_action_thread), "queued start action should run on worker thread");
    ASSERT_MSG(pthread_equal(init_thread, stop_action_thread), "queued stop action should run on worker thread");

    ASSERT_MSG(player_stop(p) == 1, "player_stop should succeed");
    player_free(p);
    return 1;
}

TEST_MAIN(TEST_CASE(player_start_run_stop,
                    "starts player thread, runs queued actions, and stops cleanly"),
          TEST_CASE(player_emits_events,
                    "emits activation, metadata, playback, and cover events in order"),
          TEST_CASE(player_queues_actions_fifo,
                    "runs queued playback actions in FIFO order"));
