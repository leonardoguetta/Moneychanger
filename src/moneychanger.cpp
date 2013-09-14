#include "moneychanger.h"
#include "ot_worker.h"
#include "Widgets/MarketWindow.h"
#include "Widgets/overviewwindow.h"
#include "Widgets/addressbookwindow.h"
#include "Widgets/nymmanagerwindow.h"
#include "Widgets/assetmanagerwindow.h"
#include "accountmanagerwindow.h"
#include "Handlers/DBHandler.h"

#include "opentxs/OTAPI.h"
#include "opentxs/OT_ME.h"

/**
 ** Constructor & Destructor
 **/
Moneychanger::Moneychanger(QWidget *parent)
: QWidget(parent)
{
    /**
     ** Init variables *
     **/
    
    //Default nym
    default_nym_id = "";
    default_nym_name = "";
    
    //Default server
    
    //Thread Related
    ot_worker_background = new ot_worker();
    ot_worker_background->mc_overview_ping();
    
    //OT Related
    ot_me = new OT_ME();
    
    //SQLite database
    // This can be moved very easily into a different class
    // Which I will inevitably end up doing.
    
    /** Default Nym **/
    qDebug() << "Setting up Nym table";
    if(DBHandler::getInstance()->querySize("SELECT `nym` FROM `default_nym` LIMIT 0,1") == 0){
        DBHandler::getInstance()->runQuery("INSERT INTO `default_nym` (`nym`) VALUES('')"); // Blank Row
    }
    else{
        if(DBHandler::getInstance()->isNext("SELECT `nym` FROM `default_nym` LIMIT 0,1")){
            default_nym_id = DBHandler::getInstance()->queryString("SELECT `nym` FROM `default_nym` LIMIT 0,1", 0, 0);
        }
        //Ask OT what the display name of this nym is and store it for quick retrieval later on(mostly for "Default Nym" displaying purposes)
        if(default_nym_id != ""){
            default_nym_name =  QString::fromStdString(OTAPI_Wrap::GetNym_Name(default_nym_id.toStdString()));
        }
        else
            qDebug() << "Default Nym loaded from SQL";

    }
    
    /** Default Server **/
    //Query for the default server (So we know for setting later on -- Auto select server associations on later dialogs)
    if(DBHandler::getInstance()->querySize("SELECT `server` FROM `default_server` LIMIT 0,1") == 0){
        DBHandler::getInstance()->runQuery("INSERT INTO `default_server` (`server`) VALUES('')"); // Blank Row
    }
    else{
        if(DBHandler::getInstance()->runQuery("SELECT `server` FROM `default_server` LIMIT 0,1")){
            default_server_id = DBHandler::getInstance()->queryString("SELECT `server` FROM `default_server` LIMIT 0,1", 0, 0);
        }
        //Ask OT what the display name of this server is and store it for a quick retrieval later on(mostly for "Default Server" displaying purposes)
        if(default_server_id != ""){
            default_server_name = QString::fromStdString(OTAPI_Wrap::GetServer_Name(default_server_id.toStdString()));
        }
        else
            qDebug() << "DEFAULT SERVER LOADED FROM SQL";

    }
    qDebug() << "Database Populated";
    
    
    //Ask OT for "cash account" information (might be just "Account" balance)
    //Ask OT the purse information
    
    
    /* *** *** ***
     * Init Memory Trackers (there may be other int below than just memory trackers but generally there will be mostly memory trackers below)
     * Allows the program to boot with a low footprint -- keeps start times low no matter the program complexity;
     * Memory will expand as the operator opens dialogs;
     * Also prevents HTTP requests from overloading or spamming the operators device by only allowing one window of that request;
     * *** *** ***/
    
    //Menu
    //Address Book
    mc_addressbook_already_init = false;
    //Overview
    mc_overview_already_init = false;
    // Nym Manager
    mc_nymmanager_already_init = false;
    // Asset Manager
    mc_assetmanager_already_init = false;
    
    //Server Manager
    mc_servermanager_already_init = 0;
    mc_servermanager_refreshing = 0;
    mc_servermanager_proccessing_dataChanged = 0;
    
    //"Add server" dialog
    mc_servermanager_addserver_dialog_already_init = 0;
    mc_servermanager_addserver_dialog_advanced_showing = 0;
    
    //"Remove server" dialog
    mc_servermanager_removeserver_dialog_already_init = 0;
    


    
    //Withdraw
    //As Cash
    mc_withdraw_ascash_dialog_already_init = 0;
    mc_withdraw_ascash_confirm_dialog_already_init = 0;
    
    //As Voucher
    mc_withdraw_asvoucher_dialog_already_init = 0;
    mc_withdraw_asvoucher_confirm_dialog_already_init = 0;
    
    //Deposit
    mc_deposit_already_init = 0;
    
    //Send funds
    mc_sendfunds_already_init = 0;
    
    //Init MC System Tray Icon
    mc_systrayIcon = new QSystemTrayIcon(this);
    mc_systrayIcon->setIcon(QIcon(":/icons/moneychanger"));
    
    //Init Icon resources (Loading resources/access Harddrive first; then send to GPU; This specific order will in theory prevent bottle necking between HDD/GPU)
    mc_systrayIcon_shutdown = QIcon(":/icons/quit");
    
    mc_systrayIcon_overview = QIcon(":/icons/overview");
    
    mc_systrayIcon_nym = QIcon(":/icons/nym");
    mc_systrayIcon_server = QIcon(":/icons/server");
    
    mc_systrayIcon_goldaccount = QIcon(":/icons/account");
    mc_systrayIcon_purse = QIcon(":/icons/purse");
    
    mc_systrayIcon_withdraw = QIcon(":/icons/withdraw");
    mc_systrayIcon_deposit = QIcon(":/icons/deposit");
    
    mc_systrayIcon_sendfunds = QIcon(":/icons/sendfunds");
    mc_systrayIcon_requestfunds = QIcon(":/icons/requestpayment");
    
    mc_systrayIcon_advanced = QIcon(":/icons/advanced");
    //Submenu
    mc_systrayIcon_advanced_agreements = QIcon(":/icons/agreements");
    mc_systrayIcon_advanced_markets = QIcon(":/icons/markets");
    mc_systrayIcon_advanced_settings = QIcon(":/icons/settings");
    
    //MC System tray menu
    mc_systrayMenu = new QMenu(this);
    
    //Init Skeleton of system tray menu
    //App name
    mc_systrayMenu_headertext = new QAction("Moneychanger", 0);
    mc_systrayMenu_headertext->setDisabled(1);
    mc_systrayMenu->addAction(mc_systrayMenu_headertext);
    // --------------------------------------------------------------
    //Blank
    //            mc_systrayMenu_aboveBlank = new QAction(" ", 0);
    //            mc_systrayMenu_aboveBlank->setDisabled(1);
    //            mc_systrayMenu->addAction(mc_systrayMenu_aboveBlank);
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Overview button
    mc_systrayMenu_overview = new QAction(mc_systrayIcon_overview, "Transaction History", 0);
    mc_systrayMenu->addAction(mc_systrayMenu_overview);
    //Connect the Overview to a re-action when "clicked";
    connect(mc_systrayMenu_overview, SIGNAL(triggered()), this, SLOT(mc_overview_slot()));
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Send funds
    mc_systrayMenu_sendfunds = new QAction(mc_systrayIcon_sendfunds, "Send Funds...", 0);
    mc_systrayMenu->addAction(mc_systrayMenu_sendfunds);
    //Connect button with re-aciton
    connect(mc_systrayMenu_sendfunds, SIGNAL(triggered()), this, SLOT(mc_sendfunds_slot()));
    // --------------------------------------------------------------
    //Request payment
    mc_systrayMenu_requestfunds = new QAction(mc_systrayIcon_requestfunds, "Request Payment...", 0);
    mc_systrayMenu->addAction(mc_systrayMenu_requestfunds);
    // Currently causes a crash , likely due to malformed Dialog construction.
    //connect(mc_systrayMenu_requestfunds, SIGNAL(triggered()), this, SLOT(mc_requestfunds_slot()));
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Account section
    mc_systrayMenu_account = new QMenu("Set Default Account...", 0);
    mc_systrayMenu_account->setIcon(mc_systrayIcon_goldaccount);
    mc_systrayMenu->addMenu(mc_systrayMenu_account);
    
    //Add a "Manage accounts" action button (and connection)
    QAction * manage_accounts = new QAction("Manage Accounts...", 0);
    manage_accounts->setData(QVariant(QString("openmanager")));
    mc_systrayMenu_account->addAction(manage_accounts);
    mc_systrayMenu_account->addSeparator();
    
    //Add reaction to the "account" action.
    connect(mc_systrayMenu_account, SIGNAL(triggered(QAction*)), this, SLOT(mc_accountselection_triggered(QAction*)));
    
    //Load "default" account
    set_systrayMenu_account_setDefaultAccount(default_account_id, default_account_name);
    
    //Init account submenu
    account_list_id   = new QList<QVariant>();
    account_list_name = new QList<QVariant>();
    
    mc_systrayMenu_reload_accountlist();
    // --------------------------------------------------------------
    //Asset section
    mc_systrayMenu_asset = new QMenu("Set Default Asset Type...", 0);
    mc_systrayMenu_asset->setIcon(mc_systrayIcon_purse);
    mc_systrayMenu->addMenu(mc_systrayMenu_asset);
    
    //Add a "Manage asset types" action button (and connection)
    QAction * manage_assets = new QAction("Manage Asset Contracts...", 0);
    manage_assets->setData(QVariant(QString("openmanager")));
    mc_systrayMenu_asset->addAction(manage_assets);
    mc_systrayMenu_asset->addSeparator();
    
    //Add reaction to the "asset" action.
    connect(mc_systrayMenu_asset, SIGNAL(triggered(QAction*)), this, SLOT(mc_assetselection_triggered(QAction*)));
    
    //Load "default" asset type
    set_systrayMenu_asset_setDefaultAsset(default_asset_id, default_asset_name);
    
    //Init asset submenu
    asset_list_id   = new QList<QVariant>();
    asset_list_name = new QList<QVariant>();
    
    mc_systrayMenu_reload_assetlist();
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Withdraw
    mc_systrayMenu_withdraw = new QMenu("Withdraw", 0);
    mc_systrayMenu_withdraw->setIcon(mc_systrayIcon_withdraw);
    mc_systrayMenu->addMenu(mc_systrayMenu_withdraw);
    //(Withdraw) as Cash
    mc_systrayMenu_withdraw_ascash = new QAction("As Cash",0);
    mc_systrayMenu_withdraw->addAction(mc_systrayMenu_withdraw_ascash);
    //Connect Button with re-action
    connect(mc_systrayMenu_withdraw_ascash, SIGNAL(triggered()), this, SLOT(mc_withdraw_ascash_slot()));
    
    //(Withdraw) as Voucher
    mc_systrayMenu_withdraw_asvoucher = new QAction("As Voucher", 0);
    mc_systrayMenu_withdraw->addAction(mc_systrayMenu_withdraw_asvoucher);
    //Connect Button with re-action
    connect(mc_systrayMenu_withdraw_asvoucher, SIGNAL(triggered()), this, SLOT(mc_withdraw_asvoucher_slot()));
    // --------------------------------------------------------------
    //Deposit
    mc_systrayMenu_deposit = new QAction(mc_systrayIcon_deposit, "Deposit...", 0);
    mc_systrayMenu->addAction(mc_systrayMenu_deposit);
    //Connect button with re-action
    connect(mc_systrayMenu_deposit, SIGNAL(triggered()), this, SLOT(mc_deposit_slot()));
    // --------------------------------------------------------------
    //Gold account/cash purse/wallet
    //            mc_systrayMenu_goldaccount = new QAction("Gold Account: $60,000", 0);
    //            mc_systrayMenu_goldaccount->setIcon(mc_systrayIcon_goldaccount);
    //            mc_systrayMenu->addAction(mc_systrayMenu_goldaccount);
    //            // --------------------------------------------------------------
    //            //purse wallet
    //            mc_systrayMenu_purse = new QAction("Purse: $40,000", 0);
    //            mc_systrayMenu_purse->setIcon(mc_systrayIcon_purse);
    //            mc_systrayMenu->addAction(mc_systrayMenu_purse);
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Advanced
    mc_systrayMenu_advanced = new QMenu("Advanced", 0);
    mc_systrayMenu_advanced->setIcon(mc_systrayIcon_advanced);
    mc_systrayMenu->addMenu(mc_systrayMenu_advanced);
    //Advanced submenu
    
    mc_systrayMenu_advanced_agreements = new QAction(mc_systrayIcon_advanced_agreements, "Agreements", 0);
    mc_systrayMenu_advanced->addAction(mc_systrayMenu_advanced_agreements);
    
    mc_systrayMenu_advanced_markets = new QAction(mc_systrayIcon_advanced_markets, "Markets", 0);
    mc_systrayMenu_advanced->addAction(mc_systrayMenu_advanced_markets);
    connect(mc_systrayMenu_advanced_markets, SIGNAL(triggered()), this, SLOT(mc_market_slot()));
    mc_systrayMenu_advanced->addSeparator();
    
    mc_systrayMenu_advanced_settings = new QAction(mc_systrayIcon_advanced_settings, "Settings...", 0);
    mc_systrayMenu_advanced_settings->setMenuRole(QAction::NoRole);
    mc_systrayMenu_advanced->addAction(mc_systrayMenu_advanced_settings);
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu_advanced->addSeparator();
    // --------------------------------------------------------------
    
    // TODO: If the default isn't set, then choose the first one and select it.
    
    // TODO: If there isn't even ONE to select, then this menu item should say "Create Nym..." with no sub-menu.
    
    // TODO: When booting up, if there is already a default server and asset id, but no nyms exist, create a default nym.
    
    // TODO: When booting up, if there is already a default nym, but no accounts exist, create a default account.
    
    //Nym section
    mc_systrayMenu_nym = new QMenu("Set Default Nym...", 0);
    mc_systrayMenu_nym->setIcon(mc_systrayIcon_nym);
    mc_systrayMenu_advanced->addMenu(mc_systrayMenu_nym);
    
    //Add a "Manage pseudonym" action button (and connection)
    QAction * manage_nyms = new QAction("Manage Nyms...", 0);
    manage_nyms->setData(QVariant(QString("openmanager")));
    mc_systrayMenu_nym->addAction(manage_nyms);
    mc_systrayMenu_nym->addSeparator();
    
    //Add reaction to the "pseudonym" action.
    connect(mc_systrayMenu_nym, SIGNAL(triggered(QAction*)), this, SLOT(mc_nymselection_triggered(QAction*)));
    
    //Load "default" nym
    set_systrayMenu_nym_setDefaultNym(default_nym_id, default_nym_name);
    
    //Init nym submenu
    nym_list_id = new QList<QVariant>();
    nym_list_name = new QList<QVariant>();
    
    mc_systrayMenu_reload_nymlist();
    // --------------------------------------------------------------
    //Server section
    mc_systrayMenu_server = new QMenu("Set Default Server...", 0);
    mc_systrayMenu_server->setIcon(mc_systrayIcon_server);
    mc_systrayMenu_advanced->addMenu(mc_systrayMenu_server);
    
    //Add a "Manage server" action button (and connection)
    QAction * manage_servers = new QAction("Manage Servers...", 0);
    manage_servers->setData(QVariant(QString("openmanager")));
    mc_systrayMenu_server->addAction(manage_servers);
    mc_systrayMenu_server->addSeparator();
    
    //Add reaction to the "server selection" action
    connect(mc_systrayMenu_server, SIGNAL(triggered(QAction*)), this, SLOT(mc_serverselection_triggered(QAction*)));
    
    //Load "default" server
    mc_systrayMenu_server_setDefaultServer(default_server_id, default_server_name);
    
    //Init server submenu
    server_list_id = new QList<QVariant>();
    server_list_name = new QList<QVariant>();
    
    mc_systrayMenu_reload_serverlist();
    // --------------------------------------------------------------
    //Separator
    mc_systrayMenu->addSeparator();
    // --------------------------------------------------------------
    //Shutdown Moneychanger
    mc_systrayMenu_shutdown = new QAction(mc_systrayIcon_shutdown, "Quit", 0);
    mc_systrayMenu_shutdown->setMenuRole(QAction::NoRole);
    mc_systrayMenu_shutdown->setIcon(mc_systrayIcon_shutdown);
    mc_systrayMenu->addAction(mc_systrayMenu_shutdown);
    //connection
    connect(mc_systrayMenu_shutdown, SIGNAL(triggered()), this, SLOT(mc_shutdown_slot()));
    // --------------------------------------------------------------
    //Blank
    //            mc_systrayMenu_bottomblank = new QAction(" ", 0);
    //            mc_systrayMenu_bottomblank->setDisabled(1);
    //            mc_systrayMenu->addAction(mc_systrayMenu_bottomblank);
    // --------------------------------------------------------------
    //Set Skeleton to systrayIcon object code
    mc_systrayIcon->setContextMenu(mc_systrayMenu);
}

