
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 101
#define MAX_SUSPECTS 32
#define MAX_INPUT 200

/* ---------- Estruturas ---------- */

typedef struct Sala {
    char *nome;
    char *pista;           // pista estática associada à sala (pode ser NULL)
    struct Sala *esq;
    struct Sala *dir;
} Sala;

typedef struct PistaNode {
    char *texto;
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Nó para encadeamento na tabela hash (pista -> suspeito) */
typedef struct HashNode {
    char *chave;   // texto da pista (key)
    char *valor;   // nome do suspeito (value)
    struct HashNode *next;
} HashNode;

/* Estrutura para manter lista de suspeitos e suas contagens */
typedef struct SuspeitoCount {
    char *nome;
    int contagem;
} SuspeitoCount;

/* ---------- Utilitárias de string e memória ---------- */

// criaCopiaString: aloca e retorna uma cópia da string fornecida
char *criaCopiaString(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (!c) {
        fprintf(stderr, "Erro: memória insuficiente.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(c, s, n);
    return c;
}

/* ---------- Funções de sala (mapa) ---------- */

/*
 criarSala() – cria dinamicamente um cômodo.
  - nome: nome da sala
  - pista: texto da pista (pode ser NULL)
  - retorna: ponteiro para Sala recém-alocada
*/
Sala *criarSala(const char *nome, const char *pista) {
    Sala *s = malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: não foi possível alocar Sala.\n");
        exit(EXIT_FAILURE);
    }
    s->nome = criaCopiaString(nome);
    s->pista = pista ? criaCopiaString(pista) : NULL;
    s->esq = s->dir = NULL;
    return s;
}

/* Função para liberar memória do mapa (árvore de Salas) */
void liberarMapa(Sala *s) {
    if (!s) return;
    liberarMapa(s->esq);
    liberarMapa(s->dir);
    free(s->nome);
    if (s->pista) free(s->pista);
    free(s);
}


void inserirPista(PistaNode **raiz, const char *texto) {
    if (!texto || strlen(texto) == 0) return;
    if (*raiz == NULL) {
        PistaNode *n = malloc(sizeof(PistaNode));
        if (!n) {
            fprintf(stderr, "Erro: falha ao alocar nó de pista.\n");
            exit(EXIT_FAILURE);
        }
        n->texto = criaCopiaString(texto);
        n->esq = n->dir = NULL;
        *raiz = n;
        return;
    }
    int cmp = strcmp(texto, (*raiz)->texto);
    if (cmp == 0) {
        // duplicata: não inserir
        return;
    } else if (cmp < 0) {
        inserirPista(&(*raiz)->esq, texto);
    } else {
        inserirPista(&(*raiz)->dir, texto);
    }
}

/* exibirPistas() – imprime a árvore de pistas em ordem alfabética (in-order traversal). */
void exibirPistas(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf("- %s\n", raiz->texto);
    exibirPistas(raiz->dir);
}

/* liberarPistas: libera memória da BST de pistas */
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz->texto);
    free(raiz);
}

/* ---------- Tabela Hash pista -> suspeito (encadeamento) ---------- */

HashNode *hash_table[HASH_SIZE] = { NULL };

/* Lista de suspeitos conhecida e contagens (preenchida ao inserir na hash) */
SuspeitoCount suspeitos[MAX_SUSPECTS];
int num_suspeitos = 0;

/* função hash simples para strings */
unsigned int hash_func(const char *s) {
    unsigned long h = 5381;
    while (*s) {
        h = ((h << 5) + h) + (unsigned char)(*s); // h * 33 + c
        s++;
    }
    return (unsigned int)(h % HASH_SIZE);
}


