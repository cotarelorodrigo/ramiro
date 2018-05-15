#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ControladorCliente.h>
#include <ViewCliente.h>
#include <Log.h>


int main()
{
    Log* logger = new Log(3);
    Log::programaInicializado();

    ControladorCliente* controlador = new ControladorCliente();
    controlador->inicializarConexion();

    const int FPS = 60;
    const int frameDelay = 1000/FPS;

	Uint32 frameStart;
    int frameTime;

	while(controlador->running())
	{
        frameStart = SDL_GetTicks();

	    controlador->processInput();
	    controlador->update();
        controlador->render();

        frameTime = SDL_GetTicks() - frameStart;

        if(frameTime < frameDelay)
        {
            SDL_Delay(frameDelay - frameTime);
        }

	}

    controlador->clean();

    Log::programaCerrandoseCorrectamente();
    return 0;
}
