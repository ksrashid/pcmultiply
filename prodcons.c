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
pthread_mutex_t not_full = PTHREAD_COND_INITIALIZER;
pthread_mutex_t not_empty = PTHREAD_COND_INITIALIZER;

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
  return NULL;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  return NULL;
}
