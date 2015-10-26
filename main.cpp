#include <iostream>
#include <sys/timeb.h>
#include <random>
#include <pthread.h>

using namespace std;

#define DEBUG_MODE 0


int *arr;
long gRefTime;

pthread_mutex_t cout_without_conflict_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;
int COUNTER = 0;
int NUM_THREADS;

typedef struct _thread_data_t {
    int tid;
    int low;
    int high;
    int value;
} thread_data_t;


long GetMilliSecondTime(struct timeb timeBuf)
{
    long mliScndTime;

    mliScndTime = timeBuf.time;
    mliScndTime *= 1000;
    mliScndTime += timeBuf.millitm;
    return mliScndTime;
}

long GetCurrentTime(void)
{
    long crntTime=0;

    struct timeb timeBuf;
    ftime(&timeBuf);
    crntTime = GetMilliSecondTime(timeBuf);

    return crntTime;
}

void setTime(void)
{
    gRefTime = GetCurrentTime();
}

long getTime(void)
{
    long crntTime = GetCurrentTime();

    return (crntTime - gRefTime);
}



/**
 * 1  = success [element was found]
 * -1 = failure [element was not found]
 */
int linearSearch(const int *arr, int const len, int const value ){
    int i = 0;
    while(i < len){
        if(arr[i] == value){
            return 1;
        }
        i++;
    }
    return -1;
}

void* thr_func(void *arg) {
    thread_data_t *data = (thread_data_t *) arg;

   // pthread_mutex_lock(&cout_without_conflict_mutex);
    (linearSearch(&arr[data->low], data->high, data->value)) == 1 ? (cout << "\nKey was found in thread " << data->tid) :
    (cout << "");
    /*(cout << "\nKey wasn't found in thread " << data->tid);*/
    //pthread_mutex_unlock(&cout_without_conflict_mutex);

}


void* thr_func_with_mutex(void *arg) {

    int flag;

    thread_data_t *data = (thread_data_t *) arg;

    // pthread_mutex_lock(&cout_without_conflict_mutex);
    (flag = linearSearch(&arr[data->low], data->high, data->value)) == 1 ? (cout << "\nKey was found in thread " << data->tid) :
    (cout << "");
    //(cout << "\nKey wasn't found in thread " << data->tid);

    if(flag==1){
        pthread_mutex_lock( &condition_mutex );
             pthread_cond_signal( &condition_cond );
        pthread_mutex_unlock( &condition_mutex );
    } else {
        // in case if there is no key in the array
        // lock a mutex
        // increase a counter
        // if counter == number of threads -> make a signal
        pthread_mutex_lock( &condition_mutex );
                COUNTER++;
                if(COUNTER == NUM_THREADS){
                    pthread_cond_signal( &condition_cond );
                }
        pthread_mutex_unlock( &condition_mutex );
    }
}


/**
 * low inclusive [
 * high exclusive )
 */
void populateArrayWithRandomInt( int *&data,  const int len, int const low, int const high){

    data = (int*)calloc((size_t)len, sizeof(int));
    int temp = 0;
    while( temp < len){
        *(data+temp) = (rand()%(high-low)+low);
        //  *(data+temp) = rand();
        temp++;
    }
}

int findNumberOfIdenticalValues(int *arr, const int len, const int index){
    int i = 0;
    int counter = 0;
    while(i < index){
        if(arr[i] == arr[index]){
            counter++;
        }
        i++;
    }
    return counter;
}

void checkIndexForSearchWithoutThreads(int *arr, const int len, const int index){
    int i = 0;
    while(i < index){
        if(arr[i] == arr[index]){
            while(arr[i] == arr[index]){
                arr[i] = rand();
            }
        }
        i++;
    }
}

void replaceWithRandomVariable(int *arr, const int len, const int value){
    int i = 0;
    while(i < len){
        if(arr[i] == value){
            while(arr[i] == value){
                arr[i] = rand();
            }
        }
        i++;
    }
}

