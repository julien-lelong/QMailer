#include "smtpclient.h"

#include <QTimer>

namespace QMailer {

    SmtpClient::SmtpClient(QObject *parent) : QObject(parent),
        m_socket(0), m_flux(0),
        m_connectionType(ConnectionType::TcpConnection),
        m_port(0), m_timeout(5000)
    {
        connect(this, SIGNAL(s_smtpError(SmtpError)),
                this, SLOT(smtpError(SmtpError)));
    }

    SmtpClient::~SmtpClient()
    {
        emit s_destruct(this);
    }

    QString SmtpClient::host() const
    {
        return m_host;
    }

    void SmtpClient::setHost(const QString &host)
    {
        m_host = host;
    }

    int SmtpClient::port() const
    {
        return m_port;
    }

    void SmtpClient::setPort(const int &port)
    {
        m_port = port;
    }

    QString SmtpClient::user() const
    {
        return m_user;
    }

    void SmtpClient::setUser(const QString &user)
    {
        m_user = user;
    }

    QString SmtpClient::password() const
    {
        return m_password;
    }

    void SmtpClient::setPassword(const QString &password)
    {
        m_password = password;
    }

    void SmtpClient::setConnectionType(const ConnectionType &type)
    {
        m_connectionType = type;

        initializeSocket();
    }

    QString SmtpClient::socketErrorString()
    {
        return m_socket->errorString();
    }

    void SmtpClient::send(const QString &from, const QString &to, const QString &subject, const QString &body)
    {
        m_message = "To: " + to + "\n";
        m_message.append("From: " + from + "\n");
        m_message.append("Subject: " + subject + "\n");
        m_message.append(body);
        m_message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
        m_message.replace(QString::fromLatin1("\r\n.\r\n"), QString::fromLatin1("\r\n..\r\n"));

        m_to = to;
        m_form = from;
        m_state = Init;

        initializeFlux();

        // Pour les connexions Ssl
        if (m_connectionType == SslConnection) {
            ((QSslSocket*) m_socket.data())->connectToHostEncrypted(m_host, m_port);

        // Pour les connexions Tls
        } else if (m_connectionType == TlsConnection || m_connectionType == TcpConnection) {
            m_socket->connectToHost(m_host, m_port);
        }

        if (!m_socket->waitForConnected(m_timeout)) {
             emit s_smtpError(ConnectionTimeoutError);
        }
    }

    // PRIVATE SLOTS

    void SmtpClient::socketReadyRead()
    {
        QString responseLine;
        int responseCode = 0;

        do {
            responseLine = m_socket->readLine();
            m_response += responseLine;
        } while (m_socket->canReadLine() && responseLine[3] != ' ');

        responseLine.truncate(3);
        responseCode = responseLine.toInt();

        #ifdef QT_DEBUG
            qDebug() << "Server response code:" << responseLine;
            qDebug() << "Server response: " << m_response;
        #endif

        switch (responseCode) {

        // Premier code envoyé par le serveur lorsque la connexion s'est effectuée avec succès.
        case 220:
            socket220Response();
            break;

        case 235:
            socket235Response();
            break;

        // Confirmation de commande acceptée.
        case 250:
            socket250Response();
            break;

        case 334:
            socket334Response();
            break;

        // Réponse à la commande DATA. Le serveur attend les données du corps du message.
        // Le client indique la fin du message par un point seul sur une ligne : <CR><LF>.<CR><LF>
        case 354:
            socket354Response();
            break;

        default:
            socketDefaultCodeResponse();
            break;
        }

        m_response = "";
    }

    void SmtpClient::socketConnected()
    {
        #ifdef QT_DEBUG
            qDebug() << "Conneted";
        #endif
    }

    void SmtpClient::socketEncrypted()
    {
        #ifdef QT_DEBUG
            qDebug() << "Encrypted";
        #endif

        socketEhloEncryption();
    }

    void SmtpClient::socketDisconnected()
    {
        #ifdef QT_DEBUG
            qDebug() << "Disconneted";
            qDebug() << socketErrorString();
        #endif
    }

    void SmtpClient::socketStateChanged(const QAbstractSocket::SocketState &state)
    {
        Q_UNUSED(state)

        #ifdef QT_DEBUG
            qDebug() << "StateChanged " << state;
        #endif
    }

    void SmtpClient::socketSslErrors(const QList<QSslError> &errors)
    {
        Q_UNUSED(errors)

        #ifdef QT_DEBUG
            for (QSslError error: errors) {
                qDebug() << "SslErrors : " + socketErrorString();
            }
        #endif
    }

    void SmtpClient::socketError(const QAbstractSocket::SocketError &error)
    {
        Q_UNUSED(error)

        #ifdef QT_DEBUG
            qDebug() << "Error" << error;
        #endif
    }

    void SmtpClient::timerSocketEncrypted()
    {
        if (!((QSslSocket*) m_socket.data())->isEncrypted()) {
            emit s_smtpError(ConnectionTimeoutError);
            m_state = Close;
        }
    }

    void SmtpClient::smtpError(const SmtpError &error)
    {
        Q_UNUSED(error)

        #ifdef QT_DEBUG
            qDebug() << socketErrorString();
        #endif
    }

