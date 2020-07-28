#include <QtWidgets/QFileDialog>
#include "MainWindow.h"
#include <Document.h>
#include <QtWidgets/QLabel>
#include <include/QSSPreprocessor.h>
#include <include/Properties.h>
#include <include/Globals.h>
#include <iostream>
#include <QtGui/QtGui>
#include <include/QHBoxWidget.h>
#include <QApplication>


using namespace BRLCAD;
using namespace std;



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    loadTheme();
    prepareUi();
    prepareDockables();


    if(QCoreApplication::arguments().length()>1){
        openFile(QString(QCoreApplication::arguments().at(1)));
    }
}

MainWindow::~MainWindow()
{
    for (const std::pair<const int, Document *> pair: documents){
        Document * document = pair.second;
        delete document;
    }
}


void MainWindow::prepareUi() {
    setWindowFlags(Qt::FramelessWindowHint);    // Hide window title bar

	// Menu bar -------------------------------------------------------------------------------------------------------------
    menuTitleBar = new QMenuBar(this);
    menuTitleBar->installEventFilter(this);
    setMenuBar(menuTitleBar);

    QMenu *fileMenu = menuTitleBar->addMenu(tr("&File"));

    QAction* openAct = new QAction(tr("&Open"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Opens a .g file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFileDialog);
    fileMenu->addAction(openAct);

    QAction* saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save database"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFileDialog);
    fileMenu->addAction(saveAct);

    QMenu* editMenu = menuTitleBar->addMenu(tr("&Edit"));

    // Title bar [widgets in the menu bar] ----------------------------------------------------------------------------------------
    QPushButton* applicationIcon = new QPushButton(menuTitleBar);
    applicationIcon->setIcon(QIcon(":/icons/archer.png"));
    applicationIcon->setObjectName("topLeftAppIcon");
    menuTitleBar->setCornerWidget(applicationIcon, Qt::TopLeftCorner);

    QHBoxLayout* layoutTopRightWidget = new QHBoxLayout;
    layoutTopRightWidget->setContentsMargins(0, 0, 0, 0);
    QWidget* topRightWidget = new QWidget;
    topRightWidget->setLayout(layoutTopRightWidget);
    menuTitleBar->setCornerWidget(topRightWidget, Qt::TopRightCorner);
    layoutTopRightWidget->setSpacing(0);

    QPushButton* minimizeButton = new QPushButton(topRightWidget);
    minimizeButton->setIcon(QIcon(":/icons/minimize.png"));
    minimizeButton->setObjectName("minimizeButton");
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::minimizeButtonPressed);
    layoutTopRightWidget->addWidget(minimizeButton);

    maximizeButton = new QPushButton(topRightWidget);
    if (this->windowState() == Qt::WindowMaximized) {
        maximizeButton->setIcon(QIcon(":/icons/restore_down.png"));
    }
    else {
        maximizeButton->setIcon(QIcon(":/icons/maximize.png"));
    }
    maximizeButton->setObjectName("maximizeButton");
    connect(maximizeButton, &QPushButton::clicked, this, &MainWindow::maximizeButtonPressed);
    layoutTopRightWidget->addWidget(maximizeButton);

    QPushButton* closeButton = new QPushButton(topRightWidget);
    closeButton->setIcon(QIcon(":/icons/close.png"));
    closeButton->setObjectName("closeButton");
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::closeButtonPressed);
    layoutTopRightWidget->addWidget(closeButton);

	// Status bar ----------------------------------------------------------------------------------------------------------
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBarPathLabel = new QLabel("No document open");
    statusBarPathLabel->setObjectName("statusBarPathLabel");
    statusBar->addWidget(statusBarPathLabel);
	

    // Document area --------------------------------------------------------------------------------------------------------
    documentArea = new QTabWidget(this);
    documentArea->setObjectName("documentArea");
    documentArea->setMovable(true);
    documentArea->setTabsClosable(true);
    setCentralWidget(documentArea);
    documentArea->tabBar()->setObjectName("documentAreaTabBar");
    connect(documentArea, &QTabWidget::currentChanged, this, &MainWindow::onActiveDocumentChanged);
    connect(documentArea, &QTabWidget::tabCloseRequested, this, &MainWindow::tabCloseRequested);
	
    QHBoxWidget * mainTabBarCornerWidget = new QHBoxWidget();
    mainTabBarCornerWidget->setObjectName("mainTabBarCornerWidget");

    QPushButton* newFileButton = new QPushButton(menuTitleBar);
    newFileButton->setIcon(QPixmap::fromImage(QImage(":/icons/icons8-new-file-80.png")));
    newFileButton->setObjectName("toolbarButton");
    mainTabBarCornerWidget->addWidget(newFileButton);

    QPushButton* openFileButton = new QPushButton(menuTitleBar);
    openFileButton->setIcon(QPixmap::fromImage(QImage(":/icons/icons8-opened-folder-80.png")));
    openFileButton->setObjectName("toolbarButton");
    mainTabBarCornerWidget->addWidget(openFileButton);	
    connect(openFileButton, &QPushButton::clicked, this, &MainWindow::openFileDialog);

    QPushButton* saveFileButton = new QPushButton(menuTitleBar);
    saveFileButton->setIcon(QPixmap::fromImage(QImage(":/icons/icons8-save-80.png")));
    saveFileButton->setObjectName("toolbarButton");
    mainTabBarCornerWidget->addWidget(saveFileButton);
    connect(saveFileButton, &QPushButton::clicked, this, &MainWindow::saveFileDialog);

    documentArea->setCornerWidget(mainTabBarCornerWidget,Qt::Corner::TopLeftCorner);
}

