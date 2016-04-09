#include <iostream>
#include <sys/timeb.h>
#include <random>
using namespace std;


/**
 * stackoverflow - good explanation of VOLATILE
 *
 * If two threads are both reading and writing to a shared variable, then using the
 * volatile keyword for that is not enough. You need to use synchronization in that
 * case to guarantee that the reading and writing of the variable is atomic.
 *
 * But in case one thread reads and writes the value of a volatile variable,
 * and other threads only read the variable, then the reading threads are guaranteed
 * to see the latest value written to the volatile variable. Without making the
 * variable volatile, this would not be guaranteed.
 *
 * Performance considerations of using volatile:
 *
 * Reading and writing of volatile variables causes the variable to be read or written
 * to main memory. Reading from and writing to main memory is more expensive than
 * accessing the CPU cache. Accessing volatile variables also prevent instruction
 * reordering which is a normal performance enhancement technique. Thus, you should
 * only use volatile variables when you really need to enforce visibility of variables.
 */


/*
 * Global variables
 */

int *arr;
long gRefTime;
/*
 * volatile is needed here because there is no guarantee that the
 * main thread will read the updated value and not the value from
 * the cache
 */
volatile int *found;
volatile int *done;

pthread_mutex_t cout_without_conflict_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * condition_mutex guarantees the the thread will go a
 * state of sleep until it has been signaled and put in a ready queue.
 */
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;
int COUNTER = 0;
int NUM_THREADS;


enum {
    NOT_FOUND,
    FOUND
};

enum {
    NOT_DONE,
    DONE
};

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

    /*
     * cout is not thread safe. It's better to wrap
     * it up with a mutex lock; however, if you do so
     * it will slow down the speed of finding the key.
     */
    // pthread_mutex_lock(&cout_without_conflict_mutex);
    (linearSearch(&arr[data->low], data->high, data->value)) == 1 ? (cout << "\nKey was found in thread " << data->tid) :
    (cout << "");
    /*(cout << "\nKey wasn't found in thread " << data->tid);*/
    //pthread_mutex_unlock(&cout_without_conflict_mutex);

}



void* thr_func_real_busy_waiting(void *arg) {
    thread_data_t *data = (thread_data_t *) arg;

    int success;
    (success = linearSearch(&arr[data->low], data->high, data->value)) == 1 ? (cout << "\nKey was found in thread " << data->tid) :
    (cout << "");

    if(success){
        found[data->tid] = FOUND;
    } else {
        done[data->tid] = DONE;
    }

}

