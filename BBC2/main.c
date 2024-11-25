#include "def.h"

double     **pi, *alpha, *varphi, *y;
double        *x1;
double     *w;
int        S, iter, K, N, A, *from_node_arc, *to_node_arc, *source, *destination, sglobal, max_iter, *b;
int        *B;
double     *r, *f, *p, *d, *c, M, obj, *vc, rho;
double     *theta, theta0, Q;
double     MAX_DOUBLE = 10000000000;
double     MAX_DOUBLE2 = 10000000;
double     timeTotal = 0;
double	   MasterTime, SubTime;
clock_t	   startTemp, endTemp;
double TimeLimitation = 3 * 60 * 60;
double checkTime;
int *flag;
char OutName[100];
FILE *Out = NULL;
char outfile[20];
double tempgap = 0;
double bestobj;
double     *piVio, *alphaVio;
double	violation;
double  optBD;
int		numCutRelax;
int		numCutBranchLazy;
int		numCutBranchuser;
int* w_hat;
double* dh;
double gap;
int nodecount;  
int lastCheckSubProblem;

double bestUB, bestLB, *dj;
int NumberFixedW;


void main(int argc, char *argv[])
{
	char	instance[20];
	char	path[50];
	FILE		*ini;
	clock_t  start, end;
	double obj_value_ch, obj_value_ls, opt_value;
	double cputime;
	int i, j, k, s, h, l, MaxNumInst, numSample;
	start = clock();
	M = 0;
	if (argc == 1) {
		printf("Error: Input file not specified \n");
		exit(8);
	}
	ini = Open_File(argv[1], "r");
	fscanf(ini, "%d", &MaxNumInst);
	fscanf(ini, "%s", &outfile);

	ttt = time(NULL);
	tm = localtime(&ttt);
	Out = Open_File(outfile, "a+");
	fprintf(Out, "\n %s\n", asctime(tm));
	fclose(Out);

	for (numSample = 1; numSample < MaxNumInst; numSample++){
		fscanf(ini, "%s", &instance);
		sprintf(path, "./Data/");
		strcat(path, instance);

		read_instance(path);
		
		numCutRelax = 0;
		numCutBranchLazy = 0;
		numCutBranchuser = 0;
	
		
		Out = Open_File(outfile, "a+");					//Writing what instance we are solving
		fprintf(Out, "\n%s | %s ;\t", argv[1], instance);
		fprintf(Out, "%d\t%d\t%d\t%d\t|\t", N, A, K, S);
		fclose(Out);
		double tempM = 0;
		double TotalDemand = 0;
		for (k = 0; k < K; k++){
			TotalDemand += d[k];
		}
		
		for (k = 0; k < K; k++){
			from_node_arc[A + k] = source[k];
			to_node_arc[A + k] = destination[k];
			c[A + k] = TotalDemand;
			vc[A + k] = 10000;
			f[A + k] = 0;
			r[(A + k)*K + k] = 0;
			b[A + k] = 1000;
		}
		
		gettimeofday(&startTotal, NULL);
		//startTemp = clock(); ///// To run the codes in Visual Studio, you should use clock() structure for time instead of gettimeofday(&startTotal, NULL) structure.
		
		M=0;
		for (h = 0; h < A + K; h++){
			M += vc[h];
		}

		double Total_theta = 0;
		
		for (s = 0; s < S; s++){
			theta[s] = 0;
			Total_theta += theta[s];
		}
		theta0 = -1 * MAX_DOUBLE;
		Upper_bound = MAX_DOUBLE;
		bestobj = MAX_DOUBLE;
		float xx = 0;
		Q = 0;
		iter = 0;
		checkTime = 0;
		lastCheckSubProblem = 0;
		w_hat = create_int_vector(A + K);
		for (h = 0; h < A + K; h++) {
			w_hat[h] = 0;
		}

		///// We find the installation decisions in warm start functions as stated in Section 5.4.1. of the paper. Then we solve solve_SubProblem() function to create the optimality cuts.
		warm_start();
		for (sglobal = 0; sglobal < S; sglobal++) {
			Q += p[sglobal] * solve_SubProblem();
		}
		iter++;
		warm_start2();
		for (sglobal = 0; sglobal < S; sglobal++) {
			Q += p[sglobal] * solve_SubProblem();
		}
		iter++;
		warm_start3();
		for (sglobal = 0; sglobal < S; sglobal++) {
			Q += p[sglobal] * solve_SubProblem();
		}
		iter++;
		lastCheckSubProblem = 1;
		for (h = 0; h < A + K; h++) {
			w[h] = 1;
		}
		for (sglobal = 0; sglobal < S; sglobal++) {
			Q += p[sglobal] * solve_SubProblem();
		}

		NumberFixedW = 0;
		while (fabs((Q - Total_theta) / Q) >= 0.005 && checkTime < TimeLimitation){
			iter++;
			Total_theta = 0;
			obj = solve_MasterProblem();
			for (s = 0; s < S; s++){
				Total_theta += theta[s] * p[s];
			}
			Q = 0;
			for (sglobal = 0; sglobal < S; sglobal++){
				Q += p[sglobal] * solve_SubProblem();
			}
		
			SubTime = 0;
		
		//	checkTime = clock();
		//	checkTime = (double)(endTemp - startTemp) / (double)(CLOCKS_PER_SEC);
			gettimeofday(&stop, NULL);
			checkTime = ((double)(stop.tv_sec - startTotal.tv_sec) * 1000 + (double)(stop.tv_usec - startTotal.tv_usec) / 1000) / 1000;
		}
		
		bestLB = obj;
		bestUB = (1 - rho) * Q;
		for (h = 0; h < A + K; h++) {
			bestUB += f[h] * w[h];
		}
		for (h = 0; h < A + K; h++) {
			for (k = 0; k < K; k++) {
				bestUB += rho * vc[h] * y[h * K + k];
			}
		}	

		double tempobj = 0;

		if(bestUB < bestLB){
			tempobj = bestUB;
			bestUB = bestLB;
			bestLB = tempobj;
		}

		obj = solve_branch_benders_cut();
 		//end = clock();
		gettimeofday(&stopTotal, NULL);
		//timeTotal = (double)(end - start) / (double)(CLOCKS_PER_SEC);
		timeTotal = ((double)(stopTotal.tv_sec - startTotal.tv_sec) * 1000 + (double)(stopTotal.tv_usec - startTotal.tv_usec) / 1000) / 1000;

		Out = Open_File(outfile, "a+");
		fprintf(Out, "NumberFixed = %d\t", NumberFixedW);
		fprintf(Out, "node = %d\t", nodecount);
		fprintf(Out, "gap = %f\t", gap);
		fprintf(Out, "Final Upper Bound = %f\t", obj);
		fprintf(Out, "%f\t", timeTotal);

		fprintf(Out, "\t|\t w: \t|\t");
		for (h = 0; h < A; h++){
			if (w[h]>0.01)
			fprintf(Out, "%d\t", h);
		}

		fprintf(Out, "\t|\t x: \t|\t");
		for (h = 0; h < A; h++){
			for (s = 0; s<S; s++){
				if (x1[h*S + s]>0.01)
					fprintf(Out, "|\t%d\t%d\t|\t", h, s);
			}
		}

		fprintf(Out, "RelaxedCuts:\t %d\t", numCutRelax);
		fprintf(Out, "LazyCuts:\t %d\t", numCutBranchLazy);
		fprintf(Out, "UserCuts:\t %d\t", numCutBranchuser);

		fclose(Out);

		free_memory();

	}
	fclose(ini);
}


void Print_solution(void)
{

}
