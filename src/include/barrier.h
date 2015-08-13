
#ifndef MT_FS_TESTS_BARRIER_H_
#define MT_FS_TESTS_BARRIER_H_

#include <pthread.h>
#include <unistd.h>

#ifdef _POSIX_BARRIERS

#define mt_fs_tests_barrier_t pthread_barrier_t
#define mt_fs_tests_barrier_init(barrier, count) pthread_barrier_init(barrier, \
                                                                      NULL, \
                                                                      count)
#define mt_fs_tests_barrier_wait(barrier) pthread_barrier_wait(barrier)
#define mt_fs_tests_barrier_destroy(barrier) pthread_barrier_destroy(barrier)

#else /* _POSIX_BARRIERS */
#warning "No barrier, using internal implementation"

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int count;

} mt_fs_tests_barrier_t;

static inline int mt_fs_tests_barrier_init(mt_fs_tests_barrier_t * const barrier,
                                           unsigned int const count)
{
    int result = 0;
    assert(barrier != NULL);

    if (count > 0)
    {
        result = pthread_mutex_init(&barrier->mutex, NULL);

        if (result == 0)
        {
            result = pthread_cond_init(&barrier->cond, NULL);

            if (result == 0)
            {
                barrier->count = count;
            }
            else
            {
                pthread_mutex_destroy(&barrier->mutex);
            }
        }
    }

    return result;
}

static inline int mt_fs_tests_barrier_destroy(mt_fs_tests_barrier_t * barrier)
{
    int result = 0;
    assert(barrier != NULL);
    pthread_cond_destroy(&barrier->cond);
    pthread_mutex_destroy(&barrier->mutex);

    return result;
}

static inline int mt_fs_tests_barrier_wait(mt_fs_tests_barrier_t * const barrier)
{
    int result = 0;
    assert(barrier != NULL);

    result = pthread_mutex_lock(&barrier->mutex);

    if (result == 0)
    {
        barrier->count--;

        if(barrier->count == 0)
        {
            pthread_cond_broadcast(&barrier->cond);

            result = 1;
        }
        else
        {
            pthread_cond_wait(&barrier->cond, &(barrier->mutex));
        }

        pthread_mutex_unlock(&barrier->mutex);
    }

    return result;
}

#endif /* _POSIX_BARRIERS */

#endif /* MT_FS_TESTS_BARRIER_H_ */
