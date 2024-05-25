#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define THREAD_MAX 4068
#define FILENAME    "/nfs/test.txt"

static char *dir;
int thread_max;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

void *thr( void * arg ) {

    int  tid = *( int * )arg;
        int  fd;
        int  rc = 0;

    pthread_mutex_lock( &mutex );
    pthread_cond_wait( &cond, &mutex );
    pthread_mutex_unlock( &mutex ) ;

        while(1){
        errno = 0;
        /* fd = open(FILENAME, O_RDWR|O_CREAT|O_SYNC, 0666); */
        fd = open(FILENAME, O_RDWR|O_CREAT, 0666);
        if(errno == 16){
                        printf("open:%d\n",errno);
                        exit(EXIT_FAILURE);
        }
        close(fd);
        errno = 0;
        rc = unlink(FILENAME);
        if(errno == 16){
                        printf("unlink:%d\n",errno);
                        exit(EXIT_FAILURE);
        }
        }
    return 0;
}


int main (int argc, char *argv[]) {

    pthread_t   th[THREAD_MAX];
    int th_num = ( argc == 2 ) ? atol( argv[1] ) : 0;
    int i;
    int id[th_num];
        struct stat  st,st2;
        int fd;

        if (argc != 2) {
                fprintf(stderr, "Usage: %s <th_num> <dir>\n", argv[0]);
                exit(1);
        }

    if( th_num <= 0 || th_num > THREAD_MAX ) {
        return( 1 );
    }

    for( i = 0; i < th_num; i ++ ) {
                id[i] = i;
        pthread_create( &th[i], 0, thr, &id[i] );
    }

    sleep(5);
    pthread_cond_broadcast( &cond );

    for( i = 0; i < th_num; i ++ ) {
        pthread_join( th[i], 0 );
    }
    return 0;
}
