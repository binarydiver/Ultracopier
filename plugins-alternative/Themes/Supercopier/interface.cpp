/** \file interface.cpp
\brief Define the interface core
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QMessageBox>
#include <QMimeData>
#include <QScrollArea>
#include <QColorDialog>
#include <math.h>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "interface.h"
#include "ui_interface.h"

Themes::Themes(const bool &alwaysOnTop,
               const bool &showProgressionInTheTitle,
               const QColor &progressColorWrite,
               const QColor &progressColorRead,
               const QColor &progressColorRemaining,
               const bool &showDualProgression,
               const quint8 &comboBox_copyEnd,
               const bool &speedWithProgressBar,
               const qint32 &currentSpeed,
               const bool &checkBoxShowSpeed,
               FacilityInterface * facilityEngine,
               const bool &moreButtonPushed) :
    ui(new Ui::interfaceCopy()),
    uiOptions(new Ui::themesOptions())
{
    this->facilityEngine=facilityEngine;
    ui->setupUi(this);
    uiOptions->setupUi(ui->tabWidget->widget(1));

    currentFile     = 0;
    totalFile       = 0;
    currentSize     = 0;
    totalSize       = 0;
    haveError       = false;
    stat            = status_never_started;
    modeIsForced	= false;
    haveStarted     = false;
    storeIsInPause	= false;

    this->progressColorWrite    = progressColorWrite;
    this->progressColorRead     = progressColorRead;
    this->progressColorRemaining= progressColorRemaining;
    this->currentSpeed          = currentSpeed;
    uiOptions->showProgressionInTheTitle->setChecked(showProgressionInTheTitle);
    uiOptions->speedWithProgressBar->setChecked(speedWithProgressBar);
    uiOptions->showDualProgression->setChecked(showDualProgression);
    uiOptions->alwaysOnTop->setChecked(alwaysOnTop);
    //uiOptions->setupUi(ui->tabWidget->widget(1));
    uiOptions->labelStartWithMoreButtonPushed->setVisible(false);
    uiOptions->checkBoxStartWithMoreButtonPushed->setVisible(false);
    uiOptions->label_Slider_speed->setVisible(false);
    uiOptions->SliderSpeed->setVisible(false);
    uiOptions->label_SpeedMaxValue->setVisible(false);
    uiOptions->comboBox_copyEnd->setCurrentIndex(comboBox_copyEnd);
    speedWithProgressBar_toggled(speedWithProgressBar);
    showDualProgression_toggled(showDualProgression);
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    uiOptions->progressColorWrite->setIcon(pixmap);
    pixmap.fill(progressColorRead);
    uiOptions->progressColorRead->setIcon(pixmap);
    pixmap.fill(progressColorRemaining);
    uiOptions->progressColorRemaining->setIcon(pixmap);

    ui->TransferList->setModel(&transferModel);
    transferModel.setFacilityEngine(facilityEngine);
    ui->tabWidget->setCurrentIndex(0);
    uiOptions->checkBoxShowSpeed->setChecked(checkBoxShowSpeed);
    menu=new QMenu(this);
    ui->add->setMenu(menu);

    //connect the options
    checkBoxShowSpeed_toggled(uiOptions->checkBoxShowSpeed->isChecked());
    connect(uiOptions->checkBoxShowSpeed,&QCheckBox::stateChanged,this,&Themes::checkBoxShowSpeed_toggled);
    connect(uiOptions->speedWithProgressBar,&QCheckBox::stateChanged,this,&Themes::speedWithProgressBar_toggled);
    connect(uiOptions->showProgressionInTheTitle,&QCheckBox::stateChanged,this,&Themes::updateTitle);
    connect(uiOptions->showDualProgression,&QCheckBox::stateChanged,this,&Themes::showDualProgression_toggled);
    connect(uiOptions->progressColorWrite,&QAbstractButton::clicked,this,&Themes::progressColorWrite_clicked);
    connect(uiOptions->progressColorRead,	&QAbstractButton::clicked,this,&Themes::progressColorRead_clicked);
    connect(uiOptions->progressColorRemaining,&QAbstractButton::clicked,this,&Themes::progressColorRemaining_clicked);
    connect(uiOptions->alwaysOnTop,&QAbstractButton::clicked,this,&Themes::alwaysOnTop_clickedSlot);

    isInPause(false);

    connect(uiOptions->limitSpeed,		static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,	&Themes::uiUpdateSpeed);
    connect(uiOptions->checkBox_limitSpeed,&QAbstractButton::toggled,		this,	&Themes::uiUpdateSpeed);

    connect(ui->actionAddFile,&QAction::triggered,this,&Themes::forcedModeAddFile);
    connect(ui->actionAddFileToCopy,&QAction::triggered,this,&Themes::forcedModeAddFileToCopy);
    connect(ui->actionAddFileToMove,&QAction::triggered,this,&Themes::forcedModeAddFileToMove);
    connect(ui->actionAddFolderToCopy,&QAction::triggered,this,&Themes::forcedModeAddFolderToCopy);
    connect(ui->actionAddFolderToMove,&QAction::triggered,this,&Themes::forcedModeAddFolderToMove);
    connect(ui->actionAddFolder,&QAction::triggered,this,&Themes::forcedModeAddFolder);

    //setup the search part
    closeTheSearchBox();
    TimerForSearch  = new QTimer(this);
    TimerForSearch->setInterval(500);
    TimerForSearch->setSingleShot(true);
    searchShortcut  = new QShortcut(QKeySequence(QKeySequence::Find),this);
    searchShortcut2 = new QShortcut(QKeySequence(QKeySequence::FindNext),this);
    searchShortcut3 = new QShortcut(QKeySequence(Qt::Key_Escape),this);

    //connect the search part
    connect(TimerForSearch,			&QTimer::timeout,	this,	&Themes::hilightTheSearchSlot);
    connect(searchShortcut,			&QShortcut::activated,	this,	&Themes::searchBoxShortcut);
    connect(searchShortcut2,		&QShortcut::activated,	this,	&Themes::on_pushButtonSearchNext_clicked);
    connect(ui->pushButtonCloseSearch,	&QPushButton::clicked,	this,	&Themes::closeTheSearchBox);
    connect(searchShortcut3,		&QShortcut::activated,	this,	&Themes::closeTheSearchBox);

    //reload directly untranslatable text
    newLanguageLoaded();

    //unpush the more button
    ui->moreButton->setChecked(moreButtonPushed);
    on_moreButton_toggled(moreButtonPushed);

    /// \note important for drag and drop, \see dropEvent()
    setAcceptDrops(true);

    player_pause=QIcon(":/Themes/Supercopier/resources/player_pause.png");
    player_play=QIcon(":/Themes/Supercopier/resources/player_play.png");

    shutdown=facilityEngine->haveFunctionality("shutdown");
    ui->shutdown->setVisible(shutdown);

    selectionModel=ui->TransferList->selectionModel();

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&transferModel,&TransferModel::debugInformation,this,&Themes::debugInformation);
    #endif

    updateSpeed();
    alwaysOnTop_clicked(false);
    #ifdef Q_OS_WIN32
    uiOptions->alwaysOnTop->hide();
    #endif

    uiOptions->labelDualProgression->hide();
    uiOptions->showDualProgression->hide();
    ui->progressBar_all->setMaximumHeight(17);
    ui->progressBar_file->setMaximumHeight(17);
    ui->progressBarCurrentSpeed->setMaximumHeight(17);
    ui->progressBar_all->setMinimumHeight(17);
    ui->progressBar_file->setMinimumHeight(17);
    ui->progressBarCurrentSpeed->setMinimumHeight(17);
    ui->progressBar_all->setStyleSheet(QString("QProgressBar{border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));
    ui->progressBar_file->setStyleSheet(QString("QProgressBar{border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));
    ui->progressBarCurrentSpeed->setStyleSheet(QString("QProgressBar{border:1px solid black;text-align:center;background-image:url(:/Themes/Supercopier/resources/progressbarright.png);}QProgressBar::chunk{background-image: url(:/Themes/Supercopier/resources/progressbarleft.png);}"));

    show();
}

Themes::~Themes()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //disconnect(ui->actionAddFile);
    //disconnect(ui->actionAddFolder);
    delete selectionModel;
    delete menu;
}

QWidget * Themes::getOptionsEngineWidget()
{
    return &optionEngineWidget;
}

void Themes::getOptionsEngineEnabled(const bool &isEnabled)
{
    if(isEnabled)
    {
        QScrollArea *scrollArea=new QScrollArea(ui->tabWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(&optionEngineWidget);
        ui->tabWidget->addTab(scrollArea,facilityEngine->translateText("Copy engine"));
    }
}

void Themes::closeEvent(QCloseEvent *event)
{
    event->ignore();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    emit cancel();
}

void Themes::updateOverallInformation()
{
    if(uiOptions->showProgressionInTheTitle->isChecked())
        updateTitle();
    ui->overall->setText(tr("File %1/%2, size: %3/%4").arg(currentFile).arg(totalFile).arg(facilityEngine->sizeToString(currentSize)).arg(facilityEngine->sizeToString(totalSize)));
}

void Themes::actionInProgess(const Ultracopier::EngineActionInProgress &action)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"start: "+QString::number(action));
    this->action=action;
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            ui->progressBar_all->setMaximum(65535);
            ui->progressBar_all->setMinimum(0);
        break;
        case Ultracopier::Listing:
            ui->progressBar_all->setMaximum(0);
            ui->progressBar_all->setMinimum(0);
        break;
        case Ultracopier::Idle:
            ui->progressBar_all->setMaximum(65535);
            ui->progressBar_all->setMinimum(0);
            if(haveStarted && transferModel.rowCount()<=0)
            {
                if(shutdown && ui->shutdown->isChecked())
                {
                    facilityEngine->callFunctionality("shutdown");
                    return;
                }
                switch(uiOptions->comboBox_copyEnd->currentIndex())
                {
                    case 2:
                        emit cancel();
                    break;
                    case 0:
                        if(!haveError)
                            emit cancel();
                    break;
                    default:
                    break;
                }
                stat = status_stopped;
            }
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"Very wrong switch case!");
        break;
    }
    switch(action)
    {
        case Ultracopier::Copying:
        case Ultracopier::CopyingAndListing:
            ui->pauseButton->setEnabled(true);
            haveStarted=true;
            ui->cancelButton->setText(facilityEngine->translateText("Quit"));
            updatePause();
        break;
        case Ultracopier::Listing:
            ui->pauseButton->setEnabled(false);
            haveStarted=true;//to close if skip at root folder collision
        break;
        case Ultracopier::Idle:
            ui->pauseButton->setEnabled(false);
        break;
        default:
        break;
    }
}

void Themes::newFolderListing(const QString &path)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(action==Ultracopier::Listing)
        ui->from->setText(path);
}

void Themes::detectedSpeed(const quint64 &speed)//in byte per seconds
{
    if(uiOptions->speedWithProgressBar->isChecked())
    {
        quint64 tempSpeed=speed;
        if(tempSpeed>999999999)
            tempSpeed=999999999;
        if(tempSpeed>(quint64)ui->progressBarCurrentSpeed->maximum())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("set max speed to: %1").arg(tempSpeed));
            ui->progressBarCurrentSpeed->setMaximum(tempSpeed);
        }
        ui->progressBarCurrentSpeed->setValue(tempSpeed);
        ui->progressBarCurrentSpeed->setFormat(facilityEngine->speedToString(speed));
    }
    else
        ui->currentSpeed->setText(facilityEngine->speedToString(speed));
}

void Themes::remainingTime(const int &remainingSeconds)
{
    QString labelTimeRemaining("<html><body style=\"white-space:nowrap;\">"+facilityEngine->translateText("Time remaining:")+" ");
    if(remainingSeconds==-1)
        labelTimeRemaining+="&#8734;";
    else
    {
        Ultracopier::TimeDecomposition time=facilityEngine->secondsToTimeDecomposition(remainingSeconds);
        labelTimeRemaining+=QString::number(time.hour)+":"+QString::number(time.minute).rightJustified(2,'0')+":"+QString::number(time.second).rightJustified(2,'0');
    }
    labelTimeRemaining+="</body></html>";
    ui->labelTimeRemaining->setText(labelTimeRemaining);
}

void Themes::errorDetected()
{
    haveError=true;
}

/** \brief support speed limitation */
void Themes::setSupportSpeedLimitation(const bool &supportSpeedLimitationBool)
{
    if(!supportSpeedLimitationBool)
    {
        ui->label_Slider_speed->setVisible(false);
        ui->SliderSpeed->setVisible(false);
        ui->label_SpeedMaxValue->setVisible(false);
        uiOptions->labelShowSpeedAsMain->setVisible(false);
        uiOptions->checkBoxShowSpeed->setVisible(false);
    }
    else
        emit newSpeedLimitation(currentSpeed);
}

