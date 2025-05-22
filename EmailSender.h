#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <string>

class EmailSender {
public:
    EmailSender();
    ~EmailSender();

    // Envía correo con cualquier cuerpo
    bool sendEmail(const std::string& to, const std::string& subject, const std::string& body);

    // Correos predefinidos comunes
    static bool sendReminderEmail(const std::string& to, const std::string& bookTitle, const std::string& returnDate);
    static bool sendLoanNotification(const std::string& to, const std::string& bookTitle, const std::string& returnDate);

    // Inicialización global libcurl
    static bool globalInit();
    static void globalCleanup();

private:
    static size_t payloadSource(void *ptr, size_t size, size_t nmemb, void *userp);
};

#endif // EMAILSENDER_H
