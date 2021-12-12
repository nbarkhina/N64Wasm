#include <iostream>

//using STB library to load the PNG files
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>
#include <SDL/SDL_opengl.h>
#else
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#endif

#include <fstream>
#include <string>
#include <sstream>

extern "C" {
#include "neil.h"
}

#include "neil_controller.h"

#ifdef _WIN32
#define USE_XINPUT //COMMENT OUT TO FALL BACK TO SDL JOYSTICK CODE (UNRELIABLE)
#endif

#ifdef USE_XINPUT
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib,"XInput.lib")
XINPUT_STATE state;
int xGamepadIndex = -1;
#endif

#ifdef USE_XINPUT
bool IsPressed(WORD button)
{
    return (state.Gamepad.wButtons & button) != 0;
}
#endif

SDL_Joystick* joyStick = NULL;
SDL_GameController* gameController = NULL;
bool gamepadConnected = false;
int axis0 = 0;
int axis1 = 0;
int axis2 = 0;
int axis3 = 0;
int axis4 = 0;
int axis5 = 0;
Uint8* keyboardState;
struct NeilButtons neilbuttons;
bool loadEep = false;
bool loadSra = false;
bool loadFla = false;
bool showFPS = true;

void connectGamepad()
{
#ifdef USE_XINPUT
    DWORD dwResult;

    if (!gamepadConnected)
    {
        for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
        {

            ZeroMemory(&state, sizeof(XINPUT_STATE));

            // Simply get the state of the controller from XInput.
            dwResult = XInputGetState(i, &state);

            if (dwResult == ERROR_SUCCESS)
            {
                // Controller is connected
                if (!gamepadConnected)
                    printf("controller connected %d\n", i);

                xGamepadIndex = i;
                gamepadConnected = true;
            }
            else
            {
                // Controller is not connected
            }
        }
    }
    else
    {
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        dwResult = XInputGetState(xGamepadIndex, &state);

        if (dwResult != ERROR_SUCCESS)
        {
            gamepadConnected = false;
        }
    }

    return;
#endif
    if (!gamepadConnected)
    {
        //Check for joysticks
        if (SDL_NumJoysticks() < 1)
        {
            // printf("Warning: No joysticks connected!\n");
        }
        else
        {
            for (int i = 0; i < SDL_NumJoysticks(); i++)
            {
                //Load joystick
                SDL_Joystick* tempJoyStick = SDL_JoystickOpen(i);
                const char* name = SDL_JoystickName(tempJoyStick);

                //skip logitech device
                int compare = 0;
                compare = strcmp(name, "Logitech H820e (Vendor: 046d Product: 0a4a)");
                if (compare == 0)
                {
                    printf("skipping logitech device\n");
                }
                else
                {
                    joyStick = tempJoyStick;
                    printf("Connected Joystick %s\n", name);
                    if (SDL_IsGameController(i))
                    {
                        gameController = SDL_GameControllerOpen(i);
                        printf("Connected Connected %s\n", SDL_GameControllerName(gameController));
                    }
                    gamepadConnected = true;
                }

            }


        }

    }

}

enum MY_FONT_SIZE { 
    FONTSIZE_24, 
    FONTSIZE_40
};

TTF_Font* font24;
TTF_Font* font40;
SDL_Color fontcolorWhite = { 255, 255, 255 };
SDL_Color fontcolorRed = { 0, 0, 255 }; //needs to be BGRA becuse of emscripten
Uint32 frameStart, frameTime;
const int FPS   = 60;
const int DELAY = 1000.0f / FPS;
int current_fps = 0;
int current_fps_counter = 0;
Uint32 current_frameStart;
char fps_text[200];
float xLocation = 0.0f;
float yLocation = 0.0f;
unsigned int shaderProgram = 0;
char* loadFile(char* filename);
void limitFPS();
unsigned int initShaders();
char rom_name[100];
bool runApp = true;
int swapCount = 0;
int audioDeviceId = 0;
int vicount = 0;
int currentSwapCount = 0;
int audioBufferQueue = 0;
int currentAudioBufferQueue = 0;
int hadSkip = 0;
int currentHadSkip = 0;
int maxAudioBufferQueue = 0;
int currentMaxAudioBufferQueue = 0;
char toast_message[250];
int toastCounter = 0;

SDL_Window* WindowOpenGL;
void mainLoop();
bool isPressedOnce(bool condition, bool* lastPressedVar);
bool isReleasedOnce(bool condition, bool* lastPressedVar); 

GLuint imageOverlay;
void setupDrawTextOpenGL();
void drawTextOpenGL(const char* text, int x, int y, SDL_Color color, MY_FONT_SIZE fontsize);
void drawOpenglTexture(int x, int y, int w, int h, GLuint imageID);
GLuint loadOpenGLTexture(const char*);
unsigned int vertexBufferDrawText;
float positionsDrawText[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 0.0f,
};
float positionsDrawTextFinal[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 0.0f,
};