//get information about the copy
void Themes::setGeneralProgression(const quint64 &current,const quint64 &total)
{
    currentSize=current;
    totalSize=total;
    if(total>0)
    {
        int newIndicator=((double)current/total)*65535;
        ui->progressBar_all->setValue(newIndicator);
    }
    else
        ui->progressBar_all->setValue(0);
    if(current>0)
        stat = status_started;
    updateOverallInformation();
}

void Themes::setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList)
{
    QList<Ultracopier::ProgressionItem> progressionListBis=progressionList;
    transferModel.setFileProgression(progressionListBis);
    updateCurrentFileInformation();
}

//edit the transfer list
/// \todo check and re-enable to selection
void Themes::getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &returnActions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, returnActions.size(): "+QString::number(returnActions.size()));
    QList<quint64> returnValue=transferModel.synchronizeItems(returnActions);
    totalFile+=returnValue[0];
    totalSize+=returnValue[1];
    currentFile+=returnValue[2];
    if(transferModel.rowCount()==0)
    {
        ui->skipButton->setEnabled(false);
        ui->progressBar_all->setValue(65535);
        ui->progressBar_file->setValue(65535);
        currentSize=totalSize;
    }
    else
        ui->skipButton->setEnabled(true);
    updateOverallInformation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"transferModel.rowCount(): "+QString::number(transferModel.rowCount()));
}