Moneychanger::~Moneychanger()
{
    
}

//start
void Moneychanger::bootTray(){
    //Show systray
    mc_systrayIcon->show();
    
    qDebug() << "BOOTING";
}

/** ****** ****** ******  **
 ** Public Function/Calls **/


/** ****** ****** ******   **
 ** Private Function/Calls **/

/* **
 * Menu Dialog Related Calls
 */

/** Overview Dialog **/
void Moneychanger::mc_overview_dialog(){
    if(!mc_overview_already_init){
        OverviewWindow *overview = new OverviewWindow(this);
        overview->setAttribute(Qt::WA_DeleteOnClose);
        overview->dialog();
        mc_overview_already_init = true;
    }
}

//Overview slots
void Moneychanger::mc_overview_slot(){
    //The operator has requested to open the dialog to the "Overview";
    mc_overview_dialog();
}
// End Overview 

// Market Window
void Moneychanger::mc_market_slot(){
    
    // This is a glaring memory leak, but it's only a temporary placeholder before I redo how windows are handled.
    if(!mc_market_window_already_init){
        MarketWindow *market_window = new MarketWindow(this);
        market_window->setAttribute(Qt::WA_DeleteOnClose);
        market_window->show();
        mc_market_window_already_init = true;
    }
}
// End Market Window

/** Address Book **/
void Moneychanger::mc_addressbook_show(QString text){
    //The caller dosen't wish to have the address book paste to anything (they just want to see/manage the address book), just call blank.    
    if(!mc_addressbook_already_init){
        AddressBookWindow *addressbookwindow = new AddressBookWindow(this);
        addressbookwindow->setAttribute(Qt::WA_DeleteOnClose);
        addressbookwindow->show(text);
        mc_overview_already_init = true;
    }
}
// End Address Book

/**  Nym Manager **/
void Moneychanger::set_systrayMenu_nym_setDefaultNym(QString nym_id, QString nym_name){
    //Set default nym internal memory
    default_nym_id = nym_id;
    default_nym_name = nym_name;
    
    //SQL UPDATE default nym
    DBHandler::getInstance()->AddressBookUpdateDefaultNym(nym_id);
    
    //Rename "NYM:" if a nym is loaded
    if(nym_id != ""){
        mc_systrayMenu_nym->setTitle("Nym: "+nym_name);
    }
}

void Moneychanger::mc_nymmanager_dialog(){
    
    if(!mc_nymmanager_already_init){
        NymManagerWindow *nymmanagerwindow = new NymManagerWindow(this);
        nymmanagerwindow->setAttribute(Qt::WA_DeleteOnClose);
        nymmanagerwindow->dialog();
        mc_overview_already_init = true;
    }
}

