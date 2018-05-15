#include <iostream>
#include "Log.h"
#include "Server.h"


int main()
{
    std::cout << "Running!" << std::endl;
    Log* logger = new Log(3);

    Server *s;
    s = new Server();

    //Main loop
    s->AcceptAndDispatch();

    return 0;
}