void Themes::setCopyType(const Ultracopier::CopyType &type)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->type=type;
    updateModeAndType();
}

void Themes::forceCopyMode(const Ultracopier::CopyMode &mode)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    modeIsForced=true;
    this->mode=mode;
    if(mode==Ultracopier::Copy)
        ui->tabWidget->setTabText(0,tr("Copy list"));
    else
        ui->tabWidget->setTabText(0,tr("Move list"));
    updateModeAndType();
    updateTitle();
}

void Themes::setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation)
{
    ui->exportTransferList->setVisible(transferListOperation & Ultracopier::TransferListOperation_Export);
    ui->importTransferList->setVisible(transferListOperation & Ultracopier::TransferListOperation_Import);
}

void Themes::haveExternalOrder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
//	ui->moreButton->toggle();
}

void Themes::isInPause(const bool &isInPause)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"isInPause: "+QString::number(isInPause));
    //resume in auto the pause
    storeIsInPause=isInPause;
    updatePause();
}

void Themes::updatePause()
{
    if(storeIsInPause)
    {
        ui->pauseButton->setIcon(player_play);
        if(stat == status_started)
            ui->pauseButton->setText(facilityEngine->translateText("Resume"));
        else
            ui->pauseButton->setText(facilityEngine->translateText("Start"));
    }
    else
    {
        ui->pauseButton->setIcon(player_pause);
        ui->pauseButton->setText(facilityEngine->translateText("Pause"));
    }
}

