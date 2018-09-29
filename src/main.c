#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ENABLE_FILE_INPUT
//#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
#define LOG(format, ...) printf(format, ## __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#ifdef ENABLE_FILE_INPUT
#define FREOPEN(path, mode, file) freopen(path, mode, file)
#else
#define FREOPEN(path, mode, file)
#endif


typedef struct node {
    void *data;
    struct node *next;
} Node;

typedef struct {
    int index;
    int items_size;
    char *characters;
} Tape;

typedef struct {
    Tape *tape;
    int state;
} Configuration;

typedef struct {
    char out;
    char move;
    int next_state;
} Transition;

typedef struct {
    int state_number;
    bool isAcceptor;
    Node **transitions_array;
} State;

typedef struct {
    int items_size;
    State **states_array;
} TM;


/*
 * Generic list insertions
 */

void push(Node **head, void *data) {

    Node *new_node = (Node *) malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = *head;
    *head = new_node;
}

void append(Node **head, void *data) {
    Node *new_node = (Node *) malloc(sizeof(Node));
    Node *last = *head;
    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    while (last->next != NULL)
        last = last->next;

    last->next = new_node;
}

/*
 * Transitions deletions
 */

void delete_transition(Transition *transition) {
    free(transition);
}

void delete_transition_node(Node *node) {
    Transition *transition = (Transition *) node->data;
    delete_transition(transition);
    free(node);
}

void destroy_transitions(Node *transitions) {
    Node *index = transitions;
    Node *next = NULL;

    if (transitions == NULL) return;

    while (index != NULL) {
        next = index->next;
        delete_transition_node(index);
        index = next;
    }
}

/*
 * Configurations deletions
 */

void delete_configuration(Configuration *conf) {
    free(conf->tape->characters);
    free(conf->tape);
    free(conf);
}

void delete_configuration_node(Node *node) {
    Configuration *conf = (Configuration *) node->data;
    delete_configuration(conf);
    free(node);
}

void destroy_configurations(Node *configurations) {
    Node *index = configurations;
    Node *next = NULL;

    if (configurations == NULL) return;

    while (index != NULL) {
        next = index->next;
        delete_configuration_node(index);
        index = next;
    }
}

Node *delete_old_configuration_node(Node *configurations, int old_state, Tape **pTape) {
    Node *index = configurations;
    Node *prev = NULL;

    if (((Configuration *) index->data)->state == old_state && &(((Configuration *) index->data)->tape) == pTape) {
        configurations = configurations->next;
        delete_configuration_node(index);
        return configurations;
    }
    prev = index;
    index = index->next;

    while (index != NULL) {
        Configuration *configuration = index->data;
        if (configuration->state == old_state && &configuration->tape == pTape) {
            prev->next = index->next;
            delete_configuration_node(index);
            return configurations;
        }

        prev = index;
        index = index->next;
    }
    return configurations;
}

void destroy_tm(TM *tm) {
    TM *turing_machine = tm;

    for (int i = 0; i < turing_machine->items_size; i++) {
        State *state = turing_machine->states_array[i];
        for (int j = 0; j < 75; j++) {
            Node *transitions = state->transitions_array[j]->data;
            destroy_transitions(transitions);
            free(state->transitions_array);
        }
        free(state);
    }
    free(turing_machine);
}

/*
 * Creations of structs (building helpers)
 */

TM *new_tm() {
    TM *tm = malloc(sizeof(TM));
    tm->items_size = 0;
    tm->states_array = NULL;
    return tm;
}

Tape *new_tape() {
    Tape *tape = malloc(sizeof(Tape));
    tape->characters = NULL;
    tape->items_size = 0;
    tape->index = 0;
    return tape;
}

State *new_state(int state_number) {
    State *state = malloc(sizeof(State));
    state->state_number = state_number;
    state->isAcceptor = false;
    state->transitions_array = calloc(75, sizeof(Node *));
    return state;
}

