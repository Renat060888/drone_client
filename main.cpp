
#include <QCoreApplication>

#include "drone_client.h"

using namespace std;

int main(int argc, char ** argv ){

    QCoreApplication a(argc, argv);

    DroneClient droneClient;
    DroneClient::SInitSettings settings;

    if( droneClient.init(settings) ){
        droneClient.launch();
    }
    else{
        return EXIT_FAILURE;
    }

    return a.exec();
}