//gamepad
int Joy_Mapping_Up = 0;
int Joy_Mapping_Down = 0;
int Joy_Mapping_Left = 0;
int Joy_Mapping_Right = 0;
int Joy_Mapping_Action_A = 0;
int Joy_Mapping_Action_B = 0;
int Joy_Mapping_Action_Start = 0;
int Joy_Mapping_Action_Z = 0;
int Joy_Mapping_Action_L = 0;
int Joy_Mapping_Action_R = 0;
int Joy_Mapping_Menu = 0;

//keyboard
int Mapping_Left = 0;
int Mapping_Right = 0;
int Mapping_Up = 0;
int Mapping_Down = 0;
int Mapping_Action_Start = 0;
int Mapping_Action_CUP = 0;
int Mapping_Action_CDOWN = 0;
int Mapping_Action_CLEFT = 0;
int Mapping_Action_CRIGHT = 0;
int Mapping_Action_Z = 0;
int Mapping_Action_L = 0;
int Mapping_Action_R = 0;
int Mapping_Action_B = 0;
int Mapping_Action_A = 0;
int Mapping_Menu = 0;

int getKeyMapping(std::string line);

void readConfig()
{
    try
    {

        std::ifstream infile("config.txt");
        std::string line;

        std::size_t pos;
        std::string newstr;

        int counter = 0;
        while (std::getline(infile, line))
        {
            //printf("Line %s\n", line.c_str());
            int mapping = atoi(line.c_str());

            if (counter==0) Joy_Mapping_Up = mapping;
            if (counter==1) Joy_Mapping_Down = mapping;
            if (counter==2) Joy_Mapping_Left = mapping;
            if (counter==3) Joy_Mapping_Right = mapping;
            if (counter==4) Joy_Mapping_Action_A = mapping;
            if (counter==5) Joy_Mapping_Action_B = mapping;
            if (counter==6) Joy_Mapping_Action_Start = mapping;
            if (counter==7) Joy_Mapping_Action_Z = mapping;
            if (counter==8) Joy_Mapping_Action_L = mapping;
            if (counter==9) Joy_Mapping_Action_R = mapping;
            if (counter==10) Joy_Mapping_Menu = mapping;

            if (counter==11) Mapping_Left = getKeyMapping(line);
            if (counter==12) Mapping_Right = getKeyMapping(line);
            if (counter==13) Mapping_Up = getKeyMapping(line);
            if (counter==14) Mapping_Down = getKeyMapping(line);
            if (counter==15) Mapping_Action_Start = getKeyMapping(line);
            if (counter==16) Mapping_Action_CUP = getKeyMapping(line);
            if (counter==17) Mapping_Action_CDOWN = getKeyMapping(line);
            if (counter==18) Mapping_Action_CLEFT = getKeyMapping(line);
            if (counter==19) Mapping_Action_CRIGHT = getKeyMapping(line);
            if (counter==20) Mapping_Action_Z = getKeyMapping(line);
            if (counter==21) Mapping_Action_L = getKeyMapping(line);
            if (counter==22) Mapping_Action_R = getKeyMapping(line);
            if (counter==23) Mapping_Action_B = getKeyMapping(line);
            if (counter==24) Mapping_Action_A = getKeyMapping(line);
            if (counter==25) Mapping_Menu = getKeyMapping(line);

            //load eep file
            if (counter == 26)
            {
                if (mapping == 0)
                    loadEep = false;
                else
                    loadEep = true;
            }

            //load sra file
            if (counter == 27)
            {
                if (mapping == 0)
                    loadSra = false;
                else
                    loadSra = true;
            }

            //load fla file
            if (counter == 28)
            {
                if (mapping == 0)
                    loadFla = false;
                else
                    loadFla = true;
            }

            //show FPS
            if (counter == 29)
            {
                if (mapping == 1)
                    showFPS = true;
                else
                    showFPS = false;
            }

            counter++;


            //printf("Int %d\n", myint1);
            /*pos = line.find("ROM: ");
            if (pos == 0)
            {
                newstr = line.substr(pos + 5);
                sprintf(rom_name, "%s", newstr.c_str());
            }
            pos = line.find("WIDTH: ");
            if (pos == 0)
            {
                newstr = line.substr(pos + 7);
            }
            pos = line.find("HEIGHT: ");
            if (pos == 0)
            {
                newstr = line.substr(pos + 8);
            }*/
        }

        printf("Finished Reading Config\n");
    }
    catch (...)
    {
        printf("No Config File Found\n");
    }
}

