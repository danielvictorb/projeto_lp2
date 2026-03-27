# Restaurante Concorrente

Simulação de um restaurante concorrente utilizando threads POSIX (pthreads) em linguagem C, desenvolvida como projeto da disciplina de Linguagem de Programação II.

---

## Identificação

- **Nome completo:** Daniel Victor Carneiro Brandão da Costa
- **Matrícula:** 20230089678
- **Cenário escolhido:** Restaurante Concorrente

---

## Parâmetros Calculados

Utilizando **M = 9678** (últimos 4 dígitos da matrícula):

| Parâmetro | Fórmula | Cálculo | Valor |
|---|---|---|---|
| `NUM_COZINHEIROS` | (M % 4) + 2 | (9678 % 4) + 2 = 2 + 2 | **4** |
| `NUM_FOGOES` | (M % 3) + 2 | (9678 % 3) + 2 = 0 + 2 | **2** |
| `NUM_MESAS` | ((M / 10) % 5) + 4 | ((967) % 5) + 4 = 2 + 4 | **6** |
| `NUM_GARCONS` | ((M / 100) % 3) + 2 | ((96) % 3) + 2 = 0 + 2 | **2** |
| `TEMPO_PREPARO` | ((M / 100) % 3) + 1 | ((96) % 3) + 1 = 0 + 1 | **1 segundo** |

Parâmetros adicionais da simulação:
- `NUM_PEDIDOS_POR_MESA = 3` (cada mesa gera 3 pedidos)
- `TAM_FILA = 10` (capacidade do buffer circular)
- **Total de pedidos:** 6 × 3 = **18 pedidos**

---

## Decisões de Projeto

### Modelo da Simulação

O restaurante funciona como um sistema produtor-consumidor:

1. **Mesas** geram pedidos que são coletados pelos garçons.
2. **Garçons** (produtores) coletam pedidos das mesas atribuídas e os inserem em uma fila compartilhada.
3. **Cozinheiros** (consumidores) retiram pedidos da fila, adquirem um fogão e preparam o prato.
4. **Fogões** são um recurso limitado compartilhado entre os cozinheiros.

Cada garçom é responsável por 3 mesas (garçom 0 atende mesas 0-2, garçom 1 atende mesas 3-5). Os cozinheiros competem tanto pela fila de pedidos quanto pelos fogões disponíveis.

### Mecanismos de Sincronização

Foram utilizados **3 mecanismos distintos de sincronização**:

#### 1. Semáforo Contador para Fogões (`sem_t sem_fogoes`)
- **Objetivo:** Limitar o acesso concorrente aos fogões (recurso finito).
- **Funcionamento:** Inicializado com valor `NUM_FOGOES = 2`. Cada cozinheiro executa `sem_wait` antes de cozinhar e `sem_post` após liberar o fogão.
- **Justificativa:** O semáforo contador é a escolha natural para controlar o acesso a N recursos idênticos. Permite que até 2 cozinheiros cozinhem simultaneamente, enquanto os demais aguardam.

#### 2. Semáforos para Produtor-Consumidor (`sem_t sem_vagas` + `sem_t sem_itens`)
- **Objetivo:** Coordenar o acesso à fila de pedidos entre garçons (produtores) e cozinheiros (consumidores).
- **Funcionamento:** `sem_vagas` (init = 10) controla vagas disponíveis; `sem_itens` (init = 0) controla itens disponíveis. Garçons aguardam vaga antes de inserir e sinalizam novo item. Cozinheiros aguardam item antes de retirar e sinalizam nova vaga.
- **Justificativa:** O padrão clássico de produtor-consumidor com dois semáforos evita busy waiting e garante que produtores bloqueiem quando a fila está cheia e consumidores bloqueiem quando está vazia.

#### 3. Mutex para Seções Críticas (`pthread_mutex_t`)
- **Três mutexes distintos são utilizados:**
  - `mutex_fila`: Protege o buffer circular durante inserção/remoção de pedidos. Garante que apenas uma thread modifique os índices da fila por vez.
  - `mutex_estatisticas`: Protege a atualização dos contadores globais (total de pedidos e tempo acumulado de espera). Evita condições de corrida nas estatísticas.
  - `mutex_log`: Serializa a saída de log para que mensagens de diferentes threads não se intercalem no terminal.

### Alternativas Consideradas e Descartadas

- **Variáveis de condição (`pthread_cond_t`):** Poderiam substituir os semáforos no produtor-consumidor, mas os semáforos são mais diretos e naturais para este padrão, evitando o risco de sinais perdidos.
- **Barreira (`pthread_barrier_t`):** Considerada para sincronizar rodadas de atendimento, mas descartada por adicionar complexidade desnecessária. O modelo de fluxo contínuo é mais natural para um restaurante.
- **Busy waiting:** Descartado por desperdiçar ciclos de CPU. Os semáforos bloqueiam as threads de forma eficiente.

### Desafios de Concorrência e Soluções

1. **Deadlock:** A ordem consistente de aquisição de recursos (primeiro semáforo da fila, depois mutex da fila, depois semáforo dos fogões) previne deadlocks. Nenhuma thread adquire múltiplos locks em ordem inversa.

2. **Condição de corrida nas estatísticas:** Resolvida com `mutex_estatisticas` dedicado, garantindo atomicidade na atualização de `total_pedidos` e `tempo_total_espera`.

3. **Terminação limpa:** Utilizado o padrão de pedido sentinela (`id_pedido = -1`). Após todos os garçons terminarem, o `main` enfileira um sentinela para cada cozinheiro. Isso garante que todos os cozinheiros processem todos os pedidos reais antes de encerrar.

