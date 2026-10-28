# Práctica 8 — Diálogos y localización con FMOD

Reparto del trabajo y procedimiento paso a paso.
**No hace falta haber leído el enunciado:** aquí está explicado todo lo necesario.

---

## Índice

1. [Qué hay que entregar](#1-qué-hay-que-entregar)
2. [Conceptos que hay que entender](#2-conceptos-que-hay-que-entender)
3. [Qué hay en la carpeta del proyecto](#3-qué-hay-en-la-carpeta-del-proyecto)
4. [Nombres acordados](#4-nombres-acordados-no-cambiar)
5. [Reparto](#5-reparto)
6. [Parte común: proyecto de FMOD Studio (Persona A)](#6-parte-común-proyecto-de-fmod-studio-persona-a)
7. [Ejercicio 1: Patrullas (Persona A)](#7-ejercicio-1-patrullas-persona-a)
8. [Ejercicio 2: Deletrear (Persona B)](#8-ejercicio-2-deletrear-persona-b)
9. [Integración final (los dos)](#9-integración-final-los-dos)
10. [Errores típicos y cómo resolverlos](#10-errores-típicos-y-cómo-resolverlos)
11. [Checklist de entrega](#11-checklist-de-entrega)

---

## 1. Qué hay que entregar

Un **proyecto de FMOD Studio** con **dos eventos** y un **programa en C++** (basado en el `main.cpp` de ejemplo que ya tenemos en `src/`) que los use.

| Ejercicio | Evento | Qué tiene que hacer el programa |
|---|---|---|
| 1 | `Patrullas` | Dos patrullas atacan un objetivo y se hablan por radio. Si habla **el jugador** (nuestra patrulla), la frase ("Contact!", "Cover me!"…) se oye **limpia**. Si responde **la otra patrulla**, se oye **como por una radio/walkie**. |
| 2 | `Deletrear` | El usuario escribe una palabra por teclado y el programa la **deletrea con el alfabeto radiofónico**. Ejemplo: `ALTO` → "Alpha, Lima, Tango, Oscar". |

Requisitos técnicos que pide el enunciado:

- Ejercicio 1: usar una **audio table localizada** con dos localizaciones llamadas **`Normal`** y **`Radio`**.
- Ejercicio 2: usar una **audio table NO localizada** con los sonidos de `Alphabet01`.
- Los dos ejercicios: reproducir los sonidos con un **Programmer Instrument** y su **callback**.
- Ejercicio 2: la cadena se lee por la **entrada estándar** (no hace falta el menú del ejemplo).

---

## 2. Conceptos que hay que entender

### 2.1 El problema

Un juego con diálogos tiene cientos de frases y muchas veces varios idiomas. Crear un evento de FMOD por frase y por idioma sería inmanejable. FMOD lo resuelve con tres piezas: **audio tables**, **programmer instruments** y **callbacks**.

### 2.2 Audio table (tabla de audio)

- Es una **lista de archivos de audio** asociada a un **banco**. Se crea con clic derecho sobre el banco → *New Audio Table*.
- **Cada banco solo puede tener una audio table.**
- Se le indica una carpeta (*Source directory*) y FMOD incluye automáticamente los audios de esa carpeta. Al compilar los bancos (*Build*), los audios se comprimen y se meten dentro del banco.
- Cada audio tiene una **clave** (un nombre corto) para pedirlo desde el código. Las claves se definen en un fichero **`keys.txt`** dentro de la carpeta, con el formato `clave,archivo`:

  ```
  Contact,DIALOGUE_SQUAD_1Contact.wav
  Coverme,DIALOGUE_SQUAD_1Coverme.wav
  ```

  En el código pediremos `"Contact"`, no el nombre del `.wav`.

### 2.3 Audio table localizada

Es una audio table "multi-idioma":

- En las preferencias del proyecto se definen **localizaciones** (*locales*), cada una con un **código**.
- La tabla localizada **añade el código al final de la ruta de origen**. Si la ruta es `dialogosTDS` y las localizaciones son `Normal` y `Radio`, FMOD buscará en `dialogosTDS/Normal` y en `dialogosTDS/Radio`.
- Al compilar, **un banco localizado genera un archivo por localización**: el banco `Dialogue` produce `Dialogue_Normal.bank` y `Dialogue_Radio.bank`.
- Las **claves son las mismas** en todas las localizaciones. Cambiar de "idioma" consiste en **descargar un banco y cargar el otro**, sin tocar nada más del código.

**El truco del ejercicio 1:** en lugar de idiomas reales usamos `Normal` (voz limpia) y `Radio` (la misma voz filtrada como walkie). Las carpetas ya vienen preparadas con los mismos audios y las mismas claves.

### 2.4 Programmer Instrument

Es un instrumento que se coloca en el timeline de un evento, como cualquier otro, pero **no lleva un sonido fijo**. Cuando el evento llega a él, **le pregunta a nuestro programa qué sonido tiene que reproducir**.

- En FMOD Studio no puede saber qué sonará en el juego. Para escuchar algo al probarlo en Studio se puede arrastrar un audio o la tabla a su campo *Placeholder*. Eso es solo para pruebas: no añade el audio a ningún banco.

### 2.5 Callback

Una **callback** es una función nuestra que FMOD llama automáticamente cuando ocurre algo. Con un programmer instrument se llama dos veces:

1. **`FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND`**: "voy a sonar, dime qué sonido". Nuestra función:
   - busca en la audio table el audio de la clave que queremos (`getSoundInfo(clave)`),
   - crea un `FMOD::Sound` con esa información (`createSound`, de la **Core API**),
   - se lo entrega al instrumento (`props->sound = ...`).
2. **`FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND`**: "ya he terminado". Nuestra función libera el sonido (`sound->release()`).

¿Cómo sabe la callback qué clave toca? Antes de lanzar el evento guardamos un **contexto** en la instancia del evento con `setUserData`. La callback lo recupera con `getUserData`:

```cpp
struct ProgrammerSoundContext {
    FMOD::System* coreSystem;       // Core API: para createSound
    FMOD::Studio::System* system;   // Studio API: para getSoundInfo
    const char* dialogueString;     // la clave que queremos que suene
};
```

En resumen: **cambiar lo que suena = cambiar `dialogueString` y volver a hacer `start()` del evento.**

### 2.6 Flujo completo

```
main:  context.dialogueString = "Contact";
       eventInstance->start();
           │
           ▼
FMOD:  el evento llega al Programmer Instrument
           │  llama a la callback (CREATE_PROGRAMMER_SOUND)
           ▼
callback: getUserData → context
          getSoundInfo("Contact") → busca en la audio table del banco cargado
          createSound(...) → props->sound
           │
           ▼
FMOD:  suena el audio... termina
           │  llama a la callback (DESTROY_PROGRAMMER_SOUND)
           ▼
callback: sound->release()
```

Importante: **hay que llamar a `soundSystem->update()` periódicamente**. Si no, FMOD no procesa nada y no suena.

---

## 3. Qué hay en la carpeta del proyecto

```
P7-TDS/
├── dialogos/            ← Proyecto FMOD Studio del EJEMPLO (localizaciones CT/T). NO es el nuestro.
├── dialogosTDS/         ← Librería de sonidos de la práctica
│   ├── Normal/          ← 6 frases, voz limpia      (+ keys.txt)
│   ├── Radio/           ← las mismas 6, filtradas   (+ keys.txt)
│   └── Alphabet01/      ← Alpha…Zulu, 26 palabras   (+ keys.txt)
└── src/
    ├── main.cpp         ← Código de ejemplo (base para los dos ejercicios)
    ├── common.h/.cpp    ← Utilidades (Common_Sleep, etc.)
    ├── buildandrun.sh   ← Compila, copia assets a bin/ y ejecuta
    ├── assets/FMOD_BANKS/  ← Aquí van los .bank que compilemos
    └── vendor/FMOD/     ← Librerías de FMOD
```

Qué hace el `main.cpp` de ejemplo, para que no haga falta descifrarlo:

1. Inicializa FMOD (Studio + Core).
2. Carga `Master.bank`, `Master.strings.bank` y `Voices.bank` (este contiene el evento).
3. Carga **uno** de los bancos localizados (`Dialogue_CT.bank` o `Dialogue_T.bank`).
4. Crea una instancia del evento `event:/Dialogue`, le asigna el contexto y la callback.
5. Muestra un menú en bucle: `0` cambia de banco (descarga uno y carga el otro), `1` cambia de frase (cambia `dialogueString`), `2` reproduce (`start()`), `8` sale.

Nuestro trabajo es **adaptar esto** a nuestros sonidos y nuestros eventos.

**Compilar y ejecutar:** desde `src/`, ejecutar `./buildandrun.sh`. Hay que lanzarlo desde `src/` porque las rutas de los bancos son relativas (`assets/FMOD_BANKS/...`).

---

## 4. Nombres acordados (no cambiar)

El proyecto de Studio y el código tienen que usar **exactamente** estos nombres. Si alguien cambia uno, avisa al otro.

| Elemento | Nombre exacto |
|---|---|
| Proyecto FMOD Studio | `practica8/` (carpeta nueva junto a `dialogosTDS/`) |
| Localizaciones (código) | `Normal` y `Radio` |
| Evento ejercicio 1 | `event:/Patrullas` |
| Evento ejercicio 2 | `event:/Deletrear` |
| Banco con los eventos | `Voices` → `Voices.bank` |
| Banco localizado (ej. 1) | `Dialogue` → `Dialogue_Normal.bank`, `Dialogue_Radio.bank` |
| Banco no localizado (ej. 2) | `Alphabet` → `Alphabet.bank` |
| Claves ej. 1 | `Contact`, `Coverme`, `Holdtheperimeter`, `ImMoving`, `LetsGo`, `WatchyouSector` |
| Claves ej. 2 | `Alpha`, `Bravo`, `Charlie`, `Delta`, `Echo`, `Foxtrot`, `Golf`, `Hotel`, `India`, `Juliet`, `Kilo`, `Lima`, `Mike`, `November`, `Oscar`, `Papa`, `Quebec`, `Romeo`, `Sierra`, `Tango`, `Uniform`, `Victor`, `Whiskey`, `Xray`, `Yankee`, `Zulu` |

Las claves distinguen mayúsculas y minúsculas: es `Xray` (no `X-ray`) y `WatchyouSector` (con esa ortografía).

---

## 5. Reparto

| Persona | Tareas | Depende de |
|---|---|---|
| **A** | Proyecto de FMOD Studio completo (sección 6) + código del **ejercicio 1** (sección 7) | — |
| **B** | Código del **ejercicio 2** (sección 8) + preparar la función común `playAndWait` | Los bancos que genere A (puede empezar antes, ver 8.4) |
| **Los dos** | Integración en un único `main.cpp`, pruebas y entrega (sección 9) | Todo lo anterior |

**¿Por qué solo A toca el proyecto de Studio?** Los archivos de FMOD Studio son muchos XML con identificadores. Si dos personas lo editan a la vez, los conflictos de git son casi imposibles de resolver. Si B necesita un cambio en Studio, se lo pide a A.

**Orden recomendado:**

1. Los dos leen las secciones 2 y 4.
2. A hace la sección 6 y le pasa los `.bank` a B (por git o directamente) lo antes posible.
3. En paralelo: A hace el ejercicio 1 y B el 2.
4. Juntos: sección 9.

---

## 6. Parte común: proyecto de FMOD Studio (Persona A)

### 6.1 Crear el proyecto

1. FMOD Studio → *File → New*. Guardarlo como `P7-TDS/practica8/practica8.fspro`.
   Tiene que estar al lado de `dialogosTDS/` para poder usar rutas relativas.

### 6.2 Definir las localizaciones

**Qué es esto:** una lista de "idiomas" del proyecto. Un juego normal pondría `Español` e `Inglés`. Nosotros no tenemos idiomas, así que nos los inventamos: un "idioma" llamado `Normal` (voz limpia) y otro llamado `Radio` (voz de walkie).

**Por qué hace falta:** en el paso 6.3 le diremos a FMOD que busque los audios en `dialogosTDS`. FMOD coge esa ruta y le pega detrás cada "idioma" de esta lista:

```
dialogosTDS  +  /Normal  →  dialogosTDS/Normal   (esta carpeta existe)
dialogosTDS  +  /Radio   →  dialogosTDS/Radio    (esta carpeta existe)
```

Si en la lista pusiéramos otra cosa (por ejemplo `Limpio`), FMOD buscaría `dialogosTDS/Limpio`, que no existe, y no encontraría nada.

**Cómo se hace:**

1. Abrir las preferencias: en macOS, menú *FMOD Studio → Preferences* (o `Cmd + ,`); en Windows, *Edit → Preferences*.
2. Ir a la pestaña **Build**.
3. Buscar el apartado **Locales**. Es una lista con dos columnas: **Name** y **Code**.
4. Pulsar **+** dos veces y rellenar:

   | Name | Code |
   |---|---|
   | `Normal` | `Normal` |
   | `Radio` | `Radio` |

5. Si ya había alguna fila de antes, borrarla.

La columna **Name** es solo una etiqueta. La que importa es **Code**: tiene que coincidir letra por letra con el nombre de las carpetas, mayúsculas incluidas. Ese código es también el que aparece al final de los bancos compilados (`Dialogue_Normal.bank`, `Dialogue_Radio.bank`).

### 6.3 Crear los bancos

En la ventana *Banks* (*Window → Banks*):

1. **`Voices`**: banco normal. Aquí irán los dos eventos.
2. **`Dialogue`**: clic derecho → **New Localized Audio Table**.
   - *Source directory*: `../dialogosTDS` (la carpeta **padre** de `Normal` y `Radio`, no una de ellas).
   - *Include sub directories*: **desmarcado**.
   - Comprobar que aparecen las 6 frases con sus claves.
3. **`Alphabet`**: clic derecho → **New Audio Table** (la normal, NO localizada).
   - *Source directory*: `../dialogosTDS/Alphabet01`.
   - Comprobar que aparecen las 26 palabras.

### 6.4 Crear el evento `Patrullas`

1. En el *Event browser*: clic derecho → *New Event → 2D Timeline*. Nombre: `Patrullas`, en la raíz (para que su ruta sea `event:/Patrullas`).
2. En la pista de audio: clic derecho → **Add Programmer Instrument**.
3. Seleccionar el instrumento y en sus propiedades:
   - **Loop: desactivado.**
   - **Async: activado.** Así el sonido suena entero aunque el instrumento sea corto en el timeline, y el evento termina solo cuando acaba el audio.
   - *Placeholder* (opcional): arrastrar un `.wav` de `Normal` para poder escucharlo en Studio.
4. Clic derecho sobre el evento → *Assign to Bank → Voices*.

### 6.5 Crear el evento `Deletrear`

Igual que el anterior: nombre `Deletrear`, un Programmer Instrument con **Loop desactivado y Async activado**, asignado a `Voices`.

> Aunque los dos eventos son iguales por dentro, el enunciado pide uno por ejercicio. Además, si más adelante queremos un efecto distinto en cada uno (por ejemplo, más reverb en el deletreo), se puede añadir sin afectar al otro.

### 6.6 Compilar y copiar los bancos

1. *File → Build* (F7).
2. Los bancos aparecen en `practica8/Build/Desktop/`. Tienen que salir **estos 6**:
   - `Master.bank`, `Master.strings.bank`, `Voices.bank`
   - `Dialogue_Normal.bank`, `Dialogue_Radio.bank`
   - `Alphabet.bank`
3. Copiarlos a `src/assets/FMOD_BANKS/` (se pueden borrar los del ejemplo: `Dialogue_CT.bank` y `Dialogue_T.bank`).
4. Avisar a B.

> **Atajo opcional:** en *Preferences → Build* se puede poner como carpeta de salida `../src/assets/FMOD_BANKS`. Así cada Build los deja directamente donde los lee el programa.

---

## 7. Ejercicio 1: Patrullas (Persona A)

### 7.1 Idea

- Jugador habla → banco `Dialogue_Normal` cargado → suena limpio.
- Otra patrulla habla → banco `Dialogue_Radio` cargado → suena por radio.
- La clave (`"Contact"`, etc.) es la misma en los dos casos. Lo único que cambia es **qué banco localizado está cargado**.

El `main.cpp` de ejemplo **ya hace esto** con CT/T. Solo hay que adaptarlo.

### 7.2 Cambios sobre `main.cpp`

**1. Bancos localizados** (línea ~116):

```cpp
static const char* const banks[] = {
    "assets/FMOD_BANKS/Dialogue_Normal.bank",   // 0: patrulla del jugador
    "assets/FMOD_BANKS/Dialogue_Radio.bank"     // 1: otra patrulla
};
```

**2. Evento** (línea ~126):

```cpp
ERRCHECK(soundSystem->getEvent("event:/Patrullas", &eventDescription));
```

**3. Claves** (línea ~136) y el límite del índice al cambiar de frase (en el ejemplo es `< 2` porque había 3 frases; ahora son 6):

```cpp
static const char* const dialogue[] = {
    "Contact", "Coverme", "Holdtheperimeter", "ImMoving", "LetsGo", "WatchyouSector"
};
const unsigned int NUM_LINES = sizeof(dialogue) / sizeof(dialogue[0]);
// ...
dialogueIndex = (dialogueIndex + 1) % NUM_LINES;
```

**4. Callback: quitar el bucle.** En `createSound` el ejemplo usa `FMOD_LOOP_NORMAL`, y con eso la frase se repetiría sin fin y el evento nunca terminaría. Cambiarlo por `FMOD_LOOP_OFF`:

```cpp
CHECK_RESULT( context->coreSystem->createSound(info.name_or_data,
    FMOD_LOOP_OFF | FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING | info.mode,
    &info.exinfo, &sound) );
```

**5. Textos del menú:** "Team" → "Patrulla del jugador (Normal)" / "Otra patrulla (Radio)", y las 6 frases.

### 7.3 Cuidado al cambiar de banco

- **No se pueden tener cargados `Dialogue_Normal` y `Dialogue_Radio` a la vez.** Son "el mismo banco" en dos versiones y FMOD da error. Siempre hay que hacer `unload()` de uno y después `loadBankFile()` del otro, como en el ejemplo.
- **Si se descarga el banco mientras suena una frase, la frase se corta.** Antes de cambiar hay que esperar a que el evento termine. Para eso está `playAndWait` (la escribe B, ver 8.3; A puede usarla igualmente).

### 7.4 Extra recomendado: conversación automática

Para que se vea claramente que funciona, añadir una opción de menú que simule un diálogo entre patrullas:

```cpp
// Cada paso: quién habla (0 = jugador/Normal, 1 = otra patrulla/Radio) y qué dice
struct Linea { unsigned int banco; const char* clave; };
static const Linea conversacion[] = {
    {0, "Contact"}, {1, "Coverme"}, {0, "ImMoving"},
    {1, "Holdtheperimeter"}, {0, "LetsGo"}, {1, "WatchyouSector"}
};

for (const Linea& l : conversacion) {
    if (l.banco != bankIndex) {                 // cambiar de banco solo si hace falta
        ERRCHECK(localizedBank->unload());
        bankIndex = l.banco;
        ERRCHECK(soundSystem->loadBankFile(banks[bankIndex], FMOD_STUDIO_LOAD_BANK_NORMAL, &localizedBank));
    }
    programmerSoundContext.dialogueString = l.clave;
    std::cout << (bankIndex == 0 ? "[Jugador] " : "[Radio]   ") << l.clave << std::endl;
    playAndWait(soundSystem, eventInstance);
}
```

### 7.5 Cómo comprobar que funciona

- La misma frase en modo Normal suena limpia y en modo Radio suena filtrada (más "metálica", con menos graves).
- Cambiar de modo varias veces no da error.
- La frase suena una vez y para (no se repite).

---

## 8. Ejercicio 2: Deletrear (Persona B)

### 8.1 Idea

- Una única audio table **no localizada** (banco `Alphabet`) con las 26 palabras del alfabeto radiofónico.
- El usuario escribe un texto. Para cada letra se elige la clave correspondiente (`A` → `Alpha`, `B` → `Bravo`…) y se reproduce el evento `Deletrear`.
- Las palabras tienen que sonar **en secuencia**, una detrás de otra, sin solaparse.

### 8.2 Estrategia

Usar **la misma instancia del evento** y repetir para cada letra:

1. `context.dialogueString = alfabeto[letra - 'A'];`
2. `eventInstance->start();`
3. **Esperar a que termine** (estado `STOPPED`) antes de pasar a la siguiente letra.

Como el evento tiene la callback asignada, en cada `start()` FMOD vuelve a llamarla y esta lee la clave nueva del contexto.

### 8.3 Función común `playAndWait`

La usan los dos ejercicios. Ponerla encima de `main()`:

```cpp
// Lanza el evento y no vuelve hasta que ha terminado de sonar
void playAndWait(FMOD::Studio::System* sys, FMOD::Studio::EventInstance* inst)
{
    ERRCHECK(inst->start());
    FMOD_STUDIO_PLAYBACK_STATE state;
    do {
        ERRCHECK(sys->update());      // sin update() FMOD no avanza
        Common_Sleep(20);             // de common.h, para no saturar la CPU
        ERRCHECK(inst->getPlaybackState(&state));
    } while (state != FMOD_STUDIO_PLAYBACK_STOPPED);
}
```

> Para que el evento llegue a `STOPPED` hacen falta dos cosas: el instrumento en Studio **sin Loop** (sección 6.4) y el `createSound` con **`FMOD_LOOP_OFF`** (sección 7.2, punto 4). Si falta cualquiera de las dos, se quedará en la primera letra para siempre.

### 8.4 Código del ejercicio

```cpp
#include <cctype>   // toupper, isalpha

// Cargar el banco del alfabeto (junto a Master, Strings y Voices)
FMOD::Studio::Bank* alphabetBank = nullptr;
ERRCHECK(soundSystem->loadBankFile("assets/FMOD_BANKS/Alphabet.bank",
                                   FMOD_STUDIO_LOAD_BANK_NORMAL, &alphabetBank));

// Evento y callback (igual que en el ejemplo)
FMOD::Studio::EventDescription* deletrearDesc = nullptr;
ERRCHECK(soundSystem->getEvent("event:/Deletrear", &deletrearDesc));
FMOD::Studio::EventInstance* deletrear = nullptr;
ERRCHECK(deletrearDesc->createInstance(&deletrear));

ProgrammerSoundContext ctx;
ctx.system = soundSystem;
ctx.coreSystem = coreSystem;
ctx.dialogueString = nullptr;
ERRCHECK(deletrear->setUserData(&ctx));
ERRCHECK(deletrear->setCallback(programmerSoundCallback,
    FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND |
    FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND));

static const char* const alfabeto[26] = {
    "Alpha","Bravo","Charlie","Delta","Echo","Foxtrot","Golf","Hotel","India",
    "Juliet","Kilo","Lima","Mike","November","Oscar","Papa","Quebec","Romeo",
    "Sierra","Tango","Uniform","Victor","Whiskey","Xray","Yankee","Zulu"
};

// Bucle: leer una línea y deletrearla; línea vacía para salir
std::string texto;
while (true) {
    std::cout << "Texto a deletrear (vacio para salir): ";
    if (!std::getline(std::cin, texto) || texto.empty()) break;

    for (char c : texto) {
        if (std::isalpha((unsigned char)c)) {
            int i = std::toupper((unsigned char)c) - 'A';
            if (i >= 0 && i < 26) {          // descarta ñ, acentos, etc.
                ctx.dialogueString = alfabeto[i];
                std::cout << alfabeto[i] << " " << std::flush;
                playAndWait(soundSystem, deletrear);
            }
        } else if (c == ' ') {
            Common_Sleep(400);               // pausa entre palabras
        }
        // números y signos: se ignoran (no hay audios para ellos)
    }
    std::cout << std::endl;
}

ERRCHECK(alphabetBank->unload());
```

**Se puede empezar antes de tener los bancos:** comentar `playAndWait(...)` y dejar solo el `std::cout`. Así se comprueba que `ALTO` imprime `Alpha Lima Tango Oscar`. Cuando A pase los bancos, se descomenta.

### 8.5 Cómo comprobar que funciona

- `ALTO` suena "Alpha, Lima, Tango, Oscar", en orden y sin solaparse.
- Minúsculas (`alto`) funcionan igual.
- Espacios hacen una pausa; números y signos se ignoran sin error.
- Se pueden deletrear varias palabras seguidas sin reiniciar el programa.

---

## 9. Integración final (los dos)

Unir todo en un único `src/main.cpp`:

```
main()
 ├─ Inicializar FMOD
 ├─ Cargar Master, Master.strings, Voices, Alphabet y Dialogue_Normal
 ├─ Crear instancias de Patrullas y Deletrear (cada una con su contexto y la callback)
 ├─ Menú principal:
 │    1) Ejercicio 1: Patrullas  → menú del ejemplo adaptado + conversación automática
 │    2) Ejercicio 2: Deletrear  → bucle de getline de la sección 8.4
 │    0) Salir
 └─ Liberar instancias, descargar bancos (unloadAll)
```

Detalles a tener en cuenta al unir:

- **Mezclar `std::cin >> n` con `std::getline`:** después de `cin >> n` queda un salto de línea en el buffer y el primer `getline` devuelve una cadena vacía. Antes de entrar en el ejercicio 2, hacer `std::cin.ignore(10000, '\n');`.
- `Alphabet.bank` y `Dialogue_*.bank` **sí** pueden estar cargados a la vez: son bancos distintos, cada uno con su propia audio table. Lo que no se puede es cargar `Dialogue_Normal` y `Dialogue_Radio` juntos.
- Cada evento con **su propio contexto**, para que no se pisen las claves.
- Probar el programa completo en el ordenador de los dos.

### Git

El repositorio todavía no tiene commits. Antes de empezar:

1. Crear un `.gitignore`:
   ```
   .DS_Store
   **/.unsaved/
   **/.cache/
   **/.user/
   src/bin/
   ```
2. Primer commit con todo lo actual.
3. Cada uno trabaja en su rama (`ej1-patrullas`, `ej2-deletrear`) y se fusionan en la integración.
4. **Solo A hace commits del proyecto `practica8/`.**

---

## 10. Errores típicos y cómo resolverlos

| Síntoma | Causa probable | Solución |
|---|---|---|
| `FMOD_ERR_FILE_NOTFOUND` al cargar un banco | Ejecutado desde otra carpeta, o el banco no está en `assets/FMOD_BANKS` | Ejecutar `buildandrun.sh` desde `src/`; comprobar que los 6 `.bank` están copiados |
| `FMOD_ERR_EVENT_NOT_FOUND` en `getEvent` | Nombre del evento distinto, evento no asignado a `Voices`, o falta `Master.strings.bank` | Revisar la sección 4 y la 6.4 |
| Error en `getSoundInfo` | Clave mal escrita (distingue mayúsculas) o el banco con la tabla no está cargado | Comparar con `keys.txt`; comprobar qué bancos hay cargados |
| No se generan `Dialogue_Normal/Radio.bank` | Código de localización ≠ nombre de carpeta, o la tabla apunta a `Normal/` en vez de a la carpeta padre | Sección 6.2 y 6.3 |
| La tabla localizada sale vacía en Studio | Ruta de origen incorrecta | Usar `../dialogosTDS` (relativa a la carpeta del `.fspro`) |
| La frase se repite sin parar / se queda en la primera letra | Loop activado | Instrumento sin Loop en Studio **y** `FMOD_LOOP_OFF` en `createSound` |
| Error al cambiar a Radio | Se intentó cargar sin descargar Normal | `unload()` antes de `loadBankFile()` |
| La frase se corta al cambiar de patrulla | Se descargó el banco mientras sonaba | Usar `playAndWait` antes de cambiar |
| No suena nada pero no hay errores | Falta llamar a `update()` | Llamar a `soundSystem->update()` en el bucle |
| El primer texto del ejercicio 2 sale vacío | Salto de línea pendiente de `cin >> n` | `std::cin.ignore(10000, '\n')` |

---

## 11. Checklist de entrega

**Proyecto FMOD Studio (A)**
- [ ] Localizaciones `Normal` y `Radio` definidas
- [ ] Banco `Dialogue` con audio table **localizada** apuntando a `dialogosTDS`
- [ ] Banco `Alphabet` con audio table **no localizada** apuntando a `Alphabet01`
- [ ] Eventos `Patrullas` y `Deletrear`, cada uno con un Programmer Instrument (sin loop, async), en el banco `Voices`
- [ ] Build genera los 6 bancos

**Ejercicio 1 (A)**
- [ ] Las 6 frases se pueden elegir y reproducir
- [ ] Patrulla del jugador = Normal (limpio), otra patrulla = Radio (filtrado)
- [ ] Cambiar de patrulla funciona sin errores ni cortes
- [ ] (Extra) Conversación automática

**Ejercicio 2 (B)**
- [ ] Lee la cadena por entrada estándar
- [ ] Deletrea en orden y sin solapes (`ALTO` → Alpha, Lima, Tango, Oscar)
- [ ] Acepta minúsculas, ignora caracteres no válidos

**Final (los dos)**
- [ ] Un único `main.cpp` con menú para los dos ejercicios
- [ ] Compila y funciona en los dos ordenadores
- [ ] Proyecto FMOD + código + bancos subidos