void Themes::updateCurrentFileInformation()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        ui->from->setText(transfertItem.from);
        ui->to->setText(transfertItem.to);
        ui->current_file->setText(transfertItem.current_file);
        if(transfertItem.progressBar_read!=-1)
        {
            ui->progressBar_file->setRange(0,65535);
            if(uiOptions->showDualProgression->isChecked())
            {
                if(transfertItem.progressBar_read!=transfertItem.progressBar_write)
                {
                    float permilleread=round((float)transfertItem.progressBar_read/65535*1000)/1000;
                    float permillewrite=permilleread-0.001;
                    ui->progressBar_file->setStyleSheet(QString("QProgressBar{border: 1px solid grey;text-align: center;background-color: qlineargradient(spread:pad, x1:%1, y1:0, x2:%2, y2:0, stop:0 %3, stop:1 %4);}QProgressBar::chunk{background-color:%5;}")
                        .arg(permilleread)
                        .arg(permillewrite)
                        .arg(progressColorRemaining.name())
                        .arg(progressColorRead.name())
                        .arg(progressColorWrite.name())
                        );
                }
                else
                    ui->progressBar_file->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                        .arg(progressColorRemaining.name())
                        .arg(progressColorWrite.name())
                        );
                ui->progressBar_file->setValue(transfertItem.progressBar_write);
            }
            else
                ui->progressBar_file->setValue((transfertItem.progressBar_read+transfertItem.progressBar_write)/2);
        }
        else
            ui->progressBar_file->setRange(0,0);
    }
    else
    {
        ui->from->setText("");
        ui->to->setText("");
        ui->current_file->setText("-");
        if(haveStarted && transferModel.rowCount()==0)
            ui->progressBar_file->setValue(65535);
        else if(!haveStarted)
            ui->progressBar_file->setValue(0);
    }
}


