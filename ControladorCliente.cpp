#include "ControladorCliente.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#include <SDL2/SDL_image.h>
#include <InputHandler.h>
#include <ViewCliente.h>
#include <ModeloCliente.h>
#include <fstream>
#include <sys/ioctl.h>


ControladorCliente::ControladorCliente()
{
    this->handler = new InputHandler();
}

bool ControladorCliente::running(){

    return(this->isRunning);
}


ControladorCliente::~ControladorCliente()
{
    //dtor
}

void ControladorCliente::inicializarConexion()
{
    int jugadorInicial;
    struct hostent *server;
	//creo el socket
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket < 0) {
        std::cout << "Error abriendo el socket" << std::endl;
        exit(1);
    }

    server = gethostbyname("127.0.0.1");

    if (server == NULL) {
        std::cout << "Error, host inexistente" << std::endl;
        exit(1);
    }

	//especifico la direccion
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(30666);
	//server_address.sin_addr.s_addr = INADDR_ANY;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);

	//verifico la conexion
	int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	if( connection_status < 0)
	{
        std::cout << "Error al conectar con el servidor..." << std::endl;
        return;
	}
	else {
        std::cout << "Conexión exitosa con el servidor." << std::endl;
        char id;
        recv(network_socket, &id, sizeof(id), 0);
        jugadorInicial = id - '0';       // acá recibe el id 4 y le resta 0. Nada de esto tiene sentido realmente! LE restas esto a un char para q lo haga un numero
        std::cout << "charId: " << jugadorInicial << std::endl;
	}

	//Una vez establecida la conexion se inicializa lo necesario
	ViewCliente_ = new ViewCliente();
	ViewCliente_->initView();
	ModeloCliente_ = new ModeloCliente(1, 1, 1, 1, jugadorInicial);
	this->render();
	isRunning = true;
}

void ControladorCliente::processInput()
{
    //i++;
    //printf( "ACA %d\n", i);
    done = 0;
    while(done == 0)
    {
        while( SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_KEYDOWN:
                    done = 1;
                    event = this->handler->handleInput(SDL_GetKeyboardState(NULL));
                    break;

                case SDL_KEYUP:
                    done = 1;
                    event = this->handler->handleInput(SDL_GetKeyboardState(NULL));
                    break;

                default:
                    break;
            }
        }
    }

}

void ControladorCliente::update()
{
    //ENVIO EL EVENTO
    this->enviarEvento(event);
    ////////////////////////////////


    //Recibo la informacion del server
    this->recibirInformacion();
    ////////////////////////////////////

}

void ControladorCliente::render()
{

    ViewCliente_->prepareStage();

    ViewCliente_->renderCourt();

    ModeloCliente_->render();

    ViewCliente_->updateStage();


}

void ControladorCliente::enviarEvento(int* event)
{

    char message[27]; //LOngitud de lo que quiero enviar +1 por el caracter de finalizacion

    //event LSHIT LALT LCTRL W UP DOWN RIGHT LEFT


    if(event[3] == 1) //CAMBIO DE JUGADOR entonces mando el nuevo jugador al final
    {
        std::cout << "estoy haciendo el cambio!!!!!!" << std::endl;
        int nuevoJugador = ModeloCliente_->cambiarJugador();
        std::cout << "estoy haciendo el cambio2!!!!!!" << std::endl;
        if( nuevoJugador != -1)
        {
             //Evento nuevoJugador
            sprintf(message, "%d %d %d %d %d %d %d %d %d", event[0], event[1], event[2], event[3],
                    event[4], event[5], event[6], event[7], nuevoJugador); //Esta hardcodeado el 4
        }

    }
    else //OTRO EVENTO
    {
        sprintf(message, "%d %d %d %d %d %d %d %d %d", event[0], event[1], event[2], event[3],
                    event[4], event[5], event[6], event[7], 0);
         std::cout << "ENVIANDO: " << message << std::endl;
    }

    //MANDO EL MENSAJE
    int mensaje = this->enviarMensaje(message, strlen(message));
     std::cout << "ACA: " << mensaje << "longitud: " << strlen(message) << std::endl;
    if( mensaje == -1 )
    {
        std::cout << "Error al enviar el mensaje" << std::endl;
    }
}

