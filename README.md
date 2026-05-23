# Moteur de Recherche par Parcours Guidé (MaxScore Guided Traversal)

Ce projet implémente un moteur de recherche en C++ basé sur l'algorithme MaxScore avec parcours guidé (Guided Traversal). Il a été conçu pour évaluer efficacement deux stratégies de classement (GT et GTI) sur les collections MSMARCO Dev, TREC 2019 et TREC 2020.

---

## 1. Description des Fichiers du Projet

### Arborescence du Dépôt
Voici la structure visuelle des répertoires et fichiers du projet (les éléments ignorés par le `.gitignore` ne sont pas affichés) :

```text
.
├── CMakeLists.txt
├── README.md
├── article.pdf
├── context.md
├── debug_coverage.py
├── debug_ndcg.py
├── debug_runs.py
├── debug_scores.py
├── inspect_json.py
├── main.py
├── res.md
└── src/
    ├── Float16.hpp
    ├── IndexReader.cpp
    ├── IndexReader.hpp
    ├── MaxScoreGT.cpp
    ├── MaxScoreGT.hpp
    ├── Metrics.cpp
    ├── Metrics.hpp
    ├── PostingIterator.cpp
    ├── PostingIterator.hpp
    └── main.cpp
```

---

### Rôle de chaque fichier

#### Code Source C++ (Dossier `src/`)
* **`src/main.cpp`** : Point d'entrée du moteur. Il gère le chargement de l'index binaire en RAM et oriente l'exécution entre le mode d'évaluation automatique par lots (Batch) et le mode console interactif.
* **`src/MaxScoreGT.hpp` / `src/MaxScoreGT.cpp`** : Implémentation du cœur de l'algorithme MaxScore avec parcours guidé :
  * **GT (Guided Traversal)** : Élagage des documents via les bornes supérieures BM25, puis classement final basé uniquement sur DeepImpact.
  * **GTI (Guided Traversal with Interpolation)** : Même élagage, mais classement final basé sur la somme interpolée de BM25 et de DeepImpact.
* **`src/IndexReader.hpp` / `src/IndexReader.cpp`** : Gère le chargement du dictionnaire (`vocab.bin`) dans une table de hachage et utilise la projection en mémoire (`mmap`) pour lire très rapidement le fichier binaire des postings (`posting.bin`).
* **`src/PostingIterator.hpp` / `src/PostingIterator.cpp`** : Permet de parcourir les listes de postings de l'index. Gère l'avancement d'un document à l'autre et intègre la recherche dichotomique (`next_GEQ`) pour sauter les documents non pertinents lors de l'évaluation MaxScore.
* **`src/Metrics.hpp` / `src/Metrics.cpp`** : Module de calcul des métriques pour mesurer l'efficacité de la recherche : NDCG@K, Reciprocal Rank @K (MRR), ainsi que les statistiques de latence (moyenne, médiane et percentile 99).
* **`src/Float16.hpp`** : Contient un convertisseur d'encodage float16 (les scores BM25 et DeepImpact étant stockés sous forme compacte de `uint16_t` dans l'index) vers le format standard float 32 bits.

#### Outils de Compilation et d'Exécution
* **`CMakeLists.txt`** : Script CMake permettant de configurer la compilation de l'exécutable C++ de manière propre et portable.
* **`main.py`** : Script de pilotage principal en Python. Il automatise la tokenisation des requêtes brutes des datasets, exécute le moteur C++ en arrière-plan et calcule les métriques globales.

#### Scripts de Débogage et Analyse
* **`debug_coverage.py`** : Analyse le taux de couverture des requêtes par rapport au dictionnaire des termes.
* **`debug_ndcg.py`** : Vérifie la validité des calculs du NDCG par rapport aux fichiers de jugements de pertinence (Qrels).
* **`debug_runs.py` / `debug_scores.py`** : Permettent de comparer finement les scores et les documents renvoyés entre différentes versions.
* **`inspect_json.py`** : Script utilitaire court pour inspecter les fichiers JSON intermédiaires de métadonnées.

#### Rapports et Documents de Référence
* **`README.md`** : Ce guide de présentation et d'exécution du projet.
* **`article.pdf`** : L'article scientifique de recherche original qui détaille le fonctionnement de l'algorithme MaxScore Guided Traversal.
* **`context.md` / `res.md`** : Notes d'analyse complémentaires et historique des résultats.

---

## 2. Compilation et Exécution

### Compilation du moteur C++
Exécutez ces commandes depuis la racine du dépôt pour générer l'exécutable optimisé :
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
L'exécutable généré se nomme `guided_traversal_engine`.

### Utilisation

#### Mode évaluation par lots (Batch)
Pour exécuter des séries entières de requêtes et générer un fichier de résultats compatible avec TREC eval :
```bash
./guided_traversal_engine <vocab.bin> <posting.bin> <queries_file> <run_name> <output_file>
```

#### Mode console interactif
Pour entrer des requêtes à la main (en utilisant des identifiants numériques de termes séparés par des espaces) :
```bash
./guided_traversal_engine <vocab.bin> <posting.bin>
```
Saisissez par exemple `1283 502 99` puis appuyez sur Entrée (Ctrl+D pour quitter).

---

## 3. Synthèse des Résultats Expérimentaux

Le tableau ci-dessous regroupe les performances obtenues sur les 3 collections d'évaluation évaluées :

| Collection | Stratégie | Latence Moyenne | Latence Médiane | Latence P99 | Métrique de Qualité |
|---|---|---|---|---|---|
| **MSMARCO Dev** | GT | 57.0 ms | 24.4 ms | 559.3 ms | **0.3128** (RR@10) |
| **MSMARCO Dev** | GTI | 49.5 ms | 21.5 ms | 562.7 ms | **0.3073** (RR@10) |
| **TREC 2019** | GT | 20.2 ms | 13.8 ms | 106.4 ms | **0.6299** (NDCG@10) |
| **TREC 2019** | GTI | 19.9 ms | 13.7 ms | 105.6 ms | **0.6205** (NDCG@10) |
| **TREC 2020** | GT | 50.7 ms | 19.6 ms | 555.1 ms | **0.5959** (NDCG@10) |
| **TREC 2020** | GTI | 50.2 ms | 19.2 ms | 553.0 ms | **0.5918** (NDCG@10) |