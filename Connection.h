#ifndef CONNECTION_H
#define CONNECTION_H

#include "byte_fix.h" 
 
#include <libpq-fe.h>
 
PGconn* conectarDB();  // Asegúrate de que este nombre coincida
 
#endif