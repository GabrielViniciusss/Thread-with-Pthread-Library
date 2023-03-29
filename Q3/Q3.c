#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 3 // número de threads que quiser

typedef struct //struct do pixel (que é uma linha da matriz)
{   
    int r;
    int g;
    int b;
    int gray;
} Pixel;

int i, j;         //variaveis que vao receber as dimensoes da matriz,a matriz tera i*j linhas.
Pixel** bitmap;  // variavel que vai representar a matriz

/* matriz usada:    P3
                    3 2           //6 linhas que representam pixeis
                    255           
                    255 0 0      // onde cada linha apartir daqui representa um pixel
                    0 255 0
                    0   0 255
                    255 255 0
                    255 255 255
                    0   0   0

    matriz resultado esperada:  P3
                                3 2
                                255
                                76 76 76            
                                150 150 150
                                28 28 28
                                226 226 226        
                                255 255 255   // o resultado bateu, Graças a Deus.
                                0 0 0
*/

void *convert(void *ThreadsID)
{
    int ID = (*(int *)ThreadsID);          //ID da thread atual

    for(int k = ID; k < i; k += NUM_THREADS) //fazendo com que cada grupo de j pixeis seja operado por apenas por uma thread
        for(int u = 0; u < j; u++)
            bitmap[k][u].gray = bitmap[k][u].r * 0.3 + bitmap[k][u].g * 0.59 + bitmap[k][u].b * 0.11; // é passado o valor inteiro
    
    /*Preferi fazer cada thread operar um grupo de j pixeis porque assim podemos diminuir a quantidade de threads a serem criadas 
      e mantemos o código otimizado, uma vez que a criaçao de threads é bastante custosa*/
    
    //bitmap é da seguinte forma:
    //bitmap[i] é um ponteiro para um ponteiro para struct pixel que é alocado para armazenar j structs pixel
    //Dessa forma, bitmap[k][u].gray vai representar o valor em tons de cinza do pixel representado pela linha k*u da matriz input
}

int main()
{
    char format[6];
    int maxValue;
  
    FILE *input = fopen("input.ppm", "r");        // abrindo arquivo para leitura
    
    fscanf(input, " %s", format);                // vendo se o formato condiz com o esperado
    if(strcmp("P3", format) != 0)
    {
        printf("O arquivo nao possui o formato esperado");                                           
        exit(1);
    }

    fscanf(input,"%d %d", &i, &j); // pegando as dimensoes da matriz para alocação
    //printf("%d %d\n",i,j);
    bitmap = (Pixel**) malloc(i* sizeof(Pixel*)); //i ponteiros para ponteiros do tipo pixel
    for(int k=0;k<i;k++)
        bitmap[k] =(Pixel*) malloc(j*sizeof(Pixel)); //cada bitmap[i] vai apontar pra um vetor(ponteiro) de pixel que armazena j pixeis, ou seja, j linhas da matriz
    
    fscanf(input, "%d", &maxValue); //pegando o valor maximo a fim comparaçao e tambem para que o ponteiro do arquivo passe a apontar para a proxima linha

    for(int x = 0; x < i; x++)
    {
        for(int y = 0; y < j; y++)
        {
            //cada bitmap[i][j] é do tipo Pixel que é representado por uma linha da matriz,a qual lemos a cada iteraçao de y

            fscanf(input, "%d %d %d", &bitmap[x][y].r, &bitmap[x][y].g, &bitmap[x][y].b); 

            if(bitmap[x][y].r > maxValue)
            {
                printf("Valor R do pixel da linha [%d] passou do valor 255.Ele sera redefinido para o valor maximo\n", x*y);
                bitmap[x][y].r = maxValue; //se for maior que o valor maximo, colocamos o valor maximo no lugar
            }
            if(bitmap[x][y].g > maxValue)
            {
                printf("Valor G do pixel da linha [%d] passou do valor 255.Ele sera redefinido para o valor maximo\n", x*y);
                bitmap[x][y].g = maxValue; //se for maior que o valor maximo, colocamos o valor maximo no lugar
            }
            if(bitmap[x][y].b > maxValue)
            {
                printf("Valor B do pixel da linha [%d] passou do valor 255.Ele sera redefinido para o valor maximo\n", x*y);
                bitmap[x][y].b = maxValue; //se for maior que o valor maximo, colocamos o valor maximo no lugar
            }
            //printf("%d %d %d\n",bitmap[x][y].r, bitmap[x][y].g, bitmap[x][y].b);
        }
    }

    pthread_t threads[NUM_THREADS]; //declarando as threads
    int* ThreadsID[NUM_THREADS];   // mesmo esquema da Q1
    int v, rc;

    for(v = 0; v < NUM_THREADS; v++) //criando as threads de fato
    {
        ThreadsID[v] = (int*) malloc(sizeof(int)); // mesmo esquema da Q1
        *ThreadsID[v] = v; 

        rc = pthread_create(&threads[v], NULL, convert, (void*) ThreadsID[v]);
        if(rc)
            exit(1);
    }

    for(v = 0; v < NUM_THREADS;v++)
        pthread_join(threads[v], NULL); //main espera outras threads terminarem

    FILE *output = fopen("output.ppm", "w");  //criando arquivo output

    //repassando dados relacionados a formataçao do arquivo
    fprintf(output,"%s\n",format); 
    fprintf(output,"%d %d\n",i,j);
    fprintf(output,"%d\n",maxValue);

    //passando as linhas que representam os pixels da nova imagem convertida para tons de cinza para o arquivo output
    for(int a = 0; a < i; a++) 
        for(int b = 0;b<j; b++)
            fprintf(output, "%d %d %d\n", bitmap[a][b].gray, bitmap[a][b].gray, bitmap[a][b].gray);
        
    fclose(input); //'fechando' arquivos
    fclose(output);

    //desalocando memoria
    for(int k =0;k<i;k++)
        free(bitmap[k]);
    free(bitmap);

    for(int k = 0;k<NUM_THREADS;k++)
        free(ThreadsID[k]);

    pthread_exit(NULL);
}