int main(int argc, char* argv[])
{
    //use to view arguments
    // printf("arg count %d\n", argc);
    // for (int i = 0; i < argc; i++)
    // {
    //     printf("argc%d %s\n", i,argv[i]);
    // }


    Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO;;

    if (SDL_Init(flags) < 0) {
        printf("Could not initialize SDL\n");
    }

    int WindowFlags = SDL_WINDOW_OPENGL;
    WindowOpenGL = SDL_CreateWindow(NULL,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, WindowFlags);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GLContext Context = SDL_GL_CreateContext(WindowOpenGL);


#ifndef __EMSCRIPTEN__
    SDL_AudioSpec AudioSettings = { 0 };

    AudioSettings.freq = 44100; 

    AudioSettings.format = AUDIO_S16;
    AudioSettings.channels = 2;
    SDL_AudioSpec AudioSettingsObtained = { 0 };
    SDL_AudioDeviceID deviceid = SDL_OpenAudioDevice(NULL, 0, &AudioSettings, &AudioSettingsObtained, 1);
    SDL_PauseAudioDevice(deviceid, 0);
    setDeviceId(deviceid);
    audioDeviceId = deviceid;
#endif

    //initialize font system
    TTF_Init();
    font24 = TTF_OpenFont("res/arial.ttf", 24);
    font40 = TTF_OpenFont("res/arial.ttf", 40);
    //TTF_SetFontStyle(font, TTF_STYLE_BOLD);


#ifndef __EMSCRIPTEN__
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
       /* Problem: glewInit failed, something is seriously wrong. */
       printf("Error: %s\n", glewGetErrorString(err));
    }
    else
    {
       printf("GLEW Initialized\n");
    }