void Moneychanger::mc_systrayMenu_reload_nymlist(){
    qDebug() << "RELOAD NYM LIST";
    //Count nyms
    int32_t nym_count_int32_t = OTAPI_Wrap::GetNymCount();
    int nym_count = nym_count_int32_t;
    
    //Retrieve updated list of nym submenu actions list
    QList<QAction*> action_list_to_nym_submenu = mc_systrayMenu_nym->actions();
    
    //Remove all sub-menus from the nym submenu
    for(int a = action_list_to_nym_submenu.size(); a > 0; a--){
        qDebug() << "REMOVING" << a;
        QPoint tmp_point = QPoint(a, 0);
        mc_systrayMenu_nym->removeAction(mc_systrayMenu_nym->actionAt(tmp_point));
    }
    
    //Remove all nyms from the backend list
    //Remove ids from backend list
    int tmp_nym_list_id_size = nym_list_id->size();
    for(int a = 0; a < tmp_nym_list_id_size; a++){
        nym_list_id->removeLast();
    }
    
    //Remove names from the backend list
    int tmp_nym_list_name_size = nym_list_name->size();
    for(int a = 0; a < tmp_nym_list_name_size; a++){
        nym_list_name->removeLast();
    }
    
    //Add/append to the id + name lists
    for(int a = 0; a < nym_count; a++){
        //Get OT Account ID
        QString OT_nym_id = QString::fromStdString(OTAPI_Wrap::GetNym_ID(a));
        
        //Add to qlist
        nym_list_id->append(QVariant(OT_nym_id));
        
        //Get OT Account Name
        QString OT_nym_name = QString::fromStdString(OTAPI_Wrap::GetNym_Name(OTAPI_Wrap::GetNym_ID(a)));
        
        //Add to qlist
        nym_list_name->append(QVariant(OT_nym_name));
        
        //Append to submenu of nym
        QAction * next_nym_action = new QAction(mc_systrayIcon_nym, OT_nym_name, 0);
        next_nym_action->setData(QVariant(OT_nym_id));
        mc_systrayMenu_nym->addAction(next_nym_action);
    }
    
    
}

//Nym manager "clicked"
void Moneychanger::mc_defaultnym_slot(){
    //The operator has requested to open the dialog to the "Nym Manager";
    mc_nymmanager_dialog();
}

//Nym new default selected from systray
void Moneychanger::mc_nymselection_triggered(QAction*action_triggered){
    //Check if the user wants to open the nym manager (or) select a different default nym
    QString action_triggered_string = QVariant(action_triggered->data()).toString();
    qDebug() << "NYM TRIGGERED" << action_triggered_string;
    if(action_triggered_string == "openmanager"){
        //Open nym manager
        mc_defaultnym_slot();
    }else{
        //Set new nym default
        QString action_triggered_string_nym_name = QVariant(action_triggered->text()).toString();
        set_systrayMenu_nym_setDefaultNym(action_triggered_string, action_triggered_string_nym_name);
        
        //Refresh the nym default selection in the nym manager (ONLY if it is open)
        //Check if nym manager has ever been opened (then apply logic) [prevents crash if the dialog hasen't be opend before]
        if(mc_nymmanager_already_init){
            //Refresh if the nym manager is currently open
            if(mc_nymmanager_already_init){
                mc_nymmanager_dialog();
            }
        }
    }
    
}
// End Nym Manager

/** Asset Manager **/
void Moneychanger::mc_assetmanager_dialog(){
    if(!mc_assetmanager_already_init){
        AssetManagerWindow *assetmanagerwindow = new AssetManagerWindow(this);
        assetmanagerwindow->setAttribute(Qt::WA_DeleteOnClose);
        assetmanagerwindow->dialog();
        mc_assetmanager_already_init = true;
    }
}

//This was mistakenly named asset_load_asset, should be set default asset
//Set Default asset
void Moneychanger::set_systrayMenu_asset_setDefaultAsset(QString asset_id, QString asset_name){
    //Set default asset internal memory
    default_asset_id = asset_id;
    default_asset_name = asset_name;
    
    //SQL UPDATE default asset
    DBHandler::getInstance()->AddressBookUpdateDefaultAsset(asset_id);
    
    //Rename "ASSET:" if a asset is loaded
    if(asset_id != ""){
        mc_systrayMenu_asset->setTitle("Asset Type: "+asset_name);
    }
}

void Moneychanger::mc_systrayMenu_reload_assetlist(){
    qDebug() << "RELOAD asset LIST";
    //Count assets
    int32_t asset_count_int32_t = OTAPI_Wrap::GetAssetTypeCount();
    int asset_count = asset_count_int32_t;
    
    //Retrieve updated list of asset submenu actions list
    QList<QAction*> action_list_to_asset_submenu = mc_systrayMenu_asset->actions();
    
    //Remove all sub-menus from the asset submenu
    for(int a = action_list_to_asset_submenu.size(); a > 0; a--){
        qDebug() << "REMOVING" << a;
        QPoint tmp_point = QPoint(a, 0);
        mc_systrayMenu_asset->removeAction(mc_systrayMenu_asset->actionAt(tmp_point));
    }
    
    //Remove all assets from the backend list
    //Remove ids from backend list
    int tmp_asset_list_id_size = asset_list_id->size();
    for(int a = 0; a < tmp_asset_list_id_size; a++){
        asset_list_id->removeLast();
    }
    
    //Remove names from the backend list
    int tmp_asset_list_name_size = asset_list_name->size();
    for(int a = 0; a < tmp_asset_list_name_size; a++){
        asset_list_name->removeLast();
    }
    
    //Add/append to the id + name lists
    for(int a = 0; a < asset_count; a++){
        //Get OT Account ID
        QString OT_asset_id = QString::fromStdString(OTAPI_Wrap::GetAssetType_ID(a));
        
        //Add to qlist
        asset_list_id->append(QVariant(OT_asset_id));
        
        //Get OT Account Name
        QString OT_asset_name = QString::fromStdString(OTAPI_Wrap::GetAssetType_Name(OTAPI_Wrap::GetAssetType_ID(a)));
        
        //Add to qlist
        asset_list_name->append(QVariant(OT_asset_name));
        
        //Append to submenu of asset
        QAction * next_asset_action = new QAction(mc_systrayIcon_purse, OT_asset_name, 0);
        next_asset_action->setData(QVariant(OT_asset_id));
        mc_systrayMenu_asset->addAction(next_asset_action);
    }
    
    
}

//Asset new default selected from systray
void Moneychanger::mc_assetselection_triggered(QAction*action_triggered){
    //Check if the user wants to open the asset manager (or) select a different default asset
    QString action_triggered_string = QVariant(action_triggered->data()).toString();
    qDebug() << "asset TRIGGERED" << action_triggered_string;
    if(action_triggered_string == "openmanager"){
        //Open asset manager
        mc_defaultasset_slot();
    }else{
        //Set new asset default
        QString action_triggered_string_asset_name = QVariant(action_triggered->text()).toString();
        set_systrayMenu_asset_setDefaultAsset(action_triggered_string, action_triggered_string_asset_name);
        
        //Refresh the asset default selection in the asset manager (ONLY if it is open)
        //Check if asset manager has ever been opened (then apply logic) [prevents crash if the dialog hasen't be opend before]
        if(mc_assetmanager_already_init == 1){
            //Refresh if the asset manager is currently open
            if(mc_assetmanager_already_init){
                mc_assetmanager_dialog();
            }
        }
    }
    
}

//Default Asset slots
//Asset manager "clicked"
void Moneychanger::mc_defaultasset_slot(){
    //The operator has requested to open the dialog to the "Asset Manager";
    mc_assetmanager_dialog();
}
// End Asset Manager

/** Account Manager **/
void Moneychanger::mc_accountmanager_dialog(){
    // TODO
    /*
     
     I need to figure out how to best handle keeping only one instance of a window open at a time.
     I'll probably go with a singleton class.
     
     */
    //if(!mc_accountmanager_already_init){
        AccountManagerWindow *accountmanagerwindow = new AccountManagerWindow(this);
       // accountmanagerwindow->setAttribute(Qt::WA_DeleteOnClose);
        accountmanagerwindow->dialog();
      //  mc_accountmanager_already_init = true;
    //}
};

//Default Account slots
//Account manager "clicked"
void Moneychanger::mc_defaultaccount_slot(){
    //The operator has requested to open the dialog to the "account Manager";
    mc_accountmanager_dialog();
}

//account new default selected from systray
void Moneychanger::mc_accountselection_triggered(QAction*action_triggered){
    //Check if the user wants to open the account manager (or) select a different default account
    QString action_triggered_string = QVariant(action_triggered->data()).toString();
    qDebug() << "account TRIGGERED" << action_triggered_string;
    if(action_triggered_string == "openmanager"){
        //Open account manager
        mc_defaultaccount_slot();
    }else{
        //Set new account default
        QString action_triggered_string_account_name = QVariant(action_triggered->text()).toString();
        set_systrayMenu_account_setDefaultAccount(action_triggered_string, action_triggered_string_account_name);
        
        //Refresh the account default selection in the account manager (ONLY if it is open)
        //Check if account manager has ever been opened (then apply logic) [prevents crash if the dialog hasen't be opend before]
        if(mc_accountmanager_already_init){
            //Refresh if the account manager is currently open
            if(mc_accountmanager_already_init){
                mc_accountmanager_dialog();
            }
        }
    }
    
}


//This was mistakenly named account_load_account, should be set default account
//Set Default account
void Moneychanger::set_systrayMenu_account_setDefaultAccount(QString account_id, QString account_name){
    //Set default account internal memory
    default_account_id = account_id;
    default_account_name = account_name;
    
    //SQL UPDATE default account
    DBHandler::getInstance()->AddressBookUpdateDefaultAccount(account_id);
    
    //Rename "ACCOUNT:" if a account is loaded
    if(account_id != ""){
        QString result = "Account: " + account_name;
        
        int64_t     lBalance = OTAPI_Wrap::GetAccountWallet_Balance(account_id.toStdString());
        std::string strAsset = OTAPI_Wrap::GetAccountWallet_AssetTypeID(account_id.toStdString());
        
        std::string str_amount;
        
        if (!strAsset.empty())
        {
            str_amount = OTAPI_Wrap::FormatAmount(strAsset, lBalance);
            result += " ("+ QString::fromStdString(str_amount) +")";
        }
        
        mc_systrayMenu_account->setTitle(result);
    }
}