4. **Intercalação de logs:** O `mutex_log` garante que cada mensagem de log seja impressa atomicamente, incluindo timestamp e conteúdo, sem intercalação com mensagens de outras threads.

---

## Estrutura do Projeto

```
projeto_lp2/
├── Makefile                    # Sistema de build
├── README.md                   # Este documento
├── src/
│   ├── main.c                  # Função principal e threads
│   ├── sincronizacao.h         # Definições de structs e protótipos
│   └── sincronizacao.c         # Implementação da sincronização
└── resultados/
    └── log_execucao.txt        # Log de uma execução de exemplo
```

---

## Como Compilar e Executar

```bash
# Compilar
make

# Executar
make run

# Limpar arquivos gerados
make clean
```

O Makefile compila com as flags `-Wall -Wextra -pthread` para máxima detecção de warnings.

---

## Resultados Experimentais

### Saída de Exemplo

```
========================================
    RESTAURANTE CONCORRENTE
========================================
Cozinheiros: 4
Fogões:      2
Mesas:       6
Garçons:     2
Tempo de preparo: 1 segundo(s)
Pedidos por mesa: 3
Total de pedidos: 18
========================================

[  0.000s] Garçom 0 iniciou (mesas 0 a 2)
[  0.000s] Garçom 1 iniciou (mesas 3 a 5)
[  0.000s] Cozinheiro 0 iniciou
[  0.000s] Cozinheiro 1 iniciou
[  0.000s] Garçom 0: coletou pedido #1 da mesa 0
[  0.000s] Garçom 1: coletou pedido #2 da mesa 3
[  0.000s] Cozinheiro 0: pegou pedido #1 (mesa 0, espera: 0.000s)
[  0.000s] Cozinheiro 0: usando fogão para preparar pedido #1
[  0.000s] Cozinheiro 1: pegou pedido #2 (mesa 3, espera: 0.000s)
[  0.000s] Cozinheiro 1: usando fogão para preparar pedido #2
...
[  9.033s] Cozinheiro 0: pedido #18 pronto! (mesa 2)
[  9.033s] Cozinheiro 0 recebeu sinal de encerramento
[  9.033s] Cozinheiro 0 encerrou

========================================
       ESTATÍSTICAS DA SIMULAÇÃO
========================================
Total de pedidos atendidos: 18
Tempo total de execução:   9.033 segundos
Operações por segundo:     1.99
Tempo médio de espera:     2.291 segundos
========================================
```

### Estatísticas Coletadas

| Métrica | Valor |
|---|---|
| Total de pedidos atendidos | 18 |
| Tempo total de execução | ~9 segundos |
| Operações por segundo | ~2.0 |
| Tempo médio de espera | ~2.3 segundos |

### Análise do Comportamento

- **Concorrência visível:** Os logs mostram garçons e cozinheiros operando simultaneamente. Enquanto garçons coletam pedidos das mesas, cozinheiros já preparam os pedidos anteriores.
- **Contenção nos fogões:** Com 4 cozinheiros competindo por 2 fogões, observa-se que no máximo 2 pedidos são preparados simultaneamente. Os demais cozinheiros aguardam até um fogão ser liberado.
- **Tempo de espera crescente:** Os últimos pedidos enfileirados apresentam maior tempo de espera, pois aguardam os cozinheiros terminarem pedidos anteriores. Isso é esperado dado que o gargalo está nos fogões (2) em relação aos cozinheiros (4).
- **Terminação ordenada:** Todos os cozinheiros processam seus pedidos antes de receber o sentinela, garantindo que nenhum pedido é perdido.
- **Throughput:** Com 2 fogões e tempo de preparo de 1s, o throughput teórico máximo é 2 pedidos/segundo. O valor observado (~2.0 ops/s) confirma que o sistema opera próximo da capacidade máxima.

---

## Reflexão sobre Uso de IA

### Ferramentas Utilizadas
- **Cursor com Claude:** Utilizado como assistente de programação durante todo o desenvolvimento.

### Para que Foram Utilizadas
- Estruturação inicial do projeto (layout de diretórios e Makefile).
- Implementação das estruturas de dados e mecanismos de sincronização.
- Geração do código das threads (garçom e cozinheiro).
- Revisão e correção de problemas de compatibilidade (semáforos nomeados para macOS).
- Elaboração deste documento README.

### Erros da IA e Correções
- **Semáforos não nomeados no macOS:** A implementação inicial utilizou `sem_init` (semáforos anônimos), que é deprecated e não funciona no macOS. Foi necessário migrar para `sem_open` (semáforos nomeados), alterando os campos dos structs de `sem_t` para `sem_t *` e criando funções auxiliares para abertura e fechamento.
- **Necessidade de validação manual:** Cada etapa foi compilada e executada para verificar o funcionamento correto, identificando problemas que a IA não antecipou.

### O que Foi Aprendido
- **Por causa da IA:** Aprendi padrões de organização de código concorrente, como separar a lógica de sincronização em módulos próprios. O padrão de terminação por sentinela ficou mais claro ao ver a implementação completa.
- **Apesar da IA:** Foi fundamental entender o *porquê* de cada mecanismo de sincronização. A IA gera código funcional, mas compreender a motivação por trás de cada mutex e semáforo (e onde deadlocks poderiam surgir) exigiu análise própria.

### Validação do Entendimento
- Cada mecanismo de sincronização foi rastreado manualmente para confirmar que não há deadlock nem condição de corrida.
- Os logs de execução foram analisados para verificar que a concorrência é real (threads intercalam operações) e que todos os pedidos são processados.
- O fluxo produtor-consumidor foi desenhado em diagrama para confirmar que a ordem de aquisição de recursos é consistente.
