#include <string>
#include <QCoreApplication>
#include <QStringList>
#include "Server.hpp"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int port=SERVER_PORT, chanNum=DEF_CHANNELS, timeInt=TICK_INTERVAL;

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
        else if( opt.find("ch=") == 0 )
        {
            int c=0;
            if ( (sscanf(opt.substr(3).c_str(), "%d", &c) == 1) && c>=1 && c<=MAX_CHANNELS )
            {
                chanNum = c;
            }
            else
            {
                qDebug() << "Invalid number of channels. Must be between " 
                         << "1 and " << MAX_CHANNELS 
                         << ". Falling back to " << DEF_CHANNELS << ".";
            }
        }
        else if( opt.find("t=") == 0 )
        {
            int t=0;
            if ( (sscanf(opt.substr(2).c_str(), "%d", &t) == 1) && t >= 1 )
            {
                timeInt = t;
            }
            else
            {
                qDebug() << "Invalid time interval. Must start from 1 second."
                         << "Falling back to " << TICK_INTERVAL << ".";
            }
        }
        else
        {
            qDebug() << "Unknown option : " << opt.c_str() << "."
                     << " Known options : p=PORT ch=CHAN_NUMBER t=TIME_INTERVAL";
        }
    }

    Server *aServer = new Server(port, chanNum, timeInt, &app);
    QObject::connect(aServer, SIGNAL(finished()), &app, SLOT(quit()));
    QMetaObject::invokeMethod( aServer, "Start", Qt::QueuedConnection );
//  aServer.Start();

    // aServer will be deleted by its parent (app)
    return app.exec();
}