void Moneychanger::mc_systrayMenu_reload_accountlist(){
    qDebug() << "RELOAD account LIST";
    //Count accounts
    int32_t account_count_int32_t = OTAPI_Wrap::GetAccountCount();
    int account_count = account_count_int32_t;
    
    //Retrieve updated list of account submenu actions list
    QList<QAction*> action_list_to_account_submenu = mc_systrayMenu_account->actions();
    
    //Remove all sub-menus from the account submenu
    for(int a = action_list_to_account_submenu.size(); a > 0; a--){
        qDebug() << "REMOVING" << a;
        QPoint tmp_point = QPoint(a, 0);
        mc_systrayMenu_account->removeAction(mc_systrayMenu_account->actionAt(tmp_point));
    }
    
    //Remove all accounts from the backend list
    //Remove ids from backend list
    int tmp_account_list_id_size = account_list_id->size();
    for(int a = 0; a < tmp_account_list_id_size; a++){
        account_list_id->removeLast();
    }
    
    //Remove names from the backend list
    int tmp_account_list_name_size = account_list_name->size();
    for(int a = 0; a < tmp_account_list_name_size; a++){
        account_list_name->removeLast();
    }
    
    //Add/append to the id + name lists
    for(int a = 0; a < account_count; a++){
        //Get OT Account ID
        QString OT_account_id = QString::fromStdString(OTAPI_Wrap::GetAccountWallet_ID(a));
        
        //Add to qlist
        account_list_id->append(QVariant(OT_account_id));
        
        //Get OT Account Name
        QString OT_account_name = QString::fromStdString(OTAPI_Wrap::GetAccountWallet_Name(OTAPI_Wrap::GetAccountWallet_ID(a)));
        
        //Add to qlist
        account_list_name->append(QVariant(OT_account_name));
        
        //Append to submenu of account
        QAction * next_account_action = new QAction(mc_systrayIcon_goldaccount, OT_account_name, 0);
        next_account_action->setData(QVariant(OT_account_id));
        mc_systrayMenu_account->addAction(next_account_action);
    }
    
    
}
// ----------------------------------------------------------------------
/** Server **/
/** *********************************************
 * @brief Moneychanger::mc_servermanager_dialog
 * @info Will init & show the server list manager
 ** *********************************************/
void Moneychanger::mc_servermanager_dialog(){
    /** If the server list manager dialog has already been init,
     *  just show it, Other wise, init and show if this is the
     *  first time.
     **/
    if(mc_servermanager_already_init == 0){
        //Init
        mc_server_manager_dialog = new QDialog(0);
        mc_server_manager_dialog->setWindowTitle("Server Manager | Moneychanger");
        mc_server_manager_gridlayout = new QGridLayout(0);
        mc_server_manager_gridlayout->setColumnStretch(1, 0);
        mc_server_manager_dialog->setLayout(mc_server_manager_gridlayout);
        
        /** First Row (Takes up two columns) **/
        //Header (Server List Manager)
        mc_server_manager_label = new QLabel("<h2>Server Manager</h2>");
        mc_server_manager_gridlayout->addWidget(mc_server_manager_label, 0,0, 1,2, Qt::AlignRight);
        
        /** Second Row **/
        //Column One
        //Tableview (Server List)
        mc_server_manager_tableview_itemmodel = new QStandardItemModel(0);
        mc_server_manager_tableview = new QTableView(0);
        mc_server_manager_tableview->setSelectionMode(QAbstractItemView::SingleSelection);
        mc_server_manager_tableview->setModel(mc_server_manager_tableview_itemmodel);
        
        mc_server_manager_tableview_itemmodel->setColumnCount(3);
        mc_server_manager_tableview_itemmodel->setHorizontalHeaderItem(0, new QStandardItem(QString("Display Name")));
        mc_server_manager_tableview_itemmodel->setHorizontalHeaderItem(1, new QStandardItem(QString("Server ID")));
        mc_server_manager_tableview_itemmodel->setHorizontalHeaderItem(2, new QStandardItem(QString("Default")));
        
        //Add to grid
        mc_server_manager_gridlayout->addWidget(mc_server_manager_tableview, 1,0, 1,1);
        
        //Column Two
        mc_server_manager_addremove_btngroup_removebtn = new QPushButton("Remove Server", 0);
        mc_server_manager_addremove_btngroup_removebtn->setStyleSheet("QPushButton{padding:0.5em;}");
        //Make a "click" reaction to the remove server button
        //connect(mc_server_manager_addremove_btngroup_removebtn, SIGNAL(clicked()), this, SLOT(mc_server_manager_request_remove_server_slot()));
        
        //Add to grid
        mc_server_manager_gridlayout->addWidget(mc_server_manager_addremove_btngroup_removebtn, 1,1, 1,1, Qt::AlignTop);
        
        /** Flag already int **/
        mc_servermanager_already_init = 1;
    }
    
    //Resize
    mc_server_manager_dialog->resize(500,300);
    //Show
    mc_server_manager_dialog->show();
    
    /**
     ** Refresh server list data
     **/
    
    //Remove all servers in the list
    mc_server_manager_tableview_itemmodel->removeRows(0, mc_server_manager_tableview_itemmodel->rowCount(), QModelIndex());
    
    //Add/Append/Refresh server list.
    int row_index = 0;
    int32_t serverlist_count = OTAPI_Wrap::GetServerCount();
    for(int a = 0; a < serverlist_count; a++){
        std::string server_id =  OTAPI_Wrap::GetServer_ID(a);
        std::string server_name = OTAPI_Wrap::GetServer_Name(server_id);
        
        //Extract data
        QString server_name_string = QString::fromStdString(server_name);
        QString server_id_string = QString::fromStdString(server_id);
        
        //Place extracted data into the table view
        QStandardItem * col_one = new QStandardItem(server_name_string);
        QStandardItem * col_two = new QStandardItem(server_id_string);
        QStandardItem * col_three = new QStandardItem(0);
        //Set as checkbox
        col_three->setCheckable(1);
        
        //Check if this is the default server; If it is, then mark it as "Checked"
        if(server_id_string == default_server_id){
            col_three->setCheckState(Qt::Checked);
        }
        mc_server_manager_tableview_itemmodel->setItem(row_index, 0, col_one);
        mc_server_manager_tableview_itemmodel->setItem(row_index, 1, col_two);
        mc_server_manager_tableview_itemmodel->setItem(row_index, 2, col_three);
    }
    
    
}

void Moneychanger::mc_systrayMenu_server_setDefaultServer(QString server_id, QString server_name){
    //Set default server internal memory
    default_server_id = server_id;
    default_server_name = server_name;
    
    qDebug() << default_server_id;
    qDebug() << default_server_name;
    
    //SQL UPDATE default server
    DBHandler::getInstance()->AddressBookUpdateDefaultServer(default_server_id);
    
    //Update visuals
    QString new_server_title = default_server_name;
    if(new_server_title == "" || new_server_title == " "){
        new_server_title = "Set Default...";
    }
    
    mc_systrayMenu_server->setTitle("Server: "+new_server_title);
}

void Moneychanger::mc_systrayMenu_reload_serverlist(){
    qDebug() << "RELOAD SERVER LIST";
    //Count server
    int32_t server_count_int32_t = OTAPI_Wrap::GetServerCount();
    int server_count = server_count_int32_t;
    
    //Retrieve updated list of server submenu actions list
    QList<QAction*> action_list_to_server_submenu = mc_systrayMenu_server->actions();
    
    //Remove all sub-menus from the server submenu
    for(int a = action_list_to_server_submenu.size(); a > 0; a--){
        qDebug() << "REMOVING" << a;
        QPoint tmp_point = QPoint(a, 0);
        mc_systrayMenu_server->removeAction(mc_systrayMenu_server->actionAt(tmp_point));
    }
    
    //Remove all servers from the backend list
    //Remove ids from backend list
    int tmp_server_list_id_size = server_list_id->size();
    for(int a = 0; a < tmp_server_list_id_size; a++){
        server_list_id->removeLast();
    }
    
    //Remove names from the backend list
    int tmp_server_list_name_size = server_list_name->size();
    for(int a = 0; a < tmp_server_list_name_size; a++){
        server_list_name->removeLast();
    }
    
    
    //Add/append to the id + name lists
    for(int a = 0; a < server_count; a++){
        //Get OT server ID
        QString OT_server_id = QString::fromStdString(OTAPI_Wrap::GetServer_ID(a));
        
        //Add to qlist
        server_list_id->append(QVariant(OT_server_id));
        
        //Get OT server Name
        QString OT_server_name = QString::fromStdString(OTAPI_Wrap::GetServer_Name(OTAPI_Wrap::GetServer_ID(a)));
        
        //Add to qlist
        server_list_name->append(QVariant(OT_server_name));
        
        //Append to submenu of server
        QAction * next_server_action = new QAction(mc_systrayIcon_server, OT_server_name, 0);
        next_server_action->setData(QVariant(OT_server_id));
        mc_systrayMenu_server->addAction(next_server_action);
    }
}


