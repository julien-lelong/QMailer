#ifndef QMAILER_H
#define QMAILER_H

#include <QObject>
#include "smtpclient.h"

namespace QMailer {
    class QMailer : public QObject
    {
        Q_OBJECT

    public:
        explicit QMailer(QObject *parent = 0);

        QString host() const;
        void setHost(const QString &host);

        int port() const;
        void setPort(const int &port);

        QString user() const;
        void setUser(const QString &user);

        QString password() const;
        void setPassword(const QString &password);

        void setConnectionType(const ConnectionType &type);

        /**
         * @brief activeClientCount
         * Compte le nombre de mail en cours d'envoi
         *
         * @return
         */
        int activeSmtpClientCount();

        void sendMail(const QString &from, const QString &to, const QString &subject, const QString &body);

    signals:
        void s_messageIsSent();
        void s_smtpError(SmtpError);

    private slots:
        void smtpClientDestruct(SmtpClient* client);

    private:
        ConnectionType m_connectionType;

        QString m_host;
        int m_port;

        QString m_user;
        QString m_password;

        QList<SmtpClient*> m_clients;
    };
}
#endif // QMAILER_H