Configuration *new_configuration(int state, Tape *tape) {
    Configuration *conf = malloc(sizeof(Configuration));
    conf->tape = tape;
    conf->state = state;
    return conf;
}

Transition *new_transition(char out, char move, int next_state) {
    Transition *transition = malloc(sizeof(Transition));
    transition->out = out;
    transition->move = move;
    transition->next_state = next_state;
    return transition;
}

/*
  * Building helpers
  */

int max(int a, int b) {

    if (a > b) {
        return a;
    } else {
        return b;
    }
}

void print_tm(TM *tm) {
    for (int i = 0; i < tm->items_size; i++) {
        LOG("State: %d", tm->states_array[i]->state_number);
        if (tm->states_array[i]->isAcceptor) {
            LOG(" (Acceptor)\n");
        } else {
            LOG("\n");
        }
        for (int j = 0; j < 75; j++) {
            Node *transitions = tm->states_array[i]->transitions_array[j];
            if (transitions != NULL) {
                LOG("Found transition for %c\n", (char) j - 48);
                for (Node *k = transitions; k != NULL; k = k->next) {
                    Transition *transition = k->data;
                    LOG("[out: %c | move %c | next_state: %d]\n", transition->out, transition->move,
                        transition->next_state);
                }
            }

        }
        LOG("\n");
    }
}

/*
 * Building function & acceptors setting function
 */

void build_tm(TM *tm, char *buf) {

    int state_number;
    char in;
    char out;
    char move;
    int next_state;

    sscanf(buf, "%d %c %c %c %d", &state_number, &in, &out, &move, &next_state);
    LOG("Received: %s", buf);

    if (state_number >= tm->items_size || next_state >= tm->items_size) {

        int new_size = max(state_number, next_state) + 1;
        tm->states_array = (State **) realloc(tm->states_array, (size_t) (new_size) * sizeof(State *));

        for (int i = tm->items_size; i < new_size; i++) {
            tm->states_array[i] = NULL;
        }
        tm->items_size = new_size;
    }

    if (tm->states_array[next_state] == NULL)
        tm->states_array[next_state] = new_state(next_state);

    State *current_state = tm->states_array[state_number];
    Transition *transition = new_transition(out, move, next_state);

    if (current_state == NULL) {
        Node *transitions = NULL;
        push(&transitions, transition);
        State *state = new_state(state_number);
        state->transitions_array[in - 48] = transitions;
        tm->states_array[state_number] = state;
    } else {
        State *state = tm->states_array[state_number];
        Node **transitions_array = state->transitions_array;
        push(&transitions_array[in - 48], transition);
    }
}

void set_acceptor(char *buf, TM *tm) {
    int acceptor;
    sscanf(buf, "%d", &acceptor);

    for (int i = 0; i < tm->items_size; i++) {
        if (tm->states_array[i]->state_number == acceptor) {
            tm->states_array[i]->isAcceptor = true;
        }
    }
}

/*
 * Running helper functions
 */

void extend(char move, Tape *tape) {

    tape->characters = realloc(tape->characters, (sizeof(char)) * (tape->items_size + 1));

    tape->items_size = tape->items_size + 1;

    if (move == 'R') {
        tape->characters[tape->items_size - 1] = '_';
    } else if (move == 'L') {
        memmove(&tape->characters[1], tape->characters, (size_t) tape->items_size - 1);
        tape->characters[0] = '_';
        tape->index++;
    }
}

int delta(Transition *transition, Tape *tape) {

    if (transition->move == 'R') {

        if (tape->index + 1 == tape->items_size) {
            extend(transition->move, tape);
        }

        tape->characters[tape->index] = transition->out;
        tape->index++;
    } else if (transition->move == 'L') {

        if (tape->index == 0) {
            extend(transition->move, tape);
        }

        tape->characters[tape->index] = transition->out;
        tape->index--;
    } else if (transition->move == 'S') {
        tape->characters[tape->index] = transition->out;
    }

    return transition->next_state;
}

