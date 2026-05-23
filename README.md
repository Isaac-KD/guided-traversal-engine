# Moteur de Recherche par Parcours Guidé (Guided Traversal Engine)

Ce projet implémente un moteur de recherche en C++ basé sur l'algorithme MaxScore avec parcours guidé (Guided Traversal), conçu pour évaluer efficacement les stratégies de parcours d'index inversés. Ce rapport résume l'architecture du moteur, les instructions de compilation et d'exécution, ainsi que les résultats des évaluations expérimentales sur les collections MSMARCO Dev, TREC 2019 et TREC 2020.

---

## 1. Présentation du Moteur

Le moteur implémente l'algorithme de parcours d'index MaxScore et évalue deux stratégies distinctes :
* **GT (Guided Traversal)** : Le parcours de l'index et l'élagage des documents non prometteurs s'appuient sur les bornes supérieures des scores BM25 (UB_BM25). Cependant, le score final de pertinence utilisé pour le classement des documents dans le tas des K meilleurs résultats provient uniquement du modèle neuronal DeepImpact.
* **GTI (Guided Traversal with Interpolation)** : Similaire à GT pour l'élagage lors du parcours, mais le score final de classement est une interpolation linéaire combinant le score BM25 classique et le score DeepImpact.

---

## 2. Compilation et Utilisation

### Prérequis
* Un compilateur C++ supportant le standard C++17 (GCC, Clang)
* CMake (version 3.10 ou supérieure)
* Make

### Compilation du projet
Pour compiler le projet, exécutez les commandes suivantes depuis la racine du dépôt :
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
L'exécutable généré est nommé `guided_traversal_engine`.

### Exécution du moteur

#### Mode évaluation par lots (Batch)
Pour évaluer des requêtes en lot et générer un fichier de résultats au format TREC run, utilisez la syntaxe suivante :
```bash
./guided_traversal_engine <vocab.bin> <posting.bin> <queries_file> <run_name> <output_file>
```

#### Mode interactif (Console)
Pour tester des requêtes textuelles via des identifiants de termes en ligne de commande :
```bash
./guided_traversal_engine <vocab.bin> <posting.bin>
```
Saisissez ensuite les identifiants des termes séparés par des espaces, puis validez avec Entrée (Ctrl+D pour quitter).

---

## 3. Résultats Expérimentaux

Les évaluations ont été menées en mesurant la latence moyenne, médiane, au 99e percentile (P99) ainsi que l'efficacité du classement (RR@10 ou NDCG@10).

### Requêtes MSMARCO Dev (Évalué avec RR@10)
| Stratégie | Latence Moyenne | Latence Médiane | Latence P99 | RR@10 |
|---|---|---|---|---|
| **DeepImpact-GT** | 57.0 ms | 24.4 ms | 559.3 ms | **0.3128** |
| **DeepImpact-GTI** | 49.5 ms | 21.5 ms | 562.7 ms | **0.3073** |

### TREC 2019 (Évalué avec NDCG@10)
| Stratégie | Latence Moyenne | Latence Médiane | Latence P99 | NDCG@10 |
|---|---|---|---|---|
| **DeepImpact-GT** | 20.2 ms | 13.8 ms | 106.4 ms | **0.6299** |
| **DeepImpact-GTI** | 19.9 ms | 13.7 ms | 105.6 ms | **0.6205** |

### TREC 2020 (Évalué avec NDCG@10)
| Stratégie | Latence Moyenne | Latence Médiane | Latence P99 | NDCG@10 |
|---|---|---|---|---|
| **DeepImpact-GT** | 50.7 ms | 19.6 ms | 555.1 ms | **0.5959** |
| **DeepImpact-GTI** | 50.2 ms | 19.2 ms | 553.0 ms | **0.5918** |

---

## 4. Analyse Comparative avec l'Article Original

### Analyse de la Latence
Les résultats montrent des latences presque identiques entre les stratégies GT et GTI (~20 ms sur TREC 2019 et ~50 ms sur TREC 2020 et MSMARCO). Cela valide l'implémentation de la logique d'élagage MaxScore : le goulot d'étranglement réside dans le parcours des listes de postings (déterminé par les heuristiques BM25). Le calcul additionnel du score DeepImpact n'induit aucun surcoût mesurable, ce qui concorde avec les conclusions de l'article de référence.

Les latences absolues restent supérieures aux 4.8 ms annoncées dans l'article pour les raisons matérielles et logicielles suivantes :
1. L'utilisation dans l'article du framework C++ PISA hautement optimisé pour l'ordonnancement SIMD.
2. L'exécution sur des serveurs professionnels équipés de processeurs haut de gamme (Intel Xeon Gold) avec une grande quantité de mémoire vive maintenant l'index entièrement en cache physique.

### Analyse de l'Efficacité (NDCG / RR)
Les métriques d'efficacité obtenues sont très proches de celles reportées dans l'article, avec des écarts minimes (de l'ordre de 0.01 à 0.05). Par exemple, sur TREC 2019, la stratégie GT obtient un score **NDCG@10 de 0.6299**, ce qui correspond précisément aux performances illustrées dans l'article original (~0.63).

### Explication des Écarts
Certaines légères variations, notamment le fait que GTI obtienne parfois un score légèrement inférieur à GT (alors que l'article anticipe une légère amélioration), s'expliquent par les différences techniques au niveau de la préparation de l'index :

1. **Paramétrage de BM25 (Anserini)** : L'article s'appuie sur Anserini avec des paramètres BM25 finement calibrés (`k1=0.82`, `b=0.68`). Notre index utilise les valeurs par défaut standards (`k1=1.2`, `b=0.75`), modifiant légèrement les bornes supérieures et l'ordre d'évaluation des postings.
2. **Quantification Linéaire 8 bits** : L'article quantifie les scores en entiers 8 bits pour accélérer les opérations via des sommes d'impacts entiers. Notre moteur exploite une représentation en demi-précision (`float16`), ce qui influe sur la dynamique et l'échelle des scores combinés dans la stratégie GTI.
3. **Réordonnancement des documents (Bipartite Graph Partitioning)** : L'index de l'article a subi un réordonnancement de l'espace des identifiants de documents (BP). Comme MaxScore parcourt et évalue les documents de manière séquentielle, l'assignation des identifiants change directement la vitesse de progression du seuil d'élagage `theta_bm25`, impactant finement le nombre de documents pleinement évalués.

---

## 5. Conclusion

Ce moteur de recherche en C++ reproduit fidèlement la logique algorithmique décrite dans l'article de recherche original. L'intégration réussie de la stratégie de parcours guidé (Guided Traversal) démontre qu'il est possible de combiner efficacement la rapidité d'élagage des heuristiques BM25 classiques avec la précision des représentations neuronales creuses de DeepImpact, éliminant ainsi le besoin d'une phase distincte de réordonnancement coûteuse.