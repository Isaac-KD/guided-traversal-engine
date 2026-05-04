conversation avec Projet :

[19/04/2026 00:04:49] Younes: jpense ya aucun interet a generer un index compatible pisa
[19/04/2026 00:05:00] Younes: si on compte pas utiliser pisa et faire notre propre maxscore dessus
[19/04/2026 00:05:23] Younes: jpense que ca passera de faire notre propre index inversé au format parquet ou pickle un truc comme ca
[19/04/2026 00:07:00] Younes: Prcq jcrois maxscore est implementé en pisa stv utilisé le maxscore de pisa alors la jtrouve ca coherent
[19/04/2026 20:06:30] Isaac: Nan nan .parquet ou pickle
[19/04/2026 20:06:33] Isaac: C’est moi et
[19/04/2026 20:06:35] Isaac: Mort
[19/04/2026 20:06:44] Isaac: En c++ ça va être un galère
[19/04/2026 20:07:00] Isaac: Meter direct en .bin
[19/04/2026 20:07:45] Younes: Tes en retard
[19/04/2026 20:07:59] Isaac: J’avais pas vu
[19/04/2026 20:08:00] Younes: Jtai corrigé ca en pv😭
[19/04/2026 20:08:14] Younes: Gros j'ai réfléchis a la question et je pense tas raison ca pu la merde ce que j'ai proposé et je pense on va faire notre propre index maison ge,re

En gros on va definir vocab.bin

| term_id (uint32) | UB_BM25 (float32) | offset (uint64) | nb_postings (uint32) |

pour chaque terme on aura son UB max et l'offset de la ligne a regardé dans posting 

et posting.bin 

| doc_id (uint32) | bm25 (float16) | deepimpact (float16) |

pour chaque terme on aure plusieurs ligne doc_id similaire mais avec le score bm25 et deepimpact associé

Donc 

t1 -> d1 d2 

devient 

vocab 

id t1 , score max , offset = ligne ou sa commence = 0 , taille max = 2  

posting

d1,score bm ,score deep ( ici offset = 0 )
d2,score bm, score deep  ( ici offset = 8 car 4bytes doc id 2de bm25 et 2de deep donc 2+2+4=8bytes )

Si tes ok implemente maxscore sur cette logique et ca sera plus simple pour nous de cree l'index vu qu'on aura la main sur tout en python
pour les floats exactement j'ai fait a l'oeil on pourra opti et modifié selon nos besoins
[19/04/2026 20:08:18] Younes: @⁨Samy⁩
[19/04/2026 20:08:20] Younes: Lis ca
[19/04/2026 20:08:22] Isaac: Oui oui ça j’ai vu
[19/04/2026 20:08:25] Isaac: J’avais vu l’heure
[19/04/2026 20:08:30] Younes: Comme ca tu sais de quoi on parle
[19/04/2026 20:08:36] Isaac: J’ai cru t’avais changer d’avis
[19/04/2026 20:08:48] Younes: Ptdrr en mode jsuis passe dune bonne idee a une mauvaise 😭
[19/04/2026 20:09:02] Isaac: Oué je sort de la salle j’ai pas tout qui est connecter
[19/04/2026 21:13:40] Samy: Ok bg
[02/05/2026 18:32:25] Younes: les reufs j'ai enfin fini se foutu index apres 1semaine de souffrance zbi
[02/05/2026 18:33:08] Younes: alors du coup ya les scores de bm25 et di pour les 8.8M de msmarco avec le format qu'on avait discuté
[02/05/2026 18:33:38] Younes: les corrections vis a vis de la v1 que jvous avais envoyé deja 8.8 M de doc mais chaque doc a ete expansée avec DOCT5QUERY
[02/05/2026 18:34:05] Younes: de plus di scorait trop de bail a 0 javais genre 96% de zero ptdrr
[02/05/2026 18:34:34] Younes: mtn il score a 80% de non null donc ca marche les scores nulles sont pas anormal dans son comportement il a ete entrainé comme ca
[02/05/2026 18:34:59] Younes: par contre le soucis cest que j'ai pas reussi a avoir exactement le meme nombre que le papier sur le voc
[02/05/2026 18:35:24] Younes: on a 3.6M eux 3.5 je crois jpense ils ont utilisés une tokenization un chouya differente a la notre
[02/05/2026 18:35:39] Younes: et du coup les postings sont aussi un peu different encore une fois du a la tokenisation
[02/05/2026 18:39:11] Younes: https://www.swisstransfer.com/d/7723dfb3-2d5b-48cd-9758-5be534c01107
[02/05/2026 18:39:33] Younes: le lien pour telecharger j'arrive pas a l'envoyer sur whatsapp