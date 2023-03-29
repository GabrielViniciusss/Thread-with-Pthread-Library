#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define _XOPEN_SOURCE 600 // define magico para barriers funcionarem
#define I 2               // existem 2 icognitas apenas para o exemplo que escolhemos
#define P 10              // definindo a precisao como 10

double A[I][I] = {{2, 1}, {5, 7}};  // matrizes do sistema linear do enunciado
double b[I] = {11, 13};

double solution[I] = {1,1};     //Vetor da solução parcial(x1 e x2), que valem 1 inicialmente devido ao comando da questão.
double nextSolution[I];        //vetor auxiliar que guardará as proximas soluçoes parciais para iteraçoes futuras do algoritmo de jacobi

//OBS: ambos vetores sao compartilhados pelas threads pois sao globais, porem as threads escrevem em posiçoes de memorias diferentes,logo n e preciso utilizar mutexes nesse caso

int N; // variavel global que representa o numero de threads dita pelo usuario, na minha maquina N = 4;
int K; // k vai ser o numero de icognitas associado a cada thread
pthread_barrier_t barrier; //barrier global que server pra sicronizar as threads

void* jacobi(void* ThreadID)
{
    int ID = *(int*) ThreadID;  //parametrizando corretamente o inicio e o fim de cada thread dentro das iteraçoes do algoritmo
    int start = ID * K;         //cada thread vai trabalhar em |start - end| linhas da matriz do SL
    int end = start + K;
    if (ID == N - 1)            //caso I nao seja divisivel por N, a ultima thread ficará responsavel pelas icognitas restantes
        end = I;
    
    for (int k = 0; k < P; k++)  //implementaçao de fato do algoritmo de jacobi
    {
        for (int i = start; i < end; i++) //percorre linha(cada thread terá seu intervalo)
        {
            double sum = 0;
            for (int j = 0; j < I; j++) //percorre variavel
            {
                //só mando pro outro lado da igualdade se o numero da equaçao for diferente do numero da variavel
                if (i != j)       
                    sum += A[i][j] * solution[j];
                //oq queremos no fim é isolar xi, de forma que xiC + yiK + ziU = L vire xi = (L - yiK -ziU)/C. Sum = yiK+ziU(Neste Ponto) 
            }
            nextSolution[i] = (b[i] - sum) / A[i][i]; //isolando xi e atualizando vetor da prox Soluçao parcial,pois b[i] é o L e A[i][i] é o C do exemplo acima.
        }
        pthread_barrier_wait(&barrier); //sicronizando as threads,somente depois que todas as threads já escreveram em nextSolution atualizamos solution

        if(ID == 0) // fazendo a thread 0 printar o vetor de soluçao parcial e atualiza-lo a cada iteraçao
        {
            for(int l = 0; l < I; l++)      
                solution[l] = nextSolution[l];
            printf("%d° iteracao: x[0] = %f x[1] = %f\n",k+1,solution[0],solution[1]); 
        }
        pthread_barrier_wait(&barrier); //sicronizando as threads(as threads vao esperar aq ate que a thread 0 atualize e printe o vetor de soluçao parcial) a cada iteraçao
    }
}

int main()
{
    printf("Diga o valor de N(número de cores no seu processador)\n");
    scanf("%d",&N);

    pthread_t threads[N]; //declarando threads
    int* ThreadsID[N];   // mesmo esquema da Q1

    K = I/N;    //atualizando o numero de icognitas por thread

    pthread_barrier_init(&barrier, NULL, N);  //inicializando barrier

    int i,rc;
    for(i = 0; i < N; i++)
    {
        ThreadsID[i] = (int*) malloc(sizeof(int)); // mesmo esquema da Q1
        *ThreadsID[i] = i; 
        rc = pthread_create(&threads[i], NULL, jacobi, (void*) ThreadsID[i]);
        if(rc)
            exit(1);
    }

    for (i = 0; i < N; i++) 
        pthread_join(threads[i], NULL); // main espera outras threads terminarem
    
    printf("Solução Parcial dps de P iteraçoes:\n"); // printa solução atual(dps de P iteraçoes)
    for (i = 0; i < I; i++) 
        printf("x[%d] = %f\n", i+1, solution[i]);
    
    //desalocando memoria
    pthread_barrier_destroy(&barrier);

    for(int k = 0;k<N;k++)
        free(ThreadsID[k]);

    pthread_exit(NULL);
}
//a execuçao do algoritmo de jacobi para um sistema linear pode ser conferida no site: https://vtp.ifsp.edu.br/nev/Sistema-jacobi/sistemajacobi.html


