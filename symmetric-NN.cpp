// New implementation of Symmetric Neural Network based on the idea from DeepSet and PointNet,
// ie, that any symmetric function f(x,y,...) can be represented in the form g(h(x), h(y), ...)
// where g and h are non-linear functions (implemented as deep neural networks)

// The entire network is composed of g-layers and h-layers.
// The g-layers form a regular NN.
// The h-layers form another NN but it is averaged over m inputs, where m is the multiplicity

#include <iostream>
#include <cstdio>
#include <random>
#include <algorithm>		// random_shuffle
#include <math.h>
#include "feedforward-NN.h"

using namespace std;

extern NNET *create_NN(int, int *);
extern void free_NN(NNET *, int *);
extern void forward_prop_sigmoid(NNET *, int, double *);
extern void forward_prop_ReLU(NNET *, int, double *);
extern void forward_prop_softplus(NNET *, int, double *);
extern void forward_prop_x2(NNET *, int, double *);
extern void back_prop(NNET *, double *);
extern void back_prop_ReLU(NNET *, double *);
extern void re_randomize(NNET *, int, int *);
extern double sigmoid(double);
/*
extern void pause_graphics();
extern void quit_graphics();
extern void start_NN_plot(void);
extern void start_NN2_plot(void);
extern void start_W_plot(void);
extern void start_K_plot(void);
extern void start_output_plot(void);
extern void start_LogErr_plot(void);
extern void restart_LogErr_plot(void);
extern void plot_NN(NNET *net);
extern void plot_NN2(NNET *net);
extern void plot_W(NNET *net);
extern void plot_output(NNET *net, void ());
extern void plot_LogErr(double, double);
extern void flush_output();
extern void plot_tester(double, double);
extern void plot_K();
extern int delay_vis(int);
extern void plot_trainer(double);
extern void plot_ideal(void);
*/
extern "C" void beep(void);
extern "C" void start_timer(), end_timer(char *);

#define N		3
extern "C" double K[N];
double K[N];

double random01()
	{
	return rand() * 2.0 / (float) RAND_MAX - 1.0;
	}

// To test convergence, we record the sum of squared errors for the last M and last M..2M
// trials, then compare their ratio.

// Success: time 5:58, topology = {2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1} (13 layers)
//			ReLU units, learning rate 0.05, leakage 0.0
#define ForwardPropMethod	forward_prop_ReLU
#define ErrorThreshold		0.02

