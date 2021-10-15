#ifndef _SYNC_H_
#define _SYNC_H_


typedef struct {
    int a;
} lock_t;

typedef struct {
    int a;
} condition_t;

void mutex_lock(lock_t* lock);
void mutex_unlock(lock_t* lock);

void condition_wait(condition_t *cond, lock_t *lock);
void condition_signal(condition_t *cond);




#endif /* _SYNC_H_ */
