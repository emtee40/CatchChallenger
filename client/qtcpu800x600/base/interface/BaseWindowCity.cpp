#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../../../general/base/FacilityLib.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../FacilityLibClient.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::cityCaptureUpdateTime()
{
    if(city.capture.frenquency==City::Capture::Frequency_week)
        nextCatch=QDateTime::fromMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch()+24*3600*7*1000);
    else
        nextCatch=QDateTime::fromMSecsSinceEpoch(FacilityLib::nextCaptureTime(city));
    nextCityCatchTimer.start(static_cast<uint32_t>(nextCatch.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch()));
}

void BaseWindow::updatePageZoneCatch()
{
    if(QDateTime::currentMSecsSinceEpoch()<nextCatchOnScreen.toMSecsSinceEpoch())
    {
        int sec=static_cast<uint32_t>(nextCatchOnScreen.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch())/1000+1;
        std::string timeText;
        if(sec>3600*24*365*50)
            timeText="Time player: bug";
        else
            timeText=FacilityLibClient::timeToString(sec);
        ui->zonecaptureWaitTime->setText(tr("Remaining time: %1")
                                         .arg(QStringLiteral("<b>%1</b>")
                                         .arg(QString::fromStdString(timeText)))
                                         );
        ui->zonecaptureCancel->setVisible(true);
    }
    else
    {
        ui->zonecaptureCancel->setVisible(false);
        ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting of players list")+"</i>");
        updater_page_zonecatch.stop();
    }
}

void BaseWindow::on_zonecaptureCancel_clicked()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    client->waitingForCityCapture(true);
    zonecatch=false;
}

void BaseWindow::captureCityYourAreNotLeader()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("You are not a clan leader to start a city capture").toStdString());
    zonecatch=false;
}

void BaseWindow::captureCityYourLeaderHaveStartInOtherCity(const std::string &zone)
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(QtDatapackClientLoader::datapackLoader->get_zonesExtra().find(zone)!=QtDatapackClientLoader::datapackLoader->get_zonesExtra().cend())
        showTip(tr("Your clan leader have start a capture for another city").toStdString()+": <b>"+
                QtDatapackClientLoader::datapackLoader->get_zonesExtra().at(zone).name+
                "</b>");
    else
        showTip(tr("Your clan leader have start a capture for another city").toStdString());
    zonecatch=false;
}

void BaseWindow::captureCityPreviousNotFinished()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    showTip(tr("Previous capture of this city is not finished").toStdString());
    zonecatch=false;
}

void BaseWindow::captureCityStartBattle(const uint16_t &player_count,const uint16_t &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void BaseWindow::captureCityStartBotFight(const uint16_t &player_count,const uint16_t &clan_count,const uint16_t &fightId)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
    botFight(fightId);
}

void BaseWindow::captureCityDelayedStart(const uint16_t &player_count,const uint16_t &clan_count)
{
    ui->zonecaptureCancel->setVisible(false);
    ui->zonecaptureWaitTime->setText("<i>"+tr("In waiting fight.")+" "+tr("%1 and %2 in wainting to capture the city").arg("<b>"+tr("%n player(s)","",player_count)+"</b>").arg("<b>"+tr("%n clan(s)","",clan_count)+"</b>")+"</i>");
    updater_page_zonecatch.stop();
}

void BaseWindow::captureCityWin()
{
    updater_page_zonecatch.stop();
    ui->stackedWidget->setCurrentWidget(ui->page_map);
    if(!zonecatchName.empty())
        showTip(tr("Your clan win the city").toStdString()+": <b>"+
                zonecatchName+"</b>");
    else
        showTip(tr("Your clan win the city").toStdString());
    zonecatch=false;
}