#endif

    //print OpenGL stats
    printf("VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("RENDERER: %s\n", glGetString(GL_RENDERER));
    printf("VERSION: %s\n", glGetString(GL_VERSION));
    printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    //give it an initial value so drawText doesn't crash
    sprintf(fps_text, "FPS:");

    SDL_Rect rect;

    setupDrawTextOpenGL();
    imageOverlay = loadOpenGLTexture("overlay.png");

    shaderProgram = initShaders();

    //clear the screen
    glClearColor(184.0f / 255.0f, 213.0f / 255.0f, 238.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(WindowOpenGL);


    retro_init();

    sprintf(rom_name, "%s", "mario64.z64");
    //sprintf(rom_name, "%s", "roms\\fzero.v64");
    if (argc == 2)
    {
        sprintf(rom_name, "%s", argv[1]);
    }

    readConfig();

    FILE* f = fopen(rom_name, "rb");
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    uint8_t* filecontent = (uint8_t*)malloc(fsize);
    fread(filecontent, 1, fsize, f);
    fclose(f);

    bool loaded = retro_load_game_new(filecontent, fsize, loadEep, loadSra, loadFla);
    if (!loaded)
        printf("problem loading rom\n");

#ifdef __EMSCRIPTEN__
	//emscripten_set_main_loop(mainLoop,60,1);
#else
    while (runApp){
		mainLoop();
	}

    retro_deinit();
    SDL_CloseAudio();
    SDL_Quit();
#endif
    return 0;
}

//overlay menu
void drawOverlay();
void processMenuItemButtons();
bool lastSaveStateKeyPressed = false;
bool lastLoadStateKeyPressed = false;
bool lastOverlayKeyPressed = false;
bool lastMenuUpPressed = false;
bool lastMenuDownPressed = false;
bool lastMenuOkPressed = false;
bool menuMoveUp = false;
bool menuMoveDown = false;
bool menuOk = false;
bool showOverlay = false;
char menuItems[][30] = {
    "New Rom",
    "Load State",
    "Save State",
    "Full Screen",
};
int selectedMenuItem = 0;

void resetNeilButtons()
{

    neilbuttons.upKey = 0;
    neilbuttons.downKey = 0;
    neilbuttons.leftKey = 0;
    neilbuttons.rightKey = 0;
    neilbuttons.startKey = 0;
    neilbuttons.selectKey = 0;
    neilbuttons.lKey = 0;
    neilbuttons.rKey = 0;
    neilbuttons.zKey = 0;
    neilbuttons.aKey = 0;
    neilbuttons.bKey = 0;
    neilbuttons.axis0 = 0;
    neilbuttons.axis1 = 0;
    neilbuttons.axis2 = 0;
    neilbuttons.axis3 = 0;
    neilbuttons.cbLeft = 0;
    neilbuttons.cbRight = 0;
    neilbuttons.cbUp = 0;
    neilbuttons.cbDown = 0;
}

void mainLoop()
{

    SDL_Event windowEvent;
    frameStart = SDL_GetTicks();


    connectGamepad();

    resetNeilButtons();
    menuMoveUp = false;
    menuMoveDown = false;
    menuOk = false;

    //GAMEPAD
#ifdef USE_XINPUT
    if (gamepadConnected)
    {
        //TESTING
        if (IsPressed(XINPUT_GAMEPAD_A)) neilbuttons.aKey = true;
        //if (IsPressed(XINPUT_GAMEPAD_B)) neilbuttons.bKey = true;
        if (IsPressed(XINPUT_GAMEPAD_X)) neilbuttons.bKey = true;
        //if (IsPressed(XINPUT_GAMEPAD_Y)) printf("Y pressed\n");
        //if (IsPressed(XINPUT_GAMEPAD_DPAD_UP)) printf("UP pressed\n");
        if (IsPressed(XINPUT_GAMEPAD_START)) neilbuttons.startKey = true;
        //if (IsPressed(XINPUT_GAMEPAD_BACK)) printf("BACK pressed\n");
        if (IsPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER)) neilbuttons.rKey = true;
        if (IsPressed(XINPUT_GAMEPAD_LEFT_SHOULDER)) neilbuttons.zKey = true;
        //sprintf(axis_text0, "Axis 0 Value %d\n", state.Gamepad.sThumbLX);
        //sprintf(axis_text1, "Axis 1 Value %d\n", state.Gamepad.sThumbLY);
        //sprintf(axis_text2, "Axis 2 Value %d\n", state.Gamepad.sThumbRX);
        //sprintf(axis_text3, "Axis 3 Value %d\n", state.Gamepad.sThumbRY);
        if (state.Gamepad.bLeftTrigger > 128) neilbuttons.lKey = true; //left trigger button
        if (isPressedOnce(IsPressed(XINPUT_GAMEPAD_RIGHT_THUMB), &lastOverlayKeyPressed))
        {
            if (!showOverlay)
            {
                selectedMenuItem = 0;
            }
            showOverlay = !showOverlay;
        }
        if (isPressedOnce(IsPressed(XINPUT_GAMEPAD_DPAD_UP), &lastMenuUpPressed)) menuMoveUp = true;
        if (isPressedOnce(IsPressed(XINPUT_GAMEPAD_DPAD_DOWN), &lastMenuDownPressed)) menuMoveDown = true;
        if (isReleasedOnce(IsPressed(XINPUT_GAMEPAD_A), &lastMenuOkPressed)) menuOk = true;


        //IMPLEMENT
        if (IsPressed(XINPUT_GAMEPAD_DPAD_UP)) neilbuttons.upKey = true;
        if (IsPressed(XINPUT_GAMEPAD_DPAD_DOWN)) neilbuttons.downKey = true;
        if (IsPressed(XINPUT_GAMEPAD_DPAD_LEFT)) neilbuttons.leftKey = true;
        if (IsPressed(XINPUT_GAMEPAD_DPAD_RIGHT)) neilbuttons.rightKey = true;
        neilbuttons.axis0 = state.Gamepad.sThumbLX;
        neilbuttons.axis1 = state.Gamepad.sThumbLY * -1;
        neilbuttons.axis2 = state.Gamepad.sThumbRX;
        neilbuttons.axis3 = state.Gamepad.sThumbRY;

        if (neilbuttons.axis2 < -16000) neilbuttons.cbLeft = 1;
        if (neilbuttons.axis2 > 16000) neilbuttons.cbRight = 1;
        if (neilbuttons.axis3 < -16000) neilbuttons.cbDown = 1;
        if (neilbuttons.axis3 > 16000) neilbuttons.cbUp = 1;

        //fix weird -1 * float conversion bug
        if (neilbuttons.axis1 == 0x8000)
        {
            neilbuttons.axis1--;
        }

    }
#endif

    if (joyStick != NULL)
    {
        neilbuttons.axis0 = SDL_JoystickGetAxis(joyStick, 0);
        neilbuttons.axis1 = SDL_JoystickGetAxis(joyStick, 1);
        neilbuttons.axis2 = SDL_JoystickGetAxis(joyStick, 2);
        neilbuttons.axis3 = SDL_JoystickGetAxis(joyStick, 3);

        if (neilbuttons.axis2 < -16000) neilbuttons.cbLeft = 1;
        if (neilbuttons.axis2 > 16000) neilbuttons.cbRight = 1;
        if (neilbuttons.axis3 < -16000) neilbuttons.cbUp = 1;
        if (neilbuttons.axis3 > 16000) neilbuttons.cbDown = 1;

        //neilbuttons.axis4 = SDL_JoystickGetAxis(joyStick, 4);
        //neilbuttons.axis5 = SDL_JoystickGetAxis(joyStick, 5);

#ifdef __EMSCRIPTEN__
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Up)) neilbuttons.upKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Down)) neilbuttons.downKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Left)) neilbuttons.leftKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Right)) neilbuttons.rightKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_A)) neilbuttons.aKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_B)) neilbuttons.bKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_Start)) neilbuttons.startKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_Z)) neilbuttons.zKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_L)) neilbuttons.lKey = true;
        if (SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_R)) neilbuttons.rKey = true;
        if (isPressedOnce(SDL_JoystickGetButton(joyStick, Joy_Mapping_Menu), &lastOverlayKeyPressed))
        {
            if (!showOverlay)
            {
                selectedMenuItem = 0;
            }
            showOverlay = !showOverlay;
        }
        if (isPressedOnce(SDL_JoystickGetButton(joyStick, Joy_Mapping_Up), &lastMenuUpPressed)) menuMoveUp = true;
        if (isPressedOnce(SDL_JoystickGetButton(joyStick, Joy_Mapping_Down), &lastMenuDownPressed)) menuMoveDown = true;
        if (isReleasedOnce(SDL_JoystickGetButton(joyStick, Joy_Mapping_Action_A), &lastMenuOkPressed)) menuOk = true;