void* thr_func_with_mutex(void *arg) {

    int flag;

    thread_data_t *data = (thread_data_t *) arg;

    // pthread_mutex_lock(&cout_without_conflict_mutex);
    (flag = linearSearch(&arr[data->low], data->high, data->value)) == 1 ? (cout << "\nKey was found in thread " << data->tid) :
    (cout << "");
    //(cout << "\nKey wasn't found in thread " << data->tid);

    /*
     * If the key has been found. Unlock the mutex lock
     * and signal the main thread.
     */
    if(flag==1){
        pthread_mutex_lock( &condition_mutex );
             pthread_cond_signal( &condition_cond );
        pthread_mutex_unlock( &condition_mutex );
    } else {
        // in case if there is no key in the array [-1 was entered]
        // lock a mutex
        // increase a counter
        // if counter == number of threads -> value not found in any of threads -> make a signal
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

int findNumberOfIdenticalValues(int *arr, const int len, const int value){
    int i = 0;
    int counter = 0;
    while(i < len){
        if(arr[i] == value){
            counter++;
        }
        i++;
    }
    return counter;
}

void getRidOfKeyDuplicates(int *arr, const int len, const int index){
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

void getRidOfKeyDuplicatesInWholeArray(int *arr, const int len, const int indexOfKey){
    int i = 0;
    while(i < len){
        if(arr[i] == arr[indexOfKey] && i != indexOfKey){
            while(arr[i] == arr[indexOfKey]){
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

    int VALUE_OF_KEY;
    if(argc < 3){
        cerr << "Please enter | array size [int between 1 and 100 000 000 | number of threads "
                " |\n index where the key will be found ";
        return -1;
    }

    const int ARRAY_SIZE = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);
    const int INDEX_OF_KEY =  atoi(argv[3]);

    if(NUM_THREADS > ARRAY_SIZE){
        fprintf(stderr, "error: too few elements for too many threads");
        return EXIT_FAILURE;
    }
    if(INDEX_OF_KEY > ARRAY_SIZE){
        fprintf(stderr, "error: index of key cannot be greater than the number of all elements in array");
        return EXIT_FAILURE;
    }
    if(ARRAY_SIZE > 1000000000){
        fprintf(stderr, "error: the array size is too big");
        return EXIT_FAILURE;
    }


    int **indices;
    indices = (int**)(calloc((size_t)NUM_THREADS, sizeof(int*)));
    for(int i = 0; i<NUM_THREADS; i++){
        indices[i] = (int*)calloc(2, sizeof(int));
    }
    done = (int*)(calloc((size_t)NUM_THREADS, sizeof(int)));
    found = (int*)(calloc((size_t)NUM_THREADS, sizeof(int)));


    populateArrayWithRandomInt(arr, ARRAY_SIZE, 0, ARRAY_SIZE);
    if (INDEX_OF_KEY == -1){
        VALUE_OF_KEY = 0;
        replaceWithRandomVariable(arr,ARRAY_SIZE, VALUE_OF_KEY);
    } else {
        VALUE_OF_KEY = arr[INDEX_OF_KEY];
    }

    cout << "\n------------- ARGUMENTS -----------------\n";
    cout << "number of elements\t\t" << ARRAY_SIZE;
    cout << "\nnumber of threads\t\t" << NUM_THREADS;
    cout << "\nindex of the key\t\t" << INDEX_OF_KEY << "\n";
    cout << "-----------------------------------------\n\n";

    /*
     * Get rif of duplicates
     */
    cout << "\n---------- GETTING RID OF KEY DUPLICATES ----------\n";
    cout << "The value of the key appears in the array " << (findNumberOfIdenticalValues(arr, ARRAY_SIZE, VALUE_OF_KEY))
    << " time[s]" <<  endl;
    getRidOfKeyDuplicatesInWholeArray(arr, ARRAY_SIZE, INDEX_OF_KEY);
    cout << "The value of the key appears in the array " << (findNumberOfIdenticalValues(arr, ARRAY_SIZE, VALUE_OF_KEY))
    << " time[s]" <<  endl;
    cout << "---------------------------------------------------\n\n";

    /*
     * ONE THREAD PART
     */
    cout << "\n------ [1] SEARCH WITH ONLY MAIN THREAD -------\n";
    setTime();
    (linearSearch(arr, ARRAY_SIZE,VALUE_OF_KEY)) == 1 ? (cout << "Key was found") : (cout << "Key wasn't found");
    cout << "\nThe time spent - "  << getTime() << endl;
    cout << "-----------------------------------------------\n\n";

    /*
    * MULTIPLE THREADS - THE PARENT WAITS FOR ALL THREADS TO FINISH
    */
    cout << "\n------ [2] MULT. THREADS - PARENT WAITS FOR ALL CHILDREN -------";
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

    /*
     * Go through each sub-array and get rid of
     * all duplicates, which might occur before the
     * key.
     */
    /*
    if (INDEX_OF_KEY != -1){
        int numElementOneThread = ARRAY_SIZE / NUM_THREADS;
        int INDEX_KEY_SUB_ARRAY = INDEX_OF_KEY % numElementOneThread;
        for(int i = 0; i < NUM_THREADS; i++){
            getRidOfKeyDuplicates(&arr[indices[i][0]], ARRAY_SIZE, INDEX_KEY_SUB_ARRAY);
        }
    }
    */

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
    cout << "\nThe time spent - "  << getTime() << endl;
    cout << "----------------------------------------------------------------\n\n";








  /*
  * 3. The parent keeps checking on the children in a busy waiting loop and
   * terminates as soon as one child finds the key or if all children complete
   * their search without finding the key
  */


    cout << "\n------ [3] MULT. THREADS - PARENT KEEPS CHECKING - BUSY WAITING - NO SYNC. -------";
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
            rc = pthread_create(&thr[i], NULL, thr_func_real_busy_waiting, &thr_data[i]);
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
     * incorporate it into a function and then return instead of breaking
     * */
    int counterDone = 0;
    int flag = true;
    while(flag){
        for(int i = 0; i < NUM_THREADS; i++){
            if(found[i] == FOUND){
                flag = false;
                break;
            }
        }
        for(int i = 0; i < NUM_THREADS; i++){
            if(done[i] == DONE){
                counterDone++;
            }
            if(counterDone == NUM_THREADS){
                flag = false;
                break;
            }
        }
    }


    if (counterDone == NUM_THREADS){
        for (int i = 0; i < NUM_THREADS; ++i) {
            pthread_join(thr[i], NULL);
        }
    } else {
        for (int i = 0; i < NUM_THREADS; ++i) {
            pthread_cancel(thr[i]);
        }
    }

    cout << "\nThe time spent - "  << getTime() << endl;
    cout << "----------------------------------------------------------------------------------\n\n";




    /*
    * 4. The parent waits on a semaphore that gets signaled by one of the children
     * either when that child finds the key or when all children have completed
     * their search without finding the key.
    */
    cout << "\n------ [4] MULT. THREADS WITH SYNCRONIZATION --------";

    setTime();

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

    /*
     *  simultaneously unlocks the mutex and begins
     *  waiting for the condition variable to be signalled
     */
    pthread_cond_wait( &condition_cond, &condition_mutex );

    /*
     * Kill all threads after receiving the signal.
     */
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_cancel(thr[i]);
    }
    cout << "\nThe time spent - "  << getTime() << endl;
    /*
     * Show the value of COUNTER only if -1 was entered.
     */
    if(COUNTER == NUM_THREADS){
        cout << "\nCOUNTER : " << COUNTER << endl; }

    cout << "-----------------------------------------------------\n\n";

    /*
     Test Results

     1. Array size = 100M, T = 2, index =-1
     1.                   2.                 3.                  4.
     72|76|81             31|38|45           39|37|39           33|33|44

     2. Array size = 100M, T = 2, index = 1
     1.                   2.                 3.                  4.
     3|1|0                24|27|42           1|0|2               2|4|0

     3. Array size = 100M, T = 2, index = 50M+1
     1.                   2.                 3.                  4.
     45|60|81             25|36|55           1|0|0               5|0|20

     4. Array size = 100M, T=4, index =75M+1
     1.                   2.                 3.                  4.
     52|50|43|33|72       21|36|19|19|44     13|11|19|12|15      10|21|18|19|10
     */

    return 0;
}