void inserirNaHash(const char *chave, const char *valor) {
    if (!chave || !valor) return;
    unsigned int idx = hash_func(chave);
    HashNode *node = hash_table[idx];
    while (node) {
        if (strcmp(node->chave, chave) == 0) {
            // substitui valor existente (pouco provável no nosso uso, mas seguro)
            free(node->valor);
            node->valor = criaCopiaString(valor);
            return;
        }
        node = node->next;
    }
    // inserir novo nó no início do bucket
    HashNode *novo = malloc(sizeof(HashNode));
    if (!novo) {
        fprintf(stderr, "Erro: falha ao alocar HashNode.\n");
        exit(EXIT_FAILURE);
    }
    novo->chave = criaCopiaString(chave);
    novo->valor = criaCopiaString(valor);
    novo->next = hash_table[idx];
    hash_table[idx] = novo;

    // registrar suspeito na lista (se ainda não estiver)
    for (int i = 0; i < num_suspeitos; ++i) {
        if (strcmp(suspeitos[i].nome, valor) == 0) return;
    }
    if (num_suspeitos < MAX_SUSPECTS) {
        suspeitos[num_suspeitos].nome = criaCopiaString(valor);
        suspeitos[num_suspeitos].contagem = 0;
        num_suspeitos++;
    } else {
        fprintf(stderr, "Aviso: limite de suspeitos atingido.\n");
    }
}

/*
 encontrarSuspeito() – consulta o suspeito correspondente a uma pista.
  - chave: texto da pista
  - retorna: ponteiro para o nome do suspeito (string), ou NULL se não existir associação.
*/
const char *encontrarSuspeito(const char *chave) {
    if (!chave) return NULL;
    unsigned int idx = hash_func(chave);
    HashNode *node = hash_table[idx];
    while (node) {
        if (strcmp(node->chave, chave) == 0) {
            return node->valor;
        }
        node = node->next;
    }
    return NULL;
}

/* liberar tabela hash */
void liberarHash() {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashNode *n = hash_table[i];
        while (n) {
            HashNode *next = n->next;
            free(n->chave);
            free(n->valor);
            free(n);
            n = next;
        }
        hash_table[i] = NULL;
    }
    for (int i = 0; i < num_suspeitos; ++i) {
        free(suspeitos[i].nome);
    }
    num_suspeitos = 0;
}


void explorarSalas(Sala *atual, PistaNode **raizPistas) {
    if (!atual) {
        printf("Mapa vazio.\n");
        return;
    }
    Sala *pos = atual;
    char entrada[MAX_INPUT];

    printf("\n--- Início da exploração ---\n");
    while (1) {
        printf("\nVocê está em: %s\n", pos->nome);
        if (pos->pista) {
            printf("Você encontrou uma pista: \"%s\"\n", pos->pista);
            // inserir na BST (evita duplicatas)
            inserirPista(raizPistas, pos->pista);
            // procurar suspeito na tabela hash e, se encontrado, incrementar contagem
            const char *sus = encontrarSuspeito(pos->pista);
            if (sus) {
                // encontra índice do suspeito
                for (int i = 0; i < num_suspeitos; ++i) {
                    if (strcmp(suspeitos[i].nome, sus) == 0) {
                        suspeitos[i].contagem++;
                        break;
                    }
                }
            } else {
                printf("(Nenhum suspeito associado a esta pista.)\n");
            }
        } else {
            printf("Não há pistas visíveis neste cômodo.\n");
        }

        // opções
        printf("Opções: (e) esquerda, (d) direita, (s) sair e finalizar exploração\n");
        printf("Escolha: ");
        if (!fgets(entrada, sizeof(entrada), stdin)) {
            printf("Erro de leitura. Encerrando exploração.\n");
            break;
        }
        char cmd = entrada[0];
        if (cmd == '\n') continue;
        if (cmd == 's' || cmd == 'S') {
            printf("Saindo da exploração...\n");
            break;
        } else if (cmd == 'e' || cmd == 'E') {
            if (pos->esq) pos = pos->esq;
            else printf("Não há caminho à esquerda a partir daqui.\n");
        } else if (cmd == 'd' || cmd == 'D') {
            if (pos->dir) pos = pos->dir;
            else printf("Não há caminho à direita a partir daqui.\n");
        } else {
            printf("Comando inválido. Use 'e', 'd' ou 's'.\n");
        }
    }
    printf("--- Fim da exploração ---\n\n");
}