Node *get_transitions(int state, Tape *tape, TM *tm) {

    int j = (int) tape->characters[tape->index] - 48;
    Node *transitions = tm->states_array[state]->transitions_array[j];

    return transitions;
}

Tape *duplicate(Tape *source) {

    Tape *destination = malloc(sizeof(Tape));
    destination->index = source->index;
    destination->items_size = source->items_size;
    destination->characters = malloc(source->items_size * sizeof(char));
    memcpy(destination->characters, source->characters, (size_t) source->items_size);
    return destination;
}

unsigned int MAX_MOVES = 0;


/*
 * Running function
 */

char run_configuration(Configuration *configuration, TM *tm) {
    unsigned int current_move = 0;
    Node *configurations = NULL;
    push(&configurations, configuration);
    bool unknown = false;

    while (current_move < MAX_MOVES) {

        if (configurations == NULL) {
            if (unknown) {
                return 'U';
            } else return '0';
        }

        Node *i = configurations;
        while (i != NULL) {

            Configuration *conf = i->data;

            Node *j = get_transitions(conf->state, conf->tape, tm);
            while(j != NULL) {

                Transition *transition = (Transition *) j->data;

                if (
                        (transition->move == 'S' &&
                         conf->state == transition->next_state &&
                         conf->tape->characters[conf->tape->index] == transition->out) ||

                        ((transition->move != 'S' &&
                          conf->state == transition->next_state &&
                          conf->tape->characters[conf->tape->index] == '_') &&
                         (conf->tape->index == conf->tape->items_size || conf->tape->index == 0))
                        ) {
                    unknown = true;
                    continue;
                }

                Configuration *new_conf = new_configuration(conf->state, duplicate(conf->tape));
                int next_state = delta(transition, new_conf->tape);
                if (tm->states_array[next_state]->isAcceptor) {
                    destroy_configurations(configurations);
                    return '1';
                }

                new_conf->state = next_state;
                push(&configurations, new_conf);
                j = j->next;
            }
            i = i->next;
            configurations = delete_old_configuration_node(configurations, conf->state, &conf->tape);

        }
        current_move++;
    }
    destroy_configurations(configurations);

    return 'U';
}

/*
 * Main routine
 */

int main() {

    FREOPEN("../resources/pubblico.txt", "r", stdin);
    char trailing[6];
    scanf("%s\n", trailing);
    LOG("Trailing: %s, TM Creation...\n", trailing);
    char buf[100] = {0};

    TM *tm = new_tm();

    while (true) {
        fgets(buf, 100, stdin);
        if (strcmp(buf, "acc\n") == 0) {
            break;
        } else {
            build_tm(tm, buf);
        }
    }

    LOG("Setting acceptors\n");

    while (true) {

        scanf("%s", buf);
        if (strcmp(buf, "max") == 0) {
            break;
        } else {
            set_acceptor(buf, tm);
            LOG("Received: %s\n", buf);
        }

    }

    print_tm(tm);
    scanf("%d\n", &MAX_MOVES);
    LOG("Max is: %d\n", MAX_MOVES);
    scanf("%s\n", trailing);
    LOG("Trailing: %s, Running...\n\n", trailing);

    while (true) {

        char *string = NULL;
        Configuration *configuration = new_configuration(0, new_tape());
        scanf("%ms\n", &string);
        LOG("Running: %s\n", string);
        
        if (string == NULL) {
            return 0;
        }

        configuration->tape->characters = strdup(string);
        configuration->tape->items_size = strlen(string);
        free(string);
        LOG("Result: ");
        printf("%c\n", run_configuration(configuration, tm));
    }
}

/* TODO:
 * [80% (controllare free)]    controllare le free e liberare il massimo spazio della memoria -> configurazioni e se proprio tm iniziale
 * [un caso eliminato]         ottimizzare evitando i loop e le situazioni banali
 * [0%]                        cambiare lista (quindi anche push e delete) affinchè siano doppiamente concatenate (O(1))
*/
