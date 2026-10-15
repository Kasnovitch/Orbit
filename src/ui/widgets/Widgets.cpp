#include "src/ui/widgets/Widgets.hpp"

#include <QLabel>
#include <QVBoxLayout>

StatTile::StatTile(const QString& title, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("statTile");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(4);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("tileTitle");

    m_value = new QLabel(QStringLiteral("\u2014"), this);
    m_value->setObjectName("tileValue");

    layout->addWidget(titleLabel);
    layout->addWidget(m_value);
}

void StatTile::setValue(const QString& value)
{
    m_value->setText(value);
}

Panel::Panel(const QString& title, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("panel");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);

    auto* header = new QLabel(title, this);
    header->setObjectName("panelTitle");

    m_body = new QVBoxLayout();
    m_body->setSpacing(8);

    layout->addWidget(header);
    layout->addLayout(m_body);
    layout->addStretch();
}