int main(int argc, char **argv)
{


    if(argc < 3){
        cerr << "Please enter | array size [int between 1 and 1 000 000 000 | number of threads  |\n index where the key will be found ";
        return -1;
    }

    const int ARRAY_SIZE = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    const int INDEX_OF_KEY =  atoi(argv[3]);

    int **indices;
    indices = (int**)(calloc((size_t)NUM_THREADS, sizeof(int*)));
    for(int i = 0; i<NUM_THREADS; i++){
        indices[i] = (int*)calloc(2, sizeof(int));
    }


    /**
     * ONE THREAD PART
     */
    populateArrayWithRandomInt(arr, ARRAY_SIZE, 0, ARRAY_SIZE);
    cout << "There is " << (findNumberOfIdenticalValues(arr, ARRAY_SIZE, INDEX_OF_KEY)) << " number of identical values" <<  endl;
    checkIndexForSearchWithoutThreads(arr, ARRAY_SIZE, INDEX_OF_KEY);
    cout << "There is " << (findNumberOfIdenticalValues(arr, ARRAY_SIZE, INDEX_OF_KEY)) << " number of identical values" <<  endl;
    setTime();
    int VALUE_OF_KEY = arr[INDEX_OF_KEY];

    (linearSearch(arr, ARRAY_SIZE,VALUE_OF_KEY)) == 1 ? (cout << "Key was found") : (cout << "Key wasn't found");
    cout << "\nThe time spent without multithreading is "  << getTime() << endl;


    /*
    * MULTIPLE THREADS BUSY WAITING
    */
    int low;
    int pivot = ARRAY_SIZE / NUM_THREADS;
    for (int i = 0, j = 1; i < NUM_THREADS; i++, j++) {
        low = i * pivot;
        /*
         * This case is only for the cases when division
         * of elements in the array by the number of threads
         * doesn't produce equal sections
         *
         * 11 elements and 4 threads
         * 11/4 = 3
         * 0-2 3-5 6-8 9-the rest of the array
         *
         */
        if (i == NUM_THREADS - 1) {
                indices[i][0] = low;
                indices[i][1] = ARRAY_SIZE%(NUM_THREADS) + pivot;
        } else {
            /*
             * In case of an array with 20 elements and 4 threads
             * 0-4 5-9 10-14 15-19
             *
             * pivot = 20 / 4 = 5
             */
                indices[i][0] = low;
                indices[i][1] = pivot;
        }
    }

    pthread_t thr[NUM_THREADS];
    int rc;
    /*
     * create a thread_data_t argument array
      */
    thread_data_t thr_data[NUM_THREADS];
    /*
     * create threads
     */

    setTime();

    if(NUM_THREADS <= ARRAY_SIZE){
        for(int i = 0; i < NUM_THREADS; i++){

            thr_data[i].low = indices[i][0];
            thr_data[i].high = indices[i][1];
            thr_data[i].value = VALUE_OF_KEY;

            thr_data[i].tid = i;
            /* --Signature of the function--
             *
             * int pthread_create(pthread_t *thread, pthread_attr_t *attr,
                       void *(*start_routine)(void *), void *arg);
             */
            rc = pthread_create(&thr[i], NULL, thr_func, &thr_data[i]);
            if(rc > 0){
                fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
                return EXIT_FAILURE;
            }
        }
    }else {
        cout << "Too many threads for too few elements";
    }

    /*
     * block until all threads complete
     * */
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(thr[i], NULL);
    }
    cout << "\nThe time spent without multithreading is "  << getTime() << endl;







    /*
    * MULTIPLE THREADS WITHOUT BUSY WAITING
    */

    setTime();
    //VALUE_OF_KEY = 0;
    //replaceWithRandomVariable(arr, ARRAY_SIZE, 0);
    /*
     * A lock must be used here in order to prevent the scenario
     * when one of the threads finishes its job and signals the
     * condition mutex which wasn't yet locked in the main thread.
     */
    pthread_mutex_lock( &condition_mutex );

    if(NUM_THREADS <= ARRAY_SIZE){
        for(int i = 0; i < NUM_THREADS; i++){

            thr_data[i].low = indices[i][0];
            thr_data[i].high = indices[i][1];
            thr_data[i].value = VALUE_OF_KEY;

            thr_data[i].tid = i;
            /* --Signature of the function--
             *
             * int pthread_create(pthread_t *thread, pthread_attr_t *attr,
                       void *(*start_routine)(void *), void *arg);
             */
            rc = pthread_create(&thr[i], NULL, thr_func_with_mutex, &thr_data[i]);
            if(rc > 0){
                fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
                return EXIT_FAILURE;
            }
        }
    }else {
        cout << "Too many threads for too few elements";
    }

    cout << "\nAbout to wait";

    pthread_cond_wait( &condition_cond, &condition_mutex );
   // pthread_mutex_unlock( &condition_mutex );

    /*
     * block until all threads complete
     * */
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_cancel(thr[i]);
    }
    cout << "\nThe time spent without busy waiting "  << getTime() << endl;
    cout << "\nCOUNTER : " << COUNTER;






    return 0;
}



