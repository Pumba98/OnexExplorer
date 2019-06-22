#include "MainWindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setAcceptDrops(true);

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomMenuShow(QPoint)));
}

MainWindow::~MainWindow() {
    if (contextMenu) {
        clearMenu();
        delete contextMenu;
    }

    delete ui;
}

void MainWindow::onCustomMenuShow(const QPoint &point) {
    Q_UNUSED(point);

    if (contextMenu) {
        clearMenu();
        delete contextMenu;
        contextMenu = nullptr;
    }
    OnexTreeItem *item = static_cast<OnexTreeItem *>(ui->treeWidget->currentItem());
    if (item == nullptr)
        return;

    contextMenu = new QMenu();
    if (!item->hasParent()) {
        QAction *exportAllAction = new QAction(QObject::tr("Export"), contextMenu);
        contextMenu->addAction(exportAllAction);
        QObject::connect(exportAllAction, SIGNAL(triggered(bool)), this, SLOT(on_actionExport_triggered()));

        QAction *exportOriginalAction = new QAction(QObject::tr("Export as .NOS"), contextMenu);
        contextMenu->addAction(exportOriginalAction);
        QObject::connect(exportOriginalAction, SIGNAL(triggered(bool)), this, SLOT(on_actionSave_as_triggered()));

        QAction *closeThisItem = new QAction(QObject::tr("Close"), contextMenu);
        contextMenu->addAction(closeThisItem);
        QObject::connect(closeThisItem, SIGNAL(triggered(bool)), this, SLOT(on_actionClose_selected_triggered()));
    } else {
        QAction *exportSingleAction = new QAction(QObject::tr("Export"), contextMenu);
        contextMenu->addAction(exportSingleAction);
        QObject::connect(exportSingleAction, SIGNAL(triggered(bool)), this, SLOT(on_actionExport_triggered()));

        QAction *exportSingleToRawAction = new QAction(QObject::tr("Export to raw"), contextMenu);
        contextMenu->addAction(exportSingleToRawAction);
        QObject::connect(exportSingleToRawAction, SIGNAL(triggered(bool)), this, SLOT(actionExportToRaw()));

        QAction *replaceAction = new QAction(QObject::tr("Replace"), contextMenu);
        contextMenu->addAction(replaceAction);
        QObject::connect(replaceAction, SIGNAL(triggered(bool)), this, SLOT(on_actionReplace_triggered()));

        QAction *deleteAction = new QAction(QObject::tr("Delete"), contextMenu);
        contextMenu->addAction(deleteAction);
        QObject::connect(deleteAction, SIGNAL(triggered(bool)), this, SLOT(on_actionClose_selected_triggered()));
    }

    if (contextMenu->isEmpty())
        return;
    qDebug() << contextMenu->height();
    contextMenu->exec(QCursor::pos());
}

void MainWindow::clearMenu() {
    qDebug() << "Disposing menu";
    QList<QAction *> actions = contextMenu->actions();

    for (auto &action : actions)
        delete action;

    contextMenu->clear();
}

void MainWindow::on_actionOpen_triggered() {
    QFileDialog openDialog(this);

    openDialog.setFileMode(QFileDialog::ExistingFiles);
    openDialog.setNameFilter(tr("NosTale Files (*.NOS)"));
    openDialog.setViewMode(QFileDialog::Detail);

    QStringList selectedFiles;
    if (openDialog.exec())
        selectedFiles = openDialog.selectedFiles();

    if (!selectedFiles.empty()) {
        for (auto &file : selectedFiles)
            openFile(file);
    }
}

void MainWindow::openFile(QString path) {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly))
        return;

    if (hasValidHeader(file) == 1)
        handleOpenResults(zlibOpener.decrypt(file));
    else if (hasValidHeader(file) == 2)
        handleOpenResults(ccinfOpener.decrypt(file));
    else
        handleOpenResults(textOpener.decrypt(file));

    file.close();
}

void MainWindow::handleOpenResults(OnexTreeItem *item) {
    ui->treeWidget->addTopLevelItem(item);
    item->setExpanded(true);
}

int MainWindow::hasValidHeader(QFile &file) {
    file.seek(0);
    QByteArray header = file.read(0x0B);
    if (header.mid(0, 7) == "NT Data" || header.mid(0, 10) == "32GBS V1.0" || header.mid(0, 10) == "ITEMS V1.0")
        return 1;
    else if (header.mid(0, 11) == "CCINF V1.20")
        return 2;
    return 0;
}

QString MainWindow::getSelectedDirectory() {
    QString dir = QFileDialog::getExistingDirectory(0, tr("Select Directory"));
    if (dir.isEmpty())
        return dir;

    return dir + "/";
}

void MainWindow::dropEvent(QDropEvent *e) {
    for (auto &url : e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        openFile(fileName);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    for (auto &url : e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        if (QFileInfo(fileName).suffix() != "NOS")
            return;
    }
    e->acceptProposedAction();
}

void MainWindow::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *prev) {
    Q_UNUSED(prev);

    QWidget *old = ui->gridLayout->itemAt(ui->gridLayout->count() - 1)->widget();

    OnexTreeItem *item = static_cast<OnexTreeItem *>(current);

    if (item == nullptr) {
        ui->gridLayout->replaceWidget(old, new QWidget());
        delete old;
        return;
    }

    QWidget *previewWindow = item->onClicked();

    if (!previewWindow || item->childCount() != 0) {
        ui->gridLayout->replaceWidget(old, new QWidget());
        delete old;
        return;
    }
    ui->gridLayout->replaceWidget(old, previewWindow);
    delete old;
    previewWindow->setAttribute(Qt::WA_DeleteOnClose);
    previewWindow->show();
}

void MainWindow::on_actionClose_selected_triggered() {
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();
    foreach (auto &item, selectedItems) {
        if (item->parent() && item->parent()->childCount() == 1) {
            delete item->parent();
        } else
            delete item;
    }
}

void MainWindow::on_actionReplace_triggered() {
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();
    if (selectedItems.size() == 0) {
        QMessageBox::information(NULL, tr("Info"), tr("Select file first"));
        return;
    }

    QString directory = getSelectedDirectory();
    if (directory.isEmpty())
        return;

    int count = 0;
    foreach (auto &s, selectedItems) {
        OnexTreeItem *item = static_cast<OnexTreeItem *>(s);
        if (!item->hasParent())
            on_actionImport_triggered();
        else
            count += item->onReplace(directory);
    }

    QString text = "Replaced " + QString::number(count) + " file(s).";
    QMessageBox msgBox(QMessageBox::Information, tr("End of operation"), text);
    msgBox.exec();
}

void MainWindow::on_actionExport_triggered() {
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();
    if (selectedItems.size() == 0) {
        QMessageBox::information(NULL, tr("Info"), tr("Select file first"));
        return;
    }

    QString directory = getSelectedDirectory();
    if (directory.isEmpty())
        return;

    int count = 0;
    foreach (auto &s, selectedItems) {
        OnexTreeItem *item = static_cast<OnexTreeItem *>(s);
        if (item->childCount() == 0) {
            item->onExportSingle(directory);
            count++;
        } else
            count += item->onExportAll(directory);
    }

    QString text = "Saved " + QString::number(count) + " file(s).";
    QMessageBox msgBox(QMessageBox::Information, tr("End of operation"), text);
    msgBox.exec();
}

void MainWindow::actionExportToRaw() {
    QList<QTreeWidgetItem *> selectedItems = ui->treeWidget->selectedItems();
    if (selectedItems.size() == 0) {
        QMessageBox::information(NULL, tr("Info"), tr("Select file first"));
        return;
    }

    QString directory = getSelectedDirectory();
    if (directory.isEmpty())
        return;

    int count = 0;
    foreach (auto &s, selectedItems) {
        OnexTreeItem *item = static_cast<OnexTreeItem *>(s);
        item->onExportSingle(directory);
        count++;
    }
    QString text = "Saved " + QString::number(count) + " file(s).";
    QMessageBox msgBox(QMessageBox::Information, tr("End of operation"), text);
    msgBox.exec();
}

void MainWindow::on_actionImport_triggered() {
    if (ui->treeWidget->currentItem()) {
        OnexTreeItem *item = static_cast<OnexTreeItem *>(ui->treeWidget->currentItem());
        while (item->hasParent()) {
            item = static_cast<OnexTreeItem *>(item->QTreeWidgetItem::parent());
        }

        QString directory = getSelectedDirectory();
        if (directory.isEmpty())
            return;

        int count = item->onReplace(directory);
        QString text = "Imported " + QString::number(count) + " file(s).";
        QMessageBox msgBox(QMessageBox::Information, tr("End of operation"), text);
        msgBox.exec();
    } else {
        QMessageBox::information(NULL, tr("Info"), tr("Select file first"));
    }
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::information(NULL, tr("About Project"),
                             tr("OnexExplorer is an open-source tool for unpacking and repacking .NOS "
                                "data files from game called NosTale. "
                                "<br>It can open almost all .NOS files and show the data stored in "
                                "them."
                                "<br>GitHub: <a "
                                "href='https://github.com/OnexTale/OnexExplorer'>https://github.com/"
                                "OnexTale/OnexExplorer</a>"));
}

void MainWindow::on_actionSave_as_triggered() {
    if (ui->treeWidget->currentItem()) {

        OnexTreeItem *item = static_cast<OnexTreeItem *>(ui->treeWidget->currentItem());
        while (item->hasParent()) {
            item = static_cast<OnexTreeItem *>(item->QTreeWidgetItem::parent());
        }

        if (!item->hasParent()) {
            item->onExporAsOriginal();
        } else {
            QMessageBox::information(NULL, tr("Info"), tr("Select correct *.NOS file"));
        }
    } else {
        QMessageBox::information(NULL, tr("Info"), tr("Select .NOS file first"));
    }
}

void MainWindow::on_actionExit_triggered() {
    QMessageBox::StandardButton message =
        QMessageBox::question(this, "", "Exit program? All unsaved changes will be lost!",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (message == QMessageBox::Yes)
        QApplication::quit();
}

void MainWindow::on_actionClose_all_triggered() {
    QMessageBox::StandardButton message =
        QMessageBox::question(this, "", "Close all items? All unsaved changes will be lost!",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (message == QMessageBox::Yes) {
        ui->treeWidget->clear();
    }
}

void MainWindow::on_actionOptions_triggered() {
    OnexTreeItem *item = static_cast<OnexTreeItem *>(ui->treeWidget->currentItem());
    if (item == nullptr)
        return;
    if (item->childCount() != 0)
        return;
    FileInfo dialog(item);
    dialog.exec();
}