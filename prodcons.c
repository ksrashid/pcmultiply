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
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
	
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;


int count = 0;
int idx_pro = 0;
int idx_con = 0;

// Bounded buffer put() get()
int put(Matrix * value)
{
  if (value == NULL) {
    return 0;
  }
  pthread_mutex_lock(&buffer_mutex);

  while (count == BOUNDED_BUFFER_SIZE) {
    pthread_cond_wait(&not_full, &buffer_mutex);
  }

  bigmatrix[idx_pro] = value;

  idx_pro = (idx_pro + 1) % BOUNDED_BUFFER_SIZE; //ask elijah how to insert at 0
  count++;
   
  
  pthread_cond_signal(&not_empty);
  pthread_mutex_unlock(&buffer_mutex);

  return 1;
}

Matrix * get()
{

  pthread_mutex_lock(&buffer_mutex);

  while (count == 0) {
    pthread_cond_wait(&not_empty, &buffer_mutex);
    if (count == 0) {
	    pthread_mutex_unlock(&buffer_mutex);
	    return NULL;
    }
  }

  Matrix *result = bigmatrix[idx_con];
  idx_con = (idx_con + 1) % BOUNDED_BUFFER_SIZE; //ask elijah how to insert at 0
  count--;
   
  
  pthread_cond_signal(&not_full);
  pthread_mutex_unlock(&buffer_mutex);
  return result;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
  int sumtotal = 0;
  int matrixtotal = 0;
  
  counters_t *counters = (counters_t *)arg;
  
  while (get_cnt(counters->prod) < NUMBER_OF_MATRICES) {
     Matrix *matrix = GenMatrixRandom();
     put(matrix);
     sumtotal += SumMatrix(matrix);
     matrixtotal++;
     increment_cnt(counters->prod);     
  }
  
  struct prodcons *prodstats = (struct prodcons *)malloc(sizeof(struct prodcons));
  prodstats->sumtotal = sumtotal;
  prodstats->matrixtotal = matrixtotal;
  prodstats->multtotal = 0;
  return (void *)prodstats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  int sumtotal = 0;
  int matrixtotal = 0;
  int multtotal = 0;
  
  counters_t *counters = (counters_t *)arg;
  int consumed;
  Matrix *m1, *m2, *m3;
  m1 = NULL;
  while (1) {

    // break if we have consumed all we need
    consumed = get_cnt(counters->cons);
    if (consumed >= NUMBER_OF_MATRICES) {
      break;
    }

    
    if (m1 == NULL) {
      m1 = get();
      if (m1 == NULL) {
        break;
      }
      sumtotal += SumMatrix(m1);
      matrixtotal++;
      increment_cnt(counters->cons);
    }

    
    consumed = get_cnt(counters->cons);
    if (consumed >= NUMBER_OF_MATRICES) {
      break;
    }
	
    m2 = get();
    if (m2 == NULL) {
//      consumed = get_cnt(counters->cons);
//      if (consumed < NUMBER_OF_MATRICES) {
//	      m2 = get();
//      }
      break;
    }
    sumtotal += SumMatrix(m2);
    matrixtotal++;
    increment_cnt(counters->cons);



// add lock so threads dont compete to display
    pthread_mutex_lock(&print_mutex);
    m3 = MatrixMultiply(m1, m2);

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
      multtotal++;
    } else {
      FreeMatrix(m2);
      m2 = NULL;
    }
    pthread_mutex_unlock(&print_mutex);

  }
 


  if (m1 != NULL) {
    FreeMatrix(m1);
  }
  if (m2 != NULL) {
    FreeMatrix(m2);
  }
  if (m3 != NULL) {
    FreeMatrix(m3);
  }
  struct prodcons *prodstats = (struct prodcons *)malloc(sizeof(struct prodcons));
  prodstats->sumtotal = sumtotal;
  prodstats->matrixtotal = matrixtotal;
  prodstats->multtotal = multtotal;
  return (void *)prodstats;
}
