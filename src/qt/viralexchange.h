#ifndef VIRALEXCHANGE_H
#define VIRALEXCHANGE_H

#include <QWidget>

namespace Ui {
class ViralExchange;
}

class ViralExchange : public QWidget
{
    Q_OBJECT

public:
    explicit ViralExchange(QWidget *parent = 0);
    ~ViralExchange();

private:
    Ui::ViralExchange *ui;
};

#endif // VIRALEXCHANGE_H
