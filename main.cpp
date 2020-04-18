
#include <QCoreApplication>

#include "drone_client.h"

using namespace std;

int main(int argc, char ** argv ){

    QCoreApplication a(argc, argv);

    DroneClient::SInitSettings settings;
    settings.argc = argc;
    settings.argv = argv;
    if( DroneClient::singleton().init(settings) ){
        DroneClient::singleton().launch();
    }
    else{
        return EXIT_FAILURE;
    }

    return a.exec();
}
