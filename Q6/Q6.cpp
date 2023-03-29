#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <pthread.h>

//fazendo em C++ pelas facilidades oferecidas kkk. Para rodar basta trocar 'gcc' por 'g++' no comando de compilaçao padrao linux
using namespace std;

#define NUM_THREADS 4 //número maximo de threads de execução(workers),que é o numeros de cores da maquina.Na minha maquina N=4;
#define MAX_BUFFER 200 //definindo constante do tamanho do buffer, para nao ocupar tanta memoria e por questoes de praticidade

struct Args
{
    int x, y;  // nossa funexec vai operar numeros inteiros
};

struct Task
{
    void* (*funexec)(void*); //ponteiro para função que será executada e tera como parametro um void* e retornara um void*
    Args args; //argumentos da função que a task vai operar
    int ID;
};

pthread_t threads[NUM_THREADS]; //declarando threads workers (threads que vao realizar a execução das task)
int threadsInExecution = 0; //variavel global que vai dizer o numero de threads que tao executando

queue <Task> buffer;      //buffer de tarefas pendentes
int results[MAX_BUFFER]; //Buffer de resultados das tarefas executadas, fazendo como vetor estatico para facilitar

int finishedTasks = 0;   //variavel que conta quantas tasks ja foram finalizadas
int taskID = 0;         // IDs das tasks(sequencial)

pthread_mutex_t mutexas = PTHREAD_MUTEX_INITIALIZER;    //Mutex para controle de acesso ao buffer de tarefas pendentes
pthread_mutex_t mutexas2 = PTHREAD_MUTEX_INITIALIZER;  //Mutex para controle de acesso ao buffer de resultados
pthread_mutex_t mutexas3 = PTHREAD_MUTEX_INITIALIZER; //Mutex para controle de acesso de algumas variaveis globais(finishedTask e threadsInExecution)

pthread_cond_t pendingTasks = PTHREAD_COND_INITIALIZER;     //Variável de condição para sinalizar que há tarefas pendentes no buffer
pthread_cond_t resultsCond = PTHREAD_COND_INITIALIZER;     //Variável de condição para sinalizar que alguma thread atualizou o vetor results
pthread_cond_t despachanteCond = PTHREAD_COND_INITIALIZER;//Variável de condição para sinalizar que a thread Despachante deve dormir ou acordar
pthread_cond_t bufferFull = PTHREAD_COND_INITIALIZER;    //Variável de condição para sinalizar que o buffer de tarefas ta cheio

void agendaExecucao(void* (*funexec)(void*), Args args, int* ID) //funçao que coloca tarefa requisitada no buffer de tarefas pendentes e retorna seu ID
{
    pthread_mutex_lock(&mutexas);  //acessando regiao critica(buffer de tarefas pendentes compartilhado entre as threads).Portanto devemos usar mutexes

    while (buffer.size() == MAX_BUFFER) //enquanto o buffer tiver cheio, a thread que agenda execucao vai dormir(que no caso é a main)
        pthread_cond_wait(&bufferFull, &mutexas);

    Task* task = (Task*) malloc(sizeof(Task));  //preenchendo a struct da tarefa atual com seus devidos parametros
    task->funexec = funexec;
    task->args = args;
    task->ID = taskID;        //id da task é sequencial (task1 tera id 0, task2 tera id 1)

    *ID = task->ID;           //atualizando ID da task na main (funciona como um return)
    taskID++;

    //printf("ARG da task de id %d: %d %d\n",task->ID,task->args.x,task->args.y);
    
    buffer.push(*task);                 //se o buffer nao tiver cheio, coloca-se uma nova tarefa no buffer
  
    pthread_cond_signal(&pendingTasks); //depois que uma thread conseguiu acessar o buffer e inserir nova tarefa, ela acorda a thread despachante que pode estar dormindo
    
    pthread_mutex_unlock(&mutexas);             
}

void* taskExecution(void* arg)      //funçao que executa a tarefa, vai ser acessada pelas threads de execuçao
{   
    pthread_mutex_lock(&mutexas2); //o buffer de resultados é compartilhado entre as threads, portanto devemos usar mutexes para mexer nele.

    void* result;                                    
    Task* task = (Task*) arg;                     //arg é um ponteiro para Task castado para void*
    
    result = task->funexec((void*)&task->args);  // result vai apontar pro resultado de funexec(&task->args), que será o resultado de funexec com os parametros passados
                                                //  fazemos isso pq funexec é uma funçao que retorna void*

    results[task->ID] = *((int*)result);       //o resultado da operaçao vai ficar salvo em *((int*)result)
    finishedTasks++;

    //printf("resultado da task de id %d = %d\n",task->ID,*((int*)result));

    pthread_cond_signal(&resultsCond); //acordando thread que dorme no buffer de resultados ao tentar acessar um resultado que ainda nao foi escrito

    pthread_mutex_unlock(&mutexas2);
    
    pthread_mutex_lock(&mutexas3);                 //acessando variavel global critica                             
    threadsInExecution--;                         
    pthread_mutex_unlock(&mutexas3);              //se uma thread de execuçao chegou aqui, ela ja fez oq tinha pra fazer
    
    pthread_cond_signal(&despachanteCond);      //mandando sinal para thread despachante que pode estar dormindo caso threadsInExecution fosse 4

    pthread_exit(NULL);
} 

