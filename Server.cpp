#include "Server.h"

std::vector<Cliente> Server::clients;

Server::Server()
{
    //Initialize static mutex from MyThread
    MyThread::InitMutex();

    //For setsock opt (REUSEADDR)
    int yes = 1;

    //Init serverSock and start listen()'ing
    serverSock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    //Avoid bind error if the socket was not close()'d last time;
    setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));

    if(bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(sockaddr_in)) < 0)
        std::cerr << "Failed to bind";

    // esta es la funcion que hay que hacer pasar por otro thread?
    listen(serverSock, CANTIDAD_CLIENTES);
}

Server::~Server()
{
    //dtor
}

void Server::AcceptAndDispatch()
{
    char server_message[20];
    Cliente *c;
    MyThread *t;
    datosCliente* cl;

    socklen_t cliSize = sizeof(sockaddr_in);

    model_ = new model(1, 1);

    // qué nombre de mierda para una variable eh... PUTITA
    int sock;
    int jugadorActual[4] = {4, 5, 11, 12};
    int jugadoresConectados = 0;


    std::cout << "Waiting for clients..." << std::endl;
    while(1)
    {
        t = new MyThread();

        //Blocks here;
        sock = accept(serverSock, (struct sockaddr *) &clientAddr, &cliSize);

        if(sock < 0) {
            std::cerr << "Error on accept";
        }
        else {
            c = new Cliente(sock, jugadorActual[jugadoresConectados]);
            cl = new datosCliente(model_, c);

            sprintf(server_message, "%d", jugadorActual[jugadoresConectados]); //Le digo al cliente que jugador tiene que ser
            send(c->getSocket(), &server_message, sizeof(server_message), 0);
            t->Create((void *) Server::HandleClient, cl);
            jugadoresConectados++;
        }

    }
}

void *Server::HandleClient(void *args)
{
    //Pointer to accept()'ed Client
    model *mod = ((datosCliente *)args)->modelo;
    Cliente *c = ((datosCliente *)args)->client;
    char buffer[17], message[50];
    int index;
    int n;

    //Add client in Static clients <vector> (Critical section!)
    MyThread::LockMutex((const char *) c->getName());

    //Before adding the new client, calculate its id. (Now we have the lock)
    c->SetId(Server::clients.size());
    sprintf(buffer, "Cliente n.%d", c->getId());
    c->SetName(buffer);
    std::cout << "Añadiendo cliente id: " << c->getId() << std::endl;
    Server::clients.push_back(*c);

    MyThread::UnlockMutex((const char *) c->getName());

    while(1)
    {
        memset(buffer, 0, sizeof buffer);
        n = Server::recibirMensaje(c->getSocket(), buffer, sizeof buffer);
        std::cout << "RECIBIENDO " << std::endl;
        //Cliente desconectado.
        if(n == 0)
        {
            std::cout << "Cliente " << c->getName() << " desconectado" << std::endl;
            close(c->getSocket());

            //Remove client in Static clients <vector> (Critical section!)
            MyThread::LockMutex((const char *) c->getName());

            index = Server::FindClientIndex(c);
            std::cout << "Sacando usuario en la posicion " << index << " su id es: " << Server::clients[index].getId() << std::endl;
            Server::clients.erase(Server::clients.begin() + index);

            MyThread::UnlockMutex((const char *) c->getName());

            break;
        }
        else if(n < 0)
        {
            std::cerr << "Error recibiendo mensaje del cliente: " << c->getName() << std::endl;
        }
        else
        {

            //Message received. Send to all clients.
            int evento[9];

            //evento
            sscanf(buffer, "%d %d %d %d %d %d %d %d %d", &evento[0], &evento[1], &evento[2], &evento[3]
                                    , &evento[4] , &evento[5] , &evento[6] , &evento[7], &evento[8]);

            //MyThread::LockMutex((const char *) c->getName());
            if( mod->handleEvent(evento, c->getJugadorActual()) == 1)
            {
                //idPlayer xPos yPos angle estado hayW viejoJugador nuevoJugador
                snprintf(message, sizeof message, "%d %d %d %d %d %d %d %d", ContenedorDatos::idPlayer, ContenedorDatos::x,
                    ContenedorDatos::y,  ContenedorDatos::angle,  ContenedorDatos::c_state, 1, c->getJugadorActual(), evento[8]);

                c->setJugadorActual(evento[8]); //Si devolvio uno es que se realizo un cambio de jugador, entonces
                                                //le seteo el nuevoJugador al cliente.
            }
            else //En vez de meter if habria uqe ir concatenando en el string... Lo dejo para despues
            {
                //idPlayer xPos yPos angle estado hayW viejoJugador nuevoJugador
                snprintf(message, sizeof message, "%d %d %d %d %d %d %d %d", ContenedorDatos::idPlayer, ContenedorDatos::x,
                    ContenedorDatos::y,  ContenedorDatos::angle,  ContenedorDatos::c_state, 0, 0, 0);
            }


            std::cout << "Enviando a todos: " << message << std::endl;
            Server::SendToAll(message);
            //MyThread::UnlockMutex((const char *) c->getName());

        }
    }

    //End thread
    return NULL;
}


// acá hay que lidiar con los mensajes que no se envian por completo.
void Server::SendToAll(char *message)
{
    int n;

    //Acquire the lock
    MyThread::LockMutex("'SendToAll()'");

    for(size_t i=0; i<clients.size(); i++)
    {
        int tam = strlen(message);
        std::cout << "tam_msj: " << tam << std::endl;
        //send(Server::clients[i].getSocket(), &tam, sizeof(tam), 0);
        n = Server::enviarMensaje(Server::clients[i].getSocket(), message, strlen(message));
        std::cout << n << " bytes sent." << std::endl;
        if(n < strlen(message))
        {
            std::cout <<"El mensaje no se envio COMPLETO!!!!!!!!!!!!!!!" << std::endl;
        }
    }

    //Release the lock
    MyThread::UnlockMutex("'SendToAll()'");
}

void Server::ListClients()
{
    for(size_t i=0; i<clients.size(); i++)
    {
        std::cout << clients.at(i).getName() << std::endl;
    }
}

int Server::FindClientIndex(Cliente *c)
{
    for(size_t i=0; i<clients.size(); i++)
    {
        if((Server::clients[i].getId()) == c->getId()) return (int) i;
    }

    std::cerr << "Cliente id no encontrado." << std::endl;
    return -1;
}

int Server::enviarMensaje(int sock, char* msj, int size_msj)
{
    int sent = 0;
    int s = 0;
    bool is_the_socket_valid = true;

    while(sent < size_msj && is_the_socket_valid)
    {
        s = send(sock, &msj[sent], size_msj - sent, 0);

        if (s == 0)
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


int Server::recibirMensaje(int sock, char* msj, int size_msj)
{
    int received = 0;
    int s = 0;
    bool is_the_socket_valid = true;


    while(received < size_msj && is_the_socket_valid) // tendria q ser size_msj = 17 ni idea porq
    {
        //std::cout << "4" << std::endl;
        s = recv(sock, &msj[received], size_msj - received, 0);
        //std::cout << s << std::endl;
        if (s == 0)
        {
            return 0;
        }
        else if (s < 0)
        {
            is_the_socket_valid = false;
        }
        else
        {
            received += s;
            //std::cout << "1" << std::endl;
        }
        //std::cout << "2" << std::endl;
    }
   //std::cout << "3" << std::endl;

    if(is_the_socket_valid){
        return received;

    }
    else{
        return 0;
    }
}
