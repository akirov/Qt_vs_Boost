#include <QCoreApplication>
#include <QStringList>
#include "Client.hpp"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int port=SERVER_PORT;
    std::string server = "";

    // Parse command line arguments
    for ( int i=1; i<app.arguments().size(); ++i)
    {
        std::string opt = app.arguments().at(i).toStdString();

        if( opt.find("p=") == 0 )
        {
            int p=0;
            if ( (sscanf(opt.substr(2).c_str(), "%d", &p) == 1) && p>1024 && p<65536 )
            {
                port = p;
            }
            else
            {
                qDebug() << "Invalid port number. Must be between 1024 and 65535."
                         << "Falling back to " << SERVER_PORT << ".";
            }
        }
        else if( opt.find("s=") == 0 )
        {
            server = opt.substr(2).c_str();
        }
        else
        {
            qDebug() << "Unknown option : " << opt.c_str() << "."
                     << " Known options : s=SERVER_ADDRESS p=PORT";
        }
    }

    if( server == "" )
    {
        qDebug() << "Server address not specified. Specify it with s=SERVER_ADDRESS";
        exit(0);
    }

    Client *aClient = new Client(server, port, &app);
    QObject::connect(aClient, SIGNAL(finished()), &app, SLOT(quit()));
    QMetaObject::invokeMethod( aClient, "Start", Qt::QueuedConnection );
//  aClient.Start();

    // aClient will be deleted by its parent (app)
    return app.exec();
}