/** Withdraw **/
//As Cash
void Moneychanger::mc_withdraw_ascash_dialog(){
    
    
    /** If the withdraw as cash dialog has already been init,
     *  just show it, Other wise, init and show if this is the
     *  first time.
     **/
    if(mc_withdraw_ascash_dialog_already_init == 0){
        //Init, then show
        //Init
        mc_systrayMenu_withdraw_ascash_dialog = new QDialog(0);
        /** window properties **/
        //Set window title
        mc_systrayMenu_withdraw_ascash_dialog->setWindowTitle("Withdraw as Cash | Moneychanger");
        //mc_systrayMenu_withdraw_ascash_dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
        
        /** layout and content **/
        //Grid layout
        mc_systrayMenu_withdraw_ascash_gridlayout = new QGridLayout(0);
        mc_systrayMenu_withdraw_ascash_dialog->setLayout(mc_systrayMenu_withdraw_ascash_gridlayout);
        
        //Withdraw As Cash (header label)
        mc_systrayMenu_withdraw_ascash_header_label = new QLabel("<h3>Withdraw as Cash</h3>", 0);
        mc_systrayMenu_withdraw_ascash_header_label->setAlignment(Qt::AlignRight);
        mc_systrayMenu_withdraw_ascash_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_header_label, 0, 0, 1, 1);
        
        //Account ID (label) Note: Value is set when the dropdown box is selected and/or highlighted
        mc_systrayMenu_withdraw_ascash_accountid_label = new QLabel("", 0);
        mc_systrayMenu_withdraw_ascash_accountid_label->setStyleSheet("QLabel{padding:0.5em;}");
        mc_systrayMenu_withdraw_ascash_accountid_label->setAlignment(Qt::AlignHCenter);
        mc_systrayMenu_withdraw_ascash_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_accountid_label, 1, 0, 1, 1);
        
        //Account Dropdown (combobox)
        mc_systrayMenu_withdraw_ascash_account_dropdown = new QComboBox(0);
        mc_systrayMenu_withdraw_ascash_account_dropdown->setStyleSheet("QComboBox{padding:0.5em;}");
        mc_systrayMenu_withdraw_ascash_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_account_dropdown, 2, 0, 1, 1);
        
        //Make connection to "hovering over items" to showing their IDs above the combobox (for user clarity and backend id indexing)
        connect(mc_systrayMenu_withdraw_ascash_account_dropdown, SIGNAL(highlighted(int)), this, SLOT(mc_withdraw_ascash_account_dropdown_highlighted_slot(int)));
        
        //Amount Instructions
        //TODO ^^
        
        //Amount Input
        mc_systrayMenu_withdraw_ascash_amount_input = new QLineEdit;
        mc_systrayMenu_withdraw_ascash_amount_input->setPlaceholderText("Amount");
        mc_systrayMenu_withdraw_ascash_amount_input->setStyleSheet("QLineEdit{padding:0.5em;}");
        mc_systrayMenu_withdraw_ascash_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_amount_input, 3, 0, 1, 1);
        
        //Withdraw Button
        mc_systrayMenu_withdraw_ascash_button = new QPushButton("Withdraw as Cash");
        mc_systrayMenu_withdraw_ascash_button->setStyleSheet("QPushButton{padding:0.5em;}");
        mc_systrayMenu_withdraw_ascash_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_button, 4, 0, 1, 1);
        //Connect button with re-action
        connect(mc_systrayMenu_withdraw_ascash_button, SIGNAL(pressed()), this, SLOT(mc_withdraw_ascash_confirm_amount_dialog_slot()));
        
        /** Flag already init **/
        mc_withdraw_ascash_dialog_already_init = 1;
    }
    //Resize
    mc_systrayMenu_withdraw_ascash_dialog->resize(400, 120);
    //Show
    mc_systrayMenu_withdraw_ascash_dialog->show();
    mc_systrayMenu_withdraw_ascash_dialog->activateWindow();
    
    /** Refresh dynamic lists **/
    //remove all items from nym dropdown box
    while (mc_systrayMenu_withdraw_ascash_account_dropdown->count() > 0)
        mc_systrayMenu_withdraw_ascash_account_dropdown->removeItem(0);
    
    for(int a = 0; a < nym_list_id->size(); a++){
        //Add to combobox
        //Get OT Account ID
        mc_systrayMenu_withdraw_ascash_account_dropdown->addItem(account_list_name->at(a).toString(), account_list_id->at(a).toString());
    }
}

//As Voucher
void Moneychanger::mc_withdraw_asvoucher_dialog(){
    
    
    /** If the withdraw as voucher dialog has already been init,
     *  just show it, Other wise, init and show if this is the
     *  first time.
     **/
    if(mc_withdraw_asvoucher_dialog_already_init == 0){
        //Init, then show
        //Init
        mc_systrayMenu_withdraw_asvoucher_dialog = new QDialog(0);
        /** window properties **/
        //Set window title
        mc_systrayMenu_withdraw_asvoucher_dialog->setWindowTitle("Withdraw as Voucher | Moneychanger");
        //mc_systrayMenu_withdraw_asvoucher_dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
        
        /** layout and content **/
        //Grid layout Input
        mc_systrayMenu_withdraw_asvoucher_gridlayout = new QGridLayout(0);
        mc_systrayMenu_withdraw_asvoucher_dialog->setLayout(mc_systrayMenu_withdraw_asvoucher_gridlayout);
        
        //Label (withdraw as voucher)
        mc_systrayMenu_withdraw_asvoucher_header_label = new QLabel("<h3>Withdraw as Voucher</h3>", 0);
        mc_systrayMenu_withdraw_asvoucher_header_label->setAlignment(Qt::AlignRight);
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_header_label, 0,0, 1,1);
        
        //Account ID (label) Note: Value is set when the dropdown box is selected and/or highlighted
        mc_systrayMenu_withdraw_asvoucher_accountid_label = new QLabel("", 0);
        mc_systrayMenu_withdraw_asvoucher_accountid_label->setAlignment(Qt::AlignHCenter);
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_accountid_label, 1,0, 1,1);
        
        //Account Dropdown (combobox)
        mc_systrayMenu_withdraw_asvoucher_account_dropdown = new QComboBox(0);
        
        mc_systrayMenu_withdraw_asvoucher_account_dropdown->setStyleSheet("QComboBox{padding:0.5em;}");
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_account_dropdown, 2,0, 1,1);
        
        //Make connection to "hovering over items" to showing their IDs above the combobox (for user clarity and backend id indexing)
        connect(mc_systrayMenu_withdraw_asvoucher_account_dropdown, SIGNAL(highlighted(int)), this, SLOT(mc_withdraw_asvoucher_account_dropdown_highlighted_slot(int)));
        
        //To Nym ID
        //Horizontal Box (to hold Nym Id input/Address Box Icon/QR Code Scanner Icon)
        mc_systrayMenu_withdraw_asvoucher_nym_holder = new QWidget(0);
        mc_systrayMenu_withdraw_asvoucher_nym_hbox = new QHBoxLayout(0);
        mc_systrayMenu_withdraw_asvoucher_nym_hbox->setMargin(0);
        mc_systrayMenu_withdraw_asvoucher_nym_holder->setLayout(mc_systrayMenu_withdraw_asvoucher_nym_hbox);
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_nym_holder, 3,0, 1,1);
        
        //Nym ID (Paste input)
        mc_systrayMenu_withdraw_asvoucher_nym_input = new QLineEdit;
        mc_systrayMenu_withdraw_asvoucher_nym_input->setPlaceholderText("Recipient Nym Id");
        mc_systrayMenu_withdraw_asvoucher_nym_input->setStyleSheet("QLineEdit{padding:0.5em;}");
        mc_systrayMenu_withdraw_asvoucher_nym_hbox->addWidget(mc_systrayMenu_withdraw_asvoucher_nym_input);
        
        
        //Address Book (button)
        mc_systrayMenu_withdraw_asvoucher_nym_addressbook_icon = QIcon(":/icons/addressbook");
        mc_systrayMenu_withdraw_asvoucher_nym_addressbook_btn = new QPushButton(mc_systrayMenu_withdraw_asvoucher_nym_addressbook_icon, "", 0);
        mc_systrayMenu_withdraw_asvoucher_nym_addressbook_btn->setStyleSheet("QPushButton{padding:0.5em;}");
        mc_systrayMenu_withdraw_asvoucher_nym_hbox->addWidget(mc_systrayMenu_withdraw_asvoucher_nym_addressbook_btn);
        //Connect Address book button with a re-action
        connect(mc_systrayMenu_withdraw_asvoucher_nym_addressbook_btn, SIGNAL(clicked()), this, SLOT(mc_withdraw_asvoucher_show_addressbook_slot()));
        
        //QR Code scanner (button)
        //TO DO^^
        
        
        //Amount input
        mc_systrayMenu_withdraw_asvoucher_amount_input = new QLineEdit;
        mc_systrayMenu_withdraw_asvoucher_amount_input->setPlaceholderText("Amount as Integer");
        mc_systrayMenu_withdraw_asvoucher_amount_input->setStyleSheet("QLineEdit{padding:0.5em;}");
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_amount_input, 4,0, 1,1);
        
        //Memo input box
        mc_systrayMenu_withdraw_asvoucher_memo_input = new QTextEdit("Memo", 0);
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_memo_input, 5,0, 1,1);
        
        //Withdraw Button
        mc_systrayMenu_withdraw_asvoucher_button = new QPushButton("Withdraw as Voucher");
        mc_systrayMenu_withdraw_asvoucher_button->setStyleSheet("QPushButton{padding:1em;}");
        mc_systrayMenu_withdraw_asvoucher_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_button, 6,0, 1,1);
        //Connect button with re-action
        connect(mc_systrayMenu_withdraw_asvoucher_button, SIGNAL(clicked()), this, SLOT(mc_withdraw_asvoucher_confirm_amount_dialog_slot()));
        
        /** Flag as init **/
        mc_withdraw_asvoucher_dialog_already_init = 1;
    }
    //Resize & Show
    mc_systrayMenu_withdraw_asvoucher_dialog->resize(400, 120);
    mc_systrayMenu_withdraw_asvoucher_dialog->show();
    mc_systrayMenu_withdraw_asvoucher_dialog->setFocus();
    
    /** Refresh dynamic lists **/
    //remove all items from nym dropdown box
    while (mc_systrayMenu_withdraw_asvoucher_account_dropdown->count() > 0)
        mc_systrayMenu_withdraw_asvoucher_account_dropdown->removeItem(0);
    
    for(int a = 0; a < nym_list_id->size(); a++){
        //Add to combobox
        //Get OT Account ID
        mc_systrayMenu_withdraw_asvoucher_account_dropdown->addItem(account_list_name->at(a).toString(), account_list_id->at(a).toString());
    }
}



/** ****** ****** ****** **
 ** Private Slots        **/


