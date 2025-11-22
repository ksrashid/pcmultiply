/*
 *  prodcons module
 *  Producer Consumer module
 *
 *  Implements routines for the producer consumer module based on
 *  chapter 30, section 2 of Operating Systems: Three Easy Pieces
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

// Include only libraries for this module
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "counter.h"
#include "matrix.h"
#include "pcmatrix.h"
#include "prodcons.h"


// Define Locks, Condition variables, and so on here
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t prod_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cons_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
	
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// number of items in the buffer at a given time
int count = 0;
// index for the producer
int idx_pro = 0;
// index for the consumer
int idx_con = 0;

// Bounded buffer put() get()
int put(Matrix * value)
{

// this checks if the value passed is null and returns early
  if (value == NULL) {
    return 0;
  }
// Here we create a lock and then tell the producer to wait until there is space in the buffer 
  pthread_mutex_lock(&buffer_mutex);

  while (count == BOUNDED_BUFFER_SIZE) {
    pthread_cond_wait(&not_full, &buffer_mutex);
  }
// this adds the matrix to the buffer then increments the index for the producer and the total count
  bigmatrix[idx_pro] = value;

  idx_pro = (idx_pro + 1) % BOUNDED_BUFFER_SIZE;
  count++;
   
// this signals to the consumer that there is something to consume
  pthread_cond_signal(&not_empty);
  pthread_mutex_unlock(&buffer_mutex);

  return 1;
}

Matrix * get()
{

  pthread_mutex_lock(&buffer_mutex);
// this checks if the buffer is empty and if it is we wait
  while (count == 0) {
    pthread_cond_wait(&not_empty, &buffer_mutex);
// if the buffer is still empty when we wake (race condition) we return early
    if (count == 0) {
	    pthread_mutex_unlock(&buffer_mutex);
	    return NULL;
    }
  }

// get the matrix stored in the buffer at the position and check
  Matrix *result = bigmatrix[idx_con];
// set the index of the next item to be consumed, decrease the count (item has been consumed)
  idx_con = (idx_con + 1) % BOUNDED_BUFFER_SIZE;
  count--;
   
// send a signal to producer that the buffer is not full
  pthread_cond_signal(&not_full);
  pthread_mutex_unlock(&buffer_mutex);
  return result;
}



// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
// the sum of all elements produced 
  int sumtotal = 0;
// the number of matricies produced
  int matrixtotal = 0;

// Initialize the counter
  counters_t *counters = (counters_t *)arg;
// while the number of matrices produced is less than the max number possible
// produce a random matrix, add it to the sumtotal, and increment the matrix total


  while (1) {
    pthread_mutex_lock(&prod_mutex);
    
    if (get_cnt(counters->prod) >= NUMBER_OF_MATRICES) {
      pthread_mutex_unlock(&prod_mutex);
      break;
    }    
    increment_cnt(counters->prod);
    pthread_mutex_unlock(&prod_mutex);
    Matrix *matrix = GenMatrixRandom();
    put(matrix);
    sumtotal += SumMatrix(matrix);
    matrixtotal++;
  }
  
// initialize a struct to keep track of producer statistics
  struct prodcons *prodstats = (struct prodcons *)malloc(sizeof(struct prodcons));
  prodstats->sumtotal = sumtotal;
  prodstats->matrixtotal = matrixtotal;
  prodstats->multtotal = 0;

// return a void pointer to the struct
  return (void *)prodstats;
}



// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
// initialize the statistics needed for the prodcons struct
  int sumtotal = 0;
  int matrixtotal = 0;
  int multtotal = 0;
// initialize the counter
  counters_t *counters = (counters_t *)arg;
// an int to keep track of how many we have consumed
  int consumed;
  Matrix *m1, *m2, *m3;
  m1 = NULL;

  while (1) {

    // break if we have consumed all we need
    consumed = get_cnt(counters->cons);
    if (consumed >= NUMBER_OF_MATRICES) {
      break;
    }

// if m1 is null fill it with a matrix
    
    if (m1 == NULL) {
      m1 = get();
      sumtotal += SumMatrix(m1);
      matrixtotal++;
      increment_cnt(counters->cons);
// if its still null (race condition) break
      if (m1 == NULL) {
        break;
      }
// add the matrix to the sumtotal, increment the matrixtotal and counters
    }

// check if we have consumed all we can and break if so
    consumed = get_cnt(counters->cons);
    if (consumed >= NUMBER_OF_MATRICES) {
      break;
    }

// break early if we fail to fill m2	
    m2 = get();
    sumtotal += SumMatrix(m2);
    matrixtotal++;
    increment_cnt(counters->cons);
    if (m2 == NULL) {
      break;
    }


// add lock so threads dont compete to display (fix race condition)
    
    pthread_mutex_lock(&print_mutex);
    m3 = MatrixMultiply(m1, m2);
    multtotal++;

    if (m3 != NULL) { 
      DisplayMatrix(m1,stdout);
      printf("    X\n");
      DisplayMatrix(m2,stdout);
      printf("    =\n");
      DisplayMatrix(m3,stdout);
      printf("\n");
      FreeMatrix(m3);
      FreeMatrix(m2);
      FreeMatrix(m1);
      m1=NULL;
      m2=NULL;
      m3=NULL;
    } else {
      FreeMatrix(m2);
      m2 = NULL;
    }
    pthread_mutex_unlock(&print_mutex);
    consumed = get_cnt(counters->cons);
    if (consumed >= NUMBER_OF_MATRICES) {
      break;
    }

  }
 

// free any leftover matrices
  if (m1 != NULL) {
    FreeMatrix(m1);
  }
  if (m2 != NULL) {
    FreeMatrix(m2);
  }
  if (m3 != NULL) {
    FreeMatrix(m3);
  }
// initialize and fill the consumer stats
  struct prodcons *consstats = (struct prodcons *)malloc(sizeof(struct prodcons));
  consstats->sumtotal = sumtotal;
  consstats->matrixtotal = matrixtotal;
  consstats->multtotal = multtotal;

// return pointer to struct
  return (void *)consstats;
}
