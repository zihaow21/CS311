#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/times.h>

#define NUM_THREADS 20

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

unsigned long long TOTAL_BITS = UINT32_MAX;

char *program_bitmap_array[NUM_THREADS];
uint64_t *small_primes;
char *bitmap_array;

struct thread_args
	{
        uint64_t thread_index;
        uint64_t start_bit;
        uint64_t end_bit;
        uint64_t num_primes;
	};

void *mark_primes(void *arguments)
{
    uint64_t j;
    uint64_t k;
    uint64_t i;
    uint64_t counter = 0;
    uint64_t n;

    struct thread_args *data = (struct thread_args *) arguments;
    n = ((data->end_bit - data->start_bit) + 1);

    //printf("This is thread %lld working [%lld - %lld] n is %lld \n", data->thread_index, data->start_bit, data->end_bit, n);

    if(data->start_bit == 1){
        data->start_bit = 2;
    }

    for(i = data->start_bit; i < data->end_bit; i += 3) {
		if(!BITTEST(bitmap_array, i)){
			for(j = i * i; j < data->end_bit; j += i)
				BITSET(bitmap_array, j);
				//printf("%lld is not prime\n", j);
		}
	}

    for(i = data->start_bit; i < data->end_bit; i++){
        if(!BITTEST(bitmap_array, i)){
            counter++;
            //printf("%lld is prime\n", i);
        }
    }
    //printf("number of primes for thread %lld = %lld\n\n",data->thread_index, counter);
    data->num_primes = counter;

    pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
    uint64_t thread_array_size = 0;
    uint64_t remainder = 0;
    uint64_t array_offset = 1;
    uint64_t err;
    uint64_t i;
    uint64_t j;
    uint64_t k;
    uint64_t total_num_primes = 0;
    double total_time;
    clock_t start;
    clock_t stop;
    pthread_attr_t attr;
    void *status;

    bitmap_array = (char *) malloc(sizeof(char) * ((TOTAL_BITS / CHAR_BIT)));
    memset(bitmap_array, 0, BITNSLOTS(TOTAL_BITS));

    struct thread_args thread_array[NUM_THREADS];
    pthread_t thread_id[NUM_THREADS];
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    thread_array_size = (BITNSLOTS(TOTAL_BITS) / NUM_THREADS);
    remainder = (BITNSLOTS(TOTAL_BITS) % NUM_THREADS);

    //printf("size of each thread = %lld\n", thread_array_size);
    //printf("size of remainder = %lld\n", remainder);
    //printf("total number chars = %d\n", BITNSLOTS(TOTAL_BITS));
    fprintf(stderr, "finding primes up to UINT32 please wait (aprox 109 seconds)...");
    start = clock();

    //marks all base prime numbers up to sqrt(TOTAL_BITS)
    for(i = 2; i < sqrt(TOTAL_BITS); i++) {
		if(!BITTEST(bitmap_array, i)) {
			for(j = i * i; j < TOTAL_BITS; j += i)
				BITSET(bitmap_array, j);
		}
	}

    for(i = 0; i < NUM_THREADS; i++){
        if(remainder > 0){

            //printf("(uneven)creating thread %d of size %d\n", i, (thread_array_size + 1));
            thread_array[i].start_bit = array_offset;
            array_offset += (thread_array_size + 1) * CHAR_BIT;
            thread_array[i].end_bit = array_offset - 1;
            remainder--;
        }
        else{
            //pass the size of the thread as an argument
            //printf("creating thread %d of size %d\n", i, (thread_array_size));
            thread_array[i].start_bit = array_offset;
            array_offset += thread_array_size * CHAR_BIT;
            thread_array[i].end_bit = array_offset - 1;
        }
        thread_array[i].thread_index = i;
        thread_array[i].num_primes = 0;

        //pass the char_offset of the thread as an argument

        err = pthread_create(&thread_id[i], &attr, mark_primes, (void *) &thread_array[i]);
        if (err){
            fprintf(stderr, "ERROR; return code from pthread_create() is %lld\n", err);
            exit(-1);
        }
    }

    //join the threads when they are done
    pthread_attr_destroy(&attr);
    for(i = 0; i < NUM_THREADS; i++) {
        err = pthread_join(thread_id[i], &status);
        if (err) {
            fprintf(stderr, "ERROR; return code from pthread_join() is %lld\n", err);
            exit(-1);
        }
        //printf("Main: completed join with thread %ld having a status of %ld\n", i, (long)status);
    }

    //sum the primes from each thread
    for(i = 0; i < NUM_THREADS; i++){
        total_num_primes += thread_array[i].num_primes;
    }

    stop = clock();
    total_time = (double)(stop - start) / (double)CLOCKS_PER_SEC;

    printf("Found %lld primes in %f seconds\n", total_num_primes - 1, total_time);
    printf("Main: program completed. Exiting.\n");

   //clean-up
   free(bitmap_array);
   pthread_exit(NULL);
}