// ---------------------------------------------------------
// SERVER
void Moneychanger::mc_servermanager_addserver_slot(){
    //Decide if we should init and show, or just show
    if(mc_servermanager_addserver_dialog_already_init == 0){
        //Init, then show.
        mc_server_manager_addserver_dialog = new QDialog(0);
        mc_server_manager_addserver_dialog->setWindowTitle("Add Server Contract | Moneychanger");
        mc_server_manager_addserver_dialog->setModal(1);
        
        //Gridlayout
        mc_server_manager_addserver_gridlayout = new QGridLayout(0);
        mc_server_manager_addserver_dialog->setLayout(mc_server_manager_addserver_gridlayout);
        
        //Label (header)
        mc_server_manager_addserver_header = new QLabel("<h2>Add Server Contract</h2>",0);
        mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_header, 0,0, 1,1, Qt::AlignRight);
        
        //Label (Show Advanced Option(s)) (Also a button/connection)
        mc_server_manager_addserver_subheader_toggleadvanced_options_label = new QLabel("<a href='#'>Advanced Option(s)</a>", 0);
        mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_subheader_toggleadvanced_options_label, 1,0, 1,1, Qt::AlignRight);
        //Connect with a re-action
        connect(mc_server_manager_addserver_subheader_toggleadvanced_options_label, SIGNAL(linkActivated(QString)), this, SLOT(mc_addserver_dialog_showadvanced_slot(QString)));
        
        
        //Label (instructions)
        mc_server_manager_addserver_subheader_instructions = new QLabel("Below are some options that will help determine how your server will be added.");
        mc_server_manager_addserver_subheader_instructions->setWordWrap(1);
        mc_server_manager_addserver_subheader_instructions->setStyleSheet("QLabel{padding:0.5em;}");
        mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_subheader_instructions, 2,0, 1,1);
        
        //Label (Choose Source Question)
        //                    mc_server_manager_addserver_choosesource_label = new QLabel("<h3>Enter the Server Contract</h3>");
        //                    mc_server_manager_addserver_choosesource_label->setStyleSheet("QLabel{padding:1em;}");
        //                    mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_choosesource_label, 3,0, 1,1);
        
        //                    //Combobox (Dropdown box: Choose Source)
        //                    mc_server_manager_addserver_choosesource_answer_selection = new QComboBox(0);
        //                    mc_server_manager_addserver_choosesource_answer_selection->addItem("Namecoin");
        //                    mc_server_manager_addserver_choosesource_answer_selection->addItem("No-Source");
        //                    mc_server_manager_addserver_choosesource_answer_selection->setCurrentIndex(1);
        //                    mc_server_manager_addserver_choosesource_answer_selection->setStyleSheet("QComboBox{padding:0.5em;}");
        //                    mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_choosesource_answer_selection, 4,0, 1,1);
        
        //Create server (button)
        mc_server_manager_addserver_create_server_btn = new QPushButton("Add a Server Contract", 0);
        mc_server_manager_addserver_create_server_btn->setStyleSheet("QPushButton{padding:1em;}");
        mc_server_manager_addserver_gridlayout->addWidget(mc_server_manager_addserver_create_server_btn, 5,0, 1,1, Qt::AlignHCenter);
        //Connect create server button with a re-action;
        connect(mc_server_manager_addserver_create_server_btn, SIGNAL(clicked()), this, SLOT(mc_addserver_dialog_createserver_slot()));
        
        /** Flag as already init **/
        mc_servermanager_addserver_dialog_already_init = 1;
    }
    //Resize
    mc_server_manager_addserver_dialog->resize(400, 290);
    //Show
    mc_server_manager_addserver_dialog->show();
}

void Moneychanger::mc_servermanager_removeserver_slot(){
    //Init, then show; If already init, then just show
    if(mc_servermanager_removeserver_dialog_already_init == 0){
        mc_server_manager_removeserver_dialog = new QDialog(0);
        mc_server_manager_removeserver_dialog->setWindowTitle("Remove Server Contract | Moneychanger");
        mc_server_manager_removeserver_dialog->setModal(1);
        //Grid layout
        mc_server_manager_removeserver_gridlayout = new QGridLayout(0);
        mc_server_manager_removeserver_dialog->setLayout(mc_server_manager_removeserver_gridlayout);
    }
    mc_server_manager_removeserver_dialog->show();
}

void Moneychanger::mc_servermanager_dataChanged_slot(QModelIndex topLeft, QModelIndex bottomRight){
    //Ignore triggers while "refreshing" the server manager.
    if(mc_servermanager_refreshing == 0 && mc_servermanager_proccessing_dataChanged == 0){
        /** Flag Proccessing dataChanged **/
        mc_servermanager_proccessing_dataChanged = 1;
        
        /** Proccess the "Display Name" column **/
        if(topLeft.column() == 0){
            //Get the value (as std::string) of the server id
            QStandardItem * server_id_item = mc_server_manager_tableview_itemmodel->item(topLeft.row(), 1);
            QString server_id_string = server_id_item->text();
            std::string server_id = server_id_string.toStdString();
            
            //Get the value (as std::string) of the newly set name of the server id
            QVariant new_server_name_variant = topLeft.data();
            QString new_server_name_string = new_server_name_variant.toString();
            std::string new_server_name = new_server_name_string.toStdString();
            qDebug() << server_id_string;
            //Update the newly set display name for this server in OT ( call ot for updating )
            bool setName_successfull = OTAPI_Wrap::SetServer_Name(server_id, new_server_name);
            if(setName_successfull == true){
                //Do nothing (There is nothing that needs to be done)
            }else{
                //Setting of the display name for this server failed, revert value visually, display recent error
                mc_server_manager_most_recent_erorr->setText("<span style='color:#A80000'><b>Renaming that server failed. (Error Code: 100)</b></span>");
            }
        }
        
        /** Proccess the "Default" column (if triggered) **/
        if(topLeft.column() == 2){
            //The (default) 2 column has checked(or unchecked) a box, (uncheck all checkboxes, except the newly selected one)
            int total_checkboxes = mc_server_manager_tableview_itemmodel->rowCount();
            
            //Uncheck all boxes (always checkMarked the triggered box)
            for(int a = 0; a < total_checkboxes; a++){
                
                //Check if this is a checkbox we should checkmark
                if(a != topLeft.row()){
                    
                    //Get checkbox item
                    QStandardItem * checkbox_model = mc_server_manager_tableview_itemmodel->item(a, 2);
                    
                    //Update the checkbox item at the backend.
                    checkbox_model->setCheckState(Qt::Unchecked);
                    
                    //Update the checkbox item visually.
                    mc_server_manager_tableview_itemmodel->setItem(a, 2, checkbox_model);
                }else if(a == topLeft.row()){
                    
                    //Get checkbox item
                    QStandardItem * checkbox_model = mc_server_manager_tableview_itemmodel->item(a, 2);
                    
                    //Update the checkbox item at the backend.
                    //Get server id we are targeting to update.
                    QStandardItem * server_id = mc_server_manager_tableview_itemmodel->item(a, 1);
                    QVariant server_id_variant = server_id->text();
                    QString server_id_string = server_id_variant.toString();
                    QString server_name_string = QString::fromStdString(OTAPI_Wrap::GetServer_Name(server_id_string.toStdString()));
                    //Update the checkbox item visually.
                    checkbox_model->setCheckState(Qt::Checked);
                    mc_server_manager_tableview_itemmodel->setItem(a, 2, checkbox_model);
                    
                    //Update default server at realtime memory backend
                    
                    mc_systrayMenu_server_setDefaultServer(server_id_string, server_name_string);
                    
                    
                }
                
            }
            
        }
        
        /** Unflag Proccessing Data Changed **/
        mc_servermanager_proccessing_dataChanged = 0;
    }
}


/**** ****
 **** server Manager -> Add server Dialog (Private Slots)
 **** ****/
void Moneychanger::mc_addserver_dialog_showadvanced_slot(QString link_href){
    //If advanced options are already showing, hide, if they are hidden, show them.
    if(mc_servermanager_addserver_dialog_advanced_showing == 0){
        //Show advanced options.
        //Show the Bits option
        
    }else if(mc_servermanager_addserver_dialog_advanced_showing == 1){
        //Hide advanced options.
        //Hide the Bits option
    }
}


void Moneychanger::mc_addserver_dialog_createserver_slot(){
    //            std::string pseudonym = OTAPI_Wrap::CreateNym(1024, "", "");
    //            QString new_pseudonym = QString::fromStdString(pseudonym);
    QString new_server;
    
    //Success if non null
    if(new_server != ""){
        
    }else{
        //Failed to create server type
    }
}



/* Systray menu slots */

//Shutdown slots
void Moneychanger::mc_shutdown_slot(){
    //Disconnect all signals from callin class (probubly main) to this class
    //Disconnect
    QObject::disconnect(this);
    //Close qt app (no need to deinit anything as of the time of this comment)
    //TO DO: Check if the OT queue caller is still proccessing calls.... Then quit the app. (Also tell user that the OT is still calling other wise they might think it froze during OT calls)
    qApp->quit();
}


//Server Slots
/** *****************************************
 * @brief Moneychanger::mc_defaultserver_slot
 * @info Universal call to opening the server list manager.
 ** *****************************************/
void Moneychanger::mc_defaultserver_slot(){
    mc_servermanager_dialog();
}



void Moneychanger::mc_serverselection_triggered(QAction * action_triggered){
    //Check if the user wants to open the nym manager (or) select a different default nym
    QString action_triggered_string = QVariant(action_triggered->data()).toString();
    qDebug() << "SERVER TRIGGERED" << action_triggered_string;
    if(action_triggered_string == "openmanager"){
        //Open server-list manager
        mc_defaultserver_slot();
    }else{
        //Set new server default
        QString action_triggered_string_server_name = QVariant(action_triggered->text()).toString();
        mc_systrayMenu_server_setDefaultServer(action_triggered_string, action_triggered_string_server_name);
        
        //Refresh the server default selection in the server manager (ONLY if it is open)
        //Check if server manager has ever been opened (then apply logic) [prevents crash if the dialog hasen't be opend before]
        if(mc_servermanager_already_init == 1){
            //Refresh if the server manager is currently open
            if(mc_server_manager_dialog->isVisible()){
                mc_servermanager_dialog();
            }
        }
    }
}


/**
 * @brief Moneychanger::mc_servermanager_request_remove_server_slot
 * @info This will attempt to remove the server from the loaded wallet,
 *       At the moment only "one" server can be selected but the for loop is there for
 *       future upgrades of such functionality.
 **/