void* despachante(void*) //Função que despacha as tarefas para as threads que irao executa-las,a qual é realizada pela thread despachante
{
    int k = 0;             // variavel interna da thread despachante
    while(true)            //o despacho é feito infinitamente, como dito no enunciado
    {
        pthread_mutex_lock(&mutexas);                    //lock no buffer de tarefas pendentes

        while(buffer.empty())                           //se nao tiver tarefas pendentes, colocamos a thread despachante para dormir no mutex1
            pthread_cond_wait(&pendingTasks, &mutexas);

        Task* task = (Task*) malloc(sizeof(Task));          //caso tenha tarefa no buffer,pegamos ela(FIFO)
        task = &buffer.front();                             
        buffer.pop();                                       

        pthread_cond_signal(&bufferFull);         //sinal para thread que agenda novas tarefas acordar caso o buffer estivesse cheio

        pthread_mutex_unlock(&mutexas); 

        pthread_mutex_lock(&mutexas3);                 //acessando regiao critica de threadsInExecution
                         
        while(threadsInExecution == NUM_THREADS)            //se 4 threads ja estao em execução a despachante dorme
            pthread_cond_wait(&despachanteCond, &mutexas3);

        //caso contrario podemos despachar uma thread  
        k = k%NUM_THREADS; //só existe threads[0] ate threads[NUM_THREADS-1]                                                 
        pthread_create(&threads[k], NULL, taskExecution, (void*) task);//despacho de uma thread que vai executar a tarefa retirada do buffer
        k++;
        threadsInExecution++;  // aumentando o numero de threads em execuçao

        pthread_mutex_unlock(&mutexas3); 
    }
    pthread_exit(NULL);
}

int pegarResultadoExecucao(int task_id)
{
    pthread_mutex_lock(&mutexas2);                      //para acessar buffer de resultados
    
    while(results[task_id] == -1)                      //se results[id] da tarefa é -1,quer dizer que a tarefa ainda nao foi concluida
        pthread_cond_wait(&resultsCond, &mutexas2);   // portanto colocamos a thread(no caso é main) pra dormir
    
    int result = results[task_id];                   //caso contrario, a task ja finalizou e podemos pegar seu resultado

    pthread_mutex_unlock(&mutexas2);
    
    return result;
}

void* sum(void* arg)                  // operações que podem ser realizadas por funexec
{
    Args* args = (Args*) arg;
    int a = args->x;
    int b = args->y;
    int* result = (int*) malloc(sizeof(int));
    *result = a + b;
    return (void*) result; //castando para void pq o retorno de funexec é void
}
void* mult(void* arg) 
{
    Args* args = (Args*) arg;
    int a = args->x;
    int b = args->y;
    int* result = (int*) malloc(sizeof(int));
    *result = a * b;
    return (void*) result;
}

void* sub(void* arg) {
    Args* args = (Args*) arg;
    int a = args->x;
    int b = args->y;
    int* result = (int*) malloc(sizeof(int));
    *result = a - b;
    return (void*) result;
}

int main()
{
    int task1ID,task2ID,task3ID,task4ID,task5ID,task6ID,task7ID; //declarando variaveis que vao receber os IDs das tasks do user

    for(int i = 0;i<MAX_BUFFER;i++)                 //-1 indica que o resultado ainda nao está pronto
        results[i] = -1;

    pthread_cond_init(&pendingTasks, NULL); // inicializando variaveis de condiçao
    pthread_cond_init(&resultsCond, NULL);
    pthread_cond_init(&despachanteCond, NULL);
    pthread_cond_init(&bufferFull, NULL);

    pthread_t ThreadDespachante; //declarando thread despachante
    pthread_create(&ThreadDespachante, NULL, despachante, NULL); //criando thread despachante     

    agendaExecucao(sum,{5, 3},&task1ID);                 // Agendando tarefas para serem executadas
    agendaExecucao(mult,{4, 5},&task2ID);
    agendaExecucao(sub,{10, 5},&task3ID);
    agendaExecucao(sum,{16, 5},&task4ID);
    agendaExecucao(mult,{16, 5},&task5ID);
    agendaExecucao(mult,{10, 5},&task6ID);
    agendaExecucao(mult,{10, 10},&task7ID);

    //printf("teste\n");
    int result1 = pegarResultadoExecucao(task1ID);            // Esperando os resultados das tarefas
    int result2 = pegarResultadoExecucao(task2ID);
    int result3 = pegarResultadoExecucao(task3ID);
    int result4 = pegarResultadoExecucao(task4ID);
    int result5 = pegarResultadoExecucao(task5ID);
    int result6 = pegarResultadoExecucao(task6ID);
    int result7 = pegarResultadoExecucao(task7ID);

    printf("Resultado da task de id %d: %d\n",task1ID,result1);             // Imprimindo os resultados
    printf("Resultado da task de id %d: %d\n",task2ID,result2);
    printf("Resultado da task de id %d: %d\n",task3ID,result3);
    printf("Resultado da task de id %d: %d\n",task4ID,result4);
    printf("Resultado da task de id %d: %d\n",task5ID,result5);
    printf("Resultado da task de id %d: %d\n",task6ID,result6);
    printf("Resultado da task de id %d: %d\n",task7ID,result7);

    //deu certo, graças a Deus.
    pthread_exit(NULL);
}