#endif
    }

    //KEYBOARD
    keyboardState = (Uint8*)SDL_GetKeyboardState(NULL);
    if (keyboardState[Mapping_Up]) neilbuttons.axis1 = -32000;
    if (keyboardState[Mapping_Down]) neilbuttons.axis1 = 32000;
    if (keyboardState[Mapping_Left]) neilbuttons.axis0 = -32000;
    if (keyboardState[Mapping_Right]) neilbuttons.axis0 = 32000;
    if (keyboardState[Mapping_Action_Start]) neilbuttons.startKey = true;

    if (keyboardState[Mapping_Action_A]) neilbuttons.aKey = true;
    if (keyboardState[Mapping_Action_B]) neilbuttons.bKey = true;
    if (keyboardState[Mapping_Action_Z]) neilbuttons.zKey = true;
    if (keyboardState[Mapping_Action_R]) neilbuttons.rKey = true;
    if (keyboardState[Mapping_Action_L]) neilbuttons.lKey = true;

    //TODO C Buttons
    if (keyboardState[Mapping_Action_CLEFT]) neilbuttons.cbLeft = true;
    if (keyboardState[Mapping_Action_CDOWN]) neilbuttons.cbDown = true;
    if (keyboardState[Mapping_Action_CRIGHT]) neilbuttons.cbRight = true;
    if (keyboardState[Mapping_Action_CUP]) neilbuttons.cbUp = true;

    //TODO - doesnt work in conjunction with gamepad, only use for debugging while gamepad is off
    // if (isPressedOnce(keyboardState[SDL_SCANCODE_GRAVE], &lastOverlayKeyPressed))
    // {
    //     if (!showOverlay)
    //     {
    //         selectedMenuItem = 0;
    //     }
    //     showOverlay = !showOverlay;
    // }
    // if (isPressedOnce(keyboardState[SDL_SCANCODE_UP], &lastMenuUpPressed)) menuMoveUp = true;
    // if (isPressedOnce(keyboardState[SDL_SCANCODE_DOWN], &lastMenuDownPressed)) menuMoveDown = true;
    // if (isReleasedOnce(keyboardState[SDL_SCANCODE_RETURN], &lastMenuOkPressed)) menuOk = true;

    if (showOverlay)
    {
        resetNeilButtons(); //don't process game buttons while overlay is showing
        processMenuItemButtons();
    }


    if (SDL_PollEvent(&windowEvent) != 0)
    {
        if (windowEvent.type == SDL_QUIT)
        {
            runApp = false;
            return;
        }
    }


    //allow audio buffer to shrink down
    //if emulator is too far ahead
    audioBufferQueue = SDL_GetQueuedAudioSize(audioDeviceId);
    if (audioBufferQueue < 20000)
        retro_run();
    else
    {
        hadSkip = 1;
        maxAudioBufferQueue = audioBufferQueue;
    }



    if(getReadyToSwap()==1)
    {
        if (showOverlay)
        {
            drawOverlay();
        }

        //draw fps
        if (showFPS)
        {
            drawTextOpenGL(fps_text, 0, 0, fontcolorWhite, FONTSIZE_24);
        }

        if (toastCounter > 0)
        {
            drawTextOpenGL(toast_message, 0, 30, fontcolorWhite, FONTSIZE_24);
            toastCounter--;
        }
        
        SDL_GL_SwapWindow(WindowOpenGL);
        resetReadyToSwap();
        swapCount++;
    }


    limitFPS();

}

bool isPressedOnce(bool condition, bool* lastPressedVar)
{
    if (condition)
    {
        if (!*lastPressedVar)
        {
            *lastPressedVar = true;
            return true;
        }
    }
    else
        *lastPressedVar = false;

    return false;
}


bool isReleasedOnce(bool condition, bool* lastPressedVar)
{
    bool wasReleased = false;
    if (condition)
    {
        *lastPressedVar = true;
    }
    else
    {
        if (*lastPressedVar)
        {
            wasReleased = true;
        }
        *lastPressedVar = false;
    }

    return wasReleased;
}

