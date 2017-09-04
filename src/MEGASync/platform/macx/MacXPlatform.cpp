#include "MacXPlatform.h"
#include <unistd.h>

using namespace std;

int MacXPlatform::fd = -1;
MacXSystemServiceTask* MacXPlatform::systemServiceTask = NULL;
MacXExtServer *MacXPlatform::extServer = NULL;

static const QString kFinderSyncBundleId = QString::fromUtf8("mega.mac.MEGAShellExtFinder");
static const QString kFinderSyncPath = QString::fromUtf8("/Applications/MEGAsync.app/Contents/PlugIns/MEGAShellExtFinder.appex/");

void MacXPlatform::initialize(int argc, char *argv[])
{
#ifdef QT_DEBUG
    return;
#endif

    setMacXActivationPolicy();
    SetProcessName(QString::fromUtf8("MEGAsync"));

    fd = -1;
    if (argc)
    {
        long int value = strtol(argv[argc-1], NULL, 10);
        if (value > 0 && value < INT_MAX)
        {
            fd = value;
        }
    }

    if (fd < 0)
    {
        if (!enableSetuidBit())
        {
            ::exit(0);
        }

        //Reboot
        QString app = MegaApplication::applicationDirPath();
        QString launchCommand = QString::fromUtf8("open");
        QStringList args = QStringList();
        QDir appPath(app);
        appPath.cdUp();
        appPath.cdUp();
        args.append(QString::fromAscii("-n"));
        args.append(appPath.absolutePath());
        QProcess::startDetached(launchCommand, args);
        sleep(2);
        ::exit(0);
    }
}

QStringList MacXPlatform::multipleUpload(QString uploadTitle)
{
    return uploadMultipleFiles(uploadTitle);
}
bool MacXPlatform::enableTrayIcon(QString executable)
{
    return false;
}

void MacXPlatform::notifyItemChange(string *localPath, int newState)
{
    if (extServer && localPath && localPath->size())
    {
        extServer->notifyItemChange(localPath, newState);
    }
}

bool MacXPlatform::startOnStartup(bool value)
{
   return startAtLogin(value);
}

bool MacXPlatform::isStartOnStartupActive()
{
    return isStartAtLoginActive();
}

int MacXPlatform::addFinderExtensionToSystem()
{
    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-a")
               << kFinderSyncPath;

    QProcess p;
    p.start(QString::fromAscii("pluginkit"), scriptArgs);
    if (!p.waitForFinished(2000))
    {
        return false;
    }

    return p.exitCode();
}

bool MacXPlatform::isFinderExtensionEnabled()
{
    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-m")
               << QString::fromUtf8("-i")
               << kFinderSyncBundleId;

    QProcess p;
    p.start(QString::fromAscii("pluginkit"), scriptArgs);
    if (!p.waitForFinished(2000))
    {
        return false;
    }

    QString out = QString::fromUtf8(p.readAllStandardOutput().trimmed());
    if (out.isEmpty())
    {
        return false;
    }

    if (out.at(0) != QChar::fromAscii('?') && out.at(0) != QChar::fromAscii('+'))
    {
        return false;
    }

    return true;
}

int MacXPlatform::reinstallFinderExtension()
{
    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-r")
               << kFinderSyncPath;

    QProcess p;
    p.start(QString::fromAscii("pluginkit"), scriptArgs);
    if (!p.waitForFinished(2000))
    {
        return false;
    }

    return p.exitCode();
}

int MacXPlatform::enableFinderExtension(bool value)
{
    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-e")
               << (value ? QString::fromUtf8("use") : QString::fromUtf8("ignore")) //Enable or disable extension plugin
               << QString::fromUtf8("-i")
               << kFinderSyncBundleId;

    QProcess p;
    p.start(QString::fromAscii("pluginkit"), scriptArgs);
    if (!p.waitForFinished(2000))
    {
        return false;
    }

    return p.exitCode();
}

void MacXPlatform::showInFolder(QString pathIn)
{
    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-e")
               << QString::fromUtf8("tell application \"Finder\" to reveal POSIX file \"%1\"").arg(pathIn);
    QProcess::startDetached(QString::fromUtf8("osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QString::fromUtf8("-e")
               << QString::fromUtf8("tell application \"Finder\" to activate");
    QProcess::startDetached(QString::fromAscii("osascript"), scriptArgs);
}

void MacXPlatform::startShellDispatcher(MegaApplication *receiver)
{
    if (!systemServiceTask)
    {
        systemServiceTask = new MacXSystemServiceTask(receiver);
    }

    if (!extServer)
    {
        extServer = new MacXExtServer(receiver);
    }
}

void MacXPlatform::stopShellDispatcher()
{
    if (systemServiceTask)
    {
        delete systemServiceTask;
        systemServiceTask = NULL;
    }

    if (extServer)
    {
        delete extServer;
        extServer = NULL;
    }
}

void MacXPlatform::syncFolderAdded(QString syncPath, QString syncName, QString syncID)
{
    addPathToPlaces(syncPath,syncName);
    setFolderIcon(syncPath);

    if (extServer)
    {
        extServer->notifySyncAdd(syncPath, syncName);
    }
}

void MacXPlatform::syncFolderRemoved(QString syncPath, QString syncName, QString syncID)
{
    removePathFromPlaces(syncPath);
    unSetFolderIcon(syncPath);

    if (extServer)
    {
        extServer->notifySyncDel(syncPath, syncName);
    }
}

QByteArray MacXPlatform::encrypt(QByteArray data, QByteArray key)
{
    return data;
}

QByteArray MacXPlatform::decrypt(QByteArray data, QByteArray key)
{
    return data;
}

QByteArray MacXPlatform::getLocalStorageKey()
{
    return QByteArray(128, 0);
}

QString MacXPlatform::getDefaultOpenApp(QString extension)
{
    return defaultOpenApp(extension);
}

void MacXPlatform::enableDialogBlur(QDialog *dialog)
{

}

bool MacXPlatform::enableSetuidBit()
{
    QString command = QString::fromUtf8("do shell script \"chown root /Applications/MEGAsync.app/Contents/MacOS/MEGAsync && chmod 4755 /Applications/MEGAsync.app/Contents/MacOS/MEGAsync && echo true\"");
    char *response = runWithRootPrivileges((char *)command.toStdString().c_str());
    if (!response)
    {
        return false;
    }
    bool result = strlen(response) >= 4 && !strncmp(response, "true", 4);
    delete response;
    return result;
}

void MacXPlatform::activateBackgroundWindow(QDialog *)
{

}

void MacXPlatform::uninstall()
{

}