    void SmtpClient::write(const QString &text)
    {
        if (!m_flux) {
            qDebug() << "SmtpClient::write, m_flux ne peut être null !";
            return;
        }

        *m_flux << text.toUtf8() << "\r\n";
        m_flux->flush();
    }

    void SmtpClient::socket220Response()
    {
        if (m_state == Init) {
            // Banner was okay, let's go on
            socketEhloResponse();
            m_state = (m_connectionType == TlsConnection) ? Tls : HandShake;

        } else if (m_state == HandShake) {
            socketHandshakeResponse();
        }
    }

    void SmtpClient::socket235Response()
    {
        if (m_state != Mail) {
            return;
        }

        // HELO réponse okay
        // Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated
        // the following way -> <email@gmail.com>
        qDebug() << "MAIL FROM:<" << m_form << ">";
        write("MAIL FROM:<" + m_form + ">");
        m_state = Rcpt;
    }

    void SmtpClient::socket250Response()
    {
        if (m_state == ConnectionState::Tls) {

            qDebug() << "STarting Tls";
            write("STARTTLS");
            m_state = HandShake;

        } else if (m_state == ConnectionState::HandShake) {

            socketHandshakeResponse();

        } else if (m_state == ConnectionState::Auth) {

            // Trying AUTH
            socketAuthLoginResponse();

        } else if (m_state == ConnectionState::Rcpt) {

            // Apperantly for Google it is mandatory to have MAIL FROM and RCPT email
            // formated the following way -> <email@gmail.com>
            write("RCPT TO:<" + m_to + ">");
            m_state = Data;

        } else if (m_state == ConnectionState::Data) {

            write("DATA");
            m_state = Body;

        } else if (m_state == ConnectionState::Quit) {

            write("QUIT");
            // here, we just close.
            m_state = Close;
            emit s_messageIsSent();
        }
    }

    void SmtpClient::socket334Response()
    {
        if (m_state == ConnectionState::User) {

            //Trying User
            qDebug() << "Username";
            //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
            //https://developers.google.com/gmail/xoauth2_protocol
            write(QByteArray().append(m_user).toBase64());
            m_state = Pass;

        } else if (m_state == ConnectionState::Pass) {

            //Trying pass
            qDebug() << "Pass";
            write(QByteArray().append(m_password).toBase64());
            m_state = Mail;
        }
    }

    void SmtpClient::socket354Response()
    {
        if (m_state != ConnectionState::Body) {
            return;
        }

        write(m_message + "\r\n.");
        m_state = Quit;
    }

    void SmtpClient::socketDefaultCodeResponse()
    {
        if (m_state != ConnectionState::Close) {
            m_state = Close;
            qDebug() << "Failed to send message";
        }

        deleteLater();
    }

    void SmtpClient::socketEhloEncryption()
    {
        // Send EHLO once again but now encrypted
        socketEhloResponse();
        m_state = Auth;
    }

    void SmtpClient::socketEhloResponse()
    {
        if (!m_flux) {
            return;
        }

        write("EHLO localhost");
    }

    void SmtpClient::socketHandshakeResponse()
    {
        if (m_connectionType == SslConnection || m_connectionType == TlsConnection) {

            if (((QSslSocket*) m_socket.data())->isEncrypted()) {
                socketEhloEncryption();
            } else {
                ((QSslSocket*) m_socket.data())->startClientEncryption();
                // Vérifie si la connexion a bien été crypté
                QTimer::singleShot(m_timeout, this, SLOT(timerSocketEncrypted()));
            }

        } else {
            socketAuthLoginResponse();
        }
    }

    void SmtpClient::socketAuthPlainResponse()
    {
        // Trying AUTH
        qDebug() << "AuthPlain";
        // Sending command: AUTH PLAIN base64('\0' + username + '\0' + password)
        write("AUTH PLAIN " + QByteArray().append((char) 0).append(m_user).append((char) 0).append(m_password).toBase64());
        m_state = User;
    }

    void SmtpClient::socketAuthLoginResponse()
    {
        // Trying AUTH
        qDebug() << "AuthLogin";
        write("AUTH LOGIN");
        m_state = User;
    }

    void SmtpClient::initializeSocket()
    {
        if (m_connectionType == TcpConnection) {

            m_socket.reset(new QTcpSocket);

        // On utilise l'objet QSslSocket pour les connexions ssl et tls
        } else {

            m_socket.reset(new QSslSocket);

            connect(m_socket.data(), SIGNAL(sslErrors(QList<QSslError>)),
                    this,SLOT(socketSslErrors(QList<QSslError>)));

            connect(m_socket.data(), SIGNAL(encrypted()),
                    this, SLOT(socketEncrypted()));
        }

        connect(m_socket.data(), SIGNAL(readyRead()),
                this, SLOT(socketReadyRead()));

        connect(m_socket.data(), SIGNAL(connected()),
                this, SLOT(socketConnected()));

        connect(m_socket.data(), SIGNAL(disconnected()),
                this,SLOT(socketDisconnected()));

        connect(m_socket.data(), SIGNAL(error(QAbstractSocket::SocketError)),
                this,SLOT(socketError(QAbstractSocket::SocketError)));

        connect(m_socket.data(), SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    }

    void SmtpClient::initializeFlux()
    {
        if (!m_socket) {
            return;
        }

        m_flux.reset(new QTextStream(m_socket.data()));
    }
}
