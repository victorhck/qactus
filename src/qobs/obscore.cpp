/*
 *  Qactus - A Qt based OBS notifier
 *
 *  Copyright (C) 2013-2018 Javier Llorente <javier@opensuse.org>
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

#include "obscore.h"

OBSCore *OBSCore::instance = NULL;
const QString userAgent = APP_NAME + QString(" ") + QACTUS_VERSION;

OBSCore::OBSCore()
{
    authenticated = false;
    xmlReader = OBSXmlReader::getInstance();
    manager = nullptr;
}

void OBSCore::createManager()
{
    qDebug() << "OBSCore::createManager()";
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            SLOT(provideAuthentication(QNetworkReply*,QAuthenticator*)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> &)));
}

OBSCore *OBSCore::getInstance()
{
    if (!instance) {
        instance = new OBSCore();
    }
    return instance;
}

void OBSCore::setCredentials(const QString& username, const QString& password)
{
    qDebug() << "OBSCore::setCredentials()";
//    Allow login with another username/password
    if (manager!=nullptr) {
        delete manager;
        manager = nullptr;
    }
    createManager();

    curUsername = username;
    curPassword = password;
}

QString OBSCore::getUsername()
{
    return curUsername;
}

void OBSCore::setApiUrl(const QString &apiUrl)
{
    this->apiUrl = apiUrl;
}

QString OBSCore::getApiUrl() const
{
    return apiUrl;
}

QNetworkReply *OBSCore::request(const QString &resource)
{
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl + resource));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());  
    QNetworkReply *reply = manager->get(request);
    return reply;
}

QNetworkReply *OBSCore::requestBuild(const QString &resource)
{
    return request("/build/" + resource);
}

void OBSCore::requestBuild(const QString &resource, int row)
{
    request("/build/" + resource, row);
}

QNetworkReply *OBSCore::requestSource(const QString &resource)
{
    return request("/source/" + resource);
}

void OBSCore::requestRequest(const QString &resource)
{
    request("/request/" + resource);
}

void OBSCore::getProjects()
{
    QNetworkReply *reply = requestSource("");
    reply->setProperty("reqtype", OBSCore::ProjectList);
}

void OBSCore::getProjectMetadata(const QString &resource)
{
    QNetworkReply *reply = requestSource(resource);
    reply->setProperty("reqtype", OBSCore::ProjectMetadata);
}

void OBSCore::getPackages(const QString &resource)
{
    QNetworkReply *reply = requestSource(resource);
    reply->setProperty("reqtype", OBSCore::PackageList);
}

void OBSCore::getFiles(const QString &resource)
{
    QNetworkReply *reply = requestSource(resource);
    reply->setProperty("reqtype", OBSCore::FileList);
}

void OBSCore::getAllBuildStatus(const QString &resource)
{
    QNetworkReply *reply = requestBuild(resource);
    reply->setProperty("reqtype", OBSCore::BuildStatusList);
}

void OBSCore::request(const QString &resource, int row)
{
    QNetworkReply *reply = request(resource);
    reply->setProperty("row", row);
}

QNetworkReply *OBSCore::postRequest(const QString &resource, const QByteArray &data)
{
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl + resource));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(request, data);

    return reply;
}

QNetworkReply *OBSCore::putRequest(const QString &resource, const QByteArray &data)
{
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl + resource));
    qDebug() << "User-Agent:" << userAgent;
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    QNetworkReply *reply = manager->put(request, data);

    return reply;
}

QNetworkReply *OBSCore::deleteRequest(const QString &resource)
{
    QNetworkRequest request;
    request.setUrl(QUrl(apiUrl + resource));
    request.setRawHeader("User-Agent", userAgent.toLatin1());
    QNetworkReply *reply = manager->deleteResource(request);

    return reply;
}

void OBSCore::changeSubmitRequest(const QString &resource, const QByteArray &data)
{
    QNetworkReply *reply = postRequest(resource, data);
    reply->setProperty("reqtype", OBSCore::SubmitRequest);
}

void OBSCore::provideAuthentication(QNetworkReply *reply, QAuthenticator *ator)
{
    qDebug() << "OBSCore::provideAuthentication() for" << reply->request().url().toString();
//    qDebug() << reply->readAll();

    if ((curPassword != prevPassword) || (curUsername != prevUsername)) {
        prevPassword = curPassword;
        prevUsername = curUsername;
        ator->setUser(curUsername);
        ator->setPassword(curPassword);
    } else {
        qDebug() << "OBSCore::provideAuthentication() same credentials provided!";
        // FIXME: Workaround to accept the same credentials
        // Not calling ator->setUser() or ator->setPassword() results in a signal
        // being emitted with a QNetworkReply with error AuthenticationRequiredError.
        // So, first time auth with same credentials fails, second time doesn't.
        // If we call ator->setUser()/ator->setPassword() directly (no if) with the
        // correct username and wrong password, it ends up in an infinite loop and
        // probably a blocked openSUSE account :-(
        prevUsername = "";
        prevPassword = "";
    }
}

bool OBSCore::isAuthenticated()
{
    return authenticated;
}

void OBSCore::replyFinished(QNetworkReply *reply)
{
    // QNetworkReply is a sequential-access QIODevice, which means that
    // once data is read from the object, it no longer kept by the device.
    // It is therefore the application's responsibility to keep this data if it needs to.
    // See http://doc.qt.nokia.com/latest/qnetworkreply.html for more info

    QString data = QString::fromUtf8(reply->readAll());
    qDebug() << "OBSCore::replyFinished()" << reply->url().toString();
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "OBSCore::replyFinished() HTTP status code:" << httpStatusCode;
//    qDebug() << "Network Reply: " << data;

    if (httpStatusCode==302) {
        authenticated = true;
        emit isAuthenticated(authenticated);
    } else if (httpStatusCode==401) {
        authenticated = false;
        emit isAuthenticated(authenticated);
    }

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
        qDebug() << "OBSCore::replyFinished() Request succeeded! Status code:" << httpStatusCode;

        if (reply->property("reqtype").isValid()) {
            QString reqType = "RequestType";

            switch(reply->property("reqtype").toInt()) {

            case OBSCore::ProjectList: // <directory>
                qDebug() << reqType << "ProjectList";
                xmlReader->parseProjectList(data);
                break;

            case OBSCore::ProjectMetadata: // <project>
                qDebug() << reqType << "ProjectList";
                xmlReader->parseProjectMetadata(data);
                break;

            case OBSCore::PackageList: // <directory>
                qDebug() << reqType << "PackageList";
                xmlReader->parsePackageList(data);
                break;

            case OBSCore::FileList: // <directory>
                qDebug() << reqType << "FileList";
                xmlReader->parseFileList(data);
                break;

            case OBSCore::BuildStatusList: // <resultlist>
                qDebug() << reqType << "BuildStatusList";
                xmlReader->parseResultList(data);
                break;

            case OBSCore::SubmitRequest:
                qDebug() << reqType << "SubmitRequest";
                xmlReader->parseSubmitRequest(data);
                break;

            case OBSCore::SRDiff:
                qDebug() << reqType << "SRDiff";
                emit srDiffFetched(data);
                break;

            case OBSCore::BranchPackage: {
                qDebug() << reqType << "BranchPackage";
                QString project;
                QString package;
                if (reply->property("branchprj").isValid()) {
                    project = reply->property("branchprj").toString();
                }
                if (reply->property("branchpkg").isValid()) {
                    package = reply->property("branchpkg").toString();
                }
                xmlReader->parseBranchPackage(project, package, data);
                break;
            }

            case OBSCore::CreateProject: {
                qDebug() << reqType << "CreateProject";
                QString project;
                if (reply->property("createprj").isValid()) {
                    project = reply->property("createprj").toString();
                }
                xmlReader->parseCreateProject(project, data);
                break;
            }

            case OBSCore::CreatePackage: {
                qDebug() << reqType << "CreatePackage";
                QString project;
                QString package;
                if (reply->property("createprj").isValid()) {
                    project = reply->property("createprj").toString();
                }
                if (reply->property("createpkg").isValid()) {
                    package = reply->property("createpkg").toString();
                }
                xmlReader->parseCreatePackage(project, package, data);
                break;
            }

            case OBSCore::UploadFile: {
                qDebug() << reqType << "UploadFile";
                QString project;
                QString package;
                QString file;
                if (reply->property("uploadprj").isValid()) {
                    project = reply->property("uploadprj").toString();
                }
                if (reply->property("uploadpkg").isValid()) {
                    package = reply->property("uploadpkg").toString();
                }
                if (reply->property("uploadfile").isValid()) {
                    file = reply->property("uploadfile").toString();
                }
                xmlReader->parseUploadFile(project, package, file, data);
                break;
            }

            case OBSCore::DeleteProject: {
                qDebug() << reqType << "DeleteProject";
                QString project;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                xmlReader->parseDeleteProject(project, data);
                break;
            }

            case OBSCore::DeletePackage: {
                qDebug() << reqType << "DeletePackage";
                QString project;
                QString package;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                if (reply->property("deletepkg").isValid()) {
                    package = reply->property("deletepkg").toString();
                }
                xmlReader->parseDeletePackage(project, package, data);
                break;
            }

            case OBSCore::DeleteFile: {
                qDebug() << reqType << "DeleteFile";
                QString project;
                QString package;
                QString fileName;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                if (reply->property("deletepkg").isValid()) {
                    package = reply->property("deletepkg").toString();
                }
                if (reply->property("deletefile").isValid()) {
                    fileName = reply->property("deletefile").toString();
                }
                xmlReader->parseDeleteFile(project, package, fileName, data);
                break;
            }

            case OBSCore::About:
                qDebug() << reqType << "About";
                xmlReader->parseAbout(data);
                break;
            }
            return;
        }
        xmlReader->addData(data);
        break; // end of case QNetworkReply::NoError

    case QNetworkReply::ContentNotFoundError: // 404
        // Package/Project not found
        if (isAuthenticated()) {
            xmlReader->addData(data);
        }
        break;

    case QNetworkReply::ContentAccessDenied: // 401
        qDebug() << "OBSCore::replyFinished() Access denied!";
        if (reply->property("reqtype").isValid()) {
            switch(reply->property("reqtype").toInt()) {
            OBSStatus *obsStatus;

            case OBSCore::CreateProject: {
                obsStatus = new OBSStatus();
                QString project;
                if (reply->property("createprj").isValid()) {
                    project = reply->property("createprj").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot create");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to create<br>%1")
                                      .arg(obsStatus->getProject()));
                emit cannotCreateProject(obsStatus);
                break;
            }

            case OBSCore::CreatePackage: {
                obsStatus = new OBSStatus();
                QString project;
                QString package;
                if (reply->property("createprj").isValid()) {
                    project = reply->property("createprj").toString();
                }
                if (reply->property("createpkg").isValid()) {
                    package = reply->property("createpkg").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setPackage(package);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot create");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to create<br>%1/%2")
                                      .arg(obsStatus->getProject(), obsStatus->getPackage()));
                emit cannotCreatePackage(obsStatus);
                break;
            }

            case OBSCore::UploadFile: {
                obsStatus = new OBSStatus();
                QString project;
                QString package;
                if (reply->property("uploadprj").isValid()) {
                    project = reply->property("uploadprj").toString();
                }
                if (reply->property("uploadpkg").isValid()) {
                    package = reply->property("uploadpkg").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setPackage(package);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot upload file");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to upload to <br>%1/%2")
                                      .arg(obsStatus->getProject(), obsStatus->getPackage()));
                emit cannotUploadFile(obsStatus);
                break;
            }

            case OBSCore::DeleteProject: {
                obsStatus = new OBSStatus();
                QString project;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot delete");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to delete<br>%1")
                                      .arg(obsStatus->getProject()));
                emit cannotDeleteProject(obsStatus);
                break;
            }

            case OBSCore::DeletePackage: {
                obsStatus = new OBSStatus();
                QString project;
                QString package;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                if (reply->property("deletepkg").isValid()) {
                    package = reply->property("deletepkg").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setPackage(package);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot delete");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to delete<br>%1/%2")
                                      .arg(obsStatus->getProject(), obsStatus->getPackage()));
                emit cannotDeletePackage(obsStatus);
                break;
            }

            case OBSCore::DeleteFile: {
                obsStatus = new OBSStatus();
                QString project;
                QString package;
                QString fileName;
                if (reply->property("deleteprj").isValid()) {
                    project = reply->property("deleteprj").toString();
                }
                if (reply->property("deletepkg").isValid()) {
                    package = reply->property("deletepkg").toString();
                }
                if (reply->property("deletefile").isValid()) {
                    fileName = reply->property("deletefile").toString();
                }
                obsStatus->setProject(project);
                obsStatus->setPackage(package);
                obsStatus->setCode("error");
                obsStatus->setSummary("Cannot delete");
                obsStatus->setDetails(tr("You don't have the appropriate permissions to delete<br>%1/%2/%3")
                                      .arg(obsStatus->getProject(), obsStatus->getPackage(), fileName));
                emit cannotDeleteFile(obsStatus);
                break;
            }

            }
        }
        break;

    case QNetworkReply::AuthenticationRequiredError:
        qDebug() << "OBSCore::replyFinished() Credentials provided not accepted!";
        break;

    default: // Other errors
        qDebug() << "OBSCore::replyFinished() Request failed! Error:" << reply->errorString();
        emit networkError(reply->errorString());
        break;
    }

    reply->deleteLater();
}

void OBSCore::getSRDiff(const QString &resource)
{
    QNetworkReply *reply = postRequest(resource, "");
    reply->setProperty("reqtype", OBSCore::SRDiff);
}

void OBSCore::branchPackage(const QString &project, const QString &package)
{
    QString resource = QString("/source/%1/%2?cmd=branch").arg(project, package);
    QNetworkReply *reply = postRequest(resource, "");
    reply->setProperty("reqtype", OBSCore::BranchPackage);
    reply->setProperty("branchprj", project);
    reply->setProperty("branchpkg", package);
}

void OBSCore::createProject(const QString &project, const QByteArray &data)
{
    QString resource = QString("/source/%1/_meta").arg(project);
    QNetworkReply *reply = putRequest(resource, data);
    reply->setProperty("reqtype", OBSCore::CreateProject);
    reply->setProperty("createprj", project);
}

void OBSCore::createPackage(const QString &project, const QString &package, const QByteArray &data)
{
    QString resource = QString("/source/%1/%2/_meta").arg(project, package);
    QNetworkReply *reply = putRequest(resource, data);
    reply->setProperty("reqtype", OBSCore::CreatePackage);
    reply->setProperty("createprj", project);
    reply->setProperty("createpkg", package);
}

void OBSCore::uploadFile(const QString &project, const QString &package, const QString &fileName, const QByteArray &data)
{
    QString resource = QString("/source/%1/%2/%3").arg(project, package, fileName);
    QNetworkReply *reply = putRequest(resource, data);
    reply->setProperty("reqtype", OBSCore::UploadFile);
    reply->setProperty("uploadprj", project);
    reply->setProperty("uploadpkg", package);
    reply->setProperty("uploadfile", fileName);
}

void OBSCore::deleteProject(const QString &project)
{
    QString resource = QString("/source/%1").arg(project);
    QNetworkReply *reply = deleteRequest(resource);
    reply->setProperty("reqtype", OBSCore::DeleteProject);
    reply->setProperty("deleteprj", project);
}

void OBSCore::deletePackage(const QString &project, const QString &package)
{
    QString resource = QString("/source/%1/%2").arg(project, package);
    QNetworkReply *reply = deleteRequest(resource);
    reply->setProperty("reqtype", OBSCore::DeletePackage);
    reply->setProperty("deleteprj", project);
    reply->setProperty("deletepkg", package);
}

void OBSCore::deleteFile(const QString &project, const QString &package, const QString &fileName)
{
    QString resource = QString("/source/%1/%2/%3").arg(project, package, fileName);
    QNetworkReply *reply = deleteRequest(resource);
    reply->setProperty("reqtype", OBSCore::DeleteFile);
    reply->setProperty("deleteprj", project);
    reply->setProperty("deletepkg", package);
    reply->setProperty("deletefile", fileName);
}

void OBSCore::about()
{
    QString resource = "/about";
    QNetworkReply *reply = request(resource);
    reply->setProperty("reqtype", OBSCore::About);
}

void OBSCore::onSslErrors(QNetworkReply* reply, const QList<QSslError> &list)
{
    QString errorString;
    QString message;

    foreach (const QSslError &sslError, list) {
        if (list.count() >= 1) {
            errorString += ", ";
            errorString = sslError.errorString();
            if (sslError.error() == QSslError::SelfSignedCertificateInChain) {
                qDebug() << "OBSCore::onSslErrors() Self signed certificate!";
                emit selfSignedCertificate(reply);
            }
        }
    }
    qDebug() << "OBSCore::onSslErrors() SSL Errors:" << errorString;

    if (list.count() == 1) {
        message=tr("An SSL error has occured: %1");
    } else {
        message=list.count()+tr(" SSL errors have occured: %1");
    }

   qDebug() << "OBSCore::onSslErrors() url:" << reply->url() << "row:" << reply->property("row").toInt();
}