void Themes::on_putOnTop_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    QList<int> ids;
    int index=0;
    int loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnTop(ids);
}

void Themes::on_pushUp_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    QList<int> ids;
    int index=0;
    int loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsUp(ids);
}

void Themes::on_pushDown_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    QList<int> ids;
    int index=0;
    int loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsDown(ids);
}

void Themes::on_putOnBottom_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    QList<int> ids;
    int index=0;
    int loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit moveItemsOnBottom(ids);
}

void Themes::on_del_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    selectedItems=selectionModel->selectedRows();
    QList<int> ids;
    int index=0;
    int loop_size=selectedItems.size();
    while(index<loop_size)
    {
        ids << transferModel.data(selectedItems.at(index),Qt::UserRole).toULongLong();
        index++;
    }
    if(ids.size()>0)
        emit removeItems(ids);
}

void Themes::on_cancelButton_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    this->hide();
    emit cancel();
}


void Themes::speedWithProgressBar_toggled(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    ui->progressBarCurrentSpeed->setVisible(checked);
    ui->currentSpeed->setVisible(!checked);
}

void Themes::showDualProgression_toggled(bool checked)
{
    Q_UNUSED(checked);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    updateProgressionColorBar();
}

void Themes::checkBoxShowSpeed_toggled(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    Q_UNUSED(checked);
    updateSpeed();
}

void Themes::on_SliderSpeed_valueChanged(int value)
{
    if(!uiOptions->checkBoxShowSpeed->isChecked())
        return;
    switch(value)
    {
        case 0:
            currentSpeed=0;
        break;
        case 1:
            currentSpeed=1024;
        break;
        case 2:
            currentSpeed=1024*4;
        break;
        case 3:
            currentSpeed=1024*16;
        break;
        case 4:
            currentSpeed=1024*64;
        break;
        case 5:
            currentSpeed=1024*128;
        break;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("value: %1").arg(value));
    emit newSpeedLimitation(currentSpeed);
    updateSpeed();
}

void Themes::uiUpdateSpeed()
{
    if(uiOptions->checkBoxShowSpeed->isChecked())
        return;
    if(!uiOptions->checkBox_limitSpeed->isChecked())
        currentSpeed=0;
    else
        currentSpeed=uiOptions->limitSpeed->value();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("emit newSpeedLimitation(%1)").arg(currentSpeed));
    emit newSpeedLimitation(currentSpeed);
}

