#include <fmod.hpp>
#include <fmod_studio.hpp>
#include <fmod_errors.h>
#include <string>
#include <iostream>
#include <common.h>
#include <limits>

void ERRCHECK_FMOD (FMOD_RESULT result, const char * file, int line)
{
	if(result != FMOD_OK)
	{
        std::cerr << FMOD_ErrorString(result) << std::endl;
		exit(-1);
	}
}
#undef ERRCHECK
#define ERRCHECK(_result) ERRCHECK_FMOD(_result, __FILE__, __LINE__)

FMOD_RESULT F_CALLBACK programmerSoundCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* event, void* parameters);

struct ProgrammerSoundContext
{
    FMOD::System* coreSystem;
    FMOD::Studio::System* system;
    const char* dialogueString;
};


#define CHECK_RESULT(op) \
    { \
        FMOD_RESULT res = (op); \
        if (res != FMOD_OK) \
        { \
            return res; \
        } \
    }



FMOD_RESULT F_CALLBACK programmerSoundCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* event, void *parameters)
{
    /* Casting de API C --> API C++ */
    FMOD::Studio::EventInstance* eventInstance = (FMOD::Studio::EventInstance*)event;

    /* *********************************
      El Programmer Instrument se va a reproducir. 
      Hay que indicarle un objeto de tipo FMOD::Sound asociado al sonido que queremos que reproduzca, que habremos indicado previamente en la
      estructura de datos ProgrammerSoundContext previamente asociada al evento
     ********************************** */
    if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
    {
        /* Más casting... */
        FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES* props = (FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*)parameters;

        // Recuperamos la infromación de contexto (ProgrammerSoundContext) asociada al evento
        ProgrammerSoundContext* context = nullptr;
        CHECK_RESULT( eventInstance->getUserData((void**)&context) );

        // Buscar el archivo de audio a partir de su clave asociada en la tabla de audio
        FMOD_STUDIO_SOUND_INFO info;
        CHECK_RESULT( context->system->getSoundInfo(context->dialogueString, &info) );

        // Creamos el objeto FMOD::Sound
        // info.exinfo: Metadatos acerca del sonido -> https://www.fmod.com/docs/2.02/api/core-api-system.html#fmod_createsoundexinfo
        FMOD::Sound* sound = nullptr;
        CHECK_RESULT( context->coreSystem->createSound(info.name_or_data, FMOD_LOOP_OFF | FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING | info.mode, &info.exinfo, &sound) );

        // Asociamos el objeto FMOD::Sound que queremos que se reproduzca al Programmer Instrument
        props->sound = (FMOD_SOUND*)sound;
        props->subsoundIndex = info.subsoundindex;
    }
    /* *********************************
     El Programmer Instrument ya se ha reproducido. Toca liberar memoria.
     *********************************** */
    else if (type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
    {
        FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES* props = (FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*)parameters;

        // Obtener el objeto FMOD::Sound
        FMOD::Sound* sound = (FMOD::Sound*)props->sound;

        // Liberar memoria
        CHECK_RESULT( sound->release() );
    }

    return FMOD_OK;
}

// Compartida por ambos ejercicios: no cambiar de clave hasta terminar el audio.
void playAndWait(FMOD::Studio::System* sys, FMOD::Studio::EventInstance* inst)
{
    ERRCHECK(inst->start());
    FMOD_STUDIO_PLAYBACK_STATE state;
    do
    {
        ERRCHECK(sys->update());
        Common_Sleep(20);
        ERRCHECK(inst->getPlaybackState(&state));
    } while (state != FMOD_STUDIO_PLAYBACK_STOPPED);
}

void ejecutarDeletrear(FMOD::Studio::System* soundSystem, FMOD::System* coreSystem)
{
    FMOD::Studio::Bank* alphabetBank = nullptr;
    ERRCHECK(soundSystem->loadBankFile("assets/FMOD_BANKS/Alphabet.bank",
                                     FMOD_STUDIO_LOAD_BANK_NORMAL, &alphabetBank));

    FMOD::Studio::EventDescription* deletrearDesc = nullptr;
    ERRCHECK(soundSystem->getEvent("event:/Deletrear", &deletrearDesc));
    FMOD::Studio::EventInstance* deletrear = nullptr;
    ERRCHECK(deletrearDesc->createInstance(&deletrear));

    static const char* const alfabeto[26] = {
        "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf", "Hotel",
        "India", "Juliet", "Kilo", "Lima", "Mike", "November", "Oscar", "Papa",
        "Quebec", "Romeo", "Sierra", "Tango", "Uniform", "Victor", "Whiskey",
        "Xray", "Yankee", "Zulu"
    };
    // Contexto propio, independiente del utilizado por las patrullas.
    ProgrammerSoundContext ctx = {coreSystem, soundSystem, alfabeto[0]};
    ERRCHECK(deletrear->setUserData(&ctx));
    ERRCHECK(deletrear->setCallback(programmerSoundCallback,
        FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND |
        FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND));

    std::string texto;
    while (true)
    {
        std::cout << "Texto a deletrear (vacio para volver): " << std::flush;
        if (!std::getline(std::cin, texto) || texto.empty()) break;

        for (unsigned char c : texto)
        {
            // Solo A-Z: los audios no incluyen numeros, tildes ni enie.
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            if (c >= 'A' && c <= 'Z')
            {
                ctx.dialogueString = alfabeto[c - 'A'];
                std::cout << ctx.dialogueString << " " << std::flush;
                playAndWait(soundSystem, deletrear);
            }
            else if (c == ' ')
            {
                Common_Sleep(400);
            }
        }
        std::cout << std::endl;
    }

    // Procesar la liberacion antes de que el contexto local deje de existir.
    ERRCHECK(deletrear->release());
    ERRCHECK(soundSystem->flushCommands());
    ERRCHECK(alphabetBank->unload());
    ERRCHECK(soundSystem->flushCommands());
}

int main(int argc, char* argv[])
{
    /* ************************
     Inicialización de FMOD 
    ************************ */
    FMOD::System* coreSystem;
    FMOD::Studio::System* soundSystem;
    std::string dialogueString;
    
    ERRCHECK(FMOD::Studio::System::create(&soundSystem));
    ERRCHECK(soundSystem->getCoreSystem(&coreSystem));
    ERRCHECK(coreSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));
    ERRCHECK(coreSystem->setOutput(FMOD_OUTPUTTYPE_AUTODETECT));
    ERRCHECK(soundSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0));

    /* ************************
      Cargar bancos 'normales'
    ************************ */
    FMOD::Studio::Bank * masterBank = nullptr;
    FMOD::Studio::Bank* stringsBank = nullptr;
    FMOD::Studio::Bank* voicesBank = nullptr;

    ERRCHECK(soundSystem->loadBankFile("assets/FMOD_BANKS/Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &masterBank) );
    ERRCHECK(soundSystem->loadBankFile("assets/FMOD_BANKS/Master.strings.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &stringsBank) );
    ERRCHECK(soundSystem->loadBankFile("assets/FMOD_BANKS/Voices.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &voicesBank) );
    
    // Permite probar la parte B sin cargar los bancos/eventos del ejercicio 1.
    if (argc > 1 && std::string(argv[1]) == "--deletrear")
    {
        ejecutarDeletrear(soundSystem, coreSystem);
        ERRCHECK(soundSystem->unloadAll());
        ERRCHECK(soundSystem->release());
        return 0;
    }

    /* Array de bancos para diálogos localizados:
       0 = patrulla del jugador (voz limpia), 1 = otra patrulla (voz por radio) */
    unsigned int bankIndex = 0;
    static const char* const banks[] = { "assets/FMOD_BANKS/Dialogue_Normal.bank", "assets/FMOD_BANKS/Dialogue_Radio.bank"};
    static const char* const teamNames[] = { "Patrulla del jugador (Normal)", "Otra patrulla (Radio)" };

    /* Cargamos por defecto el primero de los bancos localizados */
    FMOD::Studio::Bank* localizedBank = nullptr;
    ERRCHECK(soundSystem->loadBankFile(banks[bankIndex], FMOD_STUDIO_LOAD_BANK_NORMAL, &localizedBank) );

    /* Las dos versiones de un banco localizado no pueden estar cargadas a la vez:
       hay que descargar una antes de cargar la otra. Solo se llama con el evento parado. */
    auto cambiarPatrulla = [&](unsigned int nuevoIndex)
    {
        if (nuevoIndex == bankIndex) return;
        ERRCHECK( localizedBank->unload() );
        bankIndex = nuevoIndex;
        ERRCHECK( soundSystem->loadBankFile(banks[bankIndex], FMOD_STUDIO_LOAD_BANK_NORMAL, &localizedBank) );
    };

    /* **************************
      Creacíon de una instancia del evento 'Patrullas': eventInstance
    **************************** */
    FMOD::Studio::EventDescription* eventDescription = nullptr;
    ERRCHECK(soundSystem->getEvent("event:/Patrullas", &eventDescription) );

    FMOD::Studio::EventInstance* eventInstance = nullptr;
    ERRCHECK(eventDescription->createInstance(&eventInstance));

    /* ****************************
      Definimos el array de claves para las tablas de audio
      (debe corresponder con los archivos keys.txt que se definieron en FMOD Studio)
    ******************************* */
    unsigned int dialogueIndex = 0;
    static const char* const dialogue[] = {"Contact", "Coverme", "Holdtheperimeter", "ImMoving", "LetsGo", "WatchyouSector"};
    static const char* const dialogueText[] = {"Contact!", "Cover me!", "Hold the perimeter!", "I'm moving!", "Let's go!", "Watch your sector!"};
    const unsigned int numLines = sizeof(dialogue) / sizeof(dialogue[0]);

    /* Conversación de ejemplo entre las dos patrullas: {quién habla, qué dice} */
    struct Linea { unsigned int team; unsigned int line; };
    static const Linea conversacion[] = {
        {0, 0}, {1, 1}, {0, 3}, {1, 2}, {0, 4}, {1, 5}
    };

    /* ****************************
      Asignamos el callback al evento Patrullas.
      
      Ver https://www.fmod.com/docs/2.02/api/studio-api-eventinstance.html#fmod_studio_event_callback_type
      para los distintos tipos de callback definidos en FMOD (FMOD_STUDIO_EVENT_CALLBACK_TYPE)
    ***************************** */
    ProgrammerSoundContext programmerSoundContext;
    programmerSoundContext.system = soundSystem;
    programmerSoundContext.coreSystem = coreSystem;
    programmerSoundContext.dialogueString = dialogue[dialogueIndex];

    ERRCHECK( eventInstance->setUserData(&programmerSoundContext) );
    ERRCHECK( eventInstance->setCallback(programmerSoundCallback, FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND | FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND) );

    /* ********************************
      Este bucle realiza las acciones relacionadas con el evento Patrullas:
      1. Seleccionar la patrulla que habla (banco Normal o Radio)
      2. Seleccionar la entrada de la tabla a reproducir
      3. Disparar el evento.
      4. Reproducir una conversación entre las dos patrullas
     ********************************* */
    int n = -1;
    do
    {
        /*   1. Seleccionar la patrulla que habla */
        if (n==0)
        {
            cambiarPatrulla(bankIndex == 0 ? 1 : 0);
        }

        /* 2. Seleccionar la entrada de la tabla a reproducir */
        if (n==1)
        {
            dialogueIndex = (dialogueIndex + 1) % numLines;
            programmerSoundContext.dialogueString = dialogue[dialogueIndex];
        }

        /* 3. Disparar el evento. */
        if (n==2)
        {
            playAndWait(soundSystem, eventInstance);
        }
        
        if (n == 3)
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            ejecutarDeletrear(soundSystem, coreSystem);
        }

        /* 4. Conversación: cada línea con el banco de la patrulla que la dice */
        if (n == 4)
        {
            unsigned int prevTeam = bankIndex, prevLine = dialogueIndex;
            for (const Linea& l : conversacion)
            {
                cambiarPatrulla(l.team);
                programmerSoundContext.dialogueString = dialogue[l.line];
                std::cout << (l.team == 0 ? "[Jugador] " : "[Radio]   ") << dialogueText[l.line] << std::endl;
                playAndWait(soundSystem, eventInstance);
            }
            // Volver a la selección que había en el menú
            cambiarPatrulla(prevTeam);
            dialogueIndex = prevLine;
            programmerSoundContext.dialogueString = dialogue[dialogueIndex];
        }

        /* Recordar: ¡¡ llamar a update() !! */
        ERRCHECK(soundSystem->update() );

        std::cout<< "Press 0 to change team" << std::endl;
        std::cout<< "Press 1 to change voice line" << std::endl;
        std::cout<< "Press 2 to play the event" << std::endl;
        std::cout<< "Press 3 for Deletrear (ejercicio 2)" << std::endl;
        std::cout<< "Press 4 for a conversation between both patrols" << std::endl;
        std::cout<<std::endl;
        std::cout<<"Team:"<<std::endl;
        for (unsigned int i = 0; i < 2; i++)
            std::cout << (bankIndex == i ? ">" : " ") << " " << teamNames[i] << std::endl;
        std::cout<<std::endl;
        std::cout<<"Line:"<<std::endl;
        for (unsigned int i = 0; i < numLines; i++)
            std::cout << (dialogueIndex == i ? ">" : " ") << " " << dialogueText[i] << std::endl;
        std::cout<<std::endl;
        std::cout<<"Press 8 to quit"<<std::endl;

        if (!(std::cin >> n))
        {
            if (std::cin.eof()) break;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            n = -1;
        }
        std::cout << "\x1B[2J\x1B[H";
    }while (n != 8);

    ERRCHECK(eventDescription->releaseAllInstances());
    ERRCHECK(masterBank->unload());
    ERRCHECK(stringsBank->unload());
    ERRCHECK(localizedBank->unload());
    ERRCHECK(voicesBank->unload());
    ERRCHECK(soundSystem->unloadAll());
    ERRCHECK(soundSystem->release());

    return 0;
}