void ControladorCliente::recibirInformacion()
{
    //int tam;
    //recv(network_socket, &tam, sizeof(tam), 0);
    //std::cout << "tam_msj: " << tam << std::endl;
    //char* buffer = (char*)malloc(19*sizeof(char));
    int size;
    ioctl(network_socket, FIONREAD, &size);
    char buffer[size];
    memset(buffer, 0, sizeof buffer);
    //std::cout << "sizeof: " << sizeof(buffer) << std::endl;
    //int mensaje = this->recibirMensaje(buffer, sizeof(buffer));
    int mensaje = recv(network_socket, buffer, sizeof buffer, 0);

    if( mensaje == -1 )
    {
        std::cout << "Error al recibir el mensaje" << std::endl;
    }

    std::cout << "RECIBIDO: " << buffer << std::endl << "bytes rcv: " << mensaje << std::endl;
    int log = strlen(buffer);
    std::cout <<" long: " << log << std::endl;

    //Message received. Send to all clients.
    int evento[8];

    //evento
    //idPlayer xPos yPos angle estado hayW viejoJugador nuevoJugador
    sscanf(buffer, "%d %d %d %d %d %d %d %d", &evento[0], &evento[1], &evento[2], &evento[3]
                    , &evento[4] , &evento[5] , &evento[6] , &evento[7]);

    std::cout << "Evento0: " << evento[0] << "Evento1:" << evento[1] << std::endl;

    ModeloCliente_->actualizarJugador(evento[0], evento[1], evento[2], evento[3], evento[4]);

    if( evento[5] == 1) //Hay cambio de jugador
    {
        ModeloCliente_->cambiarJugadorExterno(evento[6], evento[7]);
    }

}


int ControladorCliente::enviarMensaje(char* msj, int size_msj)
{
    int sent = 0;
    int s = 0;
    bool is_the_socket_valid = true;

    while(sent < size_msj && is_the_socket_valid)
    {
        s = send(network_socket, &msj[sent], size_msj - sent, 0);

        if( s == 0)
        {
            is_the_socket_valid = false;
        }
        else if (s < 0)
        {
            is_the_socket_valid = false;
        }
        else
        {
            sent += s;
        }
    }

    if(is_the_socket_valid){
        return sent;
    }
    else{
        return -1;
    }
}


int ControladorCliente::recibirMensaje(char* msj, int size_msj)
{
    int received = 0;
    int s = 0;
    bool is_the_socket_valid = true;

    while(received < size_msj && is_the_socket_valid) // tendria q ser size_msj = 19 ni idea porq
    {
        std::cout <<" ACA1" << std::endl;
        s = recv(network_socket, &msj[received], size_msj - received, 0);
        //std::cout << "RECIBIDO: " << msj << "Pos 0: " << msj[0] << std::endl;
        //std::cout << s << std::endl; //NI IDEA PORQ PERO LOS BYTES QUE RECIBE POR ESE STRING DEL SERVER SON 19
        if( s == 0)
        {
            is_the_socket_valid = false;
            std::cout <<" ACA2" << std::endl;
        }
        else if (s < 0)
        {
            is_the_socket_valid = false;
            std::cout <<" ACA3" << std::endl;
        }
        else
        {
            received += s;
            std::string rcv = msj;
            std::cout << s << " bytes recv." << std::endl;
            std::cout << "RECIBIDO " << msj << "Pos 0: " << msj[0] << std::endl;
            int log = strlen(msj);
            std::cout <<" long: " << log << std::endl;
        }
    }

    if(is_the_socket_valid){
        std::cout << "TERMINO BIEN " << std::endl;
        return received;
    }
    else{
        return -1;
    }
}


void ControladorCliente::clean()
{
    ViewCliente_->clean();
}
