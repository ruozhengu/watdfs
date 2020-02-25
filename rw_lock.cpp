//
// Provided starter code for CS 454/654.
// This code implements a Reader-Writer lock for use in A3.
// You should not need to change this file.
//

#include "rw_lock.h"

#include <errno.h>

// A macro to return -EINVAL if the provided argument is NULL
#define EINVAL_IF_NULL(l)                                                      \
    do {                                                                       \
        if (l == NULL) {                                                       \
            return -EINVAL;                                                    \
        }                                                                      \
        break;                                                                 \
    } while (0)

// A macro to return the number provided, if the number is non-zero.
#define RETURN_IF_ERR(r)                                                       \
    do {                                                                       \
        if (r != 0) {                                                          \
            return r;                                                          \
        }                                                                      \
        break;                                                                 \
    } while (0)

int rw_lock_init(rw_lock_t *lock) {
    EINVAL_IF_NULL(lock);

    int ret = pthread_mutex_init(&(lock->mutex_), NULL);
    RETURN_IF_ERR(ret);

    ret = pthread_cond_init(&(lock->cv_), NULL);
    RETURN_IF_ERR(ret);

    lock->num_readers_ = 0;
    lock->num_writers_ = 0;
    lock->num_waiting_writers_ = 0;

    return ret;
}

int rw_lock_destroy(rw_lock_t *lock) {
    EINVAL_IF_NULL(lock);

    int ret = pthread_mutex_destroy(&(lock->mutex_));
    RETURN_IF_ERR(ret);

    ret = pthread_cond_destroy(&(lock->cv_));
    RETURN_IF_ERR(ret);

    return 0;
}

int rw_lock_lock(rw_lock_t *lock, rw_lock_mode_t mode) {
    EINVAL_IF_NULL(lock);

    int ret = pthread_mutex_lock(&(lock->mutex_));
    RETURN_IF_ERR(ret);

    if (mode == RW_READ_LOCK) {
        while ((lock->num_writers_ > 0) || (lock->num_waiting_writers_ > 0)) {
            // We can ignore this return call, because we are supposed to hold
            // the lock, and we know that the args are not invalid at this
            // point.
            pthread_cond_wait(&(lock->cv_), &(lock->mutex_));
        }
        lock->num_readers_ += 1;
    } else {
        lock->num_waiting_writers_ += 1;
        while ((lock->num_writers_ > 0) || (lock->num_readers_ > 0)) {
            // Same as above.
            pthread_cond_wait(&(lock->cv_), &(lock->mutex_));
        }
        lock->num_writers_ += 1;
        lock->num_waiting_writers_ -= 1;
    }

    ret = pthread_mutex_unlock(&(lock->mutex_));
    RETURN_IF_ERR(ret);

    return 0;
}

int rw_lock_unlock(rw_lock_t *lock, rw_lock_mode_t mode) {
    EINVAL_IF_NULL(lock);

    int ret = pthread_mutex_lock(&(lock->mutex_));
    RETURN_IF_ERR(ret);

    if (mode == RW_READ_LOCK) {
        if (lock->num_readers_ == 0) {
            // You don't actually hold a lock.
            ret = pthread_mutex_unlock(&(lock->mutex_));
            RETURN_IF_ERR(ret);
            return -EPERM;
        }
        lock->num_readers_ -= 1;
    } else {
        if (lock->num_writers_ == 0) {
            // You don't actually hold a lock.
            ret = pthread_mutex_unlock(&(lock->mutex_));
            RETURN_IF_ERR(ret);
            return -EPERM;
        }
        lock->num_writers_ -= 1;
    }
    // Signal waiters. There is nothing that we can do if there is an error
    // here.
    pthread_cond_broadcast(&(lock->cv_));
    ret = pthread_mutex_unlock(&(lock->mutex_));
    RETURN_IF_ERR(ret);

    return 0;
}
