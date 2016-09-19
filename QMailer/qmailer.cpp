#include "qmailer.h"

namespace QMailer {

    QMailer::QMailer(QObject *parent) : QObject(parent)
    {

    }

    QString QMailer::host() const
    {
        return m_host;
    }

    void QMailer::setHost(const QString &host)
    {
        m_host = host;
    }

    int QMailer::port() const
    {
        return m_port;
    }

    void QMailer::setPort(const int &port)
    {
        m_port = port;
    }

    QString QMailer::user() const
    {
        return m_user;
    }

    void QMailer::setUser(const QString &user)
    {
        m_user = user;
    }

    QString QMailer::password() const
    {
        return m_password;
    }

    void QMailer::setPassword(const QString &password)
    {
        m_password = password;
    }

    void QMailer::setConnectionType(const ConnectionType &type)
    {
        m_connectionType = type;
    }

    int QMailer::activeSmtpClientCount()
    {
        return m_clients.count();
    }

    void QMailer::sendMail(const QString &from, const QString &to, const QString &subject, const QString &body)
    {
        // Envoie d'un mail

        SmtpClient *client(new SmtpClient(this));
        m_clients << client;

        connect(client, SIGNAL(s_messageIsSent()),
                this, SIGNAL(s_messageIsSent()));

        connect(client, SIGNAL(s_smtpError(SmtpError)),
                this, SIGNAL(s_smtpError(SmtpError)));

        connect(client, SIGNAL(s_destruct(SmtpClient*)),
                this, SLOT(smtpClientDestruct(SmtpClient*)));

        client->setUser(m_user);
        client->setPort(m_port);

        client->setHost(m_host);
        client->setPassword(m_password);
        client->setConnectionType(m_connectionType);

        client->send(from, to, subject, body);
    }

    void QMailer::smtpClientDestruct(SmtpClient *client)
    {
        m_clients.removeOne(client);
    }
}
