#include "Connection.h"
#include "EmailSender.h"
#include "StringUtils.h"
#include <libpq-fe.h>
#include <ctime>
#include <iostream>

void EnviarRecordatoriosDevolucion() {
    PGconn* conn = conectarDB();
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Error de conexión: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return;
    }

    // Obtener la fecha de mañana en formato YYYY-MM-DD
    time_t t = time(nullptr) + 86400; // 1 día = 86400 segundos
    tm* tm_manana = localtime(&t);
    char fechaManana[11];
    strftime(fechaManana, sizeof(fechaManana), "%Y-%m-%d", tm_manana);

    const char* params[1] = { fechaManana };
    PGresult* res = PQexecParams(
        conn,
        "SELECT p.id, u.email, l.titulo, p.fecha_devolucion "
        "FROM prestamos p "
        "JOIN usuarios u ON p.usuario_id = u.id "
        "JOIN libros l ON p.libro_id = l.id "
        "WHERE p.fecha_devolucion = $1 AND p.recordatorio_enviado = FALSE",
        1, nullptr, params, nullptr, nullptr, 0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Error al obtener préstamos próximos a devolver: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        PQfinish(conn);
        return;
    }

    int total = PQntuples(res);
    for (int i = 0; i < total; ++i) {
        int prestamoId = atoi(PQgetvalue(res, i, 0));        // id del préstamo
        std::string email = PQgetvalue(res, i, 1);
        std::string tituloLibro = PQgetvalue(res, i, 2);
        std::string fechaDev = PQgetvalue(res, i, 3);

        try {
            EmailSender::sendReminderEmail(email, tituloLibro, fechaDev);
            std::cout << "[OK] Recordatorio enviado a " << email << " por el libro '" << tituloLibro << "'." << std::endl;

            // Actualizar la base de datos para marcar que ya se envió el recordatorio
            std::string updateQuery = "UPDATE prestamos SET recordatorio_enviado = TRUE WHERE id = " + std::to_string(prestamoId);
            PGresult* resUpdate = PQexec(conn, updateQuery.c_str());
            PQclear(resUpdate);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] No se pudo enviar el recordatorio a " << email << ": " << e.what() << std::endl;
        }
    }

    PQclear(res);
    PQfinish(conn);
}
