#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAXA_MUTACAO 5
#define MAX_GERACOES 50
#define MAX_GERPROC 100
#define NUM_POPULACAO 500

int NUM_ITENS, PESO_MAX;

int **populacao, **itens;

//funções
void criarPopulacao(void);
void mutacao(int **matris, int i);
void crossover(int **pop, int TAM);
void selecao(int **pop, int TAM);
void teste(int i);
void lerItens(void);
void imprimirPop(int tam);
int comparar(const void *c1, const void *c2);
void *prepararThread(void *agm);
int ** matriz(int tam);

int main (int argc, char *argv[]){
    int i, j, ger = 0, rc, ind, t; // variaveis de controle
    int tamanho; // Tamanho do intervalo
    struct agm *agm=NULL, *novo=NULL, *percorre=NULL;

    //variaveis MPI
    int id, tam, pro;

    MPI_Status status;

    MPI_Init(&argc, &argv); // inicializacao
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // Quem sou eu?
    MPI_Comm_size(MPI_COMM_WORLD, &tam); // Quantos são?


    tamanho = NUM_POPULACAO / (tam-1); // definindo o intervalo que cada thread manipulara

    if(id == 0){ // Aqui ficara o processo responsavel por enviar as partes para os outros processos, bem como fazer a selação completa

        srand(time(0));

        lerItens();

        criarPopulacao();

        printf("Numero de população: %i.\n", NUM_POPULACAO);
        printf("Numero de itens: %i.\n", NUM_ITENS);
        printf("Quantidade de Processos: %i.\n", tam);
        printf("Quantidade de grações principais: %i.\n", MAX_GERACOES);
        printf("Quantidade de gerações das threads: %i.\n", MAX_GERPROC);

        i = 0;
        for(pro=1;pro<tam;pro++){

            MPI_Send(&tamanho, 1, MPI_INT, pro, 0, MPI_COMM_WORLD);

            while(i<tamanho){
                MPI_Send(populacao[i], NUM_ITENS+2, MPI_INT, pro, 0, MPI_COMM_WORLD);
                i++;
            }

        }

        i=0;
        for(pro=1;pro<tam;pro++){

            while(i<tamanho){
                MPI_Recv(populacao[i], NUM_ITENS+2, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                i++;
            }
        }
        
        selecao(populacao, NUM_POPULACAO);

    }else{
        
        MPI_Recv(&tamanho, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        populacao = matriz(tamanho);

        for(i=0;i<tamanho;i++){
            MPI_Recv(populacao[i], NUM_ITENS+2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        }
        
        ger = 0;
        while(ger <= MAX_GERPROC){
            crossover(populacao, tamanho);
            selecao(populacao, tamanho);

            ger++;
        }

        for(i=0;i<tamanho;i++){
            MPI_Send(populacao[i], NUM_ITENS+2, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    
    // Se for um vetor de vetores da pra enviar

    //printf("\n-----------------------------\n");
    //printf("- Populacao final -\n");

    //imprimirPop(NUM_POPULACAO);

    printf("\n\n--- Melhor solução encontrada --\n");
    imprimirPop(1);
    printf("Melhor valor %d com %d de peso\n", populacao[0][NUM_ITENS + 1], populacao[0][NUM_ITENS]);

    MPI_Finalize();
}

// funções para o MPI

int ** matriz(int tam){
    int **m;

    m = (int **)malloc((NUM_ITENS*2) * sizeof(int*));
    for(int i=0;i<NUM_ITENS*2;i++){
        m[i] = (int *)malloc((NUM_ITENS+2) * sizeof(int));
    }

    return m;
}

// funções para o MPI

void criarPopulacao(void){
    int i, j, posini;
    int somaPeso = 0, somaValor = 0;
    
    populacao = (int **)malloc((NUM_POPULACAO*2) * sizeof(int*));
    for(i=0;i<NUM_POPULACAO*2;i++){
        populacao[i] = (int *)malloc((NUM_ITENS+2) * sizeof(int)); // peso = NUM_ITENS, valor = NUM_ITENS + 1
    }

    for(i=0;i<NUM_POPULACAO;i++){
        somaPeso = 0;
        somaValor = 0;
        posini = rand() % NUM_ITENS;
        j = posini;
        while(1) {
            populacao[i][j] = rand() % 2;
            if(somaPeso + itens[j][1] > PESO_MAX)
                populacao[i][j] = 0;
            somaPeso += populacao[i][j] * itens[j][1];
            somaValor += populacao[i][j] * itens[j][0];
            j = (j + 1) % NUM_ITENS;
            if(j == posini)
                break;
        }
        populacao[i][NUM_ITENS] = somaPeso;
        populacao[i][NUM_ITENS + 1] = somaValor;
    }
    
}

void lerItens(void){
    int i=0;

    scanf("%i %i", &NUM_ITENS, &PESO_MAX);

    itens = (int **)malloc(NUM_ITENS * sizeof(int*));
    for(i=0;i<NUM_ITENS;i++){
        itens[i] = (int *)malloc((2) * sizeof(int));
    }

    i=0;
    while(i < NUM_ITENS){
        scanf("%i %i", &itens[i][0], &itens[i][1]);
        i++;
    }
    
}

void crossover(int **pop, int TAM){
    int pai1, pai2, i, j=0;
    int sel[TAM]; // guarda os cromossomos que já cruzaram
    int div1, div2, posNew=TAM;

    for(i=0;i<TAM;i++){
        sel[i] = 0;
    }

    while(posNew < (TAM*2)){
        do {
            pai1 = (int)rand() % TAM;
        } while(sel[pai1]);
        sel[pai1] = 1;
        do {
            pai2 = (int)rand() % TAM;
        } while(pai2 == pai1 || sel[pai2]);
        sel[pai2] = 1;

        div1 = ((int)rand() % ((NUM_ITENS / 2) - 20)) + 10;
        div2 = ((int)rand() % ((NUM_ITENS / 2) - 20)) + (NUM_ITENS / 2) + 10;
        pop[posNew][NUM_ITENS] = 0;
        pop[posNew][NUM_ITENS + 1] = 0;
        pop[posNew + 1][NUM_ITENS] = 0;
        pop[posNew + 1][NUM_ITENS + 1] = 0;
        for(i=0;i<div1;i++){
            pop[posNew][i] = pop[pai1][i];
            pop[posNew][NUM_ITENS] += pop[pai1][i] * itens[i][1];
            pop[posNew][NUM_ITENS + 1] += pop[pai1][i] * itens[i][0];
            pop[posNew + 1][i] = pop[pai2][i];
            pop[posNew + 1][NUM_ITENS] += pop[pai2][i] * itens[i][1];
            pop[posNew + 1][NUM_ITENS + 1] += pop[pai2][i] * itens[i][0];
        }
        for(i=div1;i<div2;i++){
            pop[posNew][i] = populacao[pai2][i];
            pop[posNew][NUM_ITENS] += populacao[pai2][i] * itens[i][1];
            pop[posNew][NUM_ITENS + 1] += populacao[pai2][i] * itens[i][0];
            pop[posNew + 1][i] = populacao[pai1][i];
            pop[posNew + 1][NUM_ITENS] += populacao[pai1][i] * itens[i][1];
            pop[posNew + 1][NUM_ITENS + 1] += populacao[pai1][i] * itens[i][0];
        }
        for(i=div2;i<NUM_ITENS;i++){
            pop[posNew][i] = pop[pai1][i];
            pop[posNew][NUM_ITENS] += pop[pai1][i] * itens[i][1];
            pop[posNew][NUM_ITENS + 1] += pop[pai1][i] * itens[i][0];
            pop[posNew + 1][i] = pop[pai2][i];
            pop[posNew + 1][NUM_ITENS] += pop[pai2][i] * itens[i][1];
            pop[posNew + 1][NUM_ITENS + 1] += pop[pai2][i] * itens[i][0];
        }

        mutacao(pop, posNew);
        mutacao(pop, posNew + 1);

        posNew += 2;
    }

}

void selecao(int **pop, int TAM){
    int i, j;

    qsort(pop, TAM * 2, sizeof(int *), comparar);
    for(i = 0; i < TAM * 2; i++) {
        j = i + 1;
        while(j < TAM * 2 && pop[i][NUM_ITENS + 1] > 0 && pop[i][NUM_ITENS] == pop[j][NUM_ITENS] && pop[i][NUM_ITENS + 1] == pop[j][NUM_ITENS + 1])
            j++;
        while(++i != j)
            pop[i][NUM_ITENS + 1] = -1;
        i--;
    }
    qsort(pop, TAM * 2, sizeof(int *), comparar);
}

void mutacao(int **matris, int i){
    int j, tax, pos;

    tax = (int)rand()%100;
    
    if(tax < TAXA_MUTACAO){
        do {
            pos = (int) rand() % NUM_ITENS;
        } while(matris[i][pos] == 1);
        matris[i][pos] = 1;
        matris[i][NUM_ITENS] += itens[pos][1];
        matris[i][NUM_ITENS + 1] += itens[pos][0];
        /*pos = (int)rand() % NUM_ITENS;
        
        if(populacao[i][pos] == 1){
            populacao[i][pos] = 0;
            populacao[i][NUM_ITENS] -= itens[pos][1];
        }else{
            populacao[i][pos] = 1;
            populacao[i][NUM_ITENS] += itens[pos][1];
        }*/
    }

    if(matris[i][NUM_ITENS] > PESO_MAX)
        matris[i][NUM_ITENS + 1] = -1;

}

void imprimirPop(int tam) {
    int i, j;

    for(i=0;i<tam;i++){
        for(j=0;j<NUM_ITENS;j++){
            printf("%d", populacao[i][j]);
            if(j % 10 == 0)
                printf(" ");
        }
        printf(" %d %d\n", populacao[i][NUM_ITENS], populacao[i][NUM_ITENS + 1]);
    }
}

int comparar(const void *c1, const void *c2) {
    int *i1 = *(int **)c1;
    int *i2 = *(int **)c2;

    if(i2[NUM_ITENS + 1] == i1[NUM_ITENS + 1])
        return i1[NUM_ITENS] - i2[NUM_ITENS];
    return i2[NUM_ITENS + 1] - i1[NUM_ITENS + 1];
}