void Themes::updateSpeed()
{
    ui->label_Slider_speed->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    ui->SliderSpeed->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    ui->label_SpeedMaxValue->setVisible(uiOptions->checkBoxShowSpeed->isChecked());
    uiOptions->limitSpeed->setVisible(!uiOptions->checkBoxShowSpeed->isChecked());
    uiOptions->checkBox_limitSpeed->setVisible(!uiOptions->checkBoxShowSpeed->isChecked());

    if(uiOptions->checkBoxShowSpeed->isChecked())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("checked, currentSpeed: %1").arg(currentSpeed));
        uiOptions->limitSpeed->setEnabled(false);
        if(currentSpeed==0)
        {
            ui->SliderSpeed->setValue(0);
            ui->label_SpeedMaxValue->setText(facilityEngine->translateText("Unlimited"));
        }
        else if(currentSpeed<=1024)
        {
            if(currentSpeed!=1024)
            {
                currentSpeed=1024;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(1);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*1));
        }
        else if(currentSpeed<=1024*4)
        {
            if(currentSpeed!=1024*4)
            {
                currentSpeed=1024*4;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(2);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*4));
        }
        else if(currentSpeed<=1024*16)
        {
            if(currentSpeed!=1024*16)
            {
                currentSpeed=1024*16;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(3);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*16));
        }
        else if(currentSpeed<=1024*64)
        {
            if(currentSpeed!=1024*64)
            {
                currentSpeed=1024*64;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(4);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*64));
        }
        else
        {
            if(currentSpeed!=1024*128)
            {
                currentSpeed=1024*128;
                emit newSpeedLimitation(currentSpeed);
            }
            ui->SliderSpeed->setValue(5);
            ui->label_SpeedMaxValue->setText(facilityEngine->speedToString((double)(1024*1024)*128));
        }
    }
    else
    {
        uiOptions->checkBox_limitSpeed->setChecked(currentSpeed>0);
        if(currentSpeed>0)
            uiOptions->limitSpeed->setValue(currentSpeed);
        uiOptions->checkBox_limitSpeed->setEnabled(currentSpeed!=-1);
        uiOptions->limitSpeed->setEnabled(uiOptions->checkBox_limitSpeed->isChecked());
    }
}

void Themes::on_pauseButton_clicked()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(storeIsInPause)
        emit resume();
    else
        emit pause();
}

void Themes::on_skipButton_clicked()
{
    TransferModel::currentTransfertItem transfertItem=transferModel.getCurrentTransfertItem();
    if(transfertItem.haveItem)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("skip at running: %1").arg(transfertItem.id));
        emit skip(transfertItem.id);
    }
    else
    {
        if(transferModel.rowCount()>1)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QString("skip at idle: %1").arg(transferModel.firstId()));
            emit skip(transferModel.firstId());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to skip the transfer, because no transfer running");
    }
}

void Themes::updateModeAndType()
{
    menu->clear();
    if(modeIsForced)
    {
        menu->addAction(ui->actionAddFile);
        if(type==Ultracopier::FileAndFolder)
            menu->addAction(ui->actionAddFolder);
    }
    else
    {
        menu->addAction(ui->actionAddFileToCopy);
        menu->addAction(ui->actionAddFileToMove);
        if(type==Ultracopier::FileAndFolder)
        {
            menu->addAction(ui->actionAddFolderToCopy);
            menu->addAction(ui->actionAddFolderToMove);
        }
    }
}

void Themes::forcedModeAddFile()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(mode);
}

void Themes::forcedModeAddFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(mode);
}

void Themes::forcedModeAddFileToCopy()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(Ultracopier::Copy);
}

void Themes::forcedModeAddFolderToCopy()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(Ultracopier::Copy);
}

void Themes::forcedModeAddFileToMove()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFile(Ultracopier::Move);
}

void Themes::forcedModeAddFolderToMove()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    emit userAddFolder(Ultracopier::Move);
}

void Themes::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(modeIsForced)
        forceCopyMode(mode);
    ui->retranslateUi(this);
    uiOptions->retranslateUi(this);
    uiOptions->comboBox_copyEnd->setItemText(0,tr("Don't close if errors are found"));
    uiOptions->comboBox_copyEnd->setItemText(1,tr("Never close"));
    uiOptions->comboBox_copyEnd->setItemText(2,tr("Always close"));
    if(!haveStarted)
        ui->current_file->setText(tr("File Name, 0KB"));
    else
        updateCurrentFileInformation();
    updateOverallInformation();
    updateSpeed();
    if(ui->tabWidget->count()>=3)
        ui->tabWidget->setTabText(2,facilityEngine->translateText("Copy engine"));
    on_moreButton_toggled(ui->moreButton->isChecked());
}

