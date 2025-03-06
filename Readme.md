# Tema 1 APD
## Szabo Cristina-Andreea 334CA

Pentru aceasta tema am implementat un program care procesează fișiere text folosind paradigma MapReduce, distribuind sarcinile între mai multe thread-uri. Citesc toate cuvintele dintr-o listă de fișiere și apoi le scriu în fișiere de ieșire bazate pe prima lor literă.

Intai iau din argumente numarul de mapperi, reduceri si numele fisierului de input, apoi citesc din fisierul de input toate numele fisierelor si le aloc un index. Dupa ce am aceasta lista de fisiere, pornesc toate thread-urile si le impart, primele num_mappers sa fie thread-uri care folosesc functia mapper si restul reducer. Fiecare thread primeste argumente in functia de rolul sau prin functia care il creaza.

Functia mapper este o functie unde fiecare thread primeste un fisier, ia cuvintele din fisier si le pune intr-un map comun pentru toate threadurile unde vor fi perechi de tipul cuvant-set indecsi fisiere in care se afla cuvantul. Am un while din care thread-urile ies in momentul in care nu mai este niciun fisier de procesat, altfel iau urmatorul fisier din vector. Iau cate o linie din fisier si iau fiecare cuvant pe care il modific astfel incat sa fie lowercase si sa aiba numai caractere din alfabet si-l adaug intr-o lista auxiliara ca sa nu trebuiasca sa fac lock si unlock pentru fiecare cuvant cand scriu in lista comuna. Dupa ce iau toate cuvintele din fisierul curent, dau lock si le adaug la lista mare, apoi unlock dupa ce le am adaugat pe toate. Am pus o bariera ca sa astept toate thread-urile sa termine, iar cand termina toate le termin executia.

Functia reducer este o functie in care thread-urile reducer asteapta pana termina toate thread-urile mapper ca apoi sa sorteze lista deja agregata dupa numarul de fisiere in care apar cuvintele si alfabetic, apoi punandu-le in ordine in fisierele cu litera corespunzatoare primei litere a cuvintelor. Fiecare thread ia o litera si isi incheie executia cand nu mai sunt litere in alfabet de procesat. Daca mai sunt, iau urmatoarea litera avand grija ca 2 thread-uri sa nu ia aceeasi litera folosind un mutex si ordonez in functie de numarul de fisiere in care apare cuvantul si apoi alfabetic cuvintele pentru litera curenta. Dupa ce le-am ordonat, thread-ul le scrie in fisierul corespunzator si trece la urmatoarea litera, daca exista.

La final dau join tuturor thread-urilor intr-un singur for si eliberez mutexul si bariera folosite. Deci bariera e folosita ca sa astept toate thread-urile mapper sa-si termine executia pentru a le lasa pe cele reducer sa inceapa executia, deaoarece sunt pornite toate in acelasi timp. Mutexul il folosesc ca thread-urile sa nu ia acelasi fisier sau litera si cand modific lista agregata in mapper, ca sa scrie pe rand thread-urile.