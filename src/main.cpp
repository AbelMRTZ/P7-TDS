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

    /* Array de bancos para diálogos localizados */
    unsigned int bankIndex = 0;
    static const char* const banks[] = { "assets/FMOD_BANKS/Dialogue_CT.bank", "assets/FMOD_BANKS/Dialogue_T.bank"};
    
    /* Cargamos por defecto el primero de los bancos localizados */
    FMOD::Studio::Bank* localizedBank = nullptr;
    ERRCHECK(soundSystem->loadBankFile(banks[bankIndex], FMOD_STUDIO_LOAD_BANK_NORMAL, &localizedBank) );

    /* **************************
      Creacíon de una instancia del evento 'Dialogue': eventInstance
    **************************** */
    FMOD::Studio::EventDescription* eventDescription = nullptr;
    ERRCHECK(soundSystem->getEvent("event:/Dialogue", &eventDescription) );

    FMOD::Studio::EventInstance* eventInstance = nullptr;
    ERRCHECK(eventDescription->createInstance(&eventInstance));

    /* ****************************
      Definimos el array de claves para las tablas de audio 
      (debe corresponder con los archivos keys.txt que se definieron en FMOD Studio)
    ******************************* */
    unsigned int dialogueIndex = 0;
    static const char* const dialogue[] = {"agree", "disagree", "smoke"};

    /* ****************************
      Asignamos el callback al evento Dialogue.
      
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
      Este bucle realiza tres acciones relacionadas con el evento  Dialogue:
      1. Seleccionar el banco del idioma elegido
      2. Seleccionar la entrada de la tabla a reproducir
      3. Disparar el evento.
     ********************************* */
    int n = 2;
    do
    {
        /*   1. Seleccionar el banco del idioma elegido */
        if (n==0)
        {
            ERRCHECK( localizedBank->unload() );

            bankIndex = (bankIndex < 1) ? bankIndex + 1 : 0;
            ERRCHECK( soundSystem->loadBankFile(banks[bankIndex], FMOD_STUDIO_LOAD_BANK_NORMAL, &localizedBank) );
        }

        /* 2. Seleccionar la entrada de la tabla a reproducir */
        if (n==1)
        {
            dialogueIndex = (dialogueIndex < 2) ? dialogueIndex + 1 : 0;
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

        /* Recordar: ¡¡ llamar a update() !! */
        ERRCHECK(soundSystem->update() );

        std::cout<< "Press 0 to change team" << std::endl;
        std::cout<< "Press 1 to change voice line" << std::endl;
        std::cout<< "Press 2 to play the event" << std::endl;
        std::cout<< "Press 3 for Deletrear (ejercicio 2)" << std::endl;
        std::cout<<std::endl;
        std::cout<<"Team:"<<std::endl;
        std::string out1 = bankIndex == 0 ? ">" : " ";
        std::cout<< out1 << " Counter_Terror team" <<std::endl;
        out1 = bankIndex == 1 ? ">" : " ";
        std::cout<< out1 << " Terror team" <<std::endl;
        std::cout<<std::endl;
        std::cout<<"Line:"<<std::endl;
        out1 = dialogueIndex == 0 ? ">" : " ";
        std::cout<< out1 << " Agree!" <<std::endl;
        out1 = dialogueIndex == 1 ? ">" : " ";
        std::cout<< out1 << " Disagree!" <<std::endl;
        out1 = dialogueIndex == 2 ? ">" : " ";
        std::cout<< out1 << " Smoke!" <<std::endl;
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
