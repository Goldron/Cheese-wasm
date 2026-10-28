# Cheese-wasm

Le moteur d'échecs **Cheese** compilé en **WebAssembly**, tel qu'il est utilisé par le jeu
[CocoChess](https://cocochess.com) : il tourne dans le navigateur du joueur et anime les bots.

Ce dépôt contient le code source correspondant au moteur distribué par CocoChess, comme le demande la
licence GNU GPL version 3 : les sources d'origine du moteur, les fichiers modifiés et le script de
compilation. Le moteur compilé à partir de ce dépôt est identique, octet pour octet, à celui en
service (voir [Vérifier le moteur en service](#vérifier-le-moteur-en-service)).

*English: source code of the Cheese chess engine (GPL v3) as compiled to WebAssembly and served by
CocoChess, with the exact build script. Rebuilding gives byte-identical files.*

## Contenu

| Chemin | Contenu |
|---|---|
| `cheese/` | sources de Cheese 3.3.2, **sans aucune modification** |
| `wasm/preamble.js` | préambule JavaScript de l'auteur (`cheese/js/preamble.js`), modifié pour Emscripten 4 et plus |
| `build.sh` | compilation en WebAssembly → `dist/cheese.js` et `dist/cheese.wasm` |
| `LICENSE` | GNU General Public License, version 3 |

## Origine des sources

- Moteur : **Cheese**, © 2006-2025 Patrice Duhamel, https://cheesechess.net/
- Dépôt d'origine : https://gitlab.com/cheesechess/cheese
- Version : 3.3.2, étiquette `v3.3.2`, commit `beee1bd96ad1`

Le dossier `cheese/` est une copie exacte de cette version.

## Modifications

Les sources du moteur ne sont pas modifiées. Seules la compilation et le préambule diffèrent de ce que
prévoit l'auteur pour Emscripten (`cheese/app/CMakeLists.txt`) :

1. **Préambule** (`wasm/preamble.js`) : un point-virgule ajouté en tête du fichier. Emscripten 4 et
   plus écrit juste avant le préambule une ligne `"em-pthread"` sans point-virgule ; la parenthèse
   ouvrante du préambule en faisait un appel de fonction (erreur `"em-pthread" is not a function`).
2. **Options de compilation** : sans l'instruction POPCNT (`-DUSE_POPCOUNT=OFF`) et sans tables de
   finales (`-DUSE_TABLEBASES=OFF`), indisponibles dans une page web. Les options d'Emscripten sont
   celles de l'auteur.
3. **Sortie** : `dist/cheese.js` et `dist/cheese.wasm`, sous un nom fixe.

## Compiler

Prérequis : [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) (version utilisée
pour le moteur en service : 6.0.13), CMake 3.15 ou plus et make.

```bash
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk && ./emsdk install 6.0.13 && ./emsdk activate 6.0.13
cd <ce dépôt>
./build.sh                 # ou EMSDK=<dossier de emsdk> ./build.sh
```

Le script compile dans `build/`, écrit `dist/cheese.js` et `dist/cheese.wasm`, puis affiche leurs
empreintes SHA-256. CMake réécrit `cheese/src/makeconfig.h` pendant la compilation ; le script remet
ensuite le fichier d'origine en place.

## Vérifier le moteur en service

Compilés avec Emscripten 6.0.13, les fichiers ont ces empreintes SHA-256, identiques à celles des
fichiers servis par CocoChess dans `engine/` :

```
e2529bc59d44b7bb598c12403de9a4988b2ba2cd6e823402d195eebb59e623b3  cheese.js
6c0948400f7e0db73ebecac6bbd4a87bfe90fe853124052dfead96bb51af22d2  cheese.wasm
```

Pour comparer : `curl -s https://cocochess.com/engine/cheese.wasm | sha256sum`.

## Utiliser le moteur

Le fichier `cheese.js` est un module ES : sa fonction par défaut crée le moteur, qui se pilote en
[UCI](https://www.wbec-ridderkerk.nl/html/UCIProtocol.html) par du texte.

```js
const { default: Cheese } = await import('./cheese.js');
const engine = await Cheese();
engine.addMessageListener((line) => console.log(line)); // réponses du moteur, ligne par ligne
engine.processCommand('uci');                           // réponse attendue : « uciok »
engine.processCommand('position startpos moves e2e4');
engine.processCommand('go movetime 1000');              // réponse : « bestmove … »
```

Au démarrage, le moteur affiche son nom avant de lire ses commandes : un premier `uci` peut être
perdu. CocoChess le renvoie toutes les 200 ms jusqu'à recevoir `uciok`.

Le moteur utilise des threads WebAssembly (`SharedArrayBuffer`) : la page qui le charge doit être
*cross-origin isolated*, avec les en-têtes HTTP
`Cross-Origin-Opener-Policy: same-origin` et `Cross-Origin-Embedder-Policy: require-corp`.

## Licence

Cheese est un logiciel libre, distribué sous la GNU General Public License version 3 ou ultérieure
(`LICENSE`), © 2006-2025 Patrice Duhamel. Les fichiers ajoutés ici (`build.sh`, modifications de
`wasm/preamble.js`, ce README) sont distribués sous la même licence.

Ce dépôt est publié par l'éditeur de CocoChess. Pour toute question sur le moteur lui-même, consultez
le site et le dépôt de son auteur.