void Themes::on_pushButtonCloseSearch_clicked()
{
    closeTheSearchBox();
}

//close the search box
void Themes::closeTheSearchBox()
{
    currentIndexSearch = -1;
    ui->lineEditSearch->clear();
    ui->lineEditSearch->hide();
    ui->pushButtonSearchPrev->hide();
    ui->pushButtonSearchNext->hide();
    ui->pushButtonCloseSearch->hide();
    ui->searchButton->setChecked(false);
    hilightTheSearch();
}

//search box shortcut
void Themes::searchBoxShortcut()
{
/*	if(ui->lineEditSearch->isHidden())
    {*/
        ui->lineEditSearch->show();
        ui->pushButtonSearchPrev->show();
        ui->pushButtonSearchNext->show();
        ui->pushButtonCloseSearch->show();
        ui->lineEditSearch->setFocus(Qt::ShortcutFocusReason);
        ui->searchButton->setChecked(true);
/*	}
    else
        closeTheSearchBox();*/
}

//hilight the search
void Themes::hilightTheSearch(bool searchNext)
{
    int result=transferModel.search(ui->lineEditSearch->text(),searchNext);
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
        else
        {
            ui->lineEditSearch->setStyleSheet("background-color: rgb(193,255,176);");
            ui->TransferList->scrollTo(transferModel.index(result,0));
        }
    }
}

void Themes::hilightTheSearchSlot()
{
    hilightTheSearch();
}

void Themes::on_pushButtonSearchPrev_clicked()
{
    int result=transferModel.searchPrev(ui->lineEditSearch->text());
    if(ui->lineEditSearch->text().isEmpty())
        ui->lineEditSearch->setStyleSheet("");
    else
    {
        if(result==-1)
            ui->lineEditSearch->setStyleSheet("background-color: rgb(255, 150, 150);");
        else
        {
            ui->lineEditSearch->setStyleSheet("background-color: rgb(193,255,176);");
            ui->TransferList->scrollTo(transferModel.index(result,0));
        }
    }
}

void Themes::on_pushButtonSearchNext_clicked()
{
    hilightTheSearch(true);
}

void Themes::on_lineEditSearch_returnPressed()
{
    hilightTheSearch();
}

void Themes::on_lineEditSearch_textChanged(QString text)
{
    if(text=="")
    {
        TimerForSearch->stop();
        hilightTheSearch();
    }
    else
        TimerForSearch->start();
}

void Themes::on_moreButton_toggled(bool checked)
{
    if(checked)
        this->setMaximumHeight(16777215);
    else
        this->setMaximumHeight(130);
    // usefull under windows
    this->updateGeometry();
    this->update();
    this->adjustSize();
}

/* drag event processing

need setAcceptDrops(true); into the constructor
need implementation to accept the drop:
void dragEnterEvent(QDragEnterEvent* event);
void dragMoveEvent(QDragMoveEvent* event);
void dragLeaveEvent(QDragLeaveEvent* event);
*/
void Themes::dropEvent(QDropEvent *event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"hasUrls");
        emit urlDropped(mimeData->urls());
        event->acceptProposedAction();
    }
}

void Themes::dragEnterEvent(QDragEnterEvent* event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    // if some actions should not be usable, like move, this code must be adopted
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"hasUrls");
        event->acceptProposedAction();
    }
}

void Themes::dragMoveEvent(QDragMoveEvent* event)
{
    // if some actions should not be usable, like move, this code must be adopted
    const QMimeData* mimeData = event->mimeData();
    if(mimeData->hasUrls())
        event->acceptProposedAction();
}

void Themes::dragLeaveEvent(QDragLeaveEvent* event)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    event->accept();
}

void Themes::on_searchButton_toggled(bool checked)
{
    if(checked)
        searchBoxShortcut();
    else
        closeTheSearchBox();
}

