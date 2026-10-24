#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QApplication>
#include <QTimer>
#include <iostream>
#include <luma/qt/WorkbenchWindow.hpp>
int main(int argc, char** argv) {
    luma::configureQtSurface();
    QApplication app(argc, argv);
    app.setApplicationName("光场工坊");
    luma::WorkbenchWindow window;
    window.show();
    auto args = app.arguments();
    int p = args.indexOf("--preset");
    if (p >= 0 && p + 1 < args.size())
        window.viewport()->loadPreset(args[p + 1].toStdString());
    int page = args.indexOf("--page");
    window.showPage(page >= 0 && page + 1 < args.size() ? args[page + 1].toInt() : 0);
    int capture = args.indexOf("--screenshot");
    if (capture >= 0 && capture + 1 < args.size()) {
        const auto path = args[capture + 1];
        QTimer::singleShot(1200, &window, [&window, path] {
            if (!window.viewport()->lastError().empty()) {
                std::cerr << window.viewport()->lastError() << '\n';
                QApplication::exit(1);
                return;
            }
            bool ok = window.grab().save(path, "PNG");
            std::cout << (ok ? "Saved Qt window: " : "Screenshot failed: ") << path.toStdString()
                      << '\n';
            QApplication::exit(ok ? 0 : 1);
        });
    }
    return app.exec();
}
