#include <QWebView>
#include "viralexchange.h"
#include "ui_viralexchange.h"

ViralExchange::ViralExchange(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViralExchange)
{
    ui->setupUi(this);
}

ViralExchange::~ViralExchange()
{
    delete ui;
}