void Themes::on_exportTransferList_clicked()
{
    emit exportTransferList();
}

void Themes::on_importTransferList_clicked()
{
    emit importTransferList();
}

void Themes::progressColorWrite_clicked()
{
    QColor color=QColorDialog::getColor(progressColorWrite,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorWrite=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorWrite);
    uiOptions->progressColorWrite->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::progressColorRead_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRead,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRead=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRead);
    uiOptions->progressColorRead->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::progressColorRemaining_clicked()
{
    QColor color=QColorDialog::getColor(progressColorRemaining,this,tr("Select a color"));
    if(!color.isValid())
        return;
    progressColorRemaining=color;
    QPixmap pixmap(75,20);
    pixmap.fill(progressColorRemaining);
    uiOptions->progressColorRemaining->setIcon(pixmap);
    updateProgressionColorBar();
}

void Themes::alwaysOnTop_clicked(bool reshow)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    if(uiOptions->alwaysOnTop->isChecked())
        flags=flags | Qt::X11BypassWindowManagerHint;
    else
        flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    if(uiOptions->alwaysOnTop->isChecked())
        flags=flags | Qt::WindowStaysOnTopHint;
    else
        flags=flags & ~Qt::WindowStaysOnTopHint;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"uiOptions->alwaysOnTop->isChecked(): "+QString::number(uiOptions->alwaysOnTop->isChecked())+", flags: "+QString::number(flags));
    setWindowFlags(flags);
    if(reshow)
        show();
}

void Themes::alwaysOnTop_clickedSlot()
{
    alwaysOnTop_clicked(true);
}

void Themes::updateProgressionColorBar()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    uiOptions->labelProgressionColor->setVisible(uiOptions->showDualProgression->isChecked());
    uiOptions->frameProgressionColor->setVisible(uiOptions->showDualProgression->isChecked());
    if(!uiOptions->showDualProgression->isChecked())
    {
        /*ui->progressBar_all->setStyleSheet("");
        ui->progressBar_file->setStyleSheet("");
        ui->progressBarCurrentSpeed->setStyleSheet("");*/
    }
    else
    {
        ui->progressBar_all->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                           .arg(progressColorRemaining.name())
                                           .arg(progressColorWrite.name())
                                           );
        ui->progressBar_file->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                            .arg(progressColorRemaining.name())
                                            .arg(progressColorWrite.name())
                                            );
        ui->progressBarCurrentSpeed->setStyleSheet(QString("QProgressBar{border:1px solid grey;text-align:center;background-color:%1;}QProgressBar::chunk{background-color:%2;}")
                                           .arg(progressColorRemaining.name())
                                           .arg(progressColorWrite.name())
                                           );
    }
    if(stat==status_never_started)
        updateCurrentFileInformation();
}

void Themes::updateTitle()
{
    if(uiOptions->showProgressionInTheTitle->isChecked() && totalSize>0)
    {
        if(!modeIsForced)
            this->setWindowTitle(QString("%1 %2% of %3 - Ultracopier").arg(facilityEngine->translateText("Transfer")).arg((currentSize*100)/totalSize).arg(facilityEngine->sizeToString(totalSize)));
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(QString("%1 %2% of %3 - Ultracopier").arg(facilityEngine->translateText("Copy")).arg((currentSize*100)/totalSize).arg(facilityEngine->sizeToString(totalSize)));
            else
                this->setWindowTitle(QString("%1 %2% of %3 - Ultracopier").arg(facilityEngine->translateText("Move")).arg((currentSize*100)/totalSize).arg(facilityEngine->sizeToString(totalSize)));
        }
    }
    else
    {
        if(!modeIsForced)
            this->setWindowTitle(QString("%1 - Ultracopier").arg(facilityEngine->translateText("Transfer")));
        else
        {
            if(mode==Ultracopier::Copy)
                this->setWindowTitle(QString("%1 - Ultracopier").arg(facilityEngine->translateText("Copy")));
            else
                this->setWindowTitle(QString("%1 - Ultracopier").arg(facilityEngine->translateText("Move")));
        }
    }
}