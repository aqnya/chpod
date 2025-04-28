#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define MAX_THREADS 8
#define QUEUE_SIZE 65536

typedef struct {
    char *paths[QUEUE_SIZE];
    int front;
    int rear;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int active_workers;
} PathQueue;

static void queue_init(PathQueue *q) {
    q->front = q->rear = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->active_workers = 0;
}

static void enqueue(PathQueue *q, const char *path) {
    pthread_mutex_lock(&q->lock);
    if ((q->rear + 1) % QUEUE_SIZE != q->front) {
        q->paths[q->rear] = strdup(path);
        q->rear = (q->rear + 1) % QUEUE_SIZE;
        pthread_cond_signal(&q->cond);
    }
    pthread_mutex_unlock(&q->lock);
}

static void* worker_thread(void *arg) {
    PathQueue *queue = (PathQueue*)arg;
    
    pthread_mutex_lock(&queue->lock);
    queue->active_workers++;
    pthread_mutex_unlock(&queue->lock);
    
    while (1) {
        pthread_mutex_lock(&queue->lock);
        
        while (queue->front == queue->rear) {
            if (queue->active_workers == 1) {
                queue->active_workers--;
                pthread_cond_broadcast(&queue->cond);
                pthread_mutex_unlock(&queue->lock);
                return NULL;
            }
            
            queue->active_workers--;
            pthread_cond_wait(&queue->cond, &queue->lock);
            queue->active_workers++;
        }
        
        char *path = queue->paths[queue->front];
        queue->front = (queue->front + 1) % QUEUE_SIZE;
        pthread_mutex_unlock(&queue->lock);

        struct stat st;
        if (lstat(path, &st)) {
            if (errno != ENOENT) perror("lstat");
            free(path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            DIR *dir = opendir(path);
            if (!dir) {
                if (errno != ENOENT) perror("opendir");
                free(path);
                continue;
            }

            int has_entries = 0;
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) 
                    continue;

                has_entries = 1;
                char subpath[PATH_MAX];
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
                enqueue(queue, subpath);
            }
            closedir(dir);

            if (has_entries) {
                if (access(path, F_OK) == 0) enqueue(queue, path);
                free(path);
            } else {
                if (rmdir(path) && errno != ENOENT) {
                    perror("rmdir");
                    enqueue(queue, path);
                }
                free(path);
            }
        } else {
            if (unlink(path) && errno != ENOENT) {
                perror("unlink");
                enqueue(queue, path);
            } else {
                free(path);
            }
        }
    }
    return NULL;
}

int delete_path(const char *path) {
    if (!path || !*path) {
        fprintf(stderr, "Invalid path\n");
        return EINVAL;
    }

    PathQueue queue;
    queue_init(&queue);
    enqueue(&queue, path);

    pthread_t threads[MAX_THREADS];
    int final_status = 0;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, &queue)) {
            fprintf(stderr, "Failed to create thread %d: %s\n", i, strerror(errno));
            final_status = EAGAIN;
        }
    }

    pthread_mutex_lock(&queue.lock);
    while (queue.active_workers > 0) {
        pthread_cond_broadcast(&queue.cond);
        pthread_mutex_unlock(&queue.lock);
        usleep(10000);
        pthread_mutex_lock(&queue.lock);
    }
    pthread_mutex_unlock(&queue.lock);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_lock(&queue.lock);
    while (queue.front != queue.rear) {
        char *p = queue.paths[queue.front];
        fprintf(stderr, "Warning: Failed to delete %s\n", p);
        free(p);
        queue.front = (queue.front + 1) % QUEUE_SIZE;
        final_status = ENOTEMPTY;
    }
    pthread_mutex_unlock(&queue.lock);

    pthread_mutex_destroy(&queue.lock);
    pthread_cond_destroy(&queue.cond);
    
    return final_status;
}
