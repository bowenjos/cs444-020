#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "mt.h"
/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */



pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
struct args{
	long tid;
	long sleep_time;
};

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}


/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

/* generates a random number on [0,0x7fffffff]-interval */
long genrand_int31(void)
{
    return (long)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(void) 
{ 
    unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
}
void *consumer(void *tid)
{
//	printf("trying to lock from a consumer thread \n");
	struct args *a = (struct args*)tid;
	while(1){		
		if (a->sleep_time<10 && a->sleep_time>1 && a->tid > -1 && a->tid <32){
			pthread_mutex_lock (&mymutex);
//	printf("Consumer Sleeptime %ld\n", a->sleep_time);
			long conSleepTime = a->sleep_time;
			long conTid = a->tid;
			a->sleep_time = -1;
			a->tid = -1;
			pthread_mutex_unlock (&mymutex);
			sleep(conSleepTime);
			return (void *)	printf("Hello from thread %ld! I did %ld units of work!\n", conTid, conSleepTime);
//	return (void *)printf("stopping consumer\n");
		}
	}

}
/*
void *producer(void *tid){
	pthread_mutex_lock (&mymutex);
	long producer_sleep = genrand_int32() % 5;
	printf("producersleep %ld\n",  producer_sleep+2);
	sleep(producer_sleep);
	struct args *a = (struct args*)tid;
	a->sleep_time = genrand_int32() % 10;
	pthread_mutex_unlock (&mymutex);
	return (void*) printf("Stopping producer\n");

}*/

int main(int argc, char **argv)
{
	unsigned long init[4] = {0x123, 0x234, 0x345, 0x456};
	unsigned long length = 4;
	int mt = 0;
	printf("Number of consumers to be created %s \n", argv[1]);
	
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;

	char vendor[13];
	
	eax = 0x01;

	__asm__ __volatile__(
	                     "cpuid;"
	                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
	                     : "a"(eax)
	                     );
	
	if(ecx & 0x40000000){
		//use rdrand
	//	rdrand_32(int, int);

	}
	else{
		//use mt19937
		//init_by_array(init, length);
		mt = 1;
		time_t t;
		init_genrand((unsigned) time(&t));	
	}
	
	pthread_t threads[atoi(argv[1])];
	struct args a[atoi(argv[1])];
	//for(long j = 0; j < 32; j++){
//		pthread_mutex_lock (&mymutex);
	//	a[j].sleep_time = 100;
	//	a[j].tid = j;
//		pthread_mutex_unlock(&mymutex);
//	}
	long j = 0;
	for(long i = 0; i < atoi(argv[1]); ++i){
		/* int pthread_create(pthread_t *thread, const pthread_attr_t *attr, */
		/*                    void *(*start_routine) (void *), void *arg); */
		//		a[i].tid = i;
//		pthread_create(&threads[0], NULL, producer, (void *)( &a[i]));
		pthread_create(&(threads[i]),
		               NULL,
		               consumer,
		               (void *)( &a[j]));
		j++;
		if(i==32){
			j = 0;
		}	
	}
	for(long i = 0; i < atoi(argv[1]); ++i){
		pthread_mutex_lock (&mymutex);
		a[i].tid = i;
 		long consumerSleep = genrand_int32() % 8;
		a[i].sleep_time = consumerSleep+2;
//		printf("Current buffer: \n");
//		for(int j = 0; j < 32; j++){
//			if(a[j].sleep_time < 10 && a[j].sleep_time > 0){
//				printf("%ld : %ld \n", a[j].tid, a[j].sleep_time);
//			}
//		}
		pthread_mutex_unlock(&mymutex);
		if(mt == 1){
			long sleepTime = genrand_int32()%5;
			sleep(sleepTime+2);
			printf("Producer just slept for: %ld and filled tid %ld\n", sleepTime+2, i);	
		}
		else{
			long sleepTime = 1;//rdrand code here
			sleep(sleepTime+2);
			printf("Producer just slept for: %ld and filled tid %ld\n", sleepTime+2, i);	
		}
		if(i==31){
			i = 0;
		}
	}
	for(long i = 0; i < atoi(argv[1]); ++i){
		pthread_join(threads[i], NULL);
	}
	
	return 0;
}