int verificarSuspeitoFinal(const char *nomeAcusado) {
    if (!nomeAcusado) return 0;
    for (int i = 0; i < num_suspeitos; ++i) {
        if (strcasecmp(suspeitos[i].nome, nomeAcusado) == 0) {
            return suspeitos[i].contagem >= 2 ? 1 : 0;
        }
    }
    return 0; // suspeito não conhecido ou sem pistas
}

/* Função para imprimir contagens por suspeito (debug/feedback) */
void exibirContagensSuspeitos() {
    printf("\nContagem de pistas por suspeito:\n");
    for (int i = 0; i < num_suspeitos; ++i) {
        printf("- %s: %d pista(s)\n", suspeitos[i].nome, suspeitos[i].contagem);
    }
    printf("\n");
}

/* ---------- Programa principal (mapa fixo e associações) ---------- */

int main(void) {

    Sala *hall = criarSala("Hall de Entrada", NULL);
    Sala *salaEstar = criarSala("Sala de Estar", "Pegada molhada perto do sofa");
    Sala *cozinha = criarSala("Cozinha", "Faca com cabo quebrado");
    Sala *biblioteca = criarSala("Biblioteca", "Pagina rasgada de um diario");
    Sala *salaJantar = criarSala("Sala de Jantar", "Marca de vinho na toalha");
    Sala *corredor = criarSala("Corredor", NULL);
    Sala *quarto = criarSala("Quarto", "Bilhete com uma inicial: 'M'");

    hall->esq = salaEstar; hall->dir = cozinha;
    salaEstar->esq = biblioteca; salaEstar->dir = salaJantar;
    cozinha->esq = corredor; cozinha->dir = quarto;

    // 2) Preencher a tabela hash com associações pista -> suspeito
    // Exemplo: cada pista mapeada para um nome de suspeito
    inserirNaHash("Pegada molhada perto do sofa", "Marcos");
    inserirNaHash("Faca com cabo quebrado", "Ricardo");
    inserirNaHash("Pagina rasgada de um diario", "Mariana");
    inserirNaHash("Marca de vinho na toalha", "Marcos");
    inserirNaHash("Bilhete com uma inicial: 'M'", "Marcos");
    // Você pode adicionar mais mapeamentos se quiser.

    // 3) BST vazia para coletar pistas
    PistaNode *raizPistas = NULL;

    printf("Bem-vindo a Detective Quest - Julgamento Final\n");
    printf("Você começará a exploração no Hall de Entrada.\n");

    // 4) Exploração interativa
    explorarSalas(hall, &raizPistas);

    // 5) Ao final: listar pistas coletadas e mostrar contagens por suspeito
    printf("Pistas coletadas (ordem alfabética):\n");
    if (!raizPistas) {
        printf("Nenhuma pista foi coletada durante a exploração.\n");
    } else {
        exibirPistas(raizPistas);
    }

    exibirContagensSuspeitos();

    // 6) Solicitar acusação ao jogador
    char entrada[MAX_INPUT];
    printf("Quem você acusa? Digite o nome do suspeito: ");
    if (!fgets(entrada, sizeof(entrada), stdin)) {
        printf("Erro de leitura.\n");
    } else {
        // remover \n final
        entrada[strcspn(entrada, "\n")] = '\0';
        // trim de espaços iniciais/finais (simples)
        // converter para não sensível a maiúsculas quando comparar (verificarSuspeitoFinal usa strcasecmp)
        if (strlen(entrada) == 0) {
            printf("Nenhum nome digitado. Encerrando.\n");
        } else {
            int ok = verificarSuspeitoFinal(entrada);
            if (ok) {
                printf("\nDecisão: Você acusou '%s'.\n", entrada);
                printf("Resultado: Há pistas suficientes (>= 2) que sustentam a acusação.\n");
            } else {
                printf("\nDecisão: Você acusou '%s'.\n", entrada);
                printf("Resultado: Não há pistas suficientes para sustentar a acusação (menos de 2 pistas).\n");
            }
        }
    }

    // 7) Limpeza de memória
    liberarPistas(raizPistas);
    liberarHash();
    liberarMapa(hall);

    printf("\nFim do jogo. Obrigado por investigar!\n");
    return 0;
}
