#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define INPUT_NOEUD 2
#define HIDDEN_NOEUD 2
#define OUTPUT_NOEUD 1
#define PASSAGE 50000
#define RATE 0.5


void initialiser(double poids1[INPUT_NOEUD][HIDDEN_NOEUD], double poids2[HIDDEN_NOEUD][OUTPUT_NOEUD], double biais1[HIDDEN_NOEUD], double biais2[OUTPUT_NOEUD])
{
	for(int i = 0; i< INPUT_NOEUD; i++)
	{
		for(int j= 0; j < HIDDEN_NOEUD; j++)
		{
			poids1[i][j] = ((double)rand()/RAND_MAX)*2 -1;
		}
	}
	for(int i = 0; i< HIDDEN_NOEUD; i++)
        {
                for(int j= 0; j < OUTPUT_NOEUD; j++)
                {
                        poids2[i][j] = ((double)rand()/RAND_MAX)*2 -1;
                }
        } 
	for(int i = 0; i< HIDDEN_NOEUD; i++)
        {
                biais1[i] = ((double)rand()/RAND_MAX)*2 -1;
        } 
	for(int i = 0; i< OUTPUT_NOEUD; i++)
        {
                biais2[i] =((double)rand()/RAND_MAX)*2 -1;
        } 
}

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid(double y) {
    return y * (1.0 - y);
}


void backpropagation(double hidden[HIDDEN_NOEUD],double biais2[OUTPUT_NOEUD], double w[HIDDEN_NOEUD][OUTPUT_NOEUD], double result, double erreur, double g_hidden[HIDDEN_NOEUD])
{
        double g_sorti = erreur* dsigmoid(result);
        biais2[0] += RATE * g_sorti;

        for(int j=0; j< HIDDEN_NOEUD; j++)
        {
                w[j][0] += RATE * g_sorti * hidden[j];
        }


        for(int i= 0; i< HIDDEN_NOEUD; i++)
        {
                g_hidden[i] = g_sorti  * w[i][0] * dsigmoid(hidden[i]);
        }

}
			 

void training(double input[4][2], double output[4],
              double poids1[INPUT_NOEUD][HIDDEN_NOEUD],
              double poids2[HIDDEN_NOEUD][OUTPUT_NOEUD],
              double biais1[HIDDEN_NOEUD],
              double biais2[OUTPUT_NOEUD]) {
    for (int epoch = 0; epoch < PASSAGE; epoch++) {
        double loss = 0.0;

        for (int i = 0; i < 4; i++) {
            double x1 = input[i][0];
            double x2 = input[i][1];
            double target = output[i];

           
            double hidden[HIDDEN_NOEUD];
            for (int j = 0; j < HIDDEN_NOEUD; j++) {
                hidden[j] = sigmoid(x1 * poids1[0][j] + x2 * poids1[1][j] + biais1[j]);
            }
            double result = sigmoid(hidden[0] * poids2[0][0] + hidden[1] * poids2[1][0] + biais2[0]);

            double erreur = target - result;
            loss += erreur * erreur;

           
            double g_hidden[HIDDEN_NOEUD];
            backpropagation(hidden, biais2,poids2,  result, erreur, g_hidden);

          
            for (int j = 0; j < INPUT_NOEUD; j++) {
                double input_val = (j == 0 ? x1 : x2);
                for (int k = 0; k < HIDDEN_NOEUD; k++) {
                    poids1[j][k] += RATE * g_hidden[k] * input_val;
                }
            }
            for (int k = 0; k < HIDDEN_NOEUD; k++) {
                biais1[k] += RATE * g_hidden[k];
            }
        }

        if (epoch % 1000 == 0) {
            printf("Epoch %d, Loss = %f\n", epoch, loss);
        }
    }

    printf("\n=== Résultats finaux ===\n");
    for (int s = 0; s < 4; s++) {
        double x1 = input[s][0];
        double x2 = input[s][1];
        double hidden[HIDDEN_NOEUD];
        for (int i = 0; i < HIDDEN_NOEUD; i++) {
            hidden[i] = sigmoid(x1 * poids1[0][i] + x2 * poids1[1][i] + biais1[i]);
        }
        double result = sigmoid(hidden[0] * poids2[0][0] + hidden[1] * poids2[1][0] + biais2[0]);
        printf("%d XOR %d = %.3f\n", (int)x1, (int)x2, result);
    }
}
	



int main(void)
{
	srand(time(NULL));

	double input[4][2] = {
        {0,0}, {0,1}, {1,0}, {1,1}
    	};
    	double output[4] = {0, 0, 0, 1};
    
    	double poids_1[INPUT_NOEUD][HIDDEN_NOEUD];
    	double poids_2[HIDDEN_NOEUD][OUTPUT_NOEUD];
    	double biais_1[HIDDEN_NOEUD];
   	double biais_2[OUTPUT_NOEUD];

	initialiser(poids_1, poids_2, biais_1, biais_2);
	
	training(input, output, poids_1, poids_2, biais_1, biais_2);
	return 0;
}


	