void MainWindow::loadTheme()
{
	QFile themeFile( ":themes/arbalest_light.theme" );
	themeFile.open( QFile::ReadOnly );
	QString themeStr( themeFile.readAll() );
	Globals::theme = new QSSPreprocessor(themeStr);
	themeFile.close();

	QFile styleFile( ":styles/arbalest_simple.qss" );
	styleFile.open( QFile::ReadOnly );
	QString styleStr(styleFile.readAll() );
	qApp->setStyleSheet(Globals::theme->process(styleStr));
	styleFile.close();
}

void MainWindow::prepareDockables(){
    // Object tree
    objectTreeDockable = new Dockable("Objects", this,false,300);
    addDockWidget(Qt::RightDockWidgetArea,objectTreeDockable);

    // Properties
    objectPropertiesDockable = new Dockable("Properties", this,true,300);
    addDockWidget(Qt::RightDockWidgetArea, objectPropertiesDockable);

    // Toolbox
    toolboxDockable = new Dockable("Make", this,true,30);
    addDockWidget(Qt::LeftDockWidgetArea, toolboxDockable);
}

void MainWindow::openFile(const QString& filePath){
    Document & document = * (new Document(filePath.toUtf8().data(), documentsCount));
    document.getObjectTree()->setObjectName("dockableContent");
    document.getProperties()->setObjectName("dockableContent");
    documents[documentsCount++] = &document;
    QString filename(QFileInfo(filePath).fileName());
    const int tabIndex = documentArea->addTab(document.getDisplay(),filename);
    documentArea->setCurrentIndex(tabIndex);
    connect(documents[activeDocumentId]->getObjectTree(), &ObjectTree::SelectionChanged,
            this, &MainWindow::objectTreeSelectionChanged);
}

void MainWindow::openFileDialog()
{
    QString filePath = QFileDialog::getOpenFileName(documentArea, tr("Open BRL-CAD database"), QString(), "BRL-CAD Database (*.g)");
    if (!filePath.isEmpty()){
        openFile(filePath);
    }
}

void MainWindow::saveFileDialog(){
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save BRL-CAD database"), QString(), "BRL-CAD Database (*.g)");
    documents[activeDocumentId]->getDatabase()->Save(filePath.toUtf8().data());
}

void MainWindow::onActiveDocumentChanged(const int newIndex){
    Display * display = dynamic_cast<Display*>(documentArea->widget(newIndex));
    if (display == nullptr) return;
    if (display->getDocumentId() != activeDocumentId){
        activeDocumentId = display->getDocumentId();
        objectTreeDockable->setContent(documents[activeDocumentId]->getObjectTree());
        objectPropertiesDockable->setContent(documents[activeDocumentId]->getProperties());
        statusBarPathLabel->setText(documents[activeDocumentId]->getFilePath());
    }
}

void MainWindow::tabCloseRequested(const int i) const
{
    documentArea->removeTab(i);
    if (documentArea->currentIndex() == -1){
        objectTreeDockable->clear();
        objectPropertiesDockable->clear();
    }
}

void MainWindow::objectTreeSelectionChanged(QString fullPath) {
    documents[activeDocumentId]->getProperties()->bindObject(fullPath);
}

void MainWindow::closeButtonPressed(){
    close();
}

void MainWindow::minimizeButtonPressed() {
    showMinimized();
}

void MainWindow::maximizeButtonPressed() {
    if(!isMaximized())showMaximized();
    else showNormal();
}

// drag window by holding from menu bar (now functioning as title bar too)
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    static QPoint dragPosition{};
    if (watched == menuTitleBar)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
            if (mouse_event->button() == Qt::LeftButton)
            {
                dragPosition = mouse_event->globalPos() - frameGeometry().topLeft();
                return false;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
            if (mouse_event->buttons() & Qt::LeftButton)
            {
                if(isMaximized()) return false;//showNormal();
                //todo showNormal when dragged
                move(mouse_event->globalPos() - dragPosition);
                return false;
            }
        }


    }
    return false;
}

void MainWindow::changeEvent( QEvent* e ) {
    if (e->type() == QEvent::WindowStateChange) {
        if (this->windowState() == Qt::WindowMaximized) {
            maximizeButton->setIcon(QIcon(":/icons/restore_down.png"));
        }
        else{
            maximizeButton->setIcon(QIcon(":/icons/maximize.png"));
        }
    }
}