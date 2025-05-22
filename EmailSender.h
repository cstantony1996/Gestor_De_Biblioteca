#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <string>

using namespace std;

class EmailSender {
public:
    EmailSender();
    ~EmailSender();

    // Envía correo con cualquier cuerpo
    static bool sendEmail(const string& to, const string& subject, const string& body);

    // Correos predefinidos comunes
    static bool sendReminderEmail(const string& to, const string& bookTitle, const string& returnDate);
    static bool sendLoanNotification(const string& to, const string& bookTitle, const string& returnDate);
    static bool sendWelcomeEmail(const string& to, const string& username);


    // Inicialización global libcurl
    static bool globalInit();
    static void globalCleanup();

private:
    static size_t payloadSource(void *ptr, size_t size, size_t nmemb, void *userp);
};

#endif // EMAILSENDER_H
