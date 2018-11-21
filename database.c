//AVENIA Adrien - MARCHE Claire
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

#define NAME_LENGTH_MIN 3
#define NAME_LENGTH_MAX 10
#define TELEPHONE_LENGTH 8

#define VOWELS 5
#define CONSONANTS 15

#define OFFSET_32 2166136261
#define OFFSET_64 14695981039346656037
#define FNV_PRIME_32 16777619
#define FNV_PRIME_64 1099511628211

#define BUFSIZE 255

#define DEFAULT_DIR_SIZE 1000000

const char vowels[VOWELS] = {'A', 'E', 'I', 'O', 'U'};
const char consonants[CONSONANTS] = {'B', 'C', 'D', 'F', 'G', 'H', 'J', 'L', 'M', 'N', 'P', 'R', 'S', 'T', 'V'};

struct directory_data {
    char last_name[NAME_LENGTH_MAX + 1];
    char first_name[NAME_LENGTH_MAX + 1];
    char telephone[TELEPHONE_LENGTH + 1];
};

struct directory {
    struct directory_data **data;
    size_t size;
    size_t capacity;
};

struct index_bucket {
    const struct directory_data *data;
    struct index_bucket *next;
};

typedef size_t (*index_hash_func_t)(const struct directory_data *data);

struct index {
    struct index_bucket **buckets;
    size_t count;
    size_t size;
    index_hash_func_t func;
};

void directory_data_print(const struct directory_data *data);
void directory_data_random(struct directory_data *data);
static char name_char_random(char prev);
void directory_create(struct directory *self);
void directory_destroy(struct directory *self);
void directory_add(struct directory *self, struct directory_data *data);
void directory_random(struct directory *self, size_t n);

static size_t string_length(const char *array);
void directory_search(const struct directory *self, const char *last_name);
static bool directory_is_sorted(struct directory_data **data, size_t n);
void directory_sort(struct directory *self);
void directory_search_opt(const struct directory *self, const char *last_name);

struct index_bucket *index_bucket_add(struct index_bucket *self, const struct directory_data *data);
void index_bucket_destroy(struct index_bucket *self);
size_t fnv_hash(const char *key);
size_t index_first_name_hash(const struct directory_data *data);
size_t index_telephone_hash(const struct directory_data *data);
void index_create(struct index *self, index_hash_func_t func);
void index_destroy(struct index *self);
void index_rehash(struct index *self);
void index_add(struct index *self, const struct directory_data *data);
void index_fill_with_directory(struct index *self, const struct directory *dir);
void index_search_by_first_name(const struct index *self, const char *first_name);
void index_search_by_telephone(const struct index *self, const char *telephone);

void clean_newline(char *buf, size_t size);
static void enter_name(char *name);
static void enter_telephone(char *phone);

int main(int argc, char *argv[]) {
    long directory_size;
    if (argc >= 2) {
        directory_size = atoi(argv[1]);
    } else {
        printf("You didn't enter a size for the directory. It will have a default size of %d.\n", DEFAULT_DIR_SIZE);
        directory_size = DEFAULT_DIR_SIZE;
    }
    time_t t;
    srand((unsigned) time(&t));
    
    struct directory dir;
    struct timeval prev_tv;
    struct timeval current_tv;
    gettimeofday(&prev_tv, NULL);
    directory_create(&dir);
    directory_random(&dir, directory_size);
    gettimeofday(&current_tv, NULL);
    time_t sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
    suseconds_t usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
    printf("Creating directory... (time: %lu s %lu µs)\n", sec, usec);
    gettimeofday(&prev_tv, NULL);
    directory_sort(&dir);
    gettimeofday(&current_tv, NULL);
    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
    printf("Sorting directory... (time: %lu s %lu µs)\n", sec, usec);
    struct index index_first_name;
    gettimeofday(&prev_tv, NULL);
    index_create(&index_first_name, index_first_name_hash);
    index_fill_with_directory(&index_first_name, &dir);
    gettimeofday(&current_tv, NULL);
    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
    printf("Creating directory index by first names... (time: %lu s %lu µs)\n", sec, usec);
    struct index index_telephone;
    gettimeofday(&prev_tv, NULL);
    index_create(&index_telephone, index_telephone_hash);
    index_fill_with_directory(&index_telephone, &dir);
    gettimeofday(&current_tv, NULL);
    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
    printf("Creating directory index by phone numbers... (time: %lu s %lu µs)\n", sec, usec);
    
    
    char buf[BUFSIZE];
    char search[BUFSIZE];
    
    //affichage de toutes les entrées du directory pour effectuer des tests
    /*for (size_t i = 0; i < directory_size; ++i) {
        directory_data_print(dir.data[i]);
    }*/
    
    for (;;) {
        printf("What do you want to do ?\n");
        printf("\t 1: Search by last name (not optimized)\n");
        printf("\t 2: Search by last name (optimized)\n");
        printf("\t 3: Search by first name\n");
        printf("\t 4: Search by telephone\n");
        printf("\t q: Quit\n");
        printf("Your choice:\n");
        if (fgets(buf, BUFSIZE, stdin)) {
            clean_newline(buf, BUFSIZE);
            
            if (buf[0] == 'q') {
                break;
            }
            switch (buf[0]) {
                case '1': {
                    printf("Last name:\n");
                    enter_name(search);
                    gettimeofday(prev_tv, NULL);
                    directory_search(&dir, search);
                    gettimeofday(current_tv, NULL);
                    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
                    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
                    printf("Time taken : %lu s %lu µs\n", sec, usec);
                    break;
                }
                case '2': {
                    printf("Last name:\n");
                    enter_name(search);
                    gettimeofday(prev_tv, NULL);
                    directory_search_opt(&dir, search);
                    gettimeofday(current_tv, NULL);
                    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
                    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
                    printf("Time taken : %lu s %lu µs\n", sec, usec);
                    break;
                }
                case '3': {
                    printf("First name:\n");
                    enter_name(search);
                    gettimeofday(prev_tv, NULL);
                    index_search_by_first_name(&index_first_name, search);
                    gettimeofday(current_tv, NULL);
                    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
                    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
                    printf("Time taken : %lu s %lu µs\n", sec, usec);
                    break;
                }
                case '4': {
                    printf("Phone number:\n");
                    enter_telephone(search);
                    gettimeofday(prev_tv, NULL);
                    index_search_by_telephone(&index_telephone, search);
                    gettimeofday(current_tv, NULL);
                    sec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) / 1000000;
                    usec = ((current_tv.tv_sec - prev_tv.tv_sec) * 1000000 + (current_tv.tv_usec - prev_tv.tv_usec)) % 1000000;
                    printf("Time taken : %lu s %lu µs\n", sec, usec);
                    break;
                }
                default: {}
            }
        }
    }
    
    index_destroy(&index_first_name);
    index_destroy(&index_telephone);
    directory_destroy(&dir);
}

/* affiche une entrée de répertoire
param : data : l'entrée de répertoire à afficher 
*/
void directory_data_print(const struct directory_data *data) {
    printf("- %s %s: %s\n", data->last_name, data->first_name, data->telephone);
}

/* affecte des valeurs à une entrée de répertoire
param : data : l'entrée de répertoire qu'on souhaite affecter au hasard
*/
void directory_data_random(struct directory_data *data) {
    size_t size_first_name = rand() % (NAME_LENGTH_MAX - NAME_LENGTH_MIN) + NAME_LENGTH_MIN;
    size_t size_last_name = rand() % (NAME_LENGTH_MAX - NAME_LENGTH_MIN) + NAME_LENGTH_MIN;
    
    for (size_t i = 0; i < size_first_name; ++i) {
        if (i == 0) {
            data->first_name[i] = name_char_random('\0');
        } else {
            data->first_name[i] = name_char_random(data->first_name[i - 1]);
        }
    }
    for (size_t i = size_first_name; i <= NAME_LENGTH_MAX; ++i) {
        data->first_name[i] = '\0';
    }

    for (size_t i = 0; i < size_last_name; ++i) {
        if (i == 0) {
            data->last_name[i] = name_char_random('\0');
        } else {
            data->last_name[i] = name_char_random(data->last_name[i - 1]);
        }
    }
    for (size_t i = size_last_name; i <= NAME_LENGTH_MAX; ++i) {
        data->last_name[i] = '\0';
    }

	for (size_t i = 0; i < TELEPHONE_LENGTH; ++i) {
        data->telephone[i] = '0' + rand() % 10;
    }
    data->telephone[TELEPHONE_LENGTH] = '\0';
}

