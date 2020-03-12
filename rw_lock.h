//
// Provided starter code for CS 454/654.
// This code provides a Reader-Writer lock for use in A3.
// You should not need to change this file.
//

#ifndef RW_LOCK_H
#define RW_LOCK_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS

// The structure definition of a readers-writers lock.
typedef struct rw_lock {
    // To protect internal state.
    pthread_mutex_t mutex_;
    // To notify about changes in state.
    pthread_cond_t cv_;

    // The number of active readers.
    int num_readers_;
    // The number of active readers (should only ever be 0 or 1).
    int num_writers_;
    // The number of waiting writers, needed to prevent writer starvation.
    int num_waiting_writers_;
} rw_lock_t;

// The reader-writer lock can be held in one of two modes: as a reader, or as
// a writer. This enum defines the mode that the lock is held.
typedef enum rw_lock_mode { RW_READ_LOCK, RW_WRITE_LOCK } rw_lock_mode_t;

// You can initialize a static lock using this macro. For example:
// rw_lock_t lock = RW_LOCK_INITIALIZER;
#define RW_LOCK_INITIALIZER                                                    \
    { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0 }

// FUNCTIONS
// All functions return 0 on success or an error number to indicate the error.

// Initialize the lock, this must be called prior to all future calls on the
// lock.
int rw_lock_init(rw_lock_t *lock);
// Destroy the lock, this must be called when you are finished with the lock.
int rw_lock_destroy(rw_lock_t *lock);

// Acquire the lock in mode  (RW_READ_LOCK, RW_WRITE_LOCK).
int rw_lock_lock(rw_lock_t *lock, rw_lock_mode_t mode);
// Release the lock from mode  (RW_READ_LOCK, RW_WRITE_LOCK). To release the
// lock you must have been an owner of the lock.
int rw_lock_unlock(rw_lock_t *lock, rw_lock_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
