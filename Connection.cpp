#include "byte_fix.h" 
#include "Connection.h"
#include <iostream>

using namespace std;

PGconn *conectarDB()
{
    cout << "Inicia conexión a la base de datos. " << endl;

    const char *conninfo = "dbname=postgres user=postgres password=Myroot host=localhost port=5432 client_encoding=UTF8";
    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        cerr << "Error al conectar a la base de datos: " << PQerrorMessage(conn) << endl;
        PQfinish(conn);

        return nullptr;
    }

    cout << "Conexión exitosa." << endl;
    return conn;
}