void Moneychanger::mc_servermanager_request_remove_server_slot(){
    //Extract the currently selected server from the server-list.
    QModelIndexList selected_indexes = mc_server_manager_tableview->selectionModel()->selectedIndexes();
    
    for(int a = 0; a < selected_indexes.size(); a++){
        QModelIndex selected_index = selected_indexes.at(a);
        int selected_row = selected_index.row();
        
        //Get server id
        QModelIndex server_id_modelindex = mc_server_manager_tableview_itemmodel->index(selected_row, 1, QModelIndex());
        QVariant server_id_variant = server_id_modelindex.data();
        QString server_id_string = server_id_variant.toString();
        bool can_remove = OTAPI_Wrap::Wallet_CanRemoveServer(server_id_string.toStdString());
        
        if(can_remove == true){
            //Remove it
            OTAPI_Wrap::Wallet_RemoveServer(server_id_string.toStdString());
        }else{
            //Find out why it can't be removed and alert the user the reasoning.
            //Loop through nyms
            std::string server_id_std = server_id_string.toStdString();
            int num_nyms_registered_at_server = 0;
            int num_nyms = OTAPI_Wrap::GetNymCount();
            for(int b = 0; b < num_nyms; b++){
                bool nym_index_at_server = OTAPI_Wrap::IsNym_RegisteredAtServer(OTAPI_Wrap::GetNym_ID(b), server_id_std);
                if(nym_index_at_server == true){
                    num_nyms_registered_at_server += 1;
                }
            }
        }
        
    }
    
}




//Withdraw Slots
/*
 ** AS CASH SLOTS()
 */


/** Open the dialog window **/
void Moneychanger::mc_withdraw_ascash_slot(){
    //The operator has requested to open the dialog to withdraw as cash.
    mc_withdraw_ascash_dialog();
}

/**
 ** Button from dialog window has been activated;
 ** Confirm amount;
 ** Upon confirmation call OT withdraw_cash()
 **/
void Moneychanger::mc_withdraw_ascash_confirm_amount_dialog_slot(){
    //Close the (withdraw as cash) dialog
    mc_systrayMenu_withdraw_ascash_dialog->hide();
    
    //First confirm this is the correct amount before calling OT
    //Has this dialog already been init before?
    if(mc_withdraw_ascash_confirm_dialog_already_init == 0){
        //First time init
        mc_systrayMenu_withdraw_ascash_confirm_dialog = new QDialog(0);
        
        //Attach layout
        mc_systrayMenu_withdraw_ascash_confirm_gridlayout = new QGridLayout(0);
        mc_systrayMenu_withdraw_ascash_confirm_dialog->setLayout(mc_systrayMenu_withdraw_ascash_confirm_gridlayout);
        
        //Ask the operator to confirm the amount requested
        mc_systrayMenu_withdraw_ascash_confirm_label = new QLabel("Please confirm the amount to withdraw.");
        mc_systrayMenu_withdraw_ascash_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_confirm_label, 0,0, 1,1);
        
        //Label (Amount)
        QString confirm_amount_string = "<b>"+mc_systrayMenu_withdraw_ascash_amount_input->text()+"</b>";
        mc_systrayMenu_withdraw_ascash_confirm_amount_label = new QLabel(confirm_amount_string);
        mc_systrayMenu_withdraw_ascash_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_confirm_amount_label, 1,0, 1,1);
        
        //Set Withdraw as cash amount int
        QString confirm_amount_string_int = mc_systrayMenu_withdraw_ascash_amount_input->text();
        
        withdraw_ascash_confirm_amount_int = confirm_amount_string_int.toInt();
        
        //Spacer
        
        //Horizontal Box
        mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_widget = new QWidget(0);
        mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_layout = new QHBoxLayout(0);
        mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_widget->setLayout(mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_layout);
        mc_systrayMenu_withdraw_ascash_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_widget, 3, 0, 1, 1);
        
        //Button (Cancel amount)
        mc_systrayMenu_withdraw_ascash_confirm_amount_btn_cancel = new QPushButton("Cancel Amount", 0);
        mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_layout->addWidget(mc_systrayMenu_withdraw_ascash_confirm_amount_btn_cancel);
        //Connect the cancel button with a re-action
        connect(mc_systrayMenu_withdraw_ascash_confirm_amount_btn_cancel, SIGNAL(clicked()), this, SLOT(mc_withdraw_ascash_cancel_amount_slot()));
        
        //Button (Confirm amount)
        mc_systrayMenu_withdraw_ascash_confirm_amount_btn_confirm = new QPushButton("Confirm Amount", 0);
        mc_systrayMenu_withdraw_ascash_confirm_amount_confirm_cancel_layout->addWidget(mc_systrayMenu_withdraw_ascash_confirm_amount_btn_confirm);
        //Connect the Confirm button with a re-action
        connect(mc_systrayMenu_withdraw_ascash_confirm_amount_btn_confirm, SIGNAL(clicked()), this, SLOT(mc_withdraw_ascash_confirm_amount_slot()));
        
        /** Flag already init **/
        mc_withdraw_ascash_confirm_dialog_already_init = 1;
        
        //Show
        mc_systrayMenu_withdraw_ascash_confirm_dialog->show();
        
        
    }else{
        //Not first time init, just show the dialog.
        
        //Set Withdraw as cash amount int
        QString confirm_amount_string_int = mc_systrayMenu_withdraw_ascash_amount_input->text();
        withdraw_ascash_confirm_amount_int = confirm_amount_string_int.toInt();
        
        //Show dialog.
        mc_systrayMenu_withdraw_ascash_confirm_dialog->show();
    }
    
}


/**
 ** This will display the account id that the user has selected (for convience also for backend id tracking)
 **/
void Moneychanger::mc_withdraw_ascash_account_dropdown_highlighted_slot(int dropdown_index){
    //Change Account ID label to the highlighted(bymouse) dropdown index.
    mc_systrayMenu_withdraw_ascash_accountid_label->setText(mc_systrayMenu_withdraw_ascash_account_dropdown->itemData(dropdown_index).toString());
}

/**
 ** This will be triggered when the user click the "confirm amount" button from the withdraw/confirm dialog
 **/

void Moneychanger::mc_withdraw_ascash_confirm_amount_slot(){
    //Close the dialog/window
    mc_systrayMenu_withdraw_ascash_confirm_dialog->hide();
    
    //Collect require information to call the OT_ME::withdraw_cash(?,?,?) function
    QString selected_account_id = mc_systrayMenu_withdraw_ascash_account_dropdown->itemData(mc_systrayMenu_withdraw_ascash_account_dropdown->currentIndex()).toString();
    std::string selected_account_id_string = selected_account_id.toStdString();
    
    QString amount_to_withdraw_string = mc_systrayMenu_withdraw_ascash_amount_input->text();
    int64_t amount_to_withdraw_int = amount_to_withdraw_string.toInt();
    
    //Get Nym ID
    std::string nym_id = OTAPI_Wrap::GetAccountWallet_NymID(selected_account_id_string);
    
    //Get Server ID
    std::string selected_server_id_string = OTAPI_Wrap::GetAccountWallet_ServerID(selected_account_id_string);
    
    //Call OTAPI Withdraw cash
    std::string withdraw_cash_response = ot_me->withdraw_cash(selected_server_id_string, nym_id, selected_account_id_string, amount_to_withdraw_int);
    //qDebug() << QString::fromStdString(withdraw_cash_response);
    
    
}

/**
 ** This will be triggered when the user click the "cancel amount" button from the withdraw/confirm dialog
 **/
void Moneychanger::mc_withdraw_ascash_cancel_amount_slot(){
    //Close the dialog/window
    mc_systrayMenu_withdraw_ascash_confirm_dialog->hide();
}


/*
 ** AS VOUCHER SLOTS()
 */

/** Open a new dialog window **/
void Moneychanger::mc_withdraw_asvoucher_slot(){
    //The operator has requested to open the dialog to withdraw as cash.
    mc_withdraw_asvoucher_dialog();
}

/**
 ** This will be triggered when the user hovers over a dropdown (combobox) item in the withdraw as voucher account selection
 **/
void Moneychanger::mc_withdraw_asvoucher_account_dropdown_highlighted_slot(int dropdown_index){
    //Change Account ID label to the highlighted(bymouse) dropdown index.
    mc_systrayMenu_withdraw_asvoucher_accountid_label->setText(mc_systrayMenu_withdraw_asvoucher_account_dropdown->itemData(dropdown_index).toString());
}


//This will show the address book, the opened address book will be set to paste in recipient nym ids if/when selecting a nymid in the addressbook.
void Moneychanger::mc_withdraw_asvoucher_show_addressbook_slot(){
    //Show address book
    mc_addressbook_show("withdraw_as_voucher");
}


/**
 ** Button from dialog window has been activated;
 ** Confirm amount;
 ** Upon confirmation call OT withdraw_voucher()
 **/
void Moneychanger::mc_withdraw_asvoucher_confirm_amount_dialog_slot(){
    //Close the (withdraw as voucher) dialog
    mc_systrayMenu_withdraw_asvoucher_dialog->hide();
    
    //First confirm this is the correct amount before calling OT
    //Has this dialog already been init before?
    if(mc_withdraw_asvoucher_confirm_dialog_already_init == 0){
        //First time init
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog = new QDialog(0);
        //Set window properties
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->setWindowTitle("Confirm Amount | Withdraw as Voucher | Moneychanger");
        //mc_systrayMenu_withdraw_asvoucher_confirm_dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
        
        //Attach layout
        mc_systrayMenu_withdraw_asvoucher_confirm_gridlayout = new QGridLayout(0);
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->setLayout(mc_systrayMenu_withdraw_asvoucher_confirm_gridlayout);
        
        
        //Ask the operator to confirm the amount
        //Ask Label
        mc_systrayMenu_withdraw_asvoucher_confirm_label = new QLabel("<h3>Please confirm the amount to withdraw.</h3>", 0);
        mc_systrayMenu_withdraw_asvoucher_confirm_label->setAlignment(Qt::AlignRight);
        mc_systrayMenu_withdraw_asvoucher_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_confirm_label, 0,0, 1,1);
        
        //Label (Amount)
        QString confirm_amount_string = "<b>"+mc_systrayMenu_withdraw_asvoucher_amount_input->text()+"</b>";
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_label = new QLabel(confirm_amount_string);
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_label->setAlignment(Qt::AlignHCenter);
        mc_systrayMenu_withdraw_asvoucher_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_confirm_amount_label, 1,0, 1,1);
        
        
        //Set Withdraw as voucher amount int
        QString confirm_amount_string_int = mc_systrayMenu_withdraw_asvoucher_amount_input->text();
        withdraw_asvoucher_confirm_amount_int = confirm_amount_string_int.toInt();
        
        
        //Spacer
        
        //Horizontal Box
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_widget = new QWidget(0);
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_layout = new QHBoxLayout(0);
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_widget->setLayout(mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_layout);
        mc_systrayMenu_withdraw_asvoucher_confirm_gridlayout->addWidget(mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_widget, 3, 0, 1, 1);
        
        
        //Button (Cancel amount)
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_cancel = new QPushButton("Cancel Amount", 0);
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_layout->addWidget(mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_cancel);
        //Connect the cancel button with a re-action
        connect(mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_cancel, SIGNAL(clicked()), this, SLOT(mc_withdraw_asvoucher_cancel_amount_slot()));
        
        //Button (Confirm amount)
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_confirm = new QPushButton("Confirm Amount", 0);
        mc_systrayMenu_withdraw_asvoucher_confirm_amount_confirm_cancel_layout->addWidget(mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_confirm);
        //Connect the Confirm button with a re-action
        connect(mc_systrayMenu_withdraw_asvoucher_confirm_amount_btn_confirm, SIGNAL(clicked()), this, SLOT(mc_withdraw_asvoucher_confirm_amount_slot()));
        
        
        /** Flag already init **/
        mc_withdraw_asvoucher_confirm_dialog_already_init = 1;
        
        //Show
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->show();
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->setFocus();
    }else{
        //Show
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->show();
        mc_systrayMenu_withdraw_asvoucher_confirm_dialog->setFocus();
    }
}


