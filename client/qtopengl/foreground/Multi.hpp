#ifndef MULTI_H
#define MULTI_H

#include <QNetworkAccessManager>
#include <QString>
#include <QHash>
#include <QSet>
#include <vector>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../ConnexionInfo.hpp"
#include "../CCScrollZone.hpp"

class AddOrEditServer;
class Login;
class MultiItem;
class QNetworkReply;

class SelectedServer
{
public:
    QString unique_code;
    bool isCustom;
};

class Multi : public ScreenInput
{
    Q_OBJECT
public:
    explicit Multi();
    ~Multi();
    void displayServerList();
    void newLanServer();
    void server_add_clicked();
    void server_add_finished();
    void server_edit_clicked();
    void server_edit_finished();
    void server_select_clicked();
    void server_select_finished();
    void server_remove_clicked();
    void saveConnexionInfoList();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QByteArray &xmlContent);
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QString &file);
    std::vector<ConnexionInfo> loadConfigConnexionInfoList();
    void httpFinished();
    void downloadFile();
    void newLanguage();
    void on_server_refresh_clicked();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
private:
    std::vector<ConnexionInfo> temp_lanConnexionInfoList,temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QList<MultiItem *> serverConnexion;
    SelectedServer selectedServer;//no selected if unique_code empty
    AddOrEditServer *addServer;
    Login *login;

    QNetworkAccessManager qnam;
    QNetworkReply *reply;

    CustomButton *server_add;
    CustomButton *server_remove;
    CustomButton *server_edit;
    CustomButton *server_select;
    CustomButton *server_refresh;
    CustomButton *back;
    QGraphicsTextItem *warning;

    ImagesStrechMiddle *wdialog;
    QGraphicsTextItem *serverEmpty;
    CCScrollZone *scrollZone;
signals:
    void backMain();
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
};

#endif // MULTI_H