void processMenuItemButtons()
{
    int menuItemsCount = sizeof(menuItems) / 30;
    if (menuMoveDown) selectedMenuItem++;
    if (menuMoveUp) selectedMenuItem--;
    if (selectedMenuItem < 0) selectedMenuItem = 0;
    if (selectedMenuItem > menuItemsCount - 1) selectedMenuItem = menuItemsCount - 1;

    if (menuOk)
    {
#ifdef __EMSCRIPTEN__
        //"New Rom" - 0
        if (selectedMenuItem == 0)
        {
            EM_ASM(
                location.reload();
            );
        }
        
        //"Load State" - 1
        if (selectedMenuItem == 1)
        {
            EM_ASM(
                myApp.loadCloud();
            );
        }

        //"Save State" - 2
        if (selectedMenuItem == 2)
        {
            EM_ASM(
                myApp.saveCloud();
            );
        }

        //"Full Screen" - 3
        if (selectedMenuItem == 3)
        {
            EM_ASM(
                myApp.fullscreen();
            );
        }
#else
        //"Load State" - 1
        if (selectedMenuItem == 1)
        {
            neil_unserialize();
        }

        //"Save State" - 2
        if (selectedMenuItem == 2)
        {
            neil_serialize();
        }
#endif

        showOverlay = false;

    }
}


void drawOverlay()
{
    drawOpenglTexture(50, 50, 640 - 100, 480 - 100, imageOverlay);
    int menuItemsCount = sizeof(menuItems) / 30;
    int startX = 200;
    int startY = 480 - 120;

    for (int i = 0; i < menuItemsCount; i++)
    {
        int currentY = startY - (i * 80);
        if (i == selectedMenuItem)
        {
            drawTextOpenGL(">", startX -30, currentY, fontcolorRed, FONTSIZE_40);
        }
        drawTextOpenGL(menuItems[i], startX, currentY,fontcolorRed,FONTSIZE_40);
    }
}


void limitFPS()
{
    // Wait to mantain framerate:
    frameTime = SDL_GetTicks() - frameStart;
    if (frameTime < DELAY)
    {
#ifdef __EMSCRIPTEN__
        //emscripten_sleep(1);
#else
        SDL_Delay((int)(DELAY - frameTime));
#endif
    }


    //calculate FPS
    current_fps_counter++;
    if (current_frameStart + 1000.0f < SDL_GetTicks())
    {
        current_fps = current_fps_counter; //host fps
        current_fps_counter = 0;
        current_frameStart = SDL_GetTicks();
        currentSwapCount = swapCount; //n64 fps


        //for debugging
        currentAudioBufferQueue = audioBufferQueue;
        swapCount = 0;
        currentHadSkip = hadSkip;
        hadSkip = 0;
        currentMaxAudioBufferQueue = maxAudioBufferQueue;
    }

    sprintf(fps_text, "FPS: %d GameFPS: %d", current_fps, currentSwapCount);

    //SDL_SetWindowTitle(WindowOpenGL, fps_text);

}


extern "C" {

    void runMainLoop(){
        mainLoop();
    }

    struct NeilButtons* getNeilButtons()
    {
        return &neilbuttons;
    }
}


char* loadFile(char* filename)
{
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char* filecontent = (char*)malloc(fsize + 1);
    int readsize = fread(filecontent, 1, fsize, f);
    fclose(f);

    filecontent[readsize] = '\0';

    return filecontent;
}

unsigned int compileShader(int shaderType, char* source)
{
    unsigned int id = glCreateShader(shaderType);
    glShaderSource(id, 1, (const char**)&source, NULL);
    glCompileShader(id);

    //ERROR HANDLING
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        if (shaderType == GL_VERTEX_SHADER)
            printf("FAILED TO COMPILE VERTEX SHADER + %s\n", message);
        if (shaderType == GL_FRAGMENT_SHADER)
            printf("FAILED TO COMPILE FRAGMENT SHADER + %s\n", message);

        glDeleteShader(id);
    }

    return id;
}

unsigned int initShaders()
{
    char* vertexSource = loadFile((char*)"shader_vert.hlsl");
    char* fragmentSource = loadFile((char*)"shader_frag.hlsl");

    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void setupDrawTextOpenGL()
{
    //create the buffer
    glGenBuffers(1, &vertexBufferDrawText);
}

void translateDrawTextScreenCoordinates(int x, int y, int w, int h)
{
    float screenwidth = 640;
    float screenheight = 480;

    //openGL coordinates go from (-1,-1) to (1,1) as a square
    float newx = ((2.0f / (float)screenwidth) * (float)x) - 1.0f;
    float newy = ((2.0f / (float)screenheight) * (float)y) - 1.0f;
    float newwidth = ((2.0f / (float)screenwidth) * (float)w);
    float newheight = ((2.0f / (float)screenheight) * (float)h);

    for (int i = 0; i < 6; i++)
    {
        //handle X
        int tempX = positionsDrawText[i * 4];
        if (tempX < 0)
        {
            //convert it to x
            positionsDrawTextFinal[i * 4] = newx;
        }
        else
        {
            //convert it to x + w
            positionsDrawTextFinal[i * 4] = newx + newwidth;
        }

        //handle Y
        int tempY = positionsDrawText[(i * 4) + 1];
        if (tempY < 0)
        {
            //convert it to y
            positionsDrawTextFinal[(i * 4) + 1] = newy;
        }
        else
        {
            //convert it to y + h
            positionsDrawTextFinal[(i * 4) + 1] = newy + newheight;
        }

    }
}

//we use SDL's API to generate an image of the text but then
//we need to convert it into an OpenGL Texture
//NOTE - x and y start at the bottom left corner of the screen
void drawTextOpenGL(const char* text, int x, int y, SDL_Color color, MY_FONT_SIZE fontsize)
{
    //need this to enable transparency on the texture
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    TTF_Font* font = NULL;

    if (fontsize == FONTSIZE_24) font = font24;
    if (fontsize == FONTSIZE_40) font = font40;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);

    //surface->w TEXTURE WIDTH
    //surface->h TEXTURE HEIGHT

    translateDrawTextScreenCoordinates(x, y, surface->w, surface->h);

    //we are technically using the same shader for 
    //drawTextOpenGL() and drawOpenglTexture() so this
    //code is nearly the same in both however 
    //keeping it in both in case I ever have seperate shaders
    glUseProgram(shaderProgram);

    //bind it or "select" it
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferDrawText);
    //fill the buffer with data
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, positionsDrawTextFinal, GL_DYNAMIC_DRAW);

    //define the position attribute
    glVertexAttribPointer(
        0, //attribute number for shader
        2, //size or number of components for this attribute, so 2 floats for a position
        GL_FLOAT, //data type of each component
        GL_FALSE, //do we want it to auto normalize the data for us (or convert it to a range of 0-1)
        sizeof(float) * 4, //stride or number of bytes between each vertex
        (GLvoid*)0 //offset to this attribute - needs to be converted to a pointer (const void*)8 as an example
    );

    //texture coordinate
    glVertexAttribPointer(
        1, //attribute number for shader
        2, //size or number of components for this attribute, so 2 floats for a position
        GL_FLOAT, //data type of each component
        GL_FALSE, //do we want it to auto normalize the data for us (or convert it to a range of 0-1)
        sizeof(float) * 4, //stride or number of bytes between each vertex
        (GLvoid*)8 //offset to this attribute - needs to be converted to a pointer (const void*)8 as an example
    );


    //need to enable this attribute in this case 0th attribute (matches first parameter of glVertexAttribPointer above)
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    char colors = surface->format->BytesPerPixel;
    int texture_format;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //Emscripten doesn't support GL_BGRA ??
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //render the triangles
    glDrawArrays(
        GL_TRIANGLES, //what are we drawing
        0, //first index to draw
        6 //how many to draw
    );


    glDisable(GL_BLEND);


    glDeleteTextures(1, &texture);
    SDL_FreeSurface(surface);

    // //if we don't destroy this you will see in 
    // //windows task manager the memory will keep growing
    // SDL_DestroyTexture(fontTexture); 
}


GLuint loadOpenGLTexture(const char* file)
{
    GLuint imageID;

    int width = 0;
    int height = 0;
    int bpp = 0;
    unsigned char* localBuffer;

    //normally opengl draws things upside down
    //since the origin is at the bottom left
    //however we already flipped the texture coordinates
    //in positionsDrawText[] so we can load normally
    //otherwise you would pass 1 into this function
    stbi_set_flip_vertically_on_load(0);
    
    localBuffer = stbi_load(file, &width, &height, &bpp, 4);

    glGenTextures(1, &imageID);
    glBindTexture(GL_TEXTURE_2D, imageID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localBuffer);

    if (localBuffer)
        stbi_image_free(localBuffer);

    return imageID;
}

void drawOpenglTexture(int x, int y, int w, int h, GLuint imageID)
{
    //need this to enable transparency on the texture
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    translateDrawTextScreenCoordinates(x, y, w, h);

    glUseProgram(shaderProgram);

    //bind it or "select" it
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferDrawText);
    //fill the buffer with data
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, positionsDrawTextFinal, GL_DYNAMIC_DRAW);

    //define the position attribute
    glVertexAttribPointer(
        0, //attribute number for shader
        2, //size or number of components for this attribute, so 2 floats for a position
        GL_FLOAT, //data type of each component
        GL_FALSE, //do we want it to auto normalize the data for us (or convert it to a range of 0-1)
        sizeof(float) * 4, //stride or number of bytes between each vertex
        (GLvoid*)0 //offset to this attribute - needs to be converted to a pointer (const void*)8 as an example
    );

    //texture coordinate
    glVertexAttribPointer(
        1, //attribute number for shader
        2, //size or number of components for this attribute, so 2 floats for a position
        GL_FLOAT, //data type of each component
        GL_FALSE, //do we want it to auto normalize the data for us (or convert it to a range of 0-1)
        sizeof(float) * 4, //stride or number of bytes between each vertex
        (GLvoid*)8 //offset to this attribute - needs to be converted to a pointer (const void*)8 as an example
    );


    //need to enable this attribute in this case 0th attribute (matches first parameter of glVertexAttribPointer above)
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, imageID);

    //render the triangle
    glDrawArrays(
        GL_TRIANGLES, //what are we drawing
        0, //first index to draw
        6 //how many to draw
    );
    
}

