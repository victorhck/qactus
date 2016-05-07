/*
 *  Qactus - A Qt based OBS notifier
 *
 *  Copyright (C) 2013-2016 Javier Llorente <javier@opensuse.org>
 *  Copyright (C) 2010-2011 Sivan Greenberg <sivan@omniqueue.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "obsaccess.h"

OBSAccess *OBSAccess::instance = NULL;
const QString OBSAccess::userAgent = QString("Qactus") + " " + QACTUS_VERSION;

OBSAccess::OBSAccess()
{
    authenticated = false;
    xmlReader = OBSXmlReader::getInstance();
    createManager();
}

void OBSAccess::createManager()
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            SLOT(provideAuthentication(QNetworkReply*,QAuthenticator*)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> &)));
}

OBSAccess *OBSAccess::getInstance()
{
    if (!instance) {
        instance = new OBSAccess();
    }
    return instance;
}

void OBSAccess::setCredentials(const QString& username, const QString& password)
{
//    Allow login with another username/password
    delete manager;
    createManager();

    curUsername = username;
    curPassword = password;
}

QString OBSAccess::getUsername()
{
    return curUsername;
}

void OBSAccess::request(const QString &urlStr)
{
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());  
    manager->get(request);

//    Don't make a new request until we get a reply
    QEventLoop *loop = new QEventLoop;
    connect(manager, SIGNAL(finished(QNetworkReply *)),loop, SLOT(quit()));
    loop->exec();
}

QNetworkReply *OBSAccess::browseRequest(const QString &urlStr)
{
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    QNetworkReply *reply = manager->get(request);
    return reply;
}

void OBSAccess::getProjects(const QString &urlStr)
{
    QNetworkReply *reply = browseRequest(urlStr);
    reply->setProperty("reqtype", OBSAccess::ProjectList);
}

void OBSAccess::getPackages(const QString &urlStr)
{
    QNetworkReply *reply = browseRequest(urlStr);
    reply->setProperty("reqtype", OBSAccess::PackageList);
}

void OBSAccess::getFiles(const QString &urlStr)
{
    QNetworkReply *reply = browseRequest(urlStr);
    reply->setProperty("reqtype", OBSAccess::FileList);
}

void OBSAccess::getAllBuildStatus(const QString &urlStr)
{
    QNetworkReply *reply = browseRequest(urlStr);
    reply->setProperty("reqtype", OBSAccess::BuildStatusList);
}

void OBSAccess::request(const QString &urlStr, const int &row)
{
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("row", row);
}

void OBSAccess::postRequest(const QString &urlStr, const QByteArray &data)
{
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(request, data);

//    Make it a synchronous call
    QEventLoop *loop = new QEventLoop;
    connect(manager, SIGNAL(finished(QNetworkReply *)),loop, SLOT(quit()));
    loop->exec();
}

void OBSAccess::provideAuthentication(QNetworkReply *reply, QAuthenticator *ator)
{
    qDebug() << "OBSAccess::provideAuthentication()";
    static QString prevPassword = "";
    static QString prevUsername = "";
//    qDebug() << reply->readAll();

    if (reply->error()!=QNetworkReply::NoError) {
            qDebug() << "Request failed!" << reply->errorString();
//            statusBar()->showMessage(tr("Connection failed"), 0);
    } else {
        if ((curPassword != prevPassword) || (curUsername != prevUsername)) {
            prevPassword = curPassword;
            prevUsername = curUsername;
            ator->setUser(curUsername);
            ator->setPassword(curPassword);
//            statusBar()->showMessage(tr("Authenticating..."), 5000);
        } else {
            qDebug() << "Authentication failed";
            prevPassword = "";
            prevUsername = "";
            authenticated = false;
            emit isAuthenticated(authenticated);
        }
    }

    if (reply->error()==QNetworkReply::NoError) {
        qDebug() << "User is authenticated";
        authenticated = true;
        emit isAuthenticated(authenticated);
    }
}

bool OBSAccess::isAuthenticated()
{
    return authenticated;
}

void OBSAccess::replyFinished(QNetworkReply *reply)
{
    qDebug() << "OBSAccess::replyFinished()";

    // QNetworkReply is a sequential-access QIODevice, which means that
    // once data is read from the object, it no longer kept by the device.
    // It is therefore the application's responsibility to keep this data if it needs to.
    // See http://doc.qt.nokia.com/latest/qnetworkreply.html for more info

    QString data = QString::fromUtf8(reply->readAll());
    qDebug() << "URL:" << reply->url();
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP status code:" << httpStatusCode;
//    qDebug() << "Network Reply: " << data;

    /* Set package row always (error/no error) if property is valid.
     * Needed for inserting the build status
     */
    if(reply->property("row").isValid()) {
        int row = reply->property("row").toInt();
        qDebug() << "Reply row property:" << QString::number(row);
        xmlReader->setPackageRow(row);
    }

    switch (reply->error()) {

    case QNetworkReply::NoError:
        qDebug() << "Request succeeded!";
        if(data.startsWith("Index")) {
            // Don't process diffs
            requestDiff = data;
            return;
        }
        if (reply->property("reqtype").isValid()) {
            QString reqType = "RequestType";

            switch(reply->property("reqtype").toInt()) {

            case OBSAccess::ProjectList: // <directory>
                qDebug() << reqType << "ProjectList";
                xmlReader->parseProjectList(data);
                break;

            case OBSAccess::PackageList: // <directory>
                qDebug() << reqType << "PackageList";
                xmlReader->parsePackageList(data);
                break;

            case OBSAccess::FileList: // <directory>
                qDebug() << reqType << "FileList";
                xmlReader->parseFileList(data);
                break;

            case OBSAccess::BuildStatusList: // <resultlist>
                qDebug() << reqType << "BuildStatusList";
                xmlReader->parseResultList(data);
                break;
            }
            return;
        }
        xmlReader->addData(data);
        break;

    case QNetworkReply::ContentNotFoundError: // 404
        // Package/Project not found
        if (isAuthenticated()) {
            xmlReader->addData(data);
        }
        break;

    case QNetworkReply::ContentAccessDenied: // 401
        qDebug() << "Access denied!";
        break;

    default: // Other errors
        qDebug() << "Request failed! Error:" << reply->errorString();
        emit networkError(reply->errorString());
        break;
    }

    reply->deleteLater();
}

QString OBSAccess::getRequestDiff()
{
    return requestDiff;
}

void OBSAccess::onSslErrors(QNetworkReply* reply, const QList<QSslError> &list)
{
    QString errorString;
    QString message;

    foreach (const QSslError &sslError, list) {
        if (list.count() >= 1) {
            errorString += ", ";
            errorString = sslError.errorString();
            if (sslError.error() == QSslError::SelfSignedCertificateInChain) {
                qDebug() << "Self signed certificate!";
                emit selfSignedCertificate(reply);
            }
        }
    }
    qDebug() << "SSL Errors:" << errorString;

    if (list.count() == 1) {
        message=tr("An SSL error has occured: %1");
    } else {
        message=list.count()+tr(" SSL errors have occured: %1");
    }

   qDebug() << "onSslErrors() url:" << reply->url() << "row:" << reply->property("row").toInt();
}
