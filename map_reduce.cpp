#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

// argumente pe care le primeste functia din pthread_create
struct args {
    // vector care contine perechi nume_fisier-index
    vector<pair<string, int>>* files;
    pthread_mutex_t* mutex;
    // map care contine cuvantul si indecsii fisierelor in care apare
    map<string, set<int>>* word_idx;
    pthread_barrier_t* barrier;
    vector<char>* letters;
};

// trec la lowercase caracterele alfabetice si le omit pe restul
void modify_word(string& word) {
    string new_word;
    for (char c : word) {
        if (isalpha(c)) {
            new_word += tolower(c);
        }
    }
    word = new_word;
}

// functie unde fiecare thread primeste un fisier, ia cuvintele din fisier
// si le pune intr-un map comun pentru toate threadurile unde vor fi
// perechi de tipul cuvant-set indecsi fisiere in care se afla cuvantul
void* mapper(void* arg) {
    args* data = static_cast<args*>(arg);
    // nume fisiere si indecsi
    vector<pair<string, int>>* files = data->files;
    pthread_mutex_t* mutex = data->mutex;
    // map comun pentru lista finala nesortata
    map<string, set<int>>* word_idx = data->word_idx;
    pthread_barrier_t* barrier = data->barrier;

    string line;
    while (true) {
        pair<string, int> file;
        pthread_mutex_lock(mutex);

        // daca nu mai e niciun fisier de procesat inchei executia
        if (files->empty()) {
            pthread_mutex_unlock(mutex);
            break;
        }

        // daca mai sunt fisiere, il iau pe urmatorul si-l sterg din vector
        file = files->back();
        files->pop_back();
        pthread_mutex_unlock(mutex);

        // citesc numele si id-ul fisierului
        ifstream f(file.first);
        int id = file.second;

        // iau cate o linie din fisier si iau fiecare cuvant
        map<string, set<int>> aux;
        while (getline(f, line)) {
            stringstream ss(line);
            string word;
            while (ss >> word) {
                // modific cuvantul astfel incat sa fie lowercase si sa aiba
                // numai caractere din alfabet
                modify_word(word);
                // il pun intr-o lista auxiliara ca sa nu trebuiasca sa fac lock
                // si unlock pentru fiecare cuvant cand scriu in lista comuna
                aux[word].insert(id);
            }
        }

        // modific lista comuna si ma asigur ca doar un thread scrie pe rand
        pthread_mutex_lock(mutex);
        // adaug tot ce am in lista auxiliara
        for (const auto& entry : aux) {
            (*word_idx)[entry.first].insert(entry.second.begin(), entry.second.end());
        }
        pthread_mutex_unlock(mutex);
    }

    // bariera ca sa astept sa termine toate thread-urile
    pthread_barrier_wait(barrier);
    // inchid executia
    pthread_exit(nullptr);
}

// functie unde thread-urile reducer asteapta pana termina toate
// thread-urile mapper ca apoi sa sorteze lista deja agregata dupa numarul
// de fisiere in care apar cuvintele si alfabetic, apoi punandu-le in ordine
// in fisierele cu litera corespunzatoare primei litere a cuvintelor
void* reducer(void* arg) {
    args* data = static_cast<args*>(arg);
    pthread_mutex_t* mutex = data->mutex;
    map<string, set<int>>* word_idx = data->word_idx;
    pthread_barrier_t* barrier = data->barrier;
    // vector cu toate literele din alfabet
    vector<char>* letters = data->letters;

    // astept sa termine toate thread-urile mapper
    pthread_barrier_wait(barrier);

    while (true) {
        char letter;

        // mutex ca 2 thread-uri sa nu ia aceeasi litera
        pthread_mutex_lock(mutex);
        // mai sunt litere in vector, iau urmatoarea si o sterg din vector
        if (!letters->empty()) {
            letter = letters->back();
            letters->pop_back();
        // nu mai sunt litere neprocesate
        } else {
            // thread-ul nu mai ia alta litera
            pthread_mutex_unlock(mutex);
            break;
        }
        pthread_mutex_unlock(mutex);

        // calculez in cate fisiere apare fiecare cuvant cu litera curenta
        map<string, int> word_counts;
        for (const auto& word : *word_idx) {
            if (tolower(word.first[0]) == letter) {
                word_counts[word.first] = word.second.size();
            }
        }

        // sortez lista cu cuvintele care incep cu litera curenta in functie de numarul
        // de fisiere in care apar
        vector<pair<string, int>> sorted_words(word_counts.begin(), word_counts.end());
        sort(sorted_words.begin(), sorted_words.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
            if (a.second != b.second)
                return a.second > b.second;
            return a.first < b.first;
        });

        // creez fisierul pentru cuvintele care incep cu litera curenta
        string output_file = string(1, letter) + ".txt";
        ofstream out(output_file);
        // iau fiecare cuvant din lista sortata si indecsii fisierelor in care
        // apare si scriu in fisier
        for (const auto& word : sorted_words) {
            const auto& indices = (*word_idx)[word.first];
            out << word.first << ":[";
            
            // scriu fiecare index dupa ce am scris cuvantul
            for (auto i = indices.begin(); i != indices.end(); ++i) {
                if (i != indices.begin()) {
                    out << " ";
                }
                out << (*i) + 1;
            }
            out << "]" << endl;
        }
        out.close();
    }

    // inchide executia
    pthread_exit(nullptr);
}

int main(int argc, char* argv[]) {
    // daca argumentele sunt mai putine de 4
    if (argc < 4) {
        cout << "Invalid input" << endl;
        return 1;
    }

    // iau din argumente numarul de mapperi, reduceri si numele fisierului de input
    int num_mappers = atoi(argv[1]);
    int num_reducers = atoi(argv[2]);
    string file_name = argv[3];

    ifstream input(file_name);
    int num_files;
    input >> num_files;
    vector<string> files(num_files);

    // citesc din fisierul de input toate numele fisierelor si le aloc un index
    for (int i = 0; i < num_files; i++) {
        input >> files[i];
    }
    input.close();
    vector<pair<string, int>> name_id;
    for (int i = 0; i < num_files; i++) {
        name_id.push_back(make_pair(files[i], i));
    }

    // initializez muexul si bariera
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);

    // initializez lista de litere
    vector<char> letters;
    for (char c = 'a'; c <= 'z'; c++) {
        letters.push_back(c);
    }
    
    // lista in care sunt puse toate cuvintele si indecsii fisierelor in care apar
    map<string, set<int>> word_idx;
    // vector cu thread-urile
    vector<pthread_t> threads(num_mappers + num_reducers);
    // vector cu argumentele pentru functiile mapper si reducer
    vector<args> data(num_mappers + num_reducers);

    // pornesc toate thread-urile si le impart, primele num_mappers sa fie
    // thread-uri care folosesc functia mapper si restul reducer
    for (int i = 0; i < num_mappers + num_reducers; i++) {
        if (i < num_mappers) {
            data[i] = {&name_id, &mutex, &word_idx, &barrier, nullptr};
            pthread_create(&threads[i], nullptr, mapper, &data[i]);
        } else {
            data[i] = {nullptr, &mutex, &word_idx, &barrier, &letters};
            pthread_create(&threads[i], nullptr, reducer, &data[i]);
        }
    }

    // dau join la toate thread-urile
    for (int i = 0; i < num_mappers + num_reducers; i++) {
        pthread_join(threads[i], nullptr);
    }

    // eliberez bariera si mutexul
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    return 0;
}
