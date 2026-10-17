#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QQuickWindow>
#include "scanner.h"
#include "theme.h"
int main(int argc,char **argv) {
 QGuiApplication app(argc,argv);
 app.setApplicationName("Lantern"); app.setOrganizationName("Lantern"); app.setApplicationVersion(LANTERN_VERSION);
 QQuickStyle::setStyle("Basic");
 Theme theme; Scanner scanner; HostFilter hosts; hosts.setSourceModel(&scanner);
 QQmlApplicationEngine engine;
 engine.rootContext()->setContextProperty("theme",&theme);
 engine.rootContext()->setContextProperty("scanner",&scanner);
 engine.rootContext()->setContextProperty("hosts",&hosts);
 QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[] { QCoreApplication::exit(1); },Qt::QueuedConnection);
 engine.loadFromModule("Lantern","Main");
 if(app.arguments().contains("--smoke-test")) QTimer::singleShot(700,&app,[&]() {
  if(auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().value(0))) window->grabWindow().save("/tmp/lantern-preview.png");
  app.quit();
 });
 return app.exec();
}