/* choisit un charactère au hasard dans les tableaux CONSONANTS et VOWELS
param : prev : le caractère précédent dans le mot qu'on affecte au hasard (consonne, voyelle ou '\0' si c'est la première lettre qu'on affecte)
return : un caractère contenu dans VOWELS si prev était une consonne, un caractère contenu dans CONSONANTS sinon
*/
static char name_char_random(char prev) {
    bool prev_vowel = true;
    int index_of_next = rand();
    
    for (size_t i = 0; i < CONSONANTS; ++i) {
        if (prev == consonants[i]) {
            prev_vowel = false;
            break;
        }
    }
    
    if (prev_vowel) {
        index_of_next %= CONSONANTS;
        return consonants[index_of_next];
    }
    index_of_next %= VOWELS;
    return vowels[index_of_next];
}

/* initialise un répertoire
param : self : le directory à initialiser
*/
void directory_create(struct directory *self) {
    self->data = NULL;
    self->size = 0;
    self->capacity = 0;
}

/* détruit un répertoire en libérant tous ses éléments
param : self : le répertoire à détruire
*/
void directory_destroy(struct directory *self) {
    for (size_t i = 0; i < self->size; ++i) {
        free(self->data[i]);
    }
    free(self->data);
}

/* ajoute une entrée à un répertoire
param : self : le répertoire auquel on ajoute une entrée
param : data : l'entrée à ajouter
*/
void directory_add(struct directory *self, struct directory_data *data) {
    if (self->size == self->capacity) {
        self->capacity = self->capacity * 2 + 1;
        struct directory_data **new_array = calloc(self->capacity, sizeof(struct directory_data*));
        memcpy(new_array, self->data, self->size * sizeof(struct directory_data*));
        free(self->data);
        self->data = new_array;
    }
    self->data[self->size] = data;
    ++self->size;
}

/* remplit un répertoire d'entrées au hasard
param : self : le répertoire à remplir
param : n : la taille du répertoire souhaitée
*/
void directory_random(struct directory *self, size_t n) {
    for (size_t i = 0; i < n; i++) {
        struct directory_data *data = malloc(sizeof(struct directory_data));
        directory_data_random(data);
        directory_add(self, data);
    }
}

//Partie 2

/* calcule la taille d'une chaîne
param : array : la chaîne dont on veut calculer la taille
return : la taille de la chaîne
*/
static size_t string_length(const char *array) {
    size_t i = 0;
    
    while (array[i] != '\0') {
        ++i;
    }
    
    return i;
}

/* affiche les entrées correspondant à un nom de famille dans le répertoire
param : self : le répertoire dans lequel on effectue la recherche
param : last_name : le nom qu'on recherche
*/
void directory_search(const struct directory *self, const char *last_name) {
    size_t last_name_size = string_length(last_name);
    
    for (size_t i = 0; i < self->size; ++i) {
        if (string_length(self->data[i]->last_name) == last_name_size) {
            size_t j = 0;
            while (j < last_name_size && last_name[j] == self->data[i]->last_name[j]) {
                j++;
            }
            
            if (j == last_name_size) {
                directory_data_print(self->data[i]);
            }
        }
    }
}

/* échange deux entrées dans un répertoire
param : data : le tableau de pointeurs vers les entrées du répertoire
param : i : l'indice de la première donnée à échanger
param : j : l'indice de la deuxième donnée à échanger
*/
static void directory_swap(struct directory_data **data, size_t i, size_t j) {
	struct directory_data *tmp = data[i];
	data[i] = data[j];
	data[j] = tmp;
}

/* compare deux chaînes de caractères
param : first : la première chaîne
param : second :  la deuxième chaîne
return : true si la première chaîne est inférieure à la deuxième selon l'ordre alphabétique, false sinon
*/
static bool compare_data_names(char *first, char *second) {
	size_t size_first = string_length(first);
	size_t size_second = string_length(second);
	size_t size_min = size_first > size_second ? size_second : size_first;
	
	for (size_t i = 0; i < size_min; ++i) {
		if (first[i] != second[i]) {
			return first[i] < second[i] ? true : false;
		}
	}
	
	return size_first == size_min ? true : false;
}

/* partitionne un tableau selon son premier élément
param : data : le tableau à partitionner
param : i : l'indice du début du tableau et indice du pivot
param : j : l'indice de la fin du tableau
return : l'indice du pivot après partition
*/
static ssize_t directory_partition(struct directory_data **data, ssize_t i, ssize_t j) {
	ssize_t pivot_index = i;
	char *pivot = data[pivot_index]->last_name;
	directory_swap(data, pivot_index, j);
	ssize_t l = i;
	for (ssize_t k = i; k < j; ++k) {
		if (compare_data_names(data[k]->last_name, pivot)) {
			directory_swap(data, k, l);
			l++;
		}
	}
	directory_swap(data, l, j);
	return l;
}

/* trie un tableau par nom de famille d'un indice de début à un indice de fin
param : data : le tableau à trier
param : i : l'indice de début de la partie à trier
param : j : l'indice de fin de la partie à trier
*/
static void directory_quick_sort_partial(struct directory_data **data, ssize_t i, ssize_t j) {
	if (i < j) {
		ssize_t p = directory_partition(data, i, j);
		directory_quick_sort_partial(data, i, p - 1);
		directory_quick_sort_partial(data, p + 1, j);
	}
}

/* trie un tableau par nom de famille
param : data : le tableau d'entrées à trier
param : n : la taille du tableau à trier
*/
static void directory_quick_sort(struct directory_data **data, ssize_t n) {
	directory_quick_sort_partial(data, 0, n - 1);
}

/* vérifie si un tableau est trié
param : data : le tableau
param : n : la taille du tableau
return : true si le tableau est trié, false sinon
*/
static bool directory_is_sorted(struct directory_data **data, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        if (!(compare_data_names(data[i - 1]->last_name, data[i]->last_name))) {
            return false;
        }
    }
    return true;
} 

/* trie un répertoire par nom de famille
param : self : le répertoire 
*/
void directory_sort(struct directory *self) {
    if (!(directory_is_sorted(self->data, self->size))) {
	    directory_quick_sort(self->data, self->size);
	}
}

/* cherche un nom de famille de manière optimisée dans un répertoire trié par nom de famille
param : self : le répertoire
param : last_name : le nom de famille à rechercher
*/
void directory_search_opt(const struct directory *self, const char *last_name) {
	
	size_t lo = 0;
    size_t hi = self->size;      
    while (lo != hi) {
        size_t mid = (lo + hi) / 2; 
        if (strcmp(last_name, self->data[mid]->last_name) == 0) {
            size_t start = mid;
            size_t stop = mid + 1;
            
            while (stop < self->size && strcmp(last_name, self->data[stop]->last_name) == 0) {
                ++stop;
            }
            while (start > 0 && strcmp(last_name, self->data[start - 1]->last_name) == 0) {
                --start;
            }
            
            for (size_t j = start; j < stop; ++j) {
                directory_data_print(self->data[j]);
            }
            break;  
        }
        if (strcmp(last_name, self->data[mid]->last_name) < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
}

//Partie 3

/* ajoute un bucket à une liste chaînée de bucket et renvoie la liste ainsi obtenue
param : self : la liste à laquelle on veut ajouter un bucket
param : data : le bucket à ajouter
return : la nouvelle liste
*/
struct index_bucket *index_bucket_add(struct index_bucket *self, const struct directory_data *data) {
    if (self == NULL) {
        self = malloc(sizeof(struct index_bucket));
        self->next = NULL;
        self->data = data;
    } else {
        struct index_bucket *added = malloc(sizeof(struct index_bucket));
        struct index_bucket *prev = self;
        added->next = NULL;
        added->data = data;
        while (prev->next != NULL) {
            prev = prev->next;
        }
        prev->next = added;
    }
    
    return self;
}

/* détruit une liste chaînée de buckets
param : self : la liste à détruire
*/
void index_bucket_destroy(struct index_bucket *self) {   
	while(self != NULL) {
        struct index_bucket *to_destroy = self;
        self = self->next;
        free(to_destroy);
    }
}

/* calcule le hash fnv correspondant à une clé
param : key : la clé dont on veut le hash
return : le hash fnv de key
*/
size_t fnv_hash(const char *key) {
    size_t hash = OFFSET_32;
    size_t key_length = string_length(key);
    for (size_t i = 0; i < key_length; ++i) {
        hash = hash ^ key[i];
        hash = (hash * FNV_PRIME_32);
    }
    return hash;
}

/* calcule le hash correspondant au prénom d'une entrée de répertoire
param : data : l'entrée dont on veut calculer le hash du prénom
return : le hash correspondant au prénom de l'entrée
*/
size_t index_first_name_hash(const struct directory_data *data) {
    return fnv_hash(data->first_name);
}

/* calcule le hash correspondant au numéro de téléphone d'une entrée de répertoire
param : data : l'entrée dont on veut calculer le hash du numéro de téléphone
return : le hash correspondant au numéro de téléphone de l'entrée
*/
size_t index_telephone_hash(const struct directory_data *data) {
    return fnv_hash(data->telephone);
}

/* initialise un index selon une fonction de hachage
param : self : l'index à initialiser
param : func : la fonction de hachage sur laquelle on créée l'index
*/
void index_create(struct index *self, index_hash_func_t func) {
    self->count = 0;
    self->size = 1;
    self->buckets = calloc(self->size, sizeof(struct index_bucket *));
    self->func = func;
}

/* détruit un index
param : self : l'index à détruire
*/
void index_destroy(struct index *self) {
    size_t count = 0;
    for (size_t i = 0; i < self->size; ++i) {
        if (self->buckets[i] != NULL) {
            index_bucket_destroy(self->buckets[i]);
            ++count;
        }
        if (count == self->count) {
            break;
        }
    }
	free(self->buckets);
}

/* agrandit un index en recalculant les indices de chacun des éléments de l'ancien tableau
param : self : l'index à agrandir
*/
void index_rehash(struct index *self) {
    size_t old_size = self->size;
	self->size *= 2;
	struct index_bucket **new_buckets = calloc(self->size, sizeof(struct index_bucket *));
	struct index_bucket *old_bucket;
	struct index_bucket *new_bucket;
	struct index_bucket *to_remove;
	size_t hash;
	size_t count = 0;
	for (size_t i = 0; i < old_size; ++i) {
	    if (self->buckets[i] != NULL) {
		    old_bucket = self->buckets[i];
		    to_remove = old_bucket;
		    while (old_bucket != NULL) {
			    hash = self->func(old_bucket->data);
			    new_bucket = new_buckets[hash % self->size];
				new_buckets[hash % self->size] = index_bucket_add(new_bucket, old_bucket->data);
			    old_bucket = old_bucket->next;
		    }
		    ++count;
		    index_bucket_destroy(to_remove);
		}	
		if (count == self->count) {
		    break;
		}
	}
	free(self->buckets);
	self->buckets = new_buckets;
}

/* ajoute une entrée à un index, en l'agrandissant si le taux de compression est supérieur à 1/2
param : self : l'index auquel on ajoute une entrée
param : data : l'entrée à ajouter
*/
void index_add(struct index *self, const struct directory_data *data) {
	if ((self->count / self->size) > 0.5) {
		index_rehash(self);
	}
	size_t hash = self->func(data);
	struct index_bucket *bucket = self->buckets[hash % self->size];
	self->buckets[hash % self->size] = index_bucket_add(bucket, data);
	self->count++;
}

/* remplit un index avec un répertoire selon la fonction de hachage de l'index
param : self : l'index qu'on remplit
param ! dir : le répertoire avec lequel on remplit l'index
*/
void index_fill_with_directory(struct index *self, const struct directory *dir) {
	for (size_t i = 0; i < dir->size; ++i) {
		index_add(self, dir->data[i]);
	}
}

//Pour les deux fonctions à suivre, on peut envisager, afin d'éliminer totalement l'affichage des collisions, de vérifier pour chacun des membres de la liste obtenue par recherche si le prénom ou le numéro de téléphone correspond à celui qui est recherché

/* recherche les valeurs correspondant à un prénom dans un index
param : self : l'index dans lequel on effectue la recherche
param : first_name : le prénom à rechercher
*/
void index_search_by_first_name(const struct index *self, const char *first_name) {
	size_t hash = fnv_hash(first_name);
	struct index_bucket *bucket = self->buckets[hash % self->size];
	while (bucket != NULL) {
		directory_data_print(bucket->data);
		bucket = bucket->next;
	}	
}

/* recherche les valeurs correspondant à un numéro de téléphone dans un index
param : self : l'index dans lequel on effectue la recherche
param : téléphone : le numéro de téléphone à rechercher
*/
void index_search_by_telephone(const struct index *self, const char *telephone) {
	size_t hash = fnv_hash(telephone);
	struct index_bucket *bucket = self->buckets[hash % self->size];
	while (bucket != NULL) {
		directory_data_print(bucket->data);
		bucket = bucket->next;
	}	
}

//Partie 4

/* remplace le caractère de fin de ligne dans un buffer par le caractère de fin de chaîne '\0'
param : buf : le buffer où on veut effectuer la substitution
param : size : la taille maximale du buffer
*/
void clean_newline(char *buf, size_t size) {
    size_t index = 0;
    while (index < size && buf[index] != '\n' && buf[index] != '\r' && buf[index] != '\0') {
        ++index;
    }
    buf[index] = '\0';
}

/* récupère et vérifie la saisie d'un nom par l'utilisateur
param : name : où on va stocker le nom une fois correct (selon les critères de taille) saisi par l'utilisateur
*/
static void enter_name(char *name) {
    bool correct_name;
    
    do {
        correct_name = true;
        if (fgets(name, BUFSIZE, stdin)) {
            clean_newline(name, BUFSIZE);
            size_t buf_size = string_length(name);
            if (buf_size < NAME_LENGTH_MIN || buf_size > NAME_LENGTH_MAX) {
                correct_name = false;
                printf("Names have to count at least %d and at most %d characters. Please enter a name of the correct length.\n", NAME_LENGTH_MIN, NAME_LENGTH_MAX);
            }
        }
    } while (!correct_name);
}

/* récupère et vérifie la saisie d'un numéro de téléphone par l'utilisateur
param : phone : où on va stocker le numéro de téléphone une fois correct (selon les critères de taille) saisi par l'utilisateur
*/
static void enter_telephone(char *phone) {
    bool correct_phone;
 
    do {
        correct_phone = true;
        if (fgets(phone, BUFSIZE, stdin)) {
            clean_newline(phone, BUFSIZE);
            size_t buf_size = string_length(phone);
            if (buf_size != TELEPHONE_LENGTH) {
                correct_phone = false;
                printf("Phone numbers have to count exactly %d characters. Please enter a phone number of the correct length.\n", TELEPHONE_LENGTH);
            }
        }
    } while (!correct_phone);
}
