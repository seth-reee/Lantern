#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QFile>
#include "scanner.h"
#include "theme.h"
class UiTest:public QObject {
 Q_OBJECT
private slots:
 void populatedWindow() {
  QQuickStyle::setStyle("Basic");
  QTemporaryDir dir; QFile ping(dir.filePath("ping")); QVERIFY(ping.open(QIODevice::WriteOnly)); ping.write("#!/bin/sh\necho 'time=0.123 ms'\nexit 0\n"); ping.close(); ping.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
  const auto path=qgetenv("PATH"); qputenv("PATH",dir.path().toUtf8());
  Scanner scanner; scanner.start("127.0.0.1-127.0.0.3",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); qputenv("PATH",path); QCOMPARE(scanner.alive(),3);
  Theme theme; HostFilter hosts; hosts.setSourceModel(&scanner);
  QQmlApplicationEngine engine;
  QStringList warnings;
  connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors) { for(const auto &e:errors) warnings.append(e.toString()); });
  engine.rootContext()->setContextProperty("theme",&theme); engine.rootContext()->setContextProperty("scanner",&scanner); engine.rootContext()->setContextProperty("hosts",&hosts);
  engine.load(QUrl::fromLocalFile(LANTERN_QML));
  QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
  auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
  auto *results=window->findChild<QObject*>("results"); QVERIFY(results);
  QTRY_COMPARE(results->property("count").toInt(),3);
  QTest::qWait(150);
  QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
  hosts.filter("nothing-matches",""); QTRY_COMPARE(results->property("count").toInt(),0);
  hosts.filter("127.0.0.2","Live"); QTRY_COMPARE(results->property("count").toInt(),1);
  QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
 }
};
QTEST_MAIN(UiTest)
#include "ui_test.moc"
