#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define K 1000000      // valor maximo do contador
#define nThreads 6    // numero de threads que quiser

pthread_mutex_t mutexas = PTHREAD_MUTEX_INITIALIZER; //inicializando mutexas (piada interna)

int contador = 0,flag = 0; //declarando variaveis globais (compartilhadas entre as threads contadoras e ,portanto, regiao critica)

void *inc (void *ThreadsID)
{
    int ID = (*(int *)ThreadsID); // [*(int*)] = valor inteiro do endereço apontado por ThreadsID

    while(contador < K)       // acessando regiao critica
    {
        pthread_mutex_lock(&mutexas); //se chega aq é pq verificou que contador é <1000000,logo podemos tentar travar o mutex

        if(contador < K) //recheck por precauçao(por que o valor do contador pode estar errado para a thread atual devido ao check do while nao ser atomico). E nesta linha o valor de contador ja foi escrito com seu valor correto por outra thread que ja usou a regiao critica.
        {  
            printf("A thread %d esta incrementando (CONTADOR ATUAL: %d).\n", ID, contador); 
            contador++; 
        }
        if(contador == K && !flag) // se o contador for igual a 100000 e ninguem setou o valor de flag ainda, é pq a thread atual é a thread que queremos
        {   
            printf("A Thread %d foi a ultima a incrementar!\n", ID);
            flag = 1;
        }
        pthread_mutex_unlock(&mutexas); // se chegar aq, a thread ja usou oq tinha que usar da regiao critica e portanto podemos liberar mutex
    }
    pthread_exit(NULL); // se o contador ja chegou em K, a thread acaba
}

int main ()
{
    pthread_t threads[nThreads]; //declarando threads
    int* ThreadsID[nThreads];   // vetor de ponteiro para inteiro para guardar IDs (visto na aula sobre threads)
    int i, rc;

    for(i = 0; i < nThreads; i++)
    {   
        ThreadsID[i] = (int*) malloc(sizeof(int));//para nao mandar o id errado para pthread_create(i é uma variavel local de main), o array de IDs fica na heap(alocado dinamicamente), que é compartilhado por todas as threads 
        *ThreadsID[i] = i; 

        rc = pthread_create(&threads[i], NULL, inc, (void*) ThreadsID[i]);
        if(rc)
            exit(1); //caso de algo errado na criaçao da thread i, apenas finalizamos o processo
    }

    for(i = 0; i < nThreads; i++)         //para que a thread main espere o termino das outras threads, usamos pthread_join
        pthread_join(threads[i], NULL);  

    printf("Valor final: %d\n", contador); // printando valor final do contador, caso de certo printará 1000000

    //desalocando memoria
    for(int k = 0;k<nThreads;k++)
        free(ThreadsID[k]);
       
    pthread_exit(NULL); //boa pratica para evitar o termino do processo inteiro caso a main termine antes das outras threads
}