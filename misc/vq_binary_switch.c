/*
  vq_binary_switch.c
  David Rowe Dec 2021

  C implementation of [1], that re-arranges VQ indexes so they are robust to single
  bit errors.

  [1] Psuedo Gray Coding, Zeger & Gersho 1990 
*/

#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mbest.h"

#define MAX_DIM     20
#define MAX_ENTRIES 4096

// equation (33) of [1], total cost of all hamming distance 1 vectors of vq index k
float cost_of_distance_one(float *vq, int n, int dim, float *prob, int k, int st, int en, int verbose) {
  int log2N = log2(n);
  float c = 0.0;
  for (int b=0; b<log2N; b++) {
    unsigned int index_neighbour = k ^ (1<<b);
    float dist = 0.0;
    for(int i=st; i<=en; i++)
      dist += pow(vq[k*dim+i] - vq[index_neighbour*dim+i], 2.0);
    c += prob[k]*dist;
    if (verbose)
      printf("k: %d b: %d index_neighbour: %d dist: %f prob: %f c: %f \n", k, b, index_neighbour, dist, prob[k], c);
  }
  return c;
}

// equation (39) of [1]
float distortion_of_current_mapping(float *vq, int n, int dim, float *prob, int st, int en) {
  float d = 0.0;
  for(int k=0; k<n; k++)
    d += cost_of_distance_one(vq, n, dim, prob, k, st, en, 0);
  return d;
}
 
// we sort the cost array c[], returning the indexes of sorted elements
float c[MAX_ENTRIES];

/* Note how the compare function compares the values of the
 * array to be sorted. The passed value to this function
 * by `qsort' are actually the `idx' array elements.
 */
int compare_increase (const void * a, const void * b) {
    int aa = *((int *) a), bb = *((int *) b);
    if (c[aa] < c[bb]) {
        return 1;
    } else if (c[aa] == c[bb]) {
        return 0;
    } else {
        return -1;
     }
}

void sort_c(int *idx, const size_t n) {
    for (size_t i=0; i<n; i++) idx[i] = i;
    qsort(idx, n, sizeof(int), compare_increase);
}

void swap(float *vq, int dim, int index1, int index2) {
  float tmp[dim];
  for(int i=0; i<dim; i++) tmp[i] = vq[index1*dim+i];
  for(int i=0; i<dim; i++) vq[index1*dim+i] = vq[index2*dim+i];
  for(int i=0; i<dim; i++) vq[index2*dim+i] = tmp[i];
}

int main(int argc, char *argv[]) {
    float vq[MAX_DIM*MAX_ENTRIES];
    int   dim = MAX_DIM;
    int   max_iter = 200;
    int   st = -1;
    int   en = -1;
    int   verbose = 0;
    int   n = 0;
    
    int o = 0; int opt_idx = 0;
    while (o != -1) {
       static struct option long_opts[] = {
           {"st",      required_argument, 0, 't'},
           {"en",      required_argument, 0, 'e'},
	   {0, 0, 0, 0}
        };
        o = getopt_long(argc,argv,"hd:m:vt:e:n:",long_opts,&opt_idx);
        switch (o) {
	case 'd':
	    dim = atoi(optarg);
	    assert(dim <= MAX_DIM);
	    break; 
        case 'm':
            max_iter = atoi(optarg);
            break;
        case 't':
            st = atoi(optarg);
            break;
        case 'e':
            en = atoi(optarg);
            break;
        case 'n':
            n = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
	help:
	    fprintf(stderr, "\n");
            fprintf(stderr, "usage: %s -d dimension [-m max_iterations -v --st Kst --en Ken -n nVQ] vq_in.f32 vq_out.f32\n", argv[0]);
	    fprintf(stderr, "\n");
            fprintf(stderr, "-n nVQ      Run with just the first nVQ entries of the VQ\n");
            fprintf(stderr, "--st Kst    Start vector element for error calculation (default 0)\n");
            fprintf(stderr, "--en Ken    End vector element for error calculation (default K-1)\n");
            fprintf(stderr, "-v          verbose\n");
            exit(1);
        }
    }

    int dx = optind;
    if ((argc - dx) < 2) {
        fprintf(stderr, "Too few arguments\n");
	goto help;
    }
    if (dim == 0) goto help;

    /* default to measuring error on entire vector */
    if (st == -1) st = 0; 
    if (en == -1) en = dim-1;

    /* load VQ quantiser file --------------------*/

    fprintf(stderr, "loading %s ... ", argv[dx]);
    FILE *fq=fopen(argv[dx], "rb");
    if (fq == NULL) {
      fprintf(stderr, "Couldn't open: %s\n", argv[dx]);
      exit(1);
    }

    if (n) {
      /* count how many entries m of dimension k are in this VQ file */
      int n = 0;
      float dummy[dim];
      while (fread(dummy, sizeof(float), dim, fq) == (size_t)dim)
	n++;
      assert(n <= MAX_ENTRIES);
      fprintf(stderr, "%d entries of vectors width %d\n", n, dim);

      rewind(fq);
    }
    /* load VQ into memory */
    int rd = fread(vq, sizeof(float), n*dim, fq);
    assert(rd == n*dim);
    fclose(fq);

    /* set probability of each vector to 1.0 for now */
    float prob[n];
    for(int l=0; l<n; l++) prob[l] = 1.0;
    
    int iteration = 0;
    int i = 0;
    int finished = 0;
    int switches = 0;
    float distortion0 = distortion_of_current_mapping(vq, n, dim, prob, st, en);
    fprintf(stderr, "distortion0: %f\n", distortion0);
    
    while(!finished) {

      // generate a list A(i) of which vectors have the largest cost of bit errors
      for(int k=0; k<n; k++) {
	c[k] = cost_of_distance_one(vq, n, dim, prob, k, st, en, verbose);
	//fprintf(stderr, "c[%d] = %f\n", k, c[k]);
      }
      
      int A[n];
      sort_c(A, n);
      for(int k=0; k<n; k++) {
	//fprintf(stderr, "A[%d] = %d\n", k, A[k]);
      }
      
      // Try switching each vector with A(i)
      float best_delta = 0; int best_j = 0;
      for(int j=1; j<n; j++) {
	// we can't switch with ourself
	if (j != A[i]) {
	  float distortion1 = distortion_of_current_mapping(vq, n, dim, prob, st, en);
	  // switch vq entries A(i) and j
	  swap(vq, dim, A[i], j);
	  float distortion2 = distortion_of_current_mapping(vq, n, dim, prob, st, en);
	  float delta = distortion2 - distortion1;
	  if (delta < 0.0) {
	    if (fabs(delta) > best_delta) {
	      best_delta = fabs(delta);
	      best_j = j;
	    }
	  }
	  // unswitch
	  swap(vq, dim, A[i], j);
	}
      } //next j

      // printf("best_delta: %f best_j: %d\n", best_delta, best_j);
      if (best_delta == 0.0) {
	// Hmm, no improvement, lets try the next vector in the sorted cost list
	if (i == n-1) finished = 1; else i++;
      } else {
	// OK keep the switch that minimised the distortion
	swap(vq, dim, A[i], best_j);
	switches++;

	// set up for next iteration
	iteration++;
	float distortion = distortion_of_current_mapping(vq, n, dim, prob, st, en);
	printf("it: %3d dist: %f %3.2f i: %3d sw: %3d\n", iteration, distortion,
	       distortion/distortion0, i, switches);
	if (iteration >= max_iter) finished = 1;
	i = 0;
      }
    }
    
    return 0;
}