int getKeyMapping(std::string line)
{
    //remove carriage return
    #ifdef __EMSCRIPTEN__
    line.erase(line.size() - 1);
    #endif

    if (line.compare("ArrowLeft")==0) return SDL_SCANCODE_LEFT;
    if (line.compare("ArrowRight")==0) return SDL_SCANCODE_RIGHT;
    if (line.compare("ArrowUp")==0) return SDL_SCANCODE_UP;
    if (line.compare("ArrowDown")==0) return SDL_SCANCODE_DOWN;
    if (line.compare("Enter")==0) return SDL_SCANCODE_RETURN;
    if (line.compare("a")==0) return SDL_SCANCODE_A;
    if (line.compare("b")==0) return SDL_SCANCODE_B;
    if (line.compare("c")==0) return SDL_SCANCODE_C;
    if (line.compare("d")==0) return SDL_SCANCODE_D;
    if (line.compare("e")==0) return SDL_SCANCODE_E;
    if (line.compare("f")==0) return SDL_SCANCODE_F;
    if (line.compare("g")==0) return SDL_SCANCODE_G;
    if (line.compare("h")==0) return SDL_SCANCODE_H;
    if (line.compare("i")==0) return SDL_SCANCODE_I;
    if (line.compare("j")==0) return SDL_SCANCODE_J;
    if (line.compare("k")==0) return SDL_SCANCODE_K;
    if (line.compare("l")==0) return SDL_SCANCODE_L;
    if (line.compare("m")==0) return SDL_SCANCODE_M;
    if (line.compare("n")==0) return SDL_SCANCODE_N;
    if (line.compare("o")==0) return SDL_SCANCODE_O;
    if (line.compare("p")==0) return SDL_SCANCODE_P;
    if (line.compare("q")==0) return SDL_SCANCODE_Q;
    if (line.compare("r")==0) return SDL_SCANCODE_R;
    if (line.compare("s")==0) return SDL_SCANCODE_S;
    if (line.compare("t")==0) return SDL_SCANCODE_T;
    if (line.compare("u")==0) return SDL_SCANCODE_U;
    if (line.compare("v")==0) return SDL_SCANCODE_V;
    if (line.compare("w")==0) return SDL_SCANCODE_W;
    if (line.compare("x")==0) return SDL_SCANCODE_X;
    if (line.compare("y")==0) return SDL_SCANCODE_Y;
    if (line.compare("z")==0) return SDL_SCANCODE_Z;
    if (line.compare("`")==0) return SDL_SCANCODE_GRAVE;
    if (line.compare("1") == 0) return SDL_SCANCODE_1;
    if (line.compare("2") == 0) return SDL_SCANCODE_2;
    if (line.compare("3") == 0) return SDL_SCANCODE_3;
    if (line.compare("4") == 0) return SDL_SCANCODE_4;
    if (line.compare("5") == 0) return SDL_SCANCODE_5;
    if (line.compare("6") == 0) return SDL_SCANCODE_6;
    if (line.compare("7") == 0) return SDL_SCANCODE_7;
    if (line.compare("8") == 0) return SDL_SCANCODE_8;
    if (line.compare("9") == 0) return SDL_SCANCODE_9;
    if (line.compare("0") == 0) return SDL_SCANCODE_0;
    if (line.compare(" ") == 0) return SDL_SCANCODE_SPACE;
    if (line.compare(",") == 0) return SDL_SCANCODE_COMMA;
    if (line.compare(".") == 0) return SDL_SCANCODE_PERIOD;
    if (line.compare("/") == 0) return SDL_SCANCODE_SLASH;
    if (line.compare(";") == 0) return SDL_SCANCODE_SEMICOLON;
    if (line.compare("'") == 0) return SDL_SCANCODE_APOSTROPHE;
    if (line.compare("[") == 0) return SDL_SCANCODE_LEFTBRACKET;
    if (line.compare("]") == 0) return SDL_SCANCODE_RIGHTBRACKET;
    if (line.compare("\\") == 0) return SDL_SCANCODE_BACKSLASH;
    if (line.compare("-") == 0) return SDL_SCANCODE_MINUS;
    if (line.compare("=") == 0) return SDL_SCANCODE_EQUALS;


    //default
    return 0;

}

extern "C" {
    void neil_toast_message(char* message)
    {
        sprintf(toast_message, message);
        toastCounter = 60;
    }
}