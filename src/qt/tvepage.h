#ifndef TVEPAGE_H
#define TVEPAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QTimer>

namespace Ui {
    class TvePage;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Trade page widget */
class TvePage : public QWidget
{
    Q_OBJECT

public:
    explicit TvePage(QWidget *parent = 0);
    ~TvePage();

    void setModel(ClientModel *clientModel);
    void setModel(WalletModel *walletModel);

public slots:

// signals:

private:
    Ui::TvePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

private slots:

};

#endif // TVEPAGE_H
