#include "byte_fix.h"
#include "EmailSender.h"
#include "Credenciales.h"
#include <curl/curl.h>
#include <cstring>
#include <iostream>
#include <ctime>
#include <vector>
#include <stdexcept>

using namespace std;

struct UploadStatus
{
    size_t bytes_read;
    const char **payload_text;
};

EmailSender::EmailSender() {}

EmailSender::~EmailSender() {}

bool EmailSender::globalInit()
{
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        cerr << "curl_global_init() failed: " << curl_easy_strerror(res) << endl;
        return false;
    }
    return true;
}

void EmailSender::globalCleanup()
{
    curl_global_cleanup();
}

size_t EmailSender::payloadSource(void *ptr, size_t size, size_t nmemb, void *userp)
{
    UploadStatus *upload_ctx = (UploadStatus *)userp;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
        return 0;

    const char *data = upload_ctx->payload_text[upload_ctx->bytes_read];

    if (data)
    {
        size_t len = strlen(data);
        memcpy(ptr, data, len);
        upload_ctx->bytes_read++;
        return len;
    }

    return 0;
}

bool EmailSender::sendEmail(const string &to, const string &subject, const string &body)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Error initializing curl" << endl;
        return false;
    }

    // Convertir saltos de línea a CRLF
    string bodyWithCRLF;
    for (char c : body)
    {
        if (c == '\n')
            bodyWithCRLF += "\r\n";
        else
            bodyWithCRLF += c;
    }

    // Construir encabezados
    string toHeader = "To: " + to + "\r\n";
    string fromHeader = "From: " + Credenciales::getEmail() + "\r\n";
    string subjectHeader = "Subject: " + subject + "\r\n";

    char dateHeader[100];
    time_t now = time(nullptr);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(dateHeader, sizeof(dateHeader), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", &tm_now);

    // Guardar las líneas como strings persistentes
    vector<string> headers = {
        string(dateHeader),
        toHeader,
        fromHeader,
        subjectHeader,
        "MIME-Version: 1.0\r\n",
        "Content-Type: text/plain; charset=utf-8\r\n",
        "Content-Transfer-Encoding: 7bit\r\n",
        "\r\n",
        bodyWithCRLF,
        "\r\n"};

    // Crear arreglo de punteros const char*
    vector<const char *> payload_text;
    for (const auto &line : headers)
        payload_text.push_back(line.c_str());
    payload_text.push_back(nullptr); // Null-terminator

    UploadStatus upload_ctx = {0, payload_text.data()};

    curl_easy_setopt(curl, CURLOPT_USERNAME, Credenciales::getEmail().c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, Credenciales::getAppPassword().c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY); // STARTTLS
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, Credenciales::getEmail().c_str());

    struct curl_slist *recipients = nullptr;
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // Ruta al certificado de CA
    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/Users/casta/OneDrive/Escritorio/gmail/cacert.pem");

    // Opcional: depuración
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
    }

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

bool EmailSender::sendReminderEmail(const string &to, const string &bookTitle, const string &returnDate)
{
    EmailSender sender; // <- Bien
    string subject = "Recordatorio: Devolución próxima de libro";
    string body = "Estimado usuario,\n\n"
                       "Le recordamos que el libro \"" +
                       bookTitle + "\" debe ser devuelto el día " + returnDate + ".\n"
                                                                                 "Por favor, realice la devolución a tiempo para evitar multas.\n\n"
                                                                                 "Saludos,\nBiblioteca";
    return sender.sendEmail(to, subject, body); // <- Aquí estaba mal
}

bool EmailSender::sendLoanNotification(const string &to, const string &bookTitle, const string &returnDate)
{
    EmailSender sender;
    string subject = "Confirmación de préstamo de libro";
    string body = "Estimado usuario,\n\n"
                       "Se ha registrado el préstamo del libro \"" +
                       bookTitle + "\".\n"
                                   "La fecha de devolución es: " +
                       returnDate + ".\n\n"
                                    "Gracias por usar nuestra biblioteca.\n\nSaludos.";
    return sender.sendEmail(to, subject, body); // <- También aquí
}

bool EmailSender::sendWelcomeEmail(const string &to, const string &username)
{
    string subject = "¡Bienvenido al sistema de biblioteca!";
    string body = "Hola " + username + ",\n\n"
                                            "Tu cuenta ha sido registrada exitosamente.\n"
                                            "Ya puedes iniciar sesión y utilizar el sistema.\n\n"
                                            "Saludos,\n"
                                            "Equipo de la Biblioteca";

    EmailSender sender;
    return sender.sendEmail(to, subject, body); // ✅ correcto
}
