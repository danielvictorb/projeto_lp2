#ifndef SINCRONIZACAO_H
#define SINCRONIZACAO_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/* Parâmetros derivados da matrícula M = 9678 */
#define NUM_COZINHEIROS 4
#define NUM_FOGOES      2
#define NUM_MESAS       6
#define NUM_GARCONS     2
#define TEMPO_PREPARO   1

/* Parâmetros da simulação */
#define NUM_PEDIDOS_POR_MESA 3
#define TAM_FILA            10

/* Representa um pedido feito por uma mesa */
typedef struct {
    int id_pedido;
    int id_mesa;
    struct timespec hora_criacao;
} Pedido;

/* Fila circular de pedidos (produtor-consumidor) */
typedef struct {
    Pedido pedidos[TAM_FILA];
    int inicio;
    int fim;
    int quantidade;
    pthread_mutex_t mutex_fila;
    sem_t *sem_vagas;
    sem_t *sem_itens;
} FilaPedidos;

/* Estatísticas globais da simulação */
typedef struct {
    int total_pedidos;
    double tempo_total_espera;
    pthread_mutex_t mutex_estatisticas;
    struct timespec inicio_execucao;
    struct timespec fim_execucao;
} Estatisticas;

/* Dados compartilhados do sistema */
typedef struct {
    FilaPedidos fila;
    Estatisticas stats;
    sem_t *sem_fogoes;
    pthread_mutex_t mutex_log;
} SistemaRestaurante;

/* Dados passados para cada thread garçom */
typedef struct {
    int id_garcom;
    int mesa_inicio;
    int mesa_fim;
    SistemaRestaurante *sistema;
} DadosGarcom;

/* Dados passados para cada thread cozinheiro */
typedef struct {
    int id_cozinheiro;
    SistemaRestaurante *sistema;
} DadosCozinheiro;

/* Funções de inicialização e destruição */
void inicializar_fila(FilaPedidos *fila);
void destruir_fila(FilaPedidos *fila);
void inicializar_estatisticas(Estatisticas *stats);
void destruir_estatisticas(Estatisticas *stats);
void inicializar_sistema(SistemaRestaurante *sistema);
void destruir_sistema(SistemaRestaurante *sistema);

/* Operações na fila de pedidos */
void enfileirar_pedido(FilaPedidos *fila, Pedido pedido);
Pedido desenfileirar_pedido(FilaPedidos *fila);

/* Registro de log com timestamp */
void registrar_log(SistemaRestaurante *sistema, const char *formato, ...);

/* Estatísticas */
void atualizar_estatisticas(Estatisticas *stats, double tempo_espera);
void imprimir_estatisticas(Estatisticas *stats);

/* Utilidade para calcular diferença de tempo em segundos */
double calcular_tempo_decorrido(struct timespec inicio, struct timespec fim);

#endif