int main(int argc, char **argv)
	{
	int neuronsPerLayer[] = {N, 10, 10, 8, N}; // first = input layer, last = output layer
	int numLayers = sizeof (neuronsPerLayer) / sizeof (int);
	NNET *Net_g = create_NN(numLayers, neuronsPerLayer);
	NNET *Net_h = create_NN(numLayers, neuronsPerLayer);
	LAYER lastLayer_g = Net_g->layers[numLayers - 1];
	LAYER lastLayer_h = Net_h->layers[numLayers - 1];

	int userKey = 0;
	#define M	50			// how many errors to record for averaging
	double errors1[M], errors2[M]; // two arrays for recording errors
	double sum_err1 = 0.0, sum_err2 = 0.0; // sums of errors
	int tail = 0; // index for cyclic arrays (last-in, first-out)

	/*
	if (argc != 2)
		{
		printf("Learning Neuromancer map\n");
		printf("      <N> = dimension of set vectors\n");
		exit(0);
		}
	else
		{
		// test_num = std::stoi(argv[1]);
		N = std::stoi(argv[1]);
		}
	*/

	srand(time(NULL));				// random seed

	for (int i = 0; i < M; ++i) // clear errors to 0.0
		errors1[i] = errors2[i] = 0.0;

	// start_NN_plot();
	// start_W_plot();
	// start_K_plot();
	// start_output_plot();
	// start_LogErr_plot();
	// plot_ideal();
	printf("Press 'Q' to quit\n\n");
	start_timer();

	char status[1024], *s;

	double errors1[N], errors2[N];
	double X1[N], X2[N], Y1[N], Y2[N];

	for (int i = 1; 1; ++i)
		{
		s = status + sprintf(status, "[%05d] ", i);

		// Create random K vector
		for (int k = 0; k < N; ++k)
			K[k] = (rand() / (float) RAND_MAX);
		// printf("*** K = <%lf, %lf>\n", K[0], K[1]);

		//		if ((i % 4) == 0)
		//			K[0] = 1.0, K[1] = 0.0;
		//		if ((i % 4) == 1)
		//			K[0] = 0.0, K[1] = 0.0;
		//		if ((i % 4) == 2)
		//			K[0] = 0.0, K[1] = 1.0;
		//		if ((i % 4) == 3)
		//			K[0] = 1.0, K[1] = 1.0;

		// Calculate the "error" which requires we evaluate the ANN on a point and its
		// permutation (or random perturbation).

		// Initially, I tried to use the idea of the "joint_penalty", ie, calculated from the
		// set distance between the original points and the Euclidean distance between the
		// transformed points.  But this penalty cannot be applied to back-prop because it
		// does not tell the "error" for each output (with respect to an ideal output);  it
		// merely measures how bad the output is.

		// The new idea is to artificially construct an "error" with respect to an ideal
		// output.  When the input point is permuted, the output should be invariant, and
		// this gives an ideal output (specific to that pair of original/permuted inputs).
		// When the input point is randomly generated, its output can be compared with the
		// previous output, and there would be a "repulsive force" between the output points
		// if they are too close.  The magnitude of the repulsion is given by the "joint penalty"
		// as before, but now it also has a direction which is along the line between the
		// output points.

		// We need to keep track of:
		//		X1, X2 = input vectors (dim N)
		//		Y1, Y2 = output vectors (dim N)
		// The neural network maps X1 to Y1, and X2 to Y2, independently.
		//		errors1 = errors for Y1 (dim N)
		//		errors2 = errors for Y2 (dim N)
		ForwardPropMethod(Net_g, N, X1);			// X1 is now Y1
		ForwardPropMethod(Net_h, N, X2);			// X2 is now Y2

		training_err += fabs(error); // record sum of errors
		// printf("sum of squared error = %lf  ", training_err);

		// update error arrays cyclically
		// (This is easier to understand by referring to the next block of code)
		sum_err2 -= errors2[tail];
		sum_err2 += errors1[tail];
		sum_err1 -= errors1[tail];
		sum_err1 += training_err;
		// printf("sum1, sum2 = %lf %lf\n", sum_err1, sum_err2);

		double mean_err = (i < M) ? (sum_err1 / i) : (sum_err1 / M);
		if (mean_err < 2.0)
			s += sprintf(s, "mean |e|=%1.06lf, ", mean_err);
		else
			s += sprintf(s, "mean |e|=%e, ", mean_err);

		// record new error in cyclic arrays
		errors2[tail] = errors1[tail];
		errors1[tail] = training_err;
		++tail;
		if (tail == M) // loop back in cycle
			tail = 0;

		// plot_W(Net);
		back_prop(Net, errors);
		// plot_W(Net);
		// pause_graphics();

		if ((i % 200) == 0)
			{
			// Testing set
			double test_err = 0.0;
			#define numTests 50
			for (int j = 0; j < numTests; ++j)
				{
				// Create random K vector
				for (int k = 0; k < 2; ++k)
					K[k] = ((double) rand() / (double) RAND_MAX);
				// plot_tester(K[0], K[1]);

				ForwardPropMethod(Net_g, 2, K);

				// Desired value = K_star
				double single_err = 0.0;
				for (int k = 0; k < 1; ++k)
					{
					// double ideal = 1.0f - (0.5f - K[0]) * (0.5f - K[1]);
					double ideal = (double) (f2b(K[0]) ^ f2b(K[1]));
					// double ideal = K[k];				/* identity function */

					// Difference between actual outcome and desired value:
					double error = ideal - lastLayer.neurons[k].output;

					single_err += fabs(error); // record sum of errors
					}
				test_err += single_err;
				}
			test_err /= ((double) numTests);
			if (test_err < 2.0)
				s += sprintf(s, "random test |e|=%1.06lf, ", test_err);
			else
				s += sprintf(s, "random test |e|=%e, ", test_err);
			if (test_err < ErrorThreshold)
				break;
			}

		if (i > 50 && (isnan(mean_err) || mean_err > 10.0))
			{
			re_randomize(Net, numLayers, neuronsPerLayer);
			sum_err1 = 0.0; sum_err2 = 0.0;
			tail = 0;
			for (int j = 0; j < M; ++j) // clear errors to 0.0
				errors1[j] = errors2[j] = 0.0;
			i = 1;

			// restart_LogErr_plot();
			start_timer();
			printf("\n****** Network re-randomized.\n");
			}

		if ((i % 50) == 0)
			{
			double ratio = (sum_err2 - sum_err1) / sum_err1;
			if (ratio > 0)
				s += sprintf(s, "|e| ratio=%e", ratio);
			else
				s += sprintf(s, "|e| ratio=\x1b[31m%e\x1b[39;49m", ratio);
			//if (isnan(ratio))
			//	break;
			}

		if ((i % 10) == 0) // display status periodically
			{
			printf("%s\n", status);
			// plot_NN(Net);
			// plot_W(Net);
			// plot_LogErr(mean_err, ErrorThreshold);
			// plot_output(Net, ForwardPropMethod);
			// flush_output();
			// plot_trainer(0);		// required to clear the window
			// plot_K();
			// userKey = delay_vis(0);
			}

		// if (ratio - 0.5f < 0.0000001)	// ratio == 0.5 means stationary
		// if (test_err < 0.01)

		if (userKey == 1)
			break;
		else if (userKey == 3)			// Re-start with new random weights
			{
			re_randomize(Net, numLayers, neuronsPerLayer);
			sum_err1 = 0.0; sum_err2 = 0.0;
			tail = 0;
			for (int j = 0; j < M; ++j) // clear errors to 0.0
				errors1[j] = errors2[j] = 0.0;
			i = 1;

			// restart_LogErr_plot();
			start_timer();
			printf("\n****** Network re-randomized.\n");
			userKey = 0;
			beep();
			// pause_key();
			}
		}

	end_timer(NULL);
	beep();
	// plot_output(Net, ForwardPropMethod);
	// flush_output();
	// plot_W(Net);

	// if (userKey == 0)
	//	pause_graphics();
	// else
	//	quit_graphics();
	free_NN(Net_g, neuronsPerLayer);
	free_NN(Net_h, neuronsPerLayer);
	}
