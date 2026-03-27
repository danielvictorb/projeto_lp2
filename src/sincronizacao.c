#include "sincronizacao.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Calcula a diferença em segundos entre dois timestamps */
double calcular_tempo_decorrido(struct timespec inicio, struct timespec fim) {
    return (fim.tv_sec - inicio.tv_sec) +
           (fim.tv_nsec - inicio.tv_nsec) / 1e9;
}

/* ========== Fila de Pedidos ========== */

/* Inicializa a fila de pedidos com mutex e semáforos */
void inicializar_fila(FilaPedidos *fila) {
    fila->inicio = 0;
    fila->fim = 0;
    fila->quantidade = 0;

    if (pthread_mutex_init(&fila->mutex_fila, NULL) != 0) {
        perror("Erro ao inicializar mutex_fila");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fila->sem_vagas, 0, TAM_FILA) != 0) {
        perror("Erro ao inicializar sem_vagas");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&fila->sem_itens, 0, 0) != 0) {
        perror("Erro ao inicializar sem_itens");
        exit(EXIT_FAILURE);
    }
}

/* Destroi os recursos de sincronização da fila */
void destruir_fila(FilaPedidos *fila) {
    pthread_mutex_destroy(&fila->mutex_fila);
    sem_destroy(&fila->sem_vagas);
    sem_destroy(&fila->sem_itens);
}

/* Insere um pedido na fila (chamado pelos garçons/produtor) */
void enfileirar_pedido(FilaPedidos *fila, Pedido pedido) {
    /* Aguarda uma vaga disponível na fila */
    sem_wait(&fila->sem_vagas);

    /* Seção crítica: insere o pedido no buffer circular */
    pthread_mutex_lock(&fila->mutex_fila);
    fila->pedidos[fila->fim] = pedido;
    fila->fim = (fila->fim + 1) % TAM_FILA;
    fila->quantidade++;
    pthread_mutex_unlock(&fila->mutex_fila);

    /* Sinaliza que há um novo item disponível */
    sem_post(&fila->sem_itens);
}

/* Remove e retorna um pedido da fila (chamado pelos cozinheiros/consumidor) */
Pedido desenfileirar_pedido(FilaPedidos *fila) {
    Pedido pedido;

    /* Aguarda um item disponível na fila */
    sem_wait(&fila->sem_itens);

    /* Seção crítica: remove o pedido do buffer circular */
    pthread_mutex_lock(&fila->mutex_fila);
    pedido = fila->pedidos[fila->inicio];
    fila->inicio = (fila->inicio + 1) % TAM_FILA;
    fila->quantidade--;
    pthread_mutex_unlock(&fila->mutex_fila);

    /* Sinaliza que há uma nova vaga disponível */
    sem_post(&fila->sem_vagas);

    return pedido;
}

/* ========== Estatísticas ========== */

/* Inicializa as estatísticas com mutex */
void inicializar_estatisticas(Estatisticas *stats) {
    stats->total_pedidos = 0;
    stats->tempo_total_espera = 0.0;

    if (pthread_mutex_init(&stats->mutex_estatisticas, NULL) != 0) {
        perror("Erro ao inicializar mutex_estatisticas");
        exit(EXIT_FAILURE);
    }
}

/* Destroi os recursos de sincronização das estatísticas */
void destruir_estatisticas(Estatisticas *stats) {
    pthread_mutex_destroy(&stats->mutex_estatisticas);
}

/* Atualiza as estatísticas globais (protegido por mutex) */
void atualizar_estatisticas(Estatisticas *stats, double tempo_espera) {
    pthread_mutex_lock(&stats->mutex_estatisticas);
    stats->total_pedidos++;
    stats->tempo_total_espera += tempo_espera;
    pthread_mutex_unlock(&stats->mutex_estatisticas);
}

/* Imprime as estatísticas finais da simulação */
void imprimir_estatisticas(Estatisticas *stats) {
    double tempo_total = calcular_tempo_decorrido(stats->inicio_execucao,
                                                   stats->fim_execucao);
    double ops_por_segundo = 0.0;
    double tempo_medio_espera = 0.0;

    if (tempo_total > 0.0)
        ops_por_segundo = stats->total_pedidos / tempo_total;
    if (stats->total_pedidos > 0)
        tempo_medio_espera = stats->tempo_total_espera / stats->total_pedidos;

    printf("\n========================================\n");
    printf("       ESTATÍSTICAS DA SIMULAÇÃO\n");
    printf("========================================\n");
    printf("Total de pedidos atendidos: %d\n", stats->total_pedidos);
    printf("Tempo total de execução:   %.3f segundos\n", tempo_total);
    printf("Operações por segundo:     %.2f\n", ops_por_segundo);
    printf("Tempo médio de espera:     %.3f segundos\n", tempo_medio_espera);
    printf("========================================\n");
}

/* ========== Sistema Restaurante ========== */

/* Inicializa todos os recursos do sistema */
void inicializar_sistema(SistemaRestaurante *sistema) {
    inicializar_fila(&sistema->fila);
    inicializar_estatisticas(&sistema->stats);

    if (sem_init(&sistema->sem_fogoes, 0, NUM_FOGOES) != 0) {
        perror("Erro ao inicializar sem_fogoes");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&sistema->mutex_log, NULL) != 0) {
        perror("Erro ao inicializar mutex_log");
        exit(EXIT_FAILURE);
    }
}

/* Destroi todos os recursos do sistema */
void destruir_sistema(SistemaRestaurante *sistema) {
    destruir_fila(&sistema->fila);
    destruir_estatisticas(&sistema->stats);
    sem_destroy(&sistema->sem_fogoes);
    pthread_mutex_destroy(&sistema->mutex_log);
}

/* ========== Log ========== */

/* Registra uma mensagem de log com timestamp (protegido por mutex) */
void registrar_log(SistemaRestaurante *sistema, const char *formato, ...) {
    struct timespec agora;
    clock_gettime(CLOCK_REALTIME, &agora);
    double tempo = calcular_tempo_decorrido(sistema->stats.inicio_execucao, agora);

    pthread_mutex_lock(&sistema->mutex_log);

    printf("[%7.3fs] ", tempo);

    va_list args;
    va_start(args, formato);
    vprintf(formato, args);
    va_end(args);

    printf("\n");
    fflush(stdout);

    pthread_mutex_unlock(&sistema->mutex_log);
}
