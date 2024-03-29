#ifndef OverMap_H
#define OverMap_H

#include <QObject>
#include <QTimer>
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class MapMonsterPreview;
class ImagesStrechMiddle;
class LineEdit;
class ComboBox;
class CustomText;
class ConnexionManager;
class CCMap;
class QGraphicsPixmapItemClick;
class MonsterDetails;

class OverMap : public ScreenInput
{
    Q_OBJECT
public:
    enum ActionClan
    {
        ActionClan_Create,
        ActionClan_Leave,
        ActionClan_Dissolve,
        ActionClan_Invite,
        ActionClan_Eject
    };

    explicit OverMap();
    ~OverMap();
    virtual void resetAll();
    virtual void setVar(CCMap *ccmap, ConnexionManager *connexionManager);
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void lineEdit_chat_text_returnPressed();
    void updateShowChat();
    void buyClicked();
    void IG_dialog_close();
    void comboBox_chat_type_currentIndexChanged(int index);
    void new_system_text(const CatchChallenger::Chat_type &chat_type, const std::string &text);
    void new_chat_text(CatchChallenger::Chat_type chat_type,std::string text,std::string pseudo,CatchChallenger::Player_type player_type);
    void removeNumberForFlood();

    void openMonster();
    void updatePlayerNumber(const uint16_t &number,const uint16_t &max_players);
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);

    //to call from chat to do test
    virtual void add_to_inventory(const std::vector<std::pair<uint16_t,uint32_t> > &items,const bool &showGain) = 0;
    virtual void showTip(const std::string &tip) = 0;
    virtual void showPlace(const std::string &place) = 0;
protected:
    QTimer stopFlood;
    int numberForFlood;
    std::string lastMessageSend;
    ConnexionManager *connexionManager;
    MonsterDetails *monsterDetails;

    QGraphicsPixmapItemClick *playerBackground;
    bool playerBackgroundBig;
    QGraphicsPixmapItem *playerImage;
    QGraphicsTextItem *name;
    QGraphicsTextItem *cash;
    QList<MapMonsterPreview *> monsters;
    QPointF m_startPress;
    bool wasDragged;
    MapMonsterPreview * monstersDragged;

    QGraphicsPixmapItem *playersCountBack;
    QGraphicsTextItem *playersCount;

    ImagesStrechMiddle *chatBack;
    QGraphicsTextItem *chatText;
    LineEdit *chatInput;
    ComboBox *chatType;
    CustomButton *chat;
    CustomText *chatOver;

    CustomButton *bag;
    CustomText *bagOver;
    CustomButton *opentolan;
    CustomText *opentolanOver;
    CustomButton *buy;
    CustomText *buyOver;

    ImagesStrechMiddle *gainBack;
    QGraphicsTextItem *gain;
    QString gainString;

    ImagesStrechMiddle *IG_dialog_textBack;
    QGraphicsTextItem *IG_dialog_name;
    QGraphicsTextItem *IG_dialog_text;
    CustomButton *IG_dialog_quit;
    QString IG_dialog_nameString;
    QString IG_dialog_textString;
    QGraphicsPixmapItem *labelSlow;

    ImagesStrechMiddle *tipBack;
    QGraphicsTextItem *tip;
    QString tipString;

    ImagesStrechMiddle *persistant_tipBack;
    QGraphicsTextItem *persistant_tip;
    QString persistant_tipString;

    std::vector<ActionClan> actionClan;
    std::string clanName;
    bool haveClanInformations;
private:
    void updateChat();
};

#endif // MULTI_H
