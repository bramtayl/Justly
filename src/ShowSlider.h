#pragma once

#include <QSlider>
#include <QSpinBox>
#include <QHBoxLayout>

class ShowSlider : public QWidget {
    Q_OBJECT
    public:
        const int minimum;
        const int maximum;
        const QString suffix;
        QSpinBox spin_box;
        QSlider slider;
        ShowSlider(int minimum, int maximum, const QString& suffix, QWidget *parent = nullptr);
        QHBoxLayout layout;
};