//This is activated when the user clicks "Confirm amount"
void Moneychanger::mc_withdraw_asvoucher_confirm_amount_slot(){
    //Close the dialog/window
    mc_systrayMenu_withdraw_asvoucher_confirm_dialog->hide();
    
    //Collect require information to call the OT_ME::withdraw_cash(?,?,?) function
    QString selected_account_id = mc_systrayMenu_withdraw_asvoucher_account_dropdown->itemData(mc_systrayMenu_withdraw_asvoucher_account_dropdown->currentIndex()).toString();
    std::string selected_account_id_string = selected_account_id.toStdString();
    
    QString amount_to_withdraw_string = mc_systrayMenu_withdraw_asvoucher_amount_input->text();
    int64_t amount_to_withdraw_int = amount_to_withdraw_string.toInt();
    
    //Get Nym ID
    std::string nym_id = OTAPI_Wrap::GetAccountWallet_NymID(selected_account_id_string);
    
    //Get Server ID
    std::string selected_server_id_string = OTAPI_Wrap::GetAccountWallet_ServerID(selected_account_id_string);
    
    //Get receipent nym id
    std::string recip_nym_string = QString(mc_systrayMenu_withdraw_asvoucher_nym_input->text()).toStdString();
    
    //Get memo string
    std::string memo_string = QString(mc_systrayMenu_withdraw_asvoucher_memo_input->toPlainText()).toStdString();
    
    //Call OTAPI Withdraw voucher
    std::string withdraw_voucher_response = ot_me->withdraw_voucher(selected_server_id_string, nym_id, selected_account_id_string, recip_nym_string, memo_string, amount_to_withdraw_int);
    qDebug() << QString::fromStdString(withdraw_voucher_response);
    
}

//This is activated when the user clicks "cancel confirmation amount"
void Moneychanger::mc_withdraw_asvoucher_cancel_amount_slot(){
    //Close the dialog/window
    mc_systrayMenu_withdraw_asvoucher_confirm_dialog->hide();
}


//Deposit Slots

/**
 ** Deposit menu button clicked
 **/
void Moneychanger::mc_deposit_slot(){
    mc_deposit_show_dialog();
}


/**
 ** Deposit dialog
 **/
void Moneychanger::mc_deposit_show_dialog(){
    if(mc_deposit_already_init == 0){
        //Init deposit, then show.
        mc_deposit_dialog = new QDialog(0);
        mc_deposit_dialog->setWindowTitle("Deposit | Moneychanger");
        //Gridlayout
        mc_deposit_gridlayout = new QGridLayout(0);
        mc_deposit_gridlayout->setColumnStretch(0, 1);
        mc_deposit_gridlayout->setColumnStretch(1,0);
        mc_deposit_dialog->setLayout(mc_deposit_gridlayout);
        
        //Label (header)
        mc_deposit_header_label = new QLabel("<h1>Deposit</h1>");
        mc_deposit_header_label->setAlignment(Qt::AlignRight);
        mc_deposit_gridlayout->addWidget(mc_deposit_header_label, 0,1, 1,1);
        //Label ("Into Account") (subheader)
        mc_deposit_account_header_label = new QLabel("<h3>Into Account</h3>");
        mc_deposit_account_header_label->setAlignment(Qt::AlignRight);
        mc_deposit_gridlayout->addWidget(mc_deposit_account_header_label, 1,1, 1,1);
        
        //Label ("Into Purse") (subheader)
        mc_deposit_purse_header_label = new QLabel("<h3>Into Purse</h3>");
        mc_deposit_purse_header_label->setAlignment(Qt::AlignRight);
        mc_deposit_gridlayout->addWidget(mc_deposit_purse_header_label, 1,1, 1,1);
        mc_deposit_purse_header_label->hide();
        
        
        //Combobox (choose deposit type)
        mc_deposit_deposit_type = new QComboBox(0);
        mc_deposit_deposit_type->setStyleSheet("QComboBox{padding:1em;}");
        mc_deposit_gridlayout->addWidget(mc_deposit_deposit_type, 0,0, 1,1, Qt::AlignHCenter);
        mc_deposit_deposit_type->addItem("Deposit into your Account", QVariant(0));
        mc_deposit_deposit_type->addItem("Deposit into your Purse", QVariant(1));
        //connect "update" to switching open depsoit account/purse screens.
        connect(mc_deposit_deposit_type, SIGNAL(currentIndexChanged(int)), this, SLOT(mc_deposit_type_changed_slot(int)));
        
        /** Deposit into Account **/
        mc_deposit_account_widget = new QWidget(0);
        mc_deposit_account_layout = new QHBoxLayout(0);
        mc_deposit_account_widget->setLayout(mc_deposit_account_layout);
        mc_deposit_gridlayout->addWidget(mc_deposit_account_widget, 1,0, 1,1);
        //Add to account screen
        
        /** Deposit into Purse **/
        mc_deposit_purse_widget = new QWidget(0);
        mc_deposit_purse_layout = new QHBoxLayout(0);
        mc_deposit_purse_widget->setLayout(mc_deposit_purse_layout);
        mc_deposit_gridlayout->addWidget(mc_deposit_purse_widget, 1,0, 1,1);
        //Add to purse screen
        
        
        //Hide by default
        mc_deposit_purse_widget->hide();
    }
    //Resize
    mc_deposit_dialog->resize(600, 300);
    //Show
    mc_deposit_dialog->show();
}

void Moneychanger::mc_deposit_type_changed_slot(int newIndex){
    /** 0 = Account; 1 = purse **/
    if(newIndex == 0){
        //Show account, hide purse.
        mc_deposit_account_widget->show();
        mc_deposit_account_header_label->show();
        
        mc_deposit_purse_widget->hide();
        mc_deposit_purse_header_label->hide();
        
    }else if(newIndex == 1){
        //Hide account, show purse.
        mc_deposit_account_widget->hide();
        mc_deposit_account_header_label->hide();
        
        mc_deposit_purse_header_label->show();
        mc_deposit_purse_widget->show();
        
    }
}

//Send Funds slots
/**
 ** Send Funds Menu Button Clicked
 **/
void Moneychanger::mc_sendfunds_slot(){
    mc_sendfunds_show_dialog();
}

void Moneychanger::mc_requestfunds_slot(){
    mc_requestfunds_show_dialog();
}

void Moneychanger::mc_sendfunds_show_dialog(){
    if(mc_sendfunds_already_init == 0){
        mc_sendfunds_dialog = new QDialog(0);
        mc_sendfunds_gridlayout = new QGridLayout(0);
        mc_sendfunds_dialog->setLayout(mc_sendfunds_gridlayout);
        //Set window title
        mc_sendfunds_dialog->setWindowTitle("Send Funds | Moneychanger");
        
        //Content
        //Select sendfunds type
        mc_sendfunds_sendtype_combobox = new QComboBox(0);
        mc_sendfunds_sendtype_combobox->setStyleSheet("QComboBox{font-size:15pt;}");
        //Add selection options
        mc_sendfunds_sendtype_combobox->addItem("Send a Payment");
        mc_sendfunds_sendtype_combobox->addItem("Send a Cheque");
        mc_sendfunds_sendtype_combobox->addItem("Send Cash");
        mc_sendfunds_sendtype_combobox->addItem("Send an Account Transfer");
        
        mc_sendfunds_gridlayout->addWidget(mc_sendfunds_sendtype_combobox, 0,0, 1,1, Qt::AlignHCenter);
    }
    
    //Resize
    mc_sendfunds_dialog->resize(500, 300);
    
    //Show
    mc_sendfunds_dialog->show();
}

void Moneychanger::mc_requestfunds_show_dialog(){
    if(mc_requestfunds_already_init == 0){
        mc_requestfunds_dialog = new QDialog(0);
        mc_requestfunds_gridlayout = new QGridLayout(0);
        mc_requestfunds_dialog->setLayout(mc_requestfunds_gridlayout);
        //Set window title
        mc_requestfunds_dialog->setWindowTitle("Request Funds | Moneychanger");
        
        //Content
        //                                //Select requestfunds type
        //                                mc_requestfunds_requesttype_combobox = new QComboBox(0);
        //                                mc_requestfunds_requesttype_combobox->setStyleSheet("QComboBox{font-size:15pt;}");
        //                                    //Add selection options
        //                                    mc_requestfunds_requesttype_combobox->addItem("Write a Cheque");
        
        //                                mc_requestfunds_gridlayout->addWidget(mc_requestfunds_requesttype_combobox, 0,0, 1,1, Qt::AlignHCenter);
    }
    
    //Resize
    mc_requestfunds_dialog->resize(500, 300);
    
    //Show
    mc_requestfunds_dialog->show();
}
