#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResource>
#include <luma/qt/WorkbenchTheme.hpp>
static void initializeWorkbenchIcons() { Q_INIT_RESOURCE(luma_ui); }
namespace luma {
QIcon workbenchIcon(const QString& name, const QString& color) {
    QPixmap image(40, 40);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2, 2);
    p.setPen(QPen(QColor(color), 1.45, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (name == "home") {
        QPolygonF poly{{3, 9},   {10, 3}, {17, 9}, {17, 17}, {12, 17},
                       {12, 12}, {8, 12}, {8, 17}, {3, 17}};
        p.drawPolygon(poly);
    } else if (name == "cube") {
        QPolygonF poly{{10, 2}, {18, 6}, {18, 14}, {10, 18}, {2, 14}, {2, 6}};
        p.drawPolygon(poly);
        p.drawLine(2, 6, 10, 10);
        p.drawLine(18, 6, 10, 10);
        p.drawLine(10, 10, 10, 18);
        p.drawLine(10, 2, 10, 10);
    } else if (name == "camera") {
        p.drawRoundedRect(QRectF(2, 6, 16, 11), 2, 2);
        p.drawEllipse(QPointF(10, 11.5), 3, 3);
        p.drawLine(6, 6, 7, 3);
        p.drawLine(7, 3, 13, 3);
        p.drawLine(13, 3, 14, 6);
    } else if (name == "export") {
        p.drawLine(10, 2, 10, 13);
        p.drawLine(6, 6, 10, 2);
        p.drawLine(14, 6, 10, 2);
        p.drawLine(3, 12, 3, 17);
        p.drawLine(3, 17, 17, 17);
        p.drawLine(17, 17, 17, 12);
    } else if (name == "plus") {
        p.drawLine(10, 4, 10, 16);
        p.drawLine(4, 10, 16, 10);
    } else if (name == "folder") {
        QPolygonF poly{{2, 5}, {8, 5}, {10, 7}, {18, 7}, {18, 17}, {2, 17}};
        p.drawPolygon(poly);
    } else if (name == "check") {
        p.drawLine(4, 10, 8, 14);
        p.drawLine(8, 14, 16, 5);
    } else if (name == "help") {
        p.drawEllipse(QRectF(2, 2, 16, 16));
        p.drawText(QRectF(2, 0, 16, 20), Qt::AlignCenter, "?");
    } else {
        p.drawRoundedRect(QRectF(3, 3, 14, 14), 2, 2);
        p.drawLine(6, 7, 14, 7);
        p.drawLine(6, 10, 14, 10);
        p.drawLine(6, 13, 11, 13);
    }
    return QIcon(image);
}
QString workbenchStyleSheet() {
    static const bool initialized = [] { initializeWorkbenchIcons(); return true; }();
    (void)initialized;
    return R"(
QWidget { color:#e7e7ed; font-family:"PingFang SC"; font-size:12px; }
QWidget#scrollContent { background:transparent; }
QMainWindow, QWidget#shell { background:#101114; }
QWidget#projectTabs { background:#1b1c21; border:1px solid #2c2e35; border-radius:9px; }
QWidget#chatPanel { background:#141519; border-right:1px solid #2b2d33; }
QWidget#homeComposer { background:#1b1c20; border:1px solid #41434c; border-radius:18px; }
QPlainTextEdit, QTextBrowser { background:#191b20; border:1px solid #30333c; border-radius:9px; padding:10px; selection-background-color:#545d72; }
QPlainTextEdit#homePrompt { background:transparent; border:0; font-size:16px; padding:2px; }
QTextBrowser#chatHistory { background:transparent; border:0; padding:4px; }
QPushButton[variant="projectTab"] { background:transparent; border:0; padding:7px 11px; color:#989ba7; }
QPushButton[variant="projectTab"]:checked { background:#373a43; color:#f5f6f9; }
QPushButton[variant="utility"] { background:transparent; border:0; color:#9b9fac; padding:4px 8px; }
QPushButton[variant="utility"]:checked { background:#292d36; color:#e0e4ef; }
QTabWidget::pane { border:1px solid #2c303a; background:#16181e; border-radius:8px; }
QTabBar::tab { background:#1a1c23; color:#999dab; padding:9px 18px; border-bottom:2px solid transparent; }
QTabBar::tab:selected { color:#f0f2f9; border-bottom:2px solid #b5bdd1; background:#262a34; }
QTableWidget { background:#17191f; alternate-background-color:#1e222a; gridline-color:#303540; border:1px solid #303540; selection-background-color:#384257; }
QHeaderView::section { background:#252932; border:0; padding:10px; color:#b6bece; }
QTableWidget::item { padding:12px; }
QWidget#topbar { background:#141519; border-bottom:1px solid #292a31; }
QWidget#sidePanel, QWidget#properties { background:#18191e; }
QWidget#viewportTools, QWidget#workflowBar { background:#1b1c22; border:1px solid #2c2d35; border-radius:10px; }
QWidget#card { background:#1b1c22; border:1px solid #30313a; border-radius:12px; }
QLabel { background:transparent; border:0; }
QLabel[variant="muted"] { color:#9395a3; }
QLabel[variant="eyebrow"] { color:#a8abb7; font-size:10px; letter-spacing:2px; }
QLabel[variant="pageTitle"] { font-size:27px; font-weight:600; }
QLabel[variant="sectionTitle"] { font-size:16px; font-weight:600; }
QLabel[variant="badge"] { background:#292b31; color:#d2d4dc; border:1px solid #444750; padding:4px 9px; border-radius:10px; font-size:10px; }
QLabel[variant="stepTitle"] { font-size:18px; font-weight:600; }
QPushButton { background:#25262e; border:1px solid #383942; border-radius:7px; padding:8px 12px; color:#dedee6; }
QPushButton:hover { background:#32323e; border-color:#626070; }
QPushButton:pressed { background:#3c3d42; }
QPushButton:focus { border:1px solid #e4e4e7; }
QPushButton:checked { background:#e5e5e7;color:#18191c;border-color:#e5e5e7; }
QPushButton:disabled { color:#666773; background:#202126; border-color:#2b2c31; }
QPushButton[variant="primary"] { background:#e6e6e8; color:#191a1d; border:0; font-weight:600; }
QPushButton[variant="primary"]:hover { background:#ffffff; }
QPushButton[variant="primary"]:pressed { background:#c1c2c7; }
QPushButton[variant="primary"]:disabled { background:#34353a;color:#777983; }
QPushButton[variant="step"] { background:transparent; color:#9798a6; border:0; padding:10px 5px; }
QPushButton[variant="step"]:checked { background:#383a41; color:#f5f5f6; }
QPushButton[variant="link"] { border:0; background:transparent; color:#cdb7ed; padding:5px; }
QLineEdit, QComboBox, QDoubleSpinBox { background:#212229; border:1px solid #363740; border-radius:6px; padding:7px; selection-background-color:#555966; }
QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color:#aaaeb9; }
QComboBox::drop-down { border:0; width:20px; }
QComboBox::down-arrow { image:url(:/luma/chevron.png);width:12px;height:12px; }
QPushButton::menu-indicator { image:none;width:0; }
QComboBox QAbstractItemView { background:#25262e; color:#efedf4; selection-background-color:#4c3c61; padding:5px; }
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:0; }
QListWidget { background:transparent; border:0; outline:0; }
QListWidget::item { border:1px solid transparent; border-radius:7px; padding:9px 7px; margin:2px 0; color:#b2b4c1; }
QListWidget::item:selected { background:#303238; border-color:#696c76; color:#f3f3f5; }
QListWidget::item:hover { background:#282831; }
QGroupBox { border:1px solid #30313b; border-radius:9px; margin-top:20px; padding:14px 10px 10px; font-weight:500; }
QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 4px; color:#b7b7c6; }
QScrollArea { border:0; background:transparent; }
QScrollBar:vertical { background:transparent; width:6px; margin:4px 0; }
QScrollBar::handle:vertical { background:#45404e; border-radius:3px; min-height:35px; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:transparent; }
QCheckBox { spacing:8px; padding:7px 0; }
QCheckBox::indicator { width:15px; height:15px; background:#25262e; border:1px solid #56505f; border-radius:4px; }
QCheckBox::indicator:checked { background:#d1d2d5; border-color:#eeeeef; image:url(:/luma/check.png); }
QCheckBox::indicator:disabled { background:#25262a;border-color:#34353a; }
QCheckBox:disabled { color:#666873; }
QMenu { background:#202125; border:1px solid #3d3e44;padding:6px; }
QMenu::item { padding:9px 24px;border-radius:5px; }
QMenu::item:selected { background:#37383d; }
QMenu::item:disabled { color:#666873; }
QMenu::separator { height:1px;background:#36373d;margin:5px 8px; }
QSplitter::handle { background:#101114; width:7px; }
QStatusBar { background:#141519; color:#8f91a0; border-top:1px solid #292a31; font-size:10px; }
QStatusBar::item { border:0; }
QDialog { background:#18191e; }
QToolTip { background:#33303d; color:#efebf5; border:1px solid #615270; padding:5px; }
)";
}
} // namespace luma
