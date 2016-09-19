#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>

#include <QSslSocket>
#include <QSslError>

namespace QMailer {

    enum ConnectionState {
        Tls, HandShake, Auth, User, Pass, Rcpt,
        Mail, Data, Init, Body, Quit, Close
    };

    enum ConnectionType {
        TcpConnection, SslConnection, TlsConnection
    };

    enum SmtpError
    {
        ConnectionTimeoutError, ResponseTimeoutError, SendDataTimeoutError,
        AuthenticationFailedError, ServerError, ClientError
    };

    class SmtpClient : public QObject
    {
        Q_OBJECT

    public:
        explicit SmtpClient(QObject *parent = 0);
        ~SmtpClient();

        QString host() const;
        void setHost(const QString &host);

        int port() const;
        void setPort(const int &port);

        QString user() const;
        void setUser(const QString &user);

        QString password() const;
        void setPassword(const QString &password);

        void setConnectionType(const ConnectionType &type);

        QString socketErrorString();

        void send(const QString &from, const QString &to, const QString &subject, const QString &body);

    signals:
        void s_messageIsSent();
        void s_smtpError(SmtpError);
        void s_destruct(SmtpClient*);

    private slots:
        void socketReadyRead();
        void socketConnected();
        void socketEncrypted();
        void socketDisconnected();
        void socketStateChanged(const QAbstractSocket::SocketState &state);
        void socketSslErrors(const QList<QSslError> &errors);
        void socketError(const QAbstractSocket::SocketError &error);

        // Vérifie si la connexion de la socket est bien crpté
        void timerSocketEncrypted();

        void smtpError(const SmtpError &error);

    private:
        QScopedPointer<QTcpSocket> m_socket;
        QScopedPointer<QTextStream> m_flux;
        ConnectionState m_state;
        ConnectionType m_connectionType;

        QString m_host;
        int m_port;
        int m_timeout;

        QString m_user;
        QString m_password;
        QString m_message;
        QString m_form;
        QString m_to;

        QString m_response;

        void write(const QString &text);
        void socket220Response();
        void socket235Response();
        void socket250Response();
        void socket334Response();
        void socket354Response();
        void socketDefaultCodeResponse();

        void socketEhloEncryption();
        void socketEhloResponse();
        void socketHandshakeResponse();

        void socketAuthPlainResponse();
        void socketAuthLoginResponse();

        void initializeSocket();
        void initializeFlux();
    };
}

#endif // SMTPCLIENT_H
