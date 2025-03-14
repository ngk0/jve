#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"

#include <QPainter>
#include <QStyleOption>

AbstractControl::AbstractControl(const QString &title, const QString &desc, const QString &icon, QWidget *parent, QList<struct ConfigButton> *btns) : QFrame(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(0);

  hlayout = new QHBoxLayout;
  hlayout->setMargin(0);
  hlayout->setSpacing(20);

  // left icon
  icon_label = new QLabel(this);
  hlayout->addWidget(icon_label);
  if (!icon.isEmpty()) {
    icon_pixmap = QPixmap(icon).scaledToWidth(80, Qt::SmoothTransformation);
    icon_label->setPixmap(icon_pixmap);
    icon_label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  }
  icon_label->setVisible(!icon.isEmpty());

  // title
  title_label = new QPushButton(title);
  title_label->setFixedHeight(120);
  title_label->setStyleSheet("font-size: 50px; font-weight: 400; text-align: left; border: none;");
  hlayout->addWidget(title_label, 1);

  // value next to control button
  value = new ElidedLabel();
  value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  value->setStyleSheet("color: #aaaaaa");
  hlayout->addWidget(value);

  // value next to control button
  value = new ElidedLabel();
  value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  value->setStyleSheet("color: #aaaaaa");
  hlayout->addWidget(value);

  main_layout->addLayout(hlayout);

  QVBoxLayout *config_layout = new QVBoxLayout;

  bool hasToggle = false;
  // description
  if (!desc.isEmpty()) {
    hasToggle = true;
    description = new QLabel(desc);
    description->setContentsMargins(40, 20, 40, 20);
    description->setStyleSheet("font-size: 40px; color: grey");
    description->setWordWrap(true);
    config_layout->addWidget(description);
  }

  if (btns && btns->size() > 0) {
    hasToggle = true;
    for (int i = 0; i < btns->size(); i++) {
      const ConfigButton btn = btns->at(i);

      const auto existng_value = Params().get(btn.param);
      const auto control_title = QString::fromStdString(btn.title.toStdString() + ": " + existng_value);
      const auto b = new ButtonControl(control_title, "CHANGE", btn.text);
      QObject::connect(b, &ButtonControl::clicked, [=]() {
          auto set_value = Params().get(btn.param);
          auto new_value = InputDialog::getConfigDecimal(btn.title, parent, set_value, btn.min, btn.max);
          if (new_value.length() > 0) {
            Params().put(btn.param, new_value.toStdString());
            b->setLabel(QString::fromStdString(btn.title.toStdString() + ": " + new_value.toStdString()));
          }
        });
      b->setContentsMargins(40, 20, 0, 0);

      config_layout->addWidget(b);
    }
  }

  if (hasToggle) {
    config_widget = new QWidget;
    config_widget->setVisible(false);
    config_widget->setLayout(config_layout);
    connect(title_label, &QPushButton::clicked, [=]() {
      if (!description->isVisible()) {
        emit showDescriptionEvent();
      }
      config_widget->setVisible(!config_widget->isVisible());
    });
    main_layout->addWidget(config_widget);
  }
  main_layout->addStretch();
}

void AbstractControl::hideEvent(QHideEvent *e) {
  if (config_widget != nullptr) {
    config_widget->hide();
  }
}

// controls

ButtonControl::ButtonControl(const QString &title, const QString &text, const QString &desc, QWidget *parent) : AbstractControl(title, desc, "", parent) {
  btn.setText(text);
  btn.setStyleSheet(R"(
    QPushButton {
      padding: 0;
      border-radius: 50px;
      font-size: 35px;
      font-weight: 500;
      color: #E4E4E4;
      background-color: #393939;
    }
    QPushButton:pressed {
      background-color: #4a4a4a;
    }
    QPushButton:disabled {
      color: #33E4E4E4;
    }
  )");
  btn.setFixedSize(250, 100);
  QObject::connect(&btn, &QPushButton::clicked, this, &ButtonControl::clicked);
  hlayout->addWidget(&btn);
}

// ElidedLabel

ElidedLabel::ElidedLabel(QWidget *parent) : ElidedLabel({}, parent) {}

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent) : QLabel(text.trimmed(), parent) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  setMinimumWidth(1);
}

void ElidedLabel::resizeEvent(QResizeEvent* event) {
  QLabel::resizeEvent(event);
  lastText_ = elidedText_ = "";
}

void ElidedLabel::paintEvent(QPaintEvent *event) {
  const QString curText = text();
  if (curText != lastText_) {
    elidedText_ = fontMetrics().elidedText(curText, Qt::ElideRight, contentsRect().width());
    lastText_ = curText;
  }

  QPainter painter(this);
  drawFrame(&painter);
  QStyleOption opt;
  opt.initFrom(this);
  style()->drawItemText(&painter, contentsRect(), alignment(), opt.palette, isEnabled(), elidedText_, foregroundRole());
}

// ParamControl

ParamControl::ParamControl(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent, QList<struct ConfigButton> *btns)
    : ToggleControl(title, desc, icon, false, parent) {
  key = param.toStdString();
  QObject::connect(this, &ParamControl::toggleFlipped, this, &ParamControl::toggleClicked);
}

void ParamControl::toggleClicked(bool state) {
  auto do_confirm = [this]() {
    QString content("<body><h2 style=\"text-align: center;\">" + title_label->text() + "</h2><br>"
                    "<p style=\"text-align: center; margin: 0 128px; font-size: 50px;\">" + getDescription() + "</p></body>");
    return ConfirmationDialog(content, tr("Enable"), tr("Cancel"), true, this).exec();
  };

  bool confirmed = store_confirm && params.getBool(key + "Confirmed");
  if (!confirm || confirmed || !state || do_confirm()) {
    if (store_confirm && state) params.putBool(key + "Confirmed", true);
    params.putBool(key, state);
    setIcon(state);
  } else {
    toggle.togglePosition();
  